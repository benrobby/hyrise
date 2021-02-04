
#include <iostream>
#include "benchmark/benchmark.h"

#include "bitpack.h"
#include "conf.h"

using ValueT = uint32_t;

namespace opossum {


// -------------- BENCHMARKS ---------------


void turboPFOR_bitcompression_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  
  std::vector<uint32_t> in(vec);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 1024);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = bsr32(max);
  benchmark::DoNotOptimize(compressedBufVec.data());

  for (auto _ : state) {

    bitpack32(in.data(), in.size(), compressedBufVec.data(), b);

    benchmark::ClobberMemory();
  }
}


void turboPFOR_bitcompression_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  std::vector<uint32_t> in(vec);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 1024);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = bsr32(max);

  bitpack32(in.data(), in.size(), compressedBufVec.data(), b);

  std::vector<uint32_t> dec(in.size());

  benchmark::DoNotOptimize(dec.data());

  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  // auto sum = 0;
  // benchmark::DoNotOptimize(sum);

  for (auto _ : state) {

    bitunpack32(compressedBufVec.data(), in.size(), dec.data(), b);

    for (int i = 0; i < in.size(); i++) {
      points[i] = dec[i];
      // if (dec[i] != in[i]) {
      //   std::cout << "not equal" << dec[i] << "    " << in[i] << std::endl;
      // }
    }

    benchmark::ClobberMemory();
    std::fill(points.begin(), points.end(), 0);
    //sum = 0;
  }
}


void _turboPFOR_bitcompression_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  std::vector<uint32_t> in(vec);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 1024);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = bsr32(max);

  bitpack32(in.data(), in.size(), compressedBufVec.data(), b);

  std::vector<uint32_t> dec(in.size());

  benchmark::DoNotOptimize(dec.data());

  uint32_t val;

  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        val = bitgetx32(compressedBufVec.data(), pointIndices[i], b);
        sum += val;
        if (val != in[pointIndices[i]]) {
           std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
        }
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        val = bitgetx32(compressedBufVec.data(), pointIndices[i], b);
        points[i] = val;
        if (val != in[pointIndices[i]]) {
           std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
        }
      }
    }
    benchmark::ClobberMemory();
    sum = 0;
  }
 
}

void turboPFOR_bitcompression_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_bitcompression_benchmark_decoding_points(vec, pointIndices, state, false);
}
void turboPFOR_bitcompression_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_bitcompression_benchmark_decoding_points(vec, pointIndices, state, true);
}

float turboPFOR_bitcompression_compute_bitsPerInt(std::vector<ValueT>& vec) {
  return 0.0;
}

}  // namespace opossum