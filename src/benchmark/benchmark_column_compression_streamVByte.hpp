
#include <iostream>
#include "benchmark/benchmark.h"
#include "streamvbyte.h"

using ValueT = uint32_t;

namespace opossum {

void streamVByte_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  benchmark::DoNotOptimize(enc.data());
  for (auto _ : state) {
    streamvbyte_encode(vec.data(), vec.size(), enc.data());
    benchmark::ClobberMemory();
  }
}

void streamVByte_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  streamvbyte_encode(vec.data(), vec.size(), enc.data());

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    streamvbyte_decode(enc.data(), dec.data(), vec.size());
    benchmark::ClobberMemory();
  }
}

void _streamVByte_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  streamvbyte_encode(vec.data(), vec.size(), enc.data());

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    streamvbyte_decode(enc.data(), dec.data(), vec.size());
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += dec[pointIndices[i]];
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = dec[pointIndices[i]];
      }
    }
    benchmark::ClobberMemory();
	}
	if (sum) {
    std::cout << "";
  }
}

void streamVByte_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _streamVByte_benchmark_decoding_points(vec, pointIndices, state, false);
}

void streamVByte_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _streamVByte_benchmark_decoding_points(vec, pointIndices, state, true);
}

float streamVByte_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  std::vector<uint8_t> enc = std::vector<uint8_t>(streamvbyte_max_compressedbytes(vec.size()));
  size_t compsize = streamvbyte_encode(vec.data(), vec.size(), enc.data());

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t compsize2 = streamvbyte_decode(enc.data(), dec.data(), vec.size());

  if (vec != dec) throw std::runtime_error("bug!");
  if (compsize != compsize2) throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return (8.0 * compsize) / static_cast<double>(vec.size());
}

}  // namespace opossum