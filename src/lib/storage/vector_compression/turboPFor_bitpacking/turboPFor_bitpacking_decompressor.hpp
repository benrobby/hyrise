#pragma once
#define TURBOPFOR_DAC

#include "storage/vector_compression/base_vector_decompressor.hpp"

#include "bitpack.h"

#include "types.hpp"

namespace opossum {

class TurboPForBitpackingVector;

class TurboPForBitpackingDecompressor : public BaseVectorDecompressor {
 public:
  explicit TurboPForBitpackingDecompressor(const pmr_vector<uint8_t>& data, uint8_t b) : _data{data}, _b{b} {}
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
    return bitgetx32(_data.data(), i, _b);
  }

  size_t size() const final { return _size; }

 private:
  const pmr_vector<uint8_t>& _data;
  size_t _size;
  uint8_t _b;
};

}  // namespace opossum
