#pragma once
#define TURBOPFOR_DAC

#include "storage/vector_compression/base_vector_decompressor.hpp"


#include "include/codecfactory.h"
#include "include/intersection.h"
#include "include/frameofreference.h"
#include "bitpack.h"

#include "types.hpp"

namespace opossum {

class TurboPForBitpackingVector;

class TurboPForBitpackingDecompressor : public BaseVectorDecompressor {
 public:
  explicit TurboPForBitpackingDecompressor(const pmr_vector<uint32_t>& data, uint8_t b, size_t size) : _data{data}, _b{b}, _size{size} {
      SIMDCompressionLib::IntegerCODEC &codec = *SIMDCompressionLib::CODECFactory::getFromName("simdframeofreference");
      _codec = &codec;
  }
  TurboPForBitpackingDecompressor(const TurboPForBitpackingDecompressor& other) = default;
  TurboPForBitpackingDecompressor(TurboPForBitpackingDecompressor&& other) = default;

  TurboPForBitpackingDecompressor& operator=(const TurboPForBitpackingDecompressor& other) {
    DebugAssert(&_data == &other._data, "Cannot reassign TurboPForBitpackingDecompressor");
    return *this;
  }

  TurboPForBitpackingDecompressor& operator=(TurboPForBitpackingDecompressor&& other) {
    DebugAssert(&_data == &other._data, "Cannot reassign TurboPForBitpackingDecompressor");
    return *this;
  }

  ~TurboPForBitpackingDecompressor() override = default;

  uint32_t get(size_t i) final {
      const auto v1 = static_cast<uint32_t>(_codec->select(_data.data(), i));
      return v1;
  }

  size_t size() const final { return _size; }

 private:
  const pmr_vector<uint32_t>& _data;
  const size_t _size;
  const uint8_t _b;
  SIMDCompressionLib::IntegerCODEC *_codec;
};

}  // namespace opossum
