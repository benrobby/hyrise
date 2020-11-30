
#include <iostream>
#include "benchmark/benchmark.h"
#include "streamvbyte.h"
#include "vp4.h"

using ValueT = uint32_t;

namespace opossum {

void turboPFOR_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  benchmark::DoNotOptimize(outBuffer);

  for (auto _ : state) {
    p4nenc32(inData, vec.size(), outBuffer);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  p4nenc32(inData, vec.size(), outBuffer);

  // Decode
  ValueT* decompressedData = (ValueT*) malloc(vec.size() * sizeof(ValueT));
  benchmark::DoNotOptimize(decompressedData);

  for (auto _ : state) {
    p4ndec32(outBuffer, vec.size(), decompressedData);
    benchmark::ClobberMemory();
  }
}

float turboPFOR_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4);
  ValueT* inData = (ValueT*) vec.data();
  p4nenc32(inData, vec.size(), outBuffer);

  // Decode
  ValueT* decompressedData = (ValueT*) malloc(vec.size() * sizeof(ValueT));
  size_t decompressedSizeInBytes = p4ndec32(outBuffer, vec.size(), decompressedData);

  for (size_t i = 0; i < vec.size(); i++) {
    if (decompressedData[i] != vec[i]){
      throw std::runtime_error("bug!");
    }
  }

  // # bits (encoded) / # elements to encode
  return decompressedSizeInBytes * 8.0 / vec.size();
}

}  // namespace opossum