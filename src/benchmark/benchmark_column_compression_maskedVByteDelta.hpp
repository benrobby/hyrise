

#include <iostream>
#include "headers/codecfactory.h"
#include "benchmark/benchmark.h"

#include "varintdecode.h"
#include "varintencode.h"

using ValueT = uint32_t;

namespace opossum {

void maskedVByteDelta_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);

  uint8_t* compressedbuffer = (uint8_t*)malloc(2 *N * sizeof(ValueT));
  benchmark::DoNotOptimize(compressedbuffer);

  for (auto _ : state) {
    vbyte_encode_delta(datain, vec.size(), compressedbuffer, 0);
    benchmark::ClobberMemory();
  }
}

void maskedVByteDelta_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 *N * sizeof(ValueT));
  vbyte_encode_delta(datain, vec.size(), compressedbuffer, 0);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  benchmark::DoNotOptimize(data_recovered);

  for (auto _ : state) {
    masked_vbyte_decode_delta(compressedbuffer, data_recovered, N, 0);
    benchmark::ClobberMemory();
  }
}

void _maskedVByteDelta_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 *N * sizeof(ValueT));
  vbyte_encode_delta(datain, vec.size(), compressedbuffer, 0);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  benchmark::DoNotOptimize(data_recovered);

  std::vector<ValueT> points(pointIndices.size());
  benchmark::DoNotOptimize(points.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    masked_vbyte_decode_delta(compressedbuffer, data_recovered, N, 0);

    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += data_recovered[pointIndices[i]];
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = data_recovered[pointIndices[i]];
      }
    }

    benchmark::ClobberMemory();
	}
	if (sum) {
    std::cout << "";
  }
}

void maskedVByteDelta_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  _maskedVByteDelta_benchmark_decoding_points(vec, pointIndices, state, false);
}

void maskedVByteDelta_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  _maskedVByteDelta_benchmark_decoding_points(vec, pointIndices, state, true);
}

float maskedVByteDelta_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  int N = vec.size();
  ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*)malloc(2 *N * sizeof(ValueT));
  size_t compressedBufferSize = vbyte_encode_delta(datain, vec.size(), compressedbuffer, 0);

  // Decode
  ValueT* data_recovered = new ValueT[N];
  masked_vbyte_decode_delta(compressedbuffer, data_recovered, N, 0);

  return 8.0 * compressedBufferSize / N;
}

}