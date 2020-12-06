
#include <iostream>
#include "benchmark/benchmark.h"
#include "streamvbyte.h"
#include "streamvbytedelta.h"

using ValueT = uint32_t;

namespace opossum {

void streamVByteDelta_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  benchmark::DoNotOptimize(enc.data());
  for (auto _ : state) {
    streamvbyte_delta_encode(vec.data(), vec.size(), enc.data(), 0);
    benchmark::ClobberMemory();
  }
}

void streamVByteDelta_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  streamvbyte_delta_encode(vec.data(), vec.size(), enc.data(), 0);

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    streamvbyte_delta_decode(enc.data(), dec.data(), vec.size(), 0);
    benchmark::ClobberMemory();
  }
}

void streamVByteDelta_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  streamvbyte_delta_encode(vec.data(), vec.size(), enc.data(), 0);

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    streamvbyte_delta_decode(enc.data(), dec.data(), vec.size(), 0);
    for (size_t i : pointIndices) {
      points[i] = dec[i];
    }
    benchmark::ClobberMemory();
  }
}

float streamVByteDelta_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  size_t compsize = streamvbyte_delta_encode(vec.data(), vec.size(), enc.data(), 0);

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t compsize2 = streamvbyte_delta_decode(enc.data(), dec.data(), vec.size(), 0);

  if (vec != dec) throw std::runtime_error("bug!");
  if (compsize != compsize2) throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return (8.0 * compsize) / static_cast<double>(vec.size());
}

}  // namespace opossum