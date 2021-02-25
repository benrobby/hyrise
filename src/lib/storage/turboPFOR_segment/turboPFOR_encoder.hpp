#pragma once

#include <memory>

#include "storage/base_segment_encoder.hpp"
#include "storage/turboPFOR_segment.hpp"
#include "vp4.h"
#include <fstream>
#include "math.h"
#include "bitpack.h"

#define TURBOPFOR_DAC

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
        values.push_back(it->is_null() ? 0u : static_cast<uint32_t>(it->value() > -1 ? it->value() : 0)); // todo: zig zag encode int to uint?
        null_values.push_back(it->is_null());
        segment_contains_null_values |= it->is_null();
      }
    });

    // The resize method of the vector might have overallocated memory - hand that memory back to the system
    values.shrink_to_fit();
    null_values.shrink_to_fit();

    auto data = pmr_vector<uint8_t>(allocator);
    data.resize(values.size() * sizeof(uint32_t) + 1024);

    // Set Bit Width
    uint32_t max = 0;
    for (const auto v : values) {
      max |= v;
    }
    uint32_t b;
    if (max <= 0) {
     b = 1;
    } else if (max == 1) {
     b = 1;
    } else {
     b = std::ceil(log2(max + 1));
    }
    const uint8_t b_1 = static_cast<uint8_t>(b);

    // Handle Edge Cases
    if (values.size() == 0) {
      data.resize(0);
      return std::make_shared<TurboPFORSegment<T>>(std::move(std::make_shared<pmr_vector<uint8_t>>(data)), values.size(), b_1, std::move(null_values), values.size());
    }

    // Encode with TurboPFOR Library
    uint8_t * out_end = bitpack32(values.data(), values.size(), data.data(), b);
    int bytes_written = (out_end) - data.data();
    data.resize(bytes_written + 32);

    if (segment_contains_null_values) {
      return std::make_shared<TurboPFORSegment<T>>(std::move(std::make_shared<pmr_vector<uint8_t>>(data)), values.size(), b_1, std::move(null_values), values.size());
    } else {
      return std::make_shared<TurboPFORSegment<T>>(std::move(std::make_shared<pmr_vector<uint8_t>>(data)), values.size(), b_1, std::nullopt, values.size());
    }
  }
};


}  // namespace opossum