
#include <iostream>
#include "benchmark/benchmark.h"

#include "bitpack.h"
#include "conf.h"
#include "math.h"

using ValueT = uint32_t;

namespace opossum {


// -------------- BENCHMARKS ---------------


void turboPFOR_bitcompression_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  
  
  std::vector<uint32_t> in(vec);
  in.resize(in.size() + 4096);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 4096);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = log2(max +1) + 1;
  benchmark::DoNotOptimize(compressedBufVec.data());

  for (auto _ : state) {
    bitpack32(in.data(), vec.size(), compressedBufVec.data(), b);
    benchmark::ClobberMemory();
  }

}


void turboPFOR_bitcompression_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  std::vector<uint32_t> in(vec);
  in.resize(vec.size() + 4096);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 4096);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = log2(max +1) + 1;

  bitpack32(in.data(), vec.size(), compressedBufVec.data(), b);

  std::vector<uint32_t> dec(vec.size() + 4096);

  benchmark::DoNotOptimize(dec.data());

  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  // auto sum = 0;
  // benchmark::DoNotOptimize(sum);

  for (auto _ : state) {

    bitunpack32(compressedBufVec.data(), vec.size(), dec.data(), b);

    // for (int i = 0; i < in.size(); i++) {
    //   points[i] = dec[i];
    //   if (dec[i] != in[i]) {
    //     std::cout << "not equal" << dec[i] << "    " << in[i] << std::endl;
    //   }
    // }

    benchmark::ClobberMemory();
    // std::fill(points.begin(), points.end(), 0);
    //sum = 0;
  }
}


void _turboPFOR_bitcompression_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, int mode) {
  std::vector<uint32_t> in(vec);

  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 4096);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = log2(max +1) + 1;

  bitpack32(in.data(), in.size(), compressedBufVec.data(), b);

  std::vector<uint32_t> dec(in.size());

  benchmark::DoNotOptimize(dec.data());

  uint32_t val;

  std::vector<ValueT> points = std::vector<ValueT>(pointIndices.size());
  benchmark::DoNotOptimize(points);

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    switch (mode) {
      case 0: {
        for (size_t i = 0; i < pointIndices.size(); i++) {
          val = bitgetx32(compressedBufVec.data(), pointIndices[i], b);
          sum += val;
          // if (val != in[pointIndices[i]]) {
          //   std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
          // }
        }
        break;
      }
      case 1: {
        for (size_t i = 0; i < pointIndices.size(); i++) {
          val = bitgetx32(compressedBufVec.data(), pointIndices[i], b);
          points[i] = val;
          if (val != in[pointIndices[i]]) {
            std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
          }
        }
        break;
      } case 2: {
        std::vector<uint32_t> dec(vec.size() + 4096);
        bitunpack32(compressedBufVec.data(), vec.size(), dec.data(), b);
        for (size_t i = 0; i < pointIndices.size(); i++) {
          val = dec[pointIndices[i]];
          sum += val;
          // if (val != in[pointIndices[i]]) {
          //   std::cout << "not equal" << val << "    " << in[pointIndices[i]] << std::endl;
          // }
        }
      }
    }
    
    benchmark::ClobberMemory();
	}
	if (sum) {
    std::cout << "";
  }
 
}

void turboPFOR_bitcompression_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_bitcompression_benchmark_decoding_points(vec, pointIndices, state, 1);
}
void turboPFOR_bitcompression_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_bitcompression_benchmark_decoding_points(vec, pointIndices, state, 0);
}
void turboPFOR_bitcompression_benchmark_decoding_points_seq(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_bitcompression_benchmark_decoding_points(vec, pointIndices, state, 2);
}

float turboPFOR_bitcompression_compute_bitsPerInt(std::vector<ValueT>& vec) {
  std::vector<uint32_t> in(vec);
  in.resize(vec.size() + 4096);
  std::vector<unsigned char> compressedBufVec(vec.size() * sizeof(uint32_t) + 1024);
  uint32_t max = 0;

  for (const auto v : in) {
    max |= v;
  }

  uint32_t b = log2(max +1) + 1;
  benchmark::DoNotOptimize(compressedBufVec.data());

  unsigned char * end = bitpack32(in.data(), vec.size(), compressedBufVec.data(), b);

  auto length_bytes = end - ((unsigned char *) compressedBufVec.data());

  std::cout << "BITCOMPRESSION: " << b << " " << length_bytes << " " << vec.size() << std::endl;

  return (length_bytes * 8.0) / vec.size();
}

}  // namespace opossum