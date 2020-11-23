
#include <iostream>
#include "headers/codecfactory.h"
#include "benchmark/benchmark.h"

using namespace FastPForLib;
using ValueT = uint32_t;

namespace opossum {

void fastPFOR_benchmark_encoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() + 1024);

  for (auto _ : state) {
    size_t compressedsize = enc.size();
    codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
    // if desired, shrink back the array:
    enc.resize(compressedsize);
    enc.shrink_to_fit();
    benchmark::ClobberMemory();
  }
}

void fastPFOR_256_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  fastPFOR_benchmark_encoding(vec, *CODECFactory::getFromName("fastpfor256"), state);
}

void fastPFOR_benchmark_decoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t recoveredsize = dec.size();

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
    dec.resize(recoveredsize);
    benchmark::ClobberMemory();
  }
}

void fastPFOR_256_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  fastPFOR_benchmark_decoding(vec, *CODECFactory::getFromName("fastpfor256"), state);
}

float fastPFOR_compute_bitsPerInt(std::vector<ValueT>& _vec) {
  IntegerCODEC& codec = *CODECFactory::getFromName("fastpfor256");

  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(_vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
  dec.resize(recoveredsize);

  if (_vec != dec) throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return 32.0 * static_cast<double>(enc.size()) / static_cast<double>(_vec.size());
}


}