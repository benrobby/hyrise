#include "bitpacking_segment.hpp"

#include <algorithm>

#include "resolve_type.hpp"
#include "utils/performance_warning.hpp"
#include "utils/size_estimation_utils.hpp"

namespace opossum {

template <typename T, typename U>
BitpackingSegment<T, U>::BitpackingSegment(const std::shared_ptr<pmr_compact_vector<uint32_t>> encoded_values, std::optional<pmr_vector<bool>> null_values)
    : AbstractEncodedSegment(data_type_from_type<T>()),
      _encoded_values{encoded_values},
      _null_values{std::move(null_values)} { }

template <typename T, typename U>
const std::shared_ptr<pmr_compact_vector<uint32_t>> BitpackingSegment<T, U>::encoded_values() const {
  return _encoded_values;
}

template <typename T, typename U>
const std::optional<pmr_vector<bool>>& BitpackingSegment<T, U>::null_values() const {
  return _null_values;
}

template <typename T, typename U>
AllTypeVariant BitpackingSegment<T,U>::operator[](const ChunkOffset chunk_offset) const {
  PerformanceWarning("operator[] used");
  const auto typed_value = get_typed_value(chunk_offset);
  if (!typed_value) {
    return NULL_VALUE;
  }
  return *typed_value;
}

template <typename T, typename U>
ChunkOffset BitpackingSegment<T,U>::size() const {
  return _encoded_values->size();
}

template <typename T, typename U>
std::shared_ptr<AbstractSegment> BitpackingSegment<T,U>::copy_using_allocator(
  const PolymorphicAllocator<size_t>& alloc) const {
  
  auto copied_data = std::make_shared<pmr_compact_vector<uint32_t>>(_encoded_values->bits(), alloc);
  copied_data->resize(_encoded_values->size());
  for (int i = 0; i < _encoded_values->size(); i++) {
    (*copied_data)[i] = (*_encoded_values)[i];
  }

  std::optional<pmr_vector<bool>> new_null_values;
  if (_null_values) {
    new_null_values = pmr_vector<bool>(*_null_values, alloc);
  }
  auto copy = std::make_shared<BitpackingSegment<T,U>>(copied_data, std::move(new_null_values));

  copy->access_counter = access_counter;

  return copy;
}

template <typename T, typename U>
size_t BitpackingSegment<T,U>::memory_usage([[maybe_unused]] const MemoryUsageCalculationMode mode) const {
  size_t segment_size = sizeof(*this);
  if (_null_values) {
    segment_size += _null_values->capacity() / CHAR_BIT;
  }
  segment_size += _encoded_values->bytes();
  return segment_size;
}

template <typename T, typename U>
EncodingType BitpackingSegment<T,U>::encoding_type() const {
  return EncodingType::Bitpacking;
}

template <typename T, typename U>
std::optional<CompressedVectorType> BitpackingSegment<T,U>::compressed_vector_type() const {
  return std::nullopt;
}

template class BitpackingSegment<int32_t>;
// int64_t disabled for now, todo enable
// template class FrameOfReferenceSegment<int64_t>;

}  // namespace opossum