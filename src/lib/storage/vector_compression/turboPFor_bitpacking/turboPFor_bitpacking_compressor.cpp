#include "turboPFor_bitpacking_compressor.hpp"

#include "conf.h"
#include "bitpack.h"
#include "math.h"

namespace opossum {

class TurboPForBitpackingVector;

std::unique_ptr<const BaseCompressedVector> TurboPForBitpackingCompressor::compress(
    const pmr_vector<uint32_t>& vector, const PolymorphicAllocator<size_t>& alloc,
    const UncompressedVectorInfo& meta_info) {
  
  SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("simdframeofreference");

  
  auto data = pmr_vector<uint32_t>(alloc);

  if (vector.size() == 0) {
      data.resize(0);
      return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), 0);
  }

  data.resize(2 * vector.size() + 1024);

  auto encodedValuesSize = data.size();
  pmr_vector<uint32_t> in(vector);
  codec.encodeArray(in.data(), vector.size(), data.data(), encodedValuesSize);


  data.resize(encodedValuesSize);
  data.shrink_to_fit();

  return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), 0);
}

std::unique_ptr<BaseVectorCompressor> TurboPForBitpackingCompressor::create_new() const {
  return std::make_unique<TurboPForBitpackingCompressor>();
}

}  // namespace opossum
