//
// Created by Linus Heinzl on 27.11.20.
//


#include <iostream>
#include <sdsl/dac_vector.hpp>
#include "benchmark/benchmark.h"
#include "streamvbyte.h"

using ValueT = uint32_t;

namespace opossum {

void sdsl_lite_dac_vector_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  for (auto _ : state) {
    sdsl::dac_vector<> encoded(vec);
    benchmark::ClobberMemory();
  }
}

void sdsl_lite_dac_vector_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  sdsl::dac_vector<> encoded(vec);

  // Decode
  std::vector<ValueT> decoded = std::vector<ValueT>(vec.size());

  for (auto _ : state) {
    for (size_t i = 0; i < vec.size(); i++) {
      decoded[i] = encoded[i];
    }
  }
}

float sdsl_lite_dac_vector_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  sdsl::dac_vector<> encoded(vec);

  // Decode
  std::vector<ValueT> decoded = std::vector<ValueT>(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    decoded[i] = encoded[i];
  }

  if (vec != decoded) throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return sdsl::size_in_bytes(encoded) * 8.0 / vec.size();
}

}  // namespace opossum


