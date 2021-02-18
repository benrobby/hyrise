

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
  };                                                            \
  void SIMDCompressionAndIntersection_ ## codec ## _benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding_points_nocopy(vec, pointIndices, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  float SIMDCompressionAndIntersection_ ## codec ## _compute_bitsPerInt(std::vector<ValueT>& _vec) { \
     return SIMDCompressionAndIntersection_compute_bitsPerInt(_vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec)); \
  };

// make sure to not have duplicates with and without select
#define SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(codec) \
  void SIMDCompressionAndIntersection_ ## codec ## _with_select_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_encoding(vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  void SIMDCompressionAndIntersection_ ## codec ## _with_select_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding(vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  void SIMDCompressionAndIntersection_ ## codec ## _with_select_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding_points_select(vec, pointIndices, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  };                                                                        \
  void SIMDCompressionAndIntersection_ ## codec ## _with_select_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) { \
    SIMDCompressionAndIntersection_benchmark_decoding_points_select_nocopy(vec, pointIndices, *SIMDCompressionLib::CODECFactory::getFromName(#codec), state); \
  }; \
  float SIMDCompressionAndIntersection_ ## codec ## _with_select_compute_bitsPerInt(std::vector<ValueT>& _vec) {                              \
     return SIMDCompressionAndIntersection_compute_bitsPerInt_select(_vec, *SIMDCompressionLib::CODECFactory::getFromName(#codec)); \
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

void _SIMDCompressionAndIntersection_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state, bool nocopy) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), compressedsize);

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    size_t size = recoveredsize;
    codec.decodeArray(compressed_output.data(), compressedsize,
                      dec.data(), size);
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
    sum = 0;
  }
}

void SIMDCompressionAndIntersection_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  return _SIMDCompressionAndIntersection_benchmark_decoding_points(vec, pointIndices, codec, state, false);
}

void SIMDCompressionAndIntersection_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  return _SIMDCompressionAndIntersection_benchmark_decoding_points(vec, pointIndices, codec, state, true);
}

void _SIMDCompressionAndIntersection_benchmark_decoding_points_select(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state, bool nocopy) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  codec.encodeArray(const_cast<uint32_t*>(vec.data()), vec.size(), compressed_output.data(), compressedsize);

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(dec.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    size_t size = recoveredsize;
    // codec.decodeArray(compressed_output.data(), compressedsize, dec.data(), size);
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += codec.select(compressed_output.data(), pointIndices[i]);
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = codec.select(compressed_output.data(), pointIndices[i]);
      }
    }
    benchmark::ClobberMemory();
    sum = 0;
  }
}

void SIMDCompressionAndIntersection_benchmark_decoding_points_select(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  return _SIMDCompressionAndIntersection_benchmark_decoding_points_select(vec, pointIndices, codec, state, false);
}

void SIMDCompressionAndIntersection_benchmark_decoding_points_select_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, SIMDCompressionLib::IntegerCODEC &codec, benchmark::State& state) {
  return _SIMDCompressionAndIntersection_benchmark_decoding_points_select(vec, pointIndices, codec, state, true);
}

float SIMDCompressionAndIntersection_compute_bitsPerInt(std::vector<ValueT>& vec, SIMDCompressionLib::IntegerCODEC &codec) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  std::vector<ValueT> input(vec); // modified in place, create a copy first
  codec.encodeArray(const_cast<uint32_t*>(input.data()), vec.size(), compressed_output.data(), compressedsize);

  compressed_output.resize(compressedsize);
  compressed_output.shrink_to_fit();

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(compressed_output.data(), compressedsize,
                    dec.data(), recoveredsize);

  for (int i = 0; i < vec.size(); i++) {
    ValueT a = vec[i];
    ValueT b = dec[i];
    if (a != b) throw std::runtime_error("bug!");
  }

  return 32.0 * static_cast<double>(compressed_output.size()) / static_cast<double>(vec.size());
}

float SIMDCompressionAndIntersection_compute_bitsPerInt_select(std::vector<ValueT>& vec, SIMDCompressionLib::IntegerCODEC &codec) {
  // Encode
  std::vector<uint32_t> compressed_output(2 * vec.size() + 1024);
  size_t compressedsize = compressed_output.size();
  std::vector<ValueT> input(vec); // modified in place
  codec.encodeArray(const_cast<uint32_t*>(input.data()), vec.size(), compressed_output.data(), compressedsize);

  compressed_output.resize(compressedsize);
  compressed_output.shrink_to_fit();

  // Decode
  std::vector<uint32_t> dec(vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(compressed_output.data(), compressedsize,
                    dec.data(), recoveredsize);

  ValueT val;
  // super slow, disabled for faster execution. I ran it once (took 30min), it was all fine.
  // for (size_t i = 0; i < vec.size(); i++) {
  //   val = codec.select(compressed_output.data(), i);
  //   if (val != vec[i]) throw std::runtime_error("bug!");
  // }

  for (int i = 0; i < vec.size(); i++) {
    ValueT a = vec[i];
    ValueT b = dec[i];
    if (a != b) throw std::runtime_error("bug!");
  }

  return 32.0 * static_cast<double>(compressed_output.size()) / static_cast<double>(vec.size());
}


SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(fastpfor);
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
// for the ones with select, also benchmark them without for comparison
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(simdframeofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(frameofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(for);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(maskedvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(streamvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(varint);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(vbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS(varintgb);


// with point access implemented, generated methods will follow the naming scheme with codec name "<codec>_with_select"
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(simdframeofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(frameofreference);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(for);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(maskedvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(streamvbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(varint);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(vbyte);
SIMDCOMPRESSIONANDINTERSECTION_BENCHMARK_METHODS_WITH_SELECT(varintgb);
} // namespace opposum