
#include <iostream>
#include "headers/codecfactory.h"
#include "benchmark/benchmark.h"

using namespace FastPForLib;
using ValueT = uint32_t;


namespace opossum {

#define FASTPFOR_BENCHMARK_METHODS(codec) \
  void fastPFOR_ ## codec ## _benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    fastPFOR_benchmark_encoding(vec, *CODECFactory::getFromName(#codec), state); \
  }; \
  void fastPFOR_ ## codec ## _benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) { \
    fastPFOR_benchmark_decoding(vec, *CODECFactory::getFromName(#codec), state); \
  }; \
  void fastPFOR_ ## codec ## _benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<ValueT>& pointIndices, benchmark::State& state) { \
    fastPFOR_benchmark_decoding_points(vec, pointIndices, *CODECFactory::getFromName(#codec), state); \
  }; \
  float fastPFOR_ ## codec ## _compute_bitsPerInt(std::vector<ValueT>& _vec) { \
     return fastPFOR_compute_bitsPerInt(_vec, *CODECFactory::getFromName(#codec)); \
  };

void fastPFOR_benchmark_encoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() * 2);

  benchmark::DoNotOptimize(enc.data());

  for (auto _ : state) {
    size_t compressedsize = enc.size();
    codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
    // if desired, shrink back the array:
    // enc.resize(compressedsize);
    // enc.shrink_to_fit();
    benchmark::ClobberMemory();
  }
}

void fastPFOR_benchmark_decoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() * 2);
  size_t compressedsize = enc.size();
  codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t recoveredsize = dec.size();

  benchmark::DoNotOptimize(dec.data());

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
    // dec.resize(recoveredsize);
    benchmark::ClobberMemory();
  }
}

void fastPFOR_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<ValueT>& pointIndices, IntegerCODEC &codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() * 2);
  size_t compressedsize = enc.size();
  codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t recoveredsize = dec.size();
  benchmark::DoNotOptimize(dec.data());

  std::vector<ValueT> points {};
  points.resize(pointIndices.size());
  benchmark::DoNotOptimize(points.data());

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
    for (size_t i = 0; i < pointIndices.size(); i++) {
      points[i] = dec[pointIndices[i]];
    }

    benchmark::ClobberMemory();
  }
}

float fastPFOR_compute_bitsPerInt(std::vector<ValueT>& _vec, IntegerCODEC &codec) {

  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() * 2);
  size_t compressedsize = enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(_vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
  dec.resize(recoveredsize);

  if (_vec != dec) {
    std::cerr << "bug in codec" << std::endl;
  }

  // # bits (encoded) / # elements to encode
  return 32.0 * static_cast<double>(enc.size()) / static_cast<double>(_vec.size());
}

FASTPFOR_BENCHMARK_METHODS(fastbinarypacking8);
FASTPFOR_BENCHMARK_METHODS(fastbinarypacking16);
FASTPFOR_BENCHMARK_METHODS(fastbinarypacking32);
FASTPFOR_BENCHMARK_METHODS(BP32);
FASTPFOR_BENCHMARK_METHODS(vsencoding);
FASTPFOR_BENCHMARK_METHODS(fastpfor128);
FASTPFOR_BENCHMARK_METHODS(fastpfor256);
FASTPFOR_BENCHMARK_METHODS(simdfastpfor128);
FASTPFOR_BENCHMARK_METHODS(simdfastpfor256);
FASTPFOR_BENCHMARK_METHODS(simplepfor);
FASTPFOR_BENCHMARK_METHODS(simdsimplepfor);
FASTPFOR_BENCHMARK_METHODS(pfor);
FASTPFOR_BENCHMARK_METHODS(simdpfor);
FASTPFOR_BENCHMARK_METHODS(pfor2008);
FASTPFOR_BENCHMARK_METHODS(simdnewpfor);
FASTPFOR_BENCHMARK_METHODS(newpfor);
FASTPFOR_BENCHMARK_METHODS(optpfor);
FASTPFOR_BENCHMARK_METHODS(simdoptpfor);
FASTPFOR_BENCHMARK_METHODS(varint);
FASTPFOR_BENCHMARK_METHODS(vbyte);
FASTPFOR_BENCHMARK_METHODS(maskedvbyte);
FASTPFOR_BENCHMARK_METHODS(streamvbyte);
FASTPFOR_BENCHMARK_METHODS(varintgb);
FASTPFOR_BENCHMARK_METHODS(simple16);
FASTPFOR_BENCHMARK_METHODS(simple9);
FASTPFOR_BENCHMARK_METHODS(simple9_rle);
FASTPFOR_BENCHMARK_METHODS(simple8b);
FASTPFOR_BENCHMARK_METHODS(simple8b_rle);
FASTPFOR_BENCHMARK_METHODS(varintg8iu);
FASTPFOR_BENCHMARK_METHODS(snappy);
FASTPFOR_BENCHMARK_METHODS(simdbinarypacking);
FASTPFOR_BENCHMARK_METHODS(simdgroupsimple);
FASTPFOR_BENCHMARK_METHODS(simdgroupsimple_ringbuf);
FASTPFOR_BENCHMARK_METHODS(copy);
}