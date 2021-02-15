#include "worker.hpp"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "abstract_scheduler.hpp"
#include "abstract_task.hpp"
#include "hyrise.hpp"
#include "task_queue.hpp"

namespace {

/**
 * On worker threads, this references the Worker running on this thread, on all other threads, this is empty.
 * Uses a weak_ptr, because otherwise the ref-count of it would not reach zero within the main() scope of the program.
 */
thread_local std::weak_ptr<opossum::Worker> this_thread_worker;
}  // namespace

// The sleep time was determined experimentally
static constexpr auto WORKER_SLEEP_TIME = std::chrono::microseconds(300);

namespace opossum {

std::shared_ptr<Worker> Worker::get_this_thread_worker() { return ::this_thread_worker.lock(); }

Worker::Worker(const std::shared_ptr<TaskQueue>& queue, WorkerID id, CpuID cpu_id)
    : _queue(queue), _id(id), _cpu_id(cpu_id) {
  // Generate a random distribution from 0-99 for later use, see below
  _random.resize(100);
  std::iota(_random.begin(), _random.end(), 0);
  std::shuffle(_random.begin(), _random.end(), std::default_random_engine{std::random_device{}()});
}

WorkerID Worker::id() const { return _id; }

std::shared_ptr<TaskQueue> Worker::queue() const { return _queue; }

CpuID Worker::cpu_id() const { return _cpu_id; }

void Worker::operator()() {
  Assert(this_thread_worker.expired(), "Thread already has a worker");

  this_thread_worker = shared_from_this();

  _set_affinity();

  while (Hyrise::get().scheduler()->active()) {
    _work();
  }
}

void Worker::_work() {
  // If execute_next has been called, run that task first, otherwise try to retrieve a task from the queue.
  auto task = std::shared_ptr<AbstractTask>{};
  if (_next_task) {
    task = std::move(_next_task);
    _next_task = nullptr;
  } else {
    task = _queue->pull();
  }

  if (!task) {
    // Simple work stealing without explicitly transferring data between nodes.
    auto work_stealing_successful = false;
    for (const auto& queue : Hyrise::get().scheduler()->queues()) {
      if (queue == _queue) {
        continue;
      }

      task = queue->steal();
      if (task) {
        task->set_node_id(_queue->node_id());
        work_stealing_successful = true;
        break;
      }
    }

    // If there is no ready task neither in our queue nor in any other, worker waits for a new task to be pushed to the
    // own queue or returns after timer exceeded (whatever occurs first).
    if (!work_stealing_successful) {
      {
        std::unique_lock<std::mutex> unique_lock(_queue->lock);
        _queue->new_task.wait_for(unique_lock, WORKER_SLEEP_TIME);
      }
      return;
    }
  }

  const auto already_started = task->about_to_be_executed.exchange(true);
  if (already_started) {
    return;
  }

  task->execute();

  // This is part of the Scheduler shutdown system. Count the number of tasks a Worker executed to allow the
  // Scheduler to determine whether all tasks finished
  _num_finished_tasks++;
}

void Worker::execute_next(const std::shared_ptr<AbstractTask>& task) {
  DebugAssert(&*get_this_thread_worker() == this,
              "execute_next must be called from the same thread that the worker works in");
  if (!_next_task) {
    const auto successfully_enqueued = task->try_mark_as_enqueued();
    if (!successfully_enqueued) {
      // The task was already enqueued. This can happen if
      //   * two tasks ARE TO BE scheduled via AbstractScheduler::schedule where one task is the other one's successor
      //   * the first one is scheduled and executed very quickly before the second one reaches the schedule method
      //   * AbstractScheduler::schedule then looks at the second task and realizes that it is ready to be enqueued
      //   * ... and both the scheduler and this method try to enqueue it.
      // If successfully_enqueued is false, we lost, and the task is already in one of the task queues.
      return;
    }
    Assert(successfully_enqueued, "Task was already enqueued, expected to be solely responsible for execution");
    _next_task = task;
  } else {
    _queue->push(task, static_cast<uint32_t>(SchedulePriority::Default));
  }
}

void Worker::start() { _thread = std::thread(&Worker::operator(), this); }

void Worker::join() {
  Assert(!Hyrise::get().scheduler()->active(), "Worker can't be join()-ed while the scheduler is still active");
  _thread.join();
}

uint64_t Worker::num_finished_tasks() const { return _num_finished_tasks; }

void Worker::_wait_for_tasks(const std::vector<std::shared_ptr<AbstractTask>>& tasks) {
  // This lambda checks if all tasks from the vector have been executed. If they are, it causes _wait_for_tasks to
  // return. If there are remaining tasks, it primarily tries to execute these. If they cannot be executed, the
  // worker performs work for others (i.e., executes tasks from the queue).
  auto check_own_tasks = [&]() {
    auto all_done = true;
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
      const auto& task = *it;
      if (task->is_done()) {
        continue;
      }

      // Task is not yet done - check if it is ready for execution
      if (!task->is_ready()) {
        all_done = false;
        continue;
      }

      // Give other tasks a certain chance of being executed, too. Anectotal evidence says that this is a good idea.
      // For some reason, this keeps the memory consumption of TPC-H Q6 low even if the scheduler is overcommitted.
      // Because generating random numbers is somewhat expensive, we keep a list of random numbers and reuse them.
      // TODO(anyone): Look deeper into scheduling theory and make this theoretically sound.
      _next_random = (_next_random + 1) % _random.size();
      if (_random[_next_random] <= 20) {
        return false;
      }

      // Run one of our own tasks. First, let everyone know that we are about to execute it. This is necessary because
      // the task is already in a queue and some other worker might pull it at the same time.
      const auto already_started = task->about_to_be_executed.exchange(true);
      if (already_started) {
        all_done = false;
        continue;
      }

      // Actually execute it.
      task->execute();
      ++_num_finished_tasks;

      // Reset loop so that we re-visit tasks that may have finished in the meantime. We need to decrement `it` because
      // it will be incremented when the loop iteration finishes.
      all_done = true;
      it = tasks.begin() - 1;
    }
    return all_done;
  };

  while (!check_own_tasks()) {
    // Do work for someone else
    _work();
  }
}

void Worker::_set_affinity() {
#if HYRISE_NUMA_SUPPORT
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(_cpu_id, &cpuset);
  auto rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    // This is not an Assert(), though maybe it should be. Not being able to pin the threads doesn't make the DB
    // unfunctional, but probably slower
    std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
  }
#endif
}

}  // namespace opossum
