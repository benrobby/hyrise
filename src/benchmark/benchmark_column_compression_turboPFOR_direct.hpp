
#include <iostream>
#include "benchmark/benchmark.h"

#include "vp4.h"


using ValueT = uint32_t;

namespace opossum {

#define P4NENC_BOUND(n) ((n + 127) / 128 + (n + 32) * sizeof(uint32_t))
#define ROUND_UP(_n_, _a_) (((_n_) + ((_a_)-1)) & ~((_a_)-1))

ValueT getValue(unsigned char* in, uint32_t b, p4 p4, uint32_t index) {
  ValueT value;
  if(unlikely(p4.isx)) {
    value = p4getx32(&p4, in, index, b);
  } else {
    value = bitgetx32(in, index, b);
  }
  return value;
}

bool isCorrect(std::vector<ValueT> original, std::vector<ValueT> decompressed) {
  for (int i = 0; i < original.size(); i++) {
    if (original[i] != decompressed[i]) {
      return false;
    }
  }
  return true;
}

void turboPFOR_direct_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  unsigned char* outBuffer = (unsigned char*) malloc(P4NENC_BOUND(vec.size()));
  std::vector<ValueT> vecCopy(vec);
  vecCopy.reserve(ROUND_UP(vec.size(), 32));

  ValueT* inData = (ValueT*) vecCopy.data();
  benchmark::DoNotOptimize(outBuffer);

  for (auto _ : state) {
    p4nenc32(inData, vec.size(), outBuffer);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_direct_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(P4NENC_BOUND(vec.size()));
  std::vector<ValueT> vecCopy(vec);
  vecCopy.reserve(ROUND_UP(vec.size(), 32));
  ValueT* inData = (ValueT*) vecCopy.data();
  p4nenc32(inData, vec.size(), outBuffer);

  // Decode
  std::vector<ValueT> decompressedData(vec.size());
  decompressedData.reserve(ROUND_UP(vec.size(),32));
  benchmark::DoNotOptimize(decompressedData);

  for (auto _ : state) {
    p4ndec32(outBuffer, vec.size(), decompressedData.data());
    benchmark::ClobberMemory();
  }
  assert(isCorrect(vec, decompressedData));
}

void _turboPFOR_direct_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  unsigned int n = vec.size();
  std::vector<ValueT> originalValues;
  for (auto i : pointIndices) {
    originalValues.push_back(vec[i]);
  }

  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(P4NENC_BOUND(vec.size()));
  std::vector<ValueT> vecCopy(vec);
  vecCopy.resize(ROUND_UP(vec.size(), 32));
  ValueT* inData = (ValueT*) vecCopy.data();
  p4encx32(inData, n, outBuffer);

  // Decode
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  ValueT sum = 0;
  p4 p4;
  unsigned b;
  unsigned char* pointerCopy = outBuffer;
  p4ini(&p4, &pointerCopy, n, &b);


  for (auto _ : state) {
    // But you pay the price with an ugly C interface. I hate, hate, hate this pattern that
    // you create new uninitialized variables and pass them to the function by reference, where
    // it mutates them. This is especially annoying for the "in" parameter. If we don't copy the outBuffer pointer,
    // it gets mutated, which just screams "annoying, hard to debug problem in the future".

    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += getValue(pointerCopy, b, p4, pointIndices[i]);
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = getValue(pointerCopy, b, p4, pointIndices[i]);
      }
      assert(isCorrect(originalValues, points));
    }
    benchmark::ClobberMemory();
    sum = 0;
  }
}

void turboPFOR_direct_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_direct_benchmark_decoding_points(vec, pointIndices, state, false);
}
void turboPFOR_direct_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_direct_benchmark_decoding_points(vec, pointIndices, state, true);
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
  p4dec32(outBuffer, n, decompressedData);


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