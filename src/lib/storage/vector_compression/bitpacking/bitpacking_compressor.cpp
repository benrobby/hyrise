#include "bitpacking_compressor.hpp"
#include "compact_vector.hpp"
#include "compact_static_vector.hpp"
#include <algorithm>
#include <math.h>

namespace opossum {

class BitpackingVector;

std::unique_ptr<const BaseCompressedVector> BitpackingCompressor::compress(
    const pmr_vector<uint32_t>& vector, const PolymorphicAllocator<size_t>& alloc,
    const UncompressedVectorInfo& meta_info) {
 
  const auto max_value = _find_max_value(vector);
  uint32_t upper_bound = log2(max_value + 1) + 1;
  uint32_t b = std::min(upper_bound, std::max(compact::vector<unsigned int, 32>::required_bits(max_value), 1u));

  CompactStaticVector data(b, alloc);
  for (uint32_t value : vector) {
    data.push_back(value);
  }
  return std::make_unique<BitpackingVector>(data);

}

std::unique_ptr<BaseVectorCompressor> BitpackingCompressor::create_new() const {
  return std::make_unique<BitpackingCompressor>();
}

uint32_t BitpackingCompressor::_find_max_value(const pmr_vector<uint32_t>& vector) const {
  uint32_t max = 0;
  for (const auto v : vector) {
    max |= v;
  }
  return max;
}


}  // namespace opossum
