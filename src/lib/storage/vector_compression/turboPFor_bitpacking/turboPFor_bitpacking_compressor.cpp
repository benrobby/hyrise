#include "turboPFor_bitpacking_compressor.hpp"

#include "conf.h"
#include "bitpack.h"
#include "math.h"

namespace opossum {

class TurboPForBitpackingVector;

std::unique_ptr<const BaseCompressedVector> TurboPForBitpackingCompressor::compress(
    const pmr_vector<uint32_t>& vector, const PolymorphicAllocator<size_t>& alloc,
    const UncompressedVectorInfo& meta_info) {
  
  
  auto data = pmr_vector<uint32_t>(alloc);

  if (vector.size() == 0) {
      data.resize(0);
      return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), 0);
  }

  data.resize(vector.size());

  for (int i = 0; i < vector.size(); i++) {
    data[i] = vector[i];
  }

  return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), 0);
}

std::unique_ptr<BaseVectorCompressor> TurboPForBitpackingCompressor::create_new() const {
  return std::make_unique<TurboPForBitpackingCompressor>();
}

}  // namespace opossum
