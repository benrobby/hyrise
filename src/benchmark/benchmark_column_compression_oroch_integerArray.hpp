

#include <iostream>
#include <memory>
#include "benchmark/benchmark.h"

#include <integer_array.h>

using ValueT = uint32_t;

namespace opossum {

void oroch_integerArray_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  for (auto _ : state) {
    oroch::integer_array<ValueT> enc;

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

  for (auto _ : state) {
    enc.decode(dec.begin(), dec.end());
    benchmark::ClobberMemory();
  }
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

  // # bits (encoded) / # elements to encode
  return 32.0 * static_cast<double>(enc.size()) / static_cast<double>(vec.size());
}

}  // namespace opossum