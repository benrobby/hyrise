

#include <iostream>
#include "benchmark/benchmark.h"

#include "include/codecfactory.h"
#include "include/intersection.h"

using ValueT = uint32_t;

namespace opossum {

#define SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(codec) \
  void SIMDCompressionAndIntersection_ ## codec ## _benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_encoding(vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  void SIMDCompressionAndIntersection_ ## codec ## _benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding(vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  void SIMDCompressionAndIntersection_ ## codec ## _benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding_points(vec, pointIndices, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  float SIMDCompressionAndIntersection_ ## codec ## _compute_bitsPerInt(std::vector<ValueT>& _vec) { \
     return SIMDCompressionAndIntersection_compute_bitsPerInt(_vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec)); \
  };

void SIMDCompressionAndIntersection_benchmark_encoding(const std::vector<ValueT>& vec, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {

  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();

  benchmark::DoNotOptimize(compressed_output.data());

  for (auto _ : state) {
    size_t size = compressedsize;
    codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), size);
    benchmark::ClobberMemory();
  }
}

void SIMDCompressionAndIntersection_benchmark_decoding(const std::vector<ValueT>& vec, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), compressedsize);

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();
  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    size_t size = recoveredsize;
    codec.decodeArray(compressed_output.data(), compressedsize,
                      dec.data(), size);
    benchmark::ClobberMemory();
  }
}

void SIMDCompressionAndIntersection_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), compressedsize);

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    size_t size = recoveredsize;
    codec.decodeArray(compressed_output.data(), compressedsize,
                      dec.data(), size);
    for (size_t i : pointIndices) {
      points[i] = dec[i];
    }
    benchmark::ClobberMemory();
  }
}

float SIMDCompressionAndIntersection_compute_bitsPerInt(std::vector<ValueT>& vec, SIMDCompressionLib::IntegerCODEC &codec) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), compressedsize);

  compressed_output.resize(compressedsize);
  compressed_output.shrink_to_fit();

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(compressed_output.data(), compressedsize,
                    dec.data(), recoveredsize);

  if (vec != dec) throw std::runtime_error("bug!");

  return 32.0 * static_cast<double>(compressed_output.size()) / static_cast<double>(vec.size());
}



SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(fastpfor);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(varint);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(vbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(maskedvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(streamvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(frameofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(simdframeofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(varintgb);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4fastpford4);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4fastpfordm);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4fastpford1);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4fastpford2);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(bp32);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(ibp32);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d1ni);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d2ni);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d4ni);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128dmni);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d1);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d2);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128d4);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(s4bp128dm);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(for);

} // namespace opposum