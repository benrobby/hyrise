

#include <iostream>
#include "benchmark/benchmark.h"

#include "include/codecfactory.h"
#include "include/intersection.h"

using ValueT = uint32_t;

namespace opossum {

void SIMDCompressionAndIntersection_s4fastpford1_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {

  SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("s4-fastpfor-d1");
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();

  benchmark::DoNotOptimize(compressed_output.data());

  for (auto _ : state) {
    size_t size = compressedsize;
    codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), size);
    benchmark::ClobberMemory();
  }
}

void SIMDCompressionAndIntersection_s4fastpford1_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("s4-fastpfor-d1");
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

void SIMDCompressionAndIntersection_s4fastpford1_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  // Encode
  SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("s4-fastpfor-d1");
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

float SIMDCompressionAndIntersection_s4fastpford1_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("s4-fastpfor-d1");
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
} // namespace opposum