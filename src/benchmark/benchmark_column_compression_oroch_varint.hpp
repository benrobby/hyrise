

#include <iostream>
#include <memory>
#include "benchmark/benchmark.h"

#include "varint.h"

using ValueT = uint32_t;

namespace opossum {

void oroch_varint_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  size_t space = oroch::varint_codec<ValueT>::space(vec.begin(), vec.end());
  std::unique_ptr<uint8_t[]> enc(new uint8_t[space]);

  for (auto _ : state) {
    uint8_t* ptr = enc.get();

    oroch::varint_codec<ValueT>::encode(ptr, vec.begin(), vec.end());

    benchmark::ClobberMemory();
  }
}

void oroch_varint_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  size_t space = oroch::varint_codec<ValueT>::space(vec.begin(), vec.end());
  std::unique_ptr<uint8_t[]> enc(new uint8_t[space]);
  uint8_t* ptr = enc.get();
  oroch::varint_codec<ValueT>::encode(ptr, vec.begin(), vec.end());

  // Decode
  std::vector<ValueT> dec(vec.size());

  for (auto _ : state) {
    const uint8_t* src_ptr = enc.get();

    oroch::varint_codec<ValueT>::decode(dec.begin(), dec.end(), src_ptr);
    benchmark::ClobberMemory();
  }
}

float oroch_varint_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  size_t space = oroch::varint_codec<ValueT>::space(vec.begin(), vec.end());
  std::unique_ptr<uint8_t[]> enc(new uint8_t[space]);
  uint8_t* ptr = enc.get();
  oroch::varint_codec<ValueT>::encode(ptr, vec.begin(), vec.end());

  // Decode
  std::vector<ValueT> dec(vec.size());
  const uint8_t* src_ptr = enc.get();
  oroch::varint_codec<ValueT>::decode(dec.begin(), dec.end(), src_ptr);

  if (vec != dec) throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return 8.0 * static_cast<double>(space) / static_cast<double>(vec.size());
}

}  // namespace opossum