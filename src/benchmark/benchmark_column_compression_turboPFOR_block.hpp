
#include <iostream>
#include "benchmark/benchmark.h"
#include "streamvbyte.h"
#include "vp4.h"

#define BLOCK_SIZE 128

using ValueT = uint32_t;

namespace opossum {

// This blockwise access is probably not really worthwile compared to direct access. It's around two
// times faster for an vector of size 65'000. But more than 10 times slower for direct access.

class TurboPFORWrapper {
 public:
  TurboPFORWrapper(int size) {
    blocks = std::vector<unsigned char*>(size / BLOCK_SIZE + 1);
    outBuffer = (unsigned char*) malloc(blocks.size() * BLOCK_SIZE * 4);
    this->size = size;
  }

  void enc(const std::vector<ValueT> &vec) {
    assert(vec.size() == size);
    ValueT* inData = (ValueT*) vec.data();

    int numberOfBlocks = size / BLOCK_SIZE + 1;
    blocks[0] = outBuffer;
    for (int i = 0; i < numberOfBlocks; i++) {
      blocks[i+1] = p4enc32(&inData[BLOCK_SIZE * i], BLOCK_SIZE, blocks[i]);
    }
  }

  size_t sizeInBytes(){
    int bufferSize = (blocks[blocks.size()-1] - blocks[0]) * sizeof(unsigned char);
    int blockSize = blocks.size() * sizeof(unsigned char*);
    return bufferSize + blockSize + sizeof(unsigned int);
  }

  ValueT get(int i) {
    int blockNumber = i / BLOCK_SIZE;
    int position = i % BLOCK_SIZE;
    ValueT* decompressedData = (ValueT*) malloc(BLOCK_SIZE * sizeof(ValueT));
    p4dec32(blocks[blockNumber], BLOCK_SIZE, decompressedData);
    return decompressedData[position];
  }

  void getAll(ValueT* decompressedDataBuffer){
    p4ndec32(outBuffer, blocks.size() * BLOCK_SIZE, decompressedDataBuffer);
  }

 private:
  unsigned char* outBuffer;
  std::vector<unsigned char*> blocks;
  unsigned int size;
};

void turboPFOR_block_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  TurboPFORWrapper turboPfor = TurboPFORWrapper(vec.size());
  benchmark::DoNotOptimize(turboPfor);

  for (auto _ : state) {
    turboPfor.enc(vec);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_block_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  TurboPFORWrapper turboPfor = TurboPFORWrapper(vec.size());
  turboPfor.enc(vec);

  // Decode
  ValueT* decompressedData = (ValueT*) malloc((vec.size()+BLOCK_SIZE) * sizeof(ValueT));
  benchmark::DoNotOptimize(decompressedData);

  for (auto _ : state) {
    turboPfor.getAll(decompressedData);
    benchmark::ClobberMemory();
  }
}

void turboPFOR_block_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<ValueT>& pointIndices, benchmark::State& state) {
  // Encode
  TurboPFORWrapper turboPfor = TurboPFORWrapper(vec.size());
  turboPfor.enc(vec);

  // Decode
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  benchmark::DoNotOptimize(points);

  for (auto _ : state) {
    for (size_t i: pointIndices) {
      points[i] = turboPfor.get(i);
    }
    benchmark::ClobberMemory();
  }
}

float turboPFOR_block_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  TurboPFORWrapper turboPfor = TurboPFORWrapper(vec.size());
  turboPfor.enc(vec);

  // Decode
  std::vector<ValueT> points = std::vector<ValueT>(vec.size());
  ValueT* decompressedData = (ValueT*) malloc((vec.size()+BLOCK_SIZE) * sizeof(ValueT));
  turboPfor.getAll(decompressedData);
  for (size_t i = 0; i < vec.size(); i++){
    points[i] = turboPfor.get(i);
  }

  for (size_t i = 0; i < vec.size(); i++) {
    if (decompressedData[i] != vec[i] || points[i] != vec[i]){
      throw std::runtime_error("bug!");
    }
  }

  // # bits (encoded) / # elements to encode
  return turboPfor.sizeInBytes() * 8.0 / vec.size();
}

}  // namespace opossum