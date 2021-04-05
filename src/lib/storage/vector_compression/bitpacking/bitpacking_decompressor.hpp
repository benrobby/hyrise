#pragma once

#include "storage/vector_compression/base_vector_decompressor.hpp"

#include "compact_vector.hpp"
#include "bitpacking_vector_type.hpp"

namespace opossum {

class BitpackingVector;

class BitpackingDecompressor : public BaseVectorDecompressor {
 public:
  explicit BitpackingDecompressor(const pmr_bitpacking_vector<uint32_t>& data) : _data{data} {}
  BitpackingDecompressor(const BitpackingDecompressor& other) = default;
  BitpackingDecompressor(BitpackingDecompressor&& other) = default;

  BitpackingDecompressor& operator=(const BitpackingDecompressor& other) {
    DebugAssert(&_data == &other._data, "Cannot reassign BitpackingDecompressor");
    return *this;
  }
  BitpackingDecompressor& operator=(BitpackingDecompressor&& other) {
    DebugAssert(&_data == &other._data, "Cannot reassign BitpackingDecompressor");
    return *this;
  }

  ~BitpackingDecompressor() override = default;

  uint32_t get(size_t i) final { return _data[i]; }

  size_t size() const final { return _data.size(); }

 private:
  const pmr_bitpacking_vector<uint32_t>& _data;
};

}  // namespace opossum
