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

std::vector<size_t> get_poslist_1() {
  return getUniformlyDistributedVector(10,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_10() {
  return getUniformlyDistributedVector(10,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_100() {
  return getUniformlyDistributedVector(100,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_1000() {
  return getUniformlyDistributedVector(1000,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_10000() {
  return getUniformlyDistributedVector(10000,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_100000() {
  return getUniformlyDistributedVector(100000,0,CHUNK_SIZE);
}

std::vector<size_t> get_poslist_10000_sorted() {
  auto poslist = getUniformlyDistributedVector(10000,0,CHUNK_SIZE);
  std::sort(poslist.begin(), poslist.end());
  return poslist;
}

std::vector<size_t> get_poslist_100000_sorted() {
  vector<size_t> vec(100000);
  int i = 0;
  generate(vec.begin(), vec.end(), [&]() {
    return i++;
  });
  return vec;
}

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