#pragma once

#include <array>
#include <memory>

#include "storage/vector_compression/base_compressed_vector.hpp"

#include "turboPFor_bitpacking_decompressor.hpp"

#include "types.hpp"
#include "utils/performance_warning.hpp"

namespace opossum {

class TurboPForBitpackingIterator : public BaseCompressedVectorIterator<TurboPForBitpackingIterator> {

 public:
  explicit TurboPForBitpackingIterator(const pmr_vector<uint32_t>& data, uint8_t b, const size_t size, const size_t absolute_index = 0u)
    : _data{data}, _absolute_index{absolute_index}, _b{b} {
      _decompressed_data = std::vector<uint32_t>(size * 2 + 2048);
      SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("simdframeofreference");
      size_t recovered_size = size;
      if (size > 0) {
        codec.decodeArray(data.data(), data.size(), _decompressed_data.data(), recovered_size);
      }
    }

  TurboPForBitpackingIterator(const TurboPForBitpackingIterator& other) = default;
  TurboPForBitpackingIterator(TurboPForBitpackingIterator&& other) = default;

  TurboPForBitpackingIterator& operator=(const TurboPForBitpackingIterator& other) {
    DebugAssert(&_data == &other._data, "Cannot reassign TurboPForBitpackingIterator");
    _absolute_index = other._absolute_index;
    return *this;
  }

  TurboPForBitpackingIterator& operator=(TurboPForBitpackingIterator&& other){
    DebugAssert(&_data == &other._data, "Cannot reassign TurboPForBitpackingIterator");
    _absolute_index = other._absolute_index;
    return *this;
  }

  ~TurboPForBitpackingIterator() = default;

 private:
  friend class boost::iterator_core_access;  // grants the boost::iterator_facade access to the private interface

  void increment() { ++_absolute_index; }

  void decrement() { --_absolute_index; }

  void advance(std::ptrdiff_t n) { _absolute_index += n; }

  bool equal(const TurboPForBitpackingIterator& other) const {
    return _absolute_index == other._absolute_index;
  }

  std::ptrdiff_t distance_to(const TurboPForBitpackingIterator& other) const {
    return other._absolute_index - _absolute_index;
  }

  uint32_t dereference() const { 
    return _decompressed_data[_absolute_index]; 
  }


 private:
  const pmr_vector<uint32_t>& _data;
  std::vector<uint32_t> _decompressed_data;
  size_t _absolute_index;
  const uint8_t _b;
};

}  // namespace opossum
