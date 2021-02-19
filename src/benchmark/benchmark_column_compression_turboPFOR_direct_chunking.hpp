
#include <iostream>
#include "benchmark/benchmark.h"

#include "vp4.h"

using ValueT = uint32_t;

namespace opossum {


#define ROUND_UP(_n_, _a_) (((_n_) + ((_a_)-1)) & ~((_a_)-1))
#define ROUND_DOWN(_n_, _a_) (((_n_)) & ~((_a_)-1))
#define P4NENC_BOUND(n, size) ((n + 127) / 128 + (n + 32) * (size))
#define P4NDEC_BOUND(n, size) (ROUND_UP(n, 32) * (size))
#define P4_BLOCK_SIZE 256
typedef struct encodedTurboPForVector {
  std::vector<unsigned char> compressedBuffer;
  std::vector<uint32_t> offsets;
  size_t size;
} encodedTurboPForVector;

encodedTurboPForVector p4EncodeVector(const std::vector<uint32_t> vec) {
  size_t numElements = vec.size();
  size_t numElementsAligned = numElements + P4_BLOCK_SIZE; // todo better bounds

  std::vector<uint32_t> v1(vec);
  v1.resize(numElements + P4_BLOCK_SIZE + 32); 
  // lib apparently has some out of bounds accesses that we want to catch with this. 
  // also ensure that we have a multiple of P4_BLOCK_SIZE values in here so we can compress full blocks only.

  uint32_t* inData = (uint32_t*) v1.data();

  std::vector<unsigned char> compressedBufferVec(P4NENC_BOUND(v1.size(), sizeof(uint32_t)));
  uint8_t *out_ptr, *compressedBufferVecPtr;
  out_ptr = (uint8_t*) compressedBufferVec.data();
  compressedBufferVecPtr = out_ptr;

  std::vector<uint32_t> offsets;
  offsets.push_back(0);
  for(size_t i = 0; i < (numElementsAligned&~(P4_BLOCK_SIZE - 1)); i += P4_BLOCK_SIZE) { 
    uint8_t* next_ptr = p4encx32(inData + i, P4_BLOCK_SIZE, out_ptr);
    offsets.push_back(next_ptr - compressedBufferVecPtr);
    out_ptr = next_ptr;
    // todo: increment out_ptr?
  }
  

  encodedTurboPForVector e;
  e.compressedBuffer = compressedBufferVec;
  e.offsets = offsets;
  e.size = numElements;

  return e;
}

std::vector<uint32_t> p4DecodeVectorSequential(encodedTurboPForVector *e) {
  size_t numElements = e->size;
  auto decodedVector = std::vector<uint32_t>( P4NDEC_BOUND(numElements, 1) );
  uint32_t *decoded_ptr = decodedVector.data();
  
  std::vector<uint32_t> offsets = e->offsets;
  std::vector<unsigned char> compressedBufferVec = e->compressedBuffer;
  uint8_t * p;
  p = (uint8_t*) compressedBufferVec.data();

  for (size_t i = 0; i < offsets.size() - 1; i++) {
    p4dec32(p + offsets[i], P4_BLOCK_SIZE, decoded_ptr + i * P4_BLOCK_SIZE);
  }

  decodedVector.resize(numElements);
  return decodedVector;
}

uint32_t p4GetVectorIndex(encodedTurboPForVector *e, size_t idx) {
  size_t numElements = e->size;

  std::vector<unsigned char> compressedBufferVec = e->compressedBuffer;
  uint8_t * p;
  p = (uint8_t*) compressedBufferVec.data();
  size_t offset_to_block = ROUND_DOWN(idx, P4_BLOCK_SIZE);
  p += e->offsets[offset_to_block/P4_BLOCK_SIZE];

  p4 p4;
  unsigned b;
  p4ini(&p4, &p, P4_BLOCK_SIZE, &b);

  uint32_t result = p4getx32(&p4, p, idx - offset_to_block, b);
  return result;
}



// -------------- BENCHMARKS ---------------


void turboPFOR_direct_chunking_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  
  encodedTurboPForVector e;
  benchmark::DoNotOptimize(e);

  for (auto _ : state) {

    e = p4EncodeVector(vec);

    benchmark::ClobberMemory();
  }
}


void turboPFOR_direct_chunking_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  
  // Encode
  encodedTurboPForVector e = p4EncodeVector(vec);


  for (auto _ : state) {

    auto decodedVector = p4DecodeVectorSequential(&e);
    benchmark::DoNotOptimize(decodedVector.data());
    benchmark::ClobberMemory();
  }
  // Decode
  

  // if (decodedVector != vec) {
  //   for (size_t i = 0; i < vec.size(); i++) {
  //     if (decodedVector[i] != vec[i]) {
  //       std::cout << "not equal" << std::endl;
  //     }
  //   }
  // }

}


void _turboPFOR_direct_chunking_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
    
  encodedTurboPForVector e = p4EncodeVector(vec);


  // Decode
  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  benchmark::DoNotOptimize(points);

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  // for (size_t i = 0; i < vec.size(); i++) {
  //   if (p4GetVectorIndex(&e, i) != vec[i]) {
  //     std::cout << "not equal" << std::endl;
  //   }
  // }

  size_t numElements = e.size;

  std::vector<unsigned char> compressedBufferVec = e.compressedBuffer;
  uint8_t * p;
  p = (uint8_t*) compressedBufferVec.data();
  uint8_t *p1;
  auto offsets = e.offsets;
  size_t idx;

  for (auto _ : state) {
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {

        
        idx = pointIndices[i];
        size_t offset_to_block = ROUND_DOWN(idx, P4_BLOCK_SIZE);
        p1 = p + offsets[offset_to_block >> 8];

        p4 p4;
        unsigned b;
        p4ini(&p4, &p1, P4_BLOCK_SIZE, &b);

        sum += p4getx32(&p4, p1, idx - offset_to_block, b);
      }
    } else {

      for (size_t i = 0; i < pointIndices.size(); i++) {
        idx = pointIndices[i];

        size_t offset_to_block = ROUND_DOWN(idx, P4_BLOCK_SIZE);
        p1 = p + offsets[offset_to_block >> 8];

        p4 p4;
        unsigned b;
        p4ini(&p4, &p1, P4_BLOCK_SIZE, &b);

        points[i] = p4getx32(&p4, p1, idx - offset_to_block, b);
      }
    }
    benchmark::ClobberMemory();
	}
	if (sum) {
    std::cout << "";
  }
}

void turboPFOR_direct_chunking_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_direct_chunking_benchmark_decoding_points(vec, pointIndices, state, false);
}
void turboPFOR_direct_chunking_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _turboPFOR_direct_chunking_benchmark_decoding_points(vec, pointIndices, state, true);
}

float turboPFOR_direct_chunking_compute_bitsPerInt(std::vector<ValueT>& vec) {
  // Encode
  unsigned char* outBuffer = (unsigned char*) malloc(vec.size()*4*sizeof(uint32_t));

  std::vector<ValueT> v1(vec);
  v1.resize(vec.size() + 32);
  ValueT* inData = (ValueT*) v1.data();

  unsigned char* end = p4encx32(inData, vec.size(), outBuffer);

  // Decode by point
  unsigned int n = vec.size();
  std::vector<ValueT> points = std::vector<ValueT>(n);
  p4 p4;
  unsigned b;
  unsigned char* pointerCopy = outBuffer;
  p4ini(&p4, &pointerCopy, n, &b);
  for (size_t i = 0; i < vec.size(); i++) {
    points[i] = p4getx32(&p4, pointerCopy, i, b);
  }

  // Decode whole buffer
  ValueT* decompressedData = (ValueT*) malloc(vec.size() * sizeof(ValueT));
  p4dec32(outBuffer, n, decompressedData);


  for (size_t i = 0; i < vec.size(); i++) {
    if (decompressedData[i] != vec[i] || points[i] != vec[i]){
      throw std::runtime_error("bug!");
    }
  }

  // # bits (encoded) / # elements to encode
  int size_in_bytes = (end - outBuffer) * sizeof(unsigned char);
  return size_in_bytes * 8.0 / vec.size();
}

}  // namespace opossum