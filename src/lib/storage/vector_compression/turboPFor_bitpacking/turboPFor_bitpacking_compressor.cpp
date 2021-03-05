#include "turboPFor_bitpacking_compressor.hpp"

#include "conf.h"
#include "bitpack.h"
#include "math.h"
#include <cmath>

namespace opossum {

class TurboPForBitpackingVector;

std::unique_ptr<const BaseCompressedVector> TurboPForBitpackingCompressor::compress(
    const pmr_vector<uint32_t>& vector, const PolymorphicAllocator<size_t>& alloc,
    const UncompressedVectorInfo& meta_info) {
  
  auto data = pmr_vector<uint8_t>(alloc);
  data.resize(vector.size() * sizeof(uint32_t) + 1024);

  pmr_vector<uint32_t> in(vector);

  const auto max_value = _find_max_value(vector);
  
  uint32_t b;
  if (max_value <= 0) {
    b = 1;
  } else if (max_value == 1) {
    b = 1;
  } else {
    b = std::ceil(log2(max_value + 1));
  }

  if (vector.size() == 0) {
      data.resize(0);
      return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), 0);
  }

  uint8_t * out_end = bitpack32(in.data(), in.size(), data.data(), b);
  int bytes_written = (out_end) - data.data();

  data.resize(bytes_written + 32);

  const uint8_t b_1 = static_cast<uint8_t>(b);

  return std::make_unique<TurboPForBitpackingVector>(std::move(data), vector.size(), b_1);
}

std::unique_ptr<BaseVectorCompressor> TurboPForBitpackingCompressor::create_new() const {
  return std::make_unique<TurboPForBitpackingCompressor>();
}

uint32_t TurboPForBitpackingCompressor::_find_max_value(const pmr_vector<uint32_t>& vector) const {
  uint32_t max = 0;
  for (const auto v : vector) {
    max |= v;
  }
  return max;
}


}  // namespace opossum
