#pragma once

#define TURBOPFOR_DAC
#include "vp4.h"
#include "bitpack.h"

 #define ROUND_UP(_n_, _a_) (((_n_) + ((_a_)-1)) & ~((_a_)-1))
 #define ROUND_DOWN(_n_, _a_) (((_n_)) & ~((_a_)-1))
 #define P4NENC_BOUND(n, size) ((n + 127) / 128 + (n + 32) * (size))
 #define P4NDEC_BOUND(n, size) (ROUND_UP(n, 32) * (size))
 #define P4_BLOCK_SIZE 256


namespace opossum {

namespace turboPFOR {
    
typedef struct EncodedTurboPForVector {
    std::vector<unsigned char> compressedBuffer;
    std::vector<uint32_t> offsets;
    size_t size;

    size_t size_in_bytes() {
      return compressedBuffer.size() * sizeof(compressedBuffer[0])
       + offsets.size() * sizeof(offsets[0])
       + sizeof(size);
    }
} EncodedTurboPForVector;

typedef struct PointBasedCache {
    std::vector<p4> p4s;
    std::vector<uint32_t> bs;
    std::vector<unsigned char*> ins;
} PointBasedCache;


inline PointBasedCache calculateP4Ini(const EncodedTurboPForVector& e) {
  unsigned char* inPointer = const_cast<unsigned char*>(e.compressedBuffer.data());
  PointBasedCache cache;
  for (const auto &offset : e.offsets) {
      p4 p4;
      unsigned b;
      unsigned char* offsetIn = inPointer + offset;
      p4ini(&p4, &offsetIn, P4_BLOCK_SIZE, &b);
      cache.p4s.push_back(p4);
      cache.bs.push_back(b);
      cache.ins.push_back(offsetIn);
  }
  return cache;
}

inline std::vector<uint32_t> p4DecodeVectorSequential(const EncodedTurboPForVector& e) {
  size_t numElements = e.size;
  if (e.size == 0) {
    return std::vector<uint32_t>(0);
  }

  auto decodedVector = std::vector<uint32_t>(P4NDEC_BOUND(numElements, 1) + P4_BLOCK_SIZE);
  uint32_t *decoded_ptr = decodedVector.data();

  std::vector<uint32_t> offsets = e.offsets;
  const std::vector<unsigned char>& compressedBufferVec = e.compressedBuffer;
  uint8_t * p;
  p = (uint8_t*) compressedBufferVec.data();

  for (size_t i = 0; i < offsets.size() - 1; i++) {
      p4dec32(p + offsets[i], P4_BLOCK_SIZE, decoded_ptr + i * P4_BLOCK_SIZE);
  }

  decodedVector.resize(numElements);
  return decodedVector;
}

inline uint32_t p4GetValue(const EncodedTurboPForVector& e, size_t idx) {
  size_t numElements = e.size;

  const std::vector<unsigned char>& compressedBufferVec = e.compressedBuffer;
  uint8_t * p;
  p = (uint8_t*) compressedBufferVec.data();
  size_t offset_to_block = ROUND_DOWN(idx, P4_BLOCK_SIZE);
  p += e.offsets[offset_to_block/P4_BLOCK_SIZE];

  p4 p4;
  unsigned b;
  p4ini(&p4, &p, P4_BLOCK_SIZE, &b);

  uint32_t result = p4getx32(&p4, p, idx - offset_to_block, b);
  return result;
}

inline uint32_t p4GetValueNoInit(const EncodedTurboPForVector& e, const PointBasedCache& cache, size_t idx) {

  const std::vector<unsigned char>& compressedBufferVec = e.compressedBuffer;
  size_t offset_to_block = ROUND_DOWN(idx, P4_BLOCK_SIZE);
  size_t i = offset_to_block/P4_BLOCK_SIZE;

  auto p4 = cache.p4s[i];
  uint32_t b = cache.bs[i];
  unsigned char* p = cache.ins[i];

  uint32_t result = p4getx32(&p4, p, idx - offset_to_block, b);
  return result;
}

inline EncodedTurboPForVector p4EncodeVector(const std::vector<uint32_t>& vec) {
  size_t numElements = vec.size();
  size_t numElementsAligned = numElements + P4_BLOCK_SIZE; // todo better bounds

  std::vector<uint32_t> v1(vec);
  v1.resize(numElements + P4_BLOCK_SIZE); 
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

  compressedBufferVec.resize(out_ptr-compressedBufferVecPtr+P4_BLOCK_SIZE);
  EncodedTurboPForVector e;
  e.compressedBuffer = compressedBufferVec;
  e.offsets = offsets;
  e.size = numElements;

  auto dec = p4DecodeVectorSequential(e);
  for (int i = 0; i < vec.size(); i++) {
    auto d = dec[i];
    auto v = vec[i];
    if (d != v) {
      std::cout << "error" << std::endl;
      auto f = p4DecodeVectorSequential(e);
    }
  }

  for (int i = 0; i < vec.size(); i++) {
    auto d = p4GetValue(e, i);
    auto v = vec[i];
    if (d != v) {
      std::cout << "error" << std::endl;
      auto f = p4GetValue(e, i);
    }
  }

  return e;
}


}
}




