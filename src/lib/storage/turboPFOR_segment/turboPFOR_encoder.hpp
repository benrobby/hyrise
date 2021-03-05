#pragma once

#include <memory>

#include "storage/base_segment_encoder.hpp"
#include "vector_types.hpp"
#include <fstream>
#include <limits>

namespace opossum {

class TurboPFOREncoder : public SegmentEncoder<TurboPFOREncoder> {

 public:
  static constexpr auto _encoding_type = enum_c<EncodingType, EncodingType::TurboPFOR>;
  static constexpr auto _uses_vector_compression = false;

  template <typename T>
  std::shared_ptr<AbstractEncodedSegment> _on_encode(const AnySegmentIterable<T> segment_iterable,
                                                     const PolymorphicAllocator<T>& allocator) {

    auto values = std::vector<uint32_t>(); // destroy it when out of scope, only used to get values in continuous mem
    auto null_values = pmr_vector<bool>(allocator);


    // we can't get a pointer so we don't have to copy everything? -> no, no guarantees for iterator.
    // also, encoding perf is not so important
    auto segment_contains_null_values = false;

    segment_iterable.with_iterators([&](auto it, auto end) {
      const auto size = std::distance(it, end);

      null_values.reserve(size);
      values.reserve(size);

      for (; it != end; ++it) {
        values.push_back(it->is_null() ? 0u : static_cast<uint32_t>(it->value())); // todo: zig zag encode int to uint?
        null_values.push_back(it->is_null());
        segment_contains_null_values |= it->is_null();
      }
    });

    // The resize method of the vector might have overallocated memory - hand that memory back to the system
    values.shrink_to_fit();
    null_values.shrink_to_fit();

    uint32_t max_value = _find_max_value(values);
    uint32_t b = _get_bit_width(max_value);
    auto data = std::make_shared<pmr_bitpacking_vector<uint32_t>>(b, allocator);
    for (int i = 0; i < values.size(); i++) {
      data->push_back(values[i]);
    }

    if (segment_contains_null_values) {
      return std::make_shared<TurboPFORSegment<T>>(std::move(data), std::move(null_values), values.size());
    } else {
      return std::make_shared<TurboPFORSegment<T>>(std::move(data), std::nullopt, values.size());
    }
  }

uint32_t _find_max_value(const std::vector<uint32_t>& vector) const {
  uint32_t max = 0;
  for (const auto v : vector) {
    max |= v;
  }
  return max;
}

uint32_t _get_bit_width(uint32_t max_value) {
  uint32_t b_1;
  if (max_value <= 0) {
    b_1 = 1;
  } else if (max_value == 1) {
    b_1 = 1;
  } else if (max_value == std::numeric_limits<unsigned int>::max()) {
    b_1 = std::ceil(log2(max_value));
  } else { 
    b_1 = std::ceil(log2(max_value + 1));
  }
  uint32_t b = std::min(b_1, std::max(compact::vector<unsigned int, 32>::required_bits(max_value), 1u));
  return b;
}
};
} // namespace opposum

