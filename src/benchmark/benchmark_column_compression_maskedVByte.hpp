

#include <iostream>
#include "headers/codecfactory.h"
#include "benchmark/benchmark.h"

#include "varintdecode.h"
#include "varintencode.h"

using ValueT = uint32_t;

namespace opossum {

void maskedVByte_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);

  uint8_t* compressedbuffer = (uint8_t*)malloc(2 * N * sizeof(ValueT));
  benchmark::DoNotOptimize(compressedbuffer);

  for (auto _ : state) {
    vbyte_encode(datain, vec.size(), compressedbuffer);
    benchmark::ClobberMemory();
  }
}

void maskedVByte_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 * N * sizeof(ValueT));
  vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  benchmark::DoNotOptimize(data_recovered);

  for (auto _ : state) {
    masked_vbyte_decode(compressedbuffer, data_recovered, N);
    benchmark::ClobberMemory();
  }
}

void maskedVByte_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 * N * sizeof(ValueT));
  vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  benchmark::DoNotOptimize(data_recovered);

  std::vector<ValueT> points {};
  points.resize(pointIndices.size());
  benchmark::DoNotOptimize(points.data());

  for (auto _ : state) {
    masked_vbyte_decode(compressedbuffer, data_recovered, N);
    for (size_t i = 0; i < pointIndices.size(); i++) {
      points[i] = data_recovered[pointIndices[i]];
    }

    benchmark::ClobberMemory();
  }
}

float maskedVByte_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 * N * sizeof(ValueT));
  size_t compressedBufferSize = vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  masked_vbyte_decode(compressedbuffer, data_recovered, N);

  return 8.0 * compressedBufferSize / N;
}
}