
#include <iostream>
#include "benchmark/benchmark.h"


#include "vp4.h"


using ValueT = uint32_t;

namespace opossum {


void turboPFOR_direct_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  benchmark::DoNotOptimize(outBuffer);

  for (auto _ : state) {
    p4encx32(inData, vec.size(), outBuffer);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_direct_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*8);
  ValueT* inData = (ValueT*) vec.data();
  p4encx32(inData, vec.size(), outBuffer);

  // Decode
  unsigned int size = vec.size();
  ValueT* decompressedData = (ValueT*) malloc(vec.size() * sizeof(ValueT));
  benchmark::DoNotOptimize(decompressedData);


  for (auto _ : state) {
    // This function calls the p4getx32 function for each i. The p4getx32 function is bascially the only resource to learn about how
    // to use direct access. And it is impressively fast - in my tests it only took double the amount of time
    // to decrompress the buffer compared to the p4dec function. This is still faster than a lot of the other algorithms
    // that operate on whole blocks.
    p4decx32(outBuffer, size, decompressedData);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_direct_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<ValueT>& pointIndices, benchmark::State& state) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  p4nenc32(inData, vec.size(), outBuffer);

  // Decode
  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  for (auto _ : state) {
    // But you pay the price with an ugly C interface. I hate, hate, hate this pattern that
    // you create new uninitialized variables and pass them to the function by reference, where
    // it mutates them. This is especially annoying for the "in" parameter. If we don't copy the outBuffer pointer,
    // it gets mutated, which just screams "annoying, hard to debug problem in the future".
    p4 p4;
    unsigned b;
    unsigned char* pointerCopy = outBuffer;
    p4ini(&p4, &pointerCopy, n, &b);
    for (size_t i : pointIndices) {
      points[i] = p4getx32(&p4, pointerCopy, i, b);
    }
    benchmark::ClobberMemory();
  }
}

float turboPFOR_direct_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  unsigned char* end = p4encx32(inData, vec.size(), outBuffer);

  // Decode by point
  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  p4 p4;
  unsigned b;
  unsigned char* pointerCopy = outBuffer;
  p4ini(&p4, &pointerCopy, n, &b);
  for (size_t i = 0; i < vec.size(); i++) {
    points[i] = p4getx32(&p4, pointerCopy, i, b);
  }

  // Decode whole buffer
  ValueT* decompressedData = (ValueT*) malloc(vec.size() * sizeof(ValueT));
  p4decx32(outBuffer, n, decompressedData);


  for (size_t i = 0; i < vec.size(); i++) {
    if (decompressedData[i] != vec[i] || points[i] != vec[i]){
      throw std::runtime_error("bug!");
    }
  }

  // # bits (encoded) / # elements to encode
  int size_in_bytes = (end - outBuffer) * sizeof(unsigned char);
  return size_in_bytes * 8.0 / vec.size();
}

}  // namespace opossum