

#include <iostream>
#include <memory>
#include "benchmark/benchmark.h"

#include <integer_array.h>

using ValueT = uint32_t;

namespace opossum {

void oroch_integerArray_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  for (auto _ : state) {
    oroch::integer_array<ValueT> enc;
    benchmark::DoNotOptimize(enc);
    for (size_t i = 0; i < vec.size(); i++) {
      enc.insert(i, vec[i]);
    }

    benchmark::ClobberMemory();
  }
}

void oroch_integerArray_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  oroch::integer_array<ValueT> enc;
  for (size_t i = 0; i < vec.size(); i++) {
    enc.insert(i, vec[i]);
  }

  // Decode
  std::vector<ValueT> dec(0);
  dec.resize(vec.size());
  benchmark::DoNotOptimize(dec.data());


  for (auto _ : state) {
    for (size_t i = 0; i < vec.size(); i++) {
      dec[i] = enc.at(i);
    }
    benchmark::ClobberMemory();
  }
}

void _oroch_integerArray_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  // Encode
  oroch::integer_array<ValueT> enc;
  for (size_t i = 0; i < vec.size(); i++) {
    enc.insert(i, vec[i]);
  }

  // Decode

  std::vector<ValueT> points(pointIndices.size());
  benchmark::DoNotOptimize(points.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += enc.at(pointIndices[i]);
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = enc.at(pointIndices[i]);
      }
    }

    benchmark::ClobberMemory();
	}
	if (sum) {
    std::cout << "";
  }
}

void oroch_integerArray_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _oroch_integerArray_benchmark_decoding_points(vec, pointIndices, state, false);
}

void oroch_integerArray_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _oroch_integerArray_benchmark_decoding_points(vec, pointIndices, state, true);
}


float oroch_integerArray_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  oroch::integer_array<ValueT> enc;
  for (size_t i = 0; i < vec.size(); i++) {
    enc.insert(i, vec[i]);
  }

  // Decode
  std::vector<ValueT> dec(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    dec[i] = enc.at(i);
  }

  if (vec != dec) throw std::runtime_error("bug!");

  // # NOT IMPLEMENTED YET - How could we do it?
  return -1;
}

}  // namespace opossum