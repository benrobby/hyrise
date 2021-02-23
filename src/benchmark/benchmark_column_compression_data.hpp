#include <iostream>
#include <random>

#define CHUNK_SIZE 65'000


using namespace std;

namespace opossum {

using ValueT = uint32_t;

// poslists

vector<size_t> getUniformlyDistributedVector(const int size, size_t start, size_t end) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<unsigned int> distrib(start, end-1); //Has no exclusive end

  vector<size_t> vec(size);
  generate(vec.begin(), vec.end(), [&]() {
    return distrib(gen);
  });
  return vec;
}

#define POSLIST_METHOD(size) \
  std::vector<size_t> get_poslist_##size () { \
    return getUniformlyDistributedVector(size,0,CHUNK_SIZE); \
  };

POSLIST_METHOD(1)
POSLIST_METHOD(10)
POSLIST_METHOD(100)
POSLIST_METHOD(1000)
POSLIST_METHOD(10000)
POSLIST_METHOD(100000)
POSLIST_METHOD(1000000)

#define POSLIST_METHOD_SORTED(size) \
  std::vector<size_t> get_poslist_##size##_sorted () { \
    vector<size_t> vec(size); \
    int i = 0; \
    generate(vec.begin(), vec.end(), [&]() { \
      i = (i + 1) % CHUNK_SIZE; \
      return i; \
    }); \
    return vec; \
  }; \

POSLIST_METHOD_SORTED(1)
POSLIST_METHOD_SORTED(10)
POSLIST_METHOD_SORTED(100)
POSLIST_METHOD_SORTED(1000)
POSLIST_METHOD_SORTED(10000)
POSLIST_METHOD_SORTED(100000)
POSLIST_METHOD_SORTED(1000000)

// Data

std::vector<ValueT> get_with_small_numbers() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 0;
    v = (v + 1) % 4;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_sequential_sorted_numbers() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 0;
    v = v + 5;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_huge_numbers() {
  std::vector<size_t> vec = getUniformlyDistributedVector(CHUNK_SIZE, 0, 1'000'000'000);
  return std::vector<ValueT>(vec.begin(), vec.end());
}

std::vector<ValueT> get_with_random_walk() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 1'000'000;
    bool goesUpwards = rand() % 2 == 0;
    v =  goesUpwards ? v + 5 : v -5;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_month_categorical_numbers() {
  std::vector<size_t> vec = getUniformlyDistributedVector(CHUNK_SIZE, 1, 12);
  return std::vector<ValueT>(vec.begin(), vec.end());
}

std::vector<ValueT> get_with_year_categorical_numbers() {
  std::vector<size_t> vec = getUniformlyDistributedVector(CHUNK_SIZE, 1900, 2100);
  return std::vector<ValueT>(vec.begin(), vec.end());
}

std::vector<ValueT> get_with_av_numbers() {
  std::vector<size_t> vec = getUniformlyDistributedVector(CHUNK_SIZE, 0, CHUNK_SIZE);
  return std::vector<ValueT>(vec.begin(), vec.end());
}

}