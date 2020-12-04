#include <iostream>
#include <random>

#define CHUNK_SIZE 65'000


using namespace std;

namespace opossum {

using ValueT = uint32_t;

// poslists

vector<size_t> getUniformPositionList(const int num_positions) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(0, CHUNK_SIZE);

  vector<size_t> vec(num_positions);
  generate(vec.begin(), vec.end(), [&]() { return distrib(gen); });
  return vec;
}

std::vector<size_t> get_poslist_10() { return getUniformPositionList(10); }

std::vector<size_t> get_poslist_100() { return getUniformPositionList(100); }

std::vector<size_t> get_poslist_1000() { return getUniformPositionList(1000); }

std::vector<size_t> get_poslist_10000() { return getUniformPositionList(10000); }

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

std::vector<ValueT> get_with_sequential_numbers() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 0;
    v = v + 5;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_huge_numbers() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 331;
    v = 1'000'000 + ((v * 7) % 1'000'000);
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_random_walk() {
  std::vector<ValueT> vec(CHUNK_SIZE);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 945713;
    if ((v * 13 % 7) > 3) {
      v = v + 13;
    } else {
      v = v - 11;
    }
    return v;
  });
  return vec;
}

}