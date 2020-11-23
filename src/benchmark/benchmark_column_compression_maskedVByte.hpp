

#include <iostream>
#include "headers/codecfactory.h"
#include "benchmark/benchmark.h"

#include "varintdecode.h"
#include "varintencode.h"

using ValueT = uint32_t;

namespace opossum {

void maskedVbyte_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);

  uint8_t* compressedbuffer = (uint8_t*)malloc(N * sizeof(ValueT));

  for (auto _ : state) {
    vbyte_encode(datain, vec.size(), compressedbuffer);
  }
}

void maskedVbyte_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(N * sizeof(ValueT));
  vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
  ValueT* data_recovered = new ValueT[N];

  for (auto _ : state) {
    masked_vbyte_decode(compressedbuffer, data_recovered, N);
  }
}

float maskedVByte_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(N * sizeof(ValueT));
  vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  size_t computed_size = masked_vbyte_decode(compressedbuffer, data_recovered, N);

  return computed_size / N * 4;  // since computed size is givenout in byte
}

}