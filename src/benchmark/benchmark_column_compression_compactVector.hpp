
#include <iostream>
#include "benchmark/benchmark.h"

#include <compact_vector/compact_vector.hpp>

using ValueT = uint32_t;

namespace opossum {


// -------------- BENCHMARKS ---------------


void compactVector_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {

  uint32_t max = 0;
  for (const auto v : vec) {
    max |= v;
  }
  uint32_t b = compact::vector<int, 32>::required_bits(max);

  compact::vector<unsigned int> compressedVector(b, vec.size());

  benchmark::DoNotOptimize(compressedVector);

  for (auto _ : state) {
    // compressedVector.resize(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
      compressedVector.push_back(vec[i]);
    }

    benchmark::ClobberMemory();
    compressedVector.clear();
  }
}

void compactVector_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  std::vector<uint32_t> in(vec);

  uint32_t max = 0;
  for (const auto v : vec) {
    max |= v;
  }
  uint32_t b = compact::vector<unsigned int, 32>::required_bits(max);


  // TODO: can we somehow generate code for all b in [0, 32]?
  compact::vector<unsigned int> compressedVector(b, vec.size());

  // compact::vector<unsigned int, 30> compressedVector(vec.size());

  for (size_t i = 0; i < vec.size(); i++) {
    compressedVector.push_back(vec[i]);
  }

  std::vector<uint32_t> dec(vec.size());
  benchmark::DoNotOptimize(dec.data());

  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  // auto sum = 0;
  // benchmark::DoNotOptimize(sum);

  for (auto _ : state) {

    for (size_t i = 0; i < vec.size(); i++) {
      points[i] = compressedVector[i];
    }

    benchmark::ClobberMemory();
    std::fill(points.begin(), points.end(), 0);
    //sum = 0;

  }
}


void _compactVector_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  std::vector<uint32_t> in(vec);
  uint32_t max = 0;
  for (const auto v : vec) {
    max |= v;
  }
  uint32_t b = compact::vector<int, 32>::required_bits(max);
  compact::vector<unsigned int> compressedVector(b);
  benchmark::DoNotOptimize(compressedVector);
  for (size_t i = 0; i < vec.size(); i++) {
    compressedVector.push_back(vec[i]);
  }

  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);
  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);
  uint32_t val;

  for (auto _ : state) {
    if (nocopy) {

      for (size_t i = 0; i < pointIndices.size(); i++) {
        val = compressedVector[pointIndices[i]];
        sum += val;
        if (val != in[pointIndices[i]]) {
           std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
        }
      }

    } else {

      for (size_t i = 0; i < pointIndices.size(); i++) {
        val = compressedVector[pointIndices[i]];
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

void compactVector_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _compactVector_benchmark_decoding_points(vec, pointIndices, state, false);
}
void compactVector_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _compactVector_benchmark_decoding_points(vec, pointIndices, state, true);
}

float compactVector_compute_bitsPerInt(std::vector<ValueT>& vec) {
  return 0.0;
}

}  // namespace opossum