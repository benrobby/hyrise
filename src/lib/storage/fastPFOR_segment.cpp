#include "fastPFOR_segment.hpp"

#include <algorithm>

#include "resolve_type.hpp"
#include "utils/assert.hpp"
#include "utils/performance_warning.hpp"
#include "utils/size_estimation_utils.hpp"

namespace opossum {

template <typename T, typename U>
FastPFORSegment<T, U>::FastPFORSegment(const std::shared_ptr<const pmr_vector<uint32_t>>& encoded_values,
                                      const std::shared_ptr<const pmr_vector<bool>>& null_values,
                                      const uint8_t codec_id)
    : AbstractEncodedSegment(data_type_from_type<T>()),
      _encoded_values{encoded_values},
      _null_values{null_values},
      _codec_id{codec_id} {}

template <typename T, typename U>
std::shared_ptr<const pmr_vector<uint32_t>> FastPFORSegment<T, U>::encoded_values() const {
  return _encoded_values;
}

template <typename T, typename U>
std::shared_ptr<const pmr_vector<bool>> FastPFORSegment<T, U>::null_values() const {
  return _null_values;
}

template <typename T, typename U>
AllTypeVariant FastPFORSegment<T,U>::operator[](const ChunkOffset chunk_offset) const {
  PerformanceWarning("operator[] used");
  const auto typed_value = get_typed_value(chunk_offset);
  if (!typed_value) {
    return NULL_VALUE;
  }
  return *typed_value;
}

template <typename T, typename U>
ChunkOffset FastPFORSegment<T,U>::size() const {
  return _null_values->size();
}

template <typename T, typename U>
std::shared_ptr<AbstractSegment> FastPFORSegment<T,U>::copy_using_allocator(
    const PolymorphicAllocator<size_t>& alloc) const {
  auto new_encoded_values = std::make_shared<pmr_vector<uint32_t>>(*_encoded_values, alloc);
  auto new_null_values = std::make_shared<pmr_vector<bool>>(*_null_values, alloc);

  auto copy = std::make_shared<FastPFORSegment<T,U>>(new_encoded_values, new_null_values, _codec_id);

  copy->access_counter = access_counter;

  return copy;
}

template <typename T, typename U>
size_t FastPFORSegment<T,U>::memory_usage([[maybe_unused]] const MemoryUsageCalculationMode mode) const {
  return
          sizeof(*this) +
          _null_values->capacity() / CHAR_BIT +
          _encoded_values->capacity() * sizeof(uint32_t) +
          1;
}

template <typename T, typename U>
EncodingType FastPFORSegment<T,U>::encoding_type() const {
  return EncodingType::FastPFOR;
}

template <typename T, typename U>
std::optional<CompressedVectorType> FastPFORSegment<T,U>::compressed_vector_type() const {
  return std::nullopt;
}

template class FastPFORSegment<int32_t>;
// int64_t disabled for now, todo enable
// template class FrameOfReferenceSegment<int64_t>;

}  // namespace opossum
