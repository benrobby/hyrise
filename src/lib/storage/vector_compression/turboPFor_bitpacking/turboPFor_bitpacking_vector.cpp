#include "turboPFor_bitpacking_vector.hpp"

namespace opossum {

TurboPForBitpackingVector::TurboPForBitpackingVector(pmr_vector<uint32_t>&& data, size_t size, uint8_t b) : _data{std::move(data)}, _size{size}, _b{b} {}

size_t TurboPForBitpackingVector::on_size() const { return _size; }
size_t TurboPForBitpackingVector::on_data_size() const { return sizeof(uint32_t) * _data.size(); }

std::unique_ptr<BaseVectorDecompressor> TurboPForBitpackingVector::on_create_base_decompressor() const {
  return std::make_unique<TurboPForBitpackingDecompressor>(_data, _b, _size);
}

TurboPForBitpackingDecompressor TurboPForBitpackingVector::on_create_decompressor() const { return TurboPForBitpackingDecompressor(_data, _b, _size); }

TurboPForBitpackingIterator TurboPForBitpackingVector::on_begin() const { return TurboPForBitpackingIterator{_data, _b,_size, 0u}; }

TurboPForBitpackingIterator TurboPForBitpackingVector::on_end() const { return TurboPForBitpackingIterator{_data, _b, _size, _size}; }

std::unique_ptr<const BaseCompressedVector> TurboPForBitpackingVector::on_copy_using_allocator(
    const PolymorphicAllocator<size_t>& alloc) const {
  pmr_vector<uint32_t> data_copy(_data, alloc);
  return std::make_unique<TurboPForBitpackingVector>(std::move(data_copy), _size, _b);
}

}  // namespace opossum
