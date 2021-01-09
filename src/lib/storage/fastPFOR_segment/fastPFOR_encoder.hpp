#pragma once

#include <memory>

#include "storage/base_segment_encoder.hpp"
#include "headers/codecfactory.h"

#include "storage/fastPFOR_segment.hpp"

namespace opossum {

class FastPFOREncoder : public SegmentEncoder<FastPFOREncoder> {

 public:
  static constexpr auto _encoding_type = enum_c<EncodingType, EncodingType::FastPFOR>;
  static constexpr auto _uses_vector_compression = false;

  template <typename T>
  std::shared_ptr<AbstractEncodedSegment> _on_encode(const AnySegmentIterable<T> segment_iterable,
                                                     const PolymorphicAllocator<T>& allocator) {

    auto values = pmr_vector<uint32_t>(allocator); // destroy it when out of scope, only used to get values in continuous mem
    auto null_values = pmr_vector<bool>(allocator);

    // we can't get a pointer so we don't have to copy everything? -> no, no guarantees for iterator.
    // also, encoding perf is not so important

    auto segment_contains_null_values = false;
    const uint8_t codec_id = 0; // todo map to name

    segment_iterable.with_iterators([&](auto it, auto end) {
      for (; it != end; ++it) {
        values.push_back(it->is_null() ? 0u : static_cast<uint32_t>(it->value())); // todo: zig zag encode int to uint?
        null_values.push_back(it->is_null());
        segment_contains_null_values |= it->is_null();
      }
    });

    // The resize method of the vector might have overallocated memory - hand that memory back to the system
    values.shrink_to_fit();
    null_values.shrink_to_fit();

    FastPForLib::IntegerCODEC &codec = *FastPForLib::CODECFactory::getFromName("simdfastpfor128");

    auto encodedValues = std::make_shared<pmr_vector<uint32_t>>(allocator);
    encodedValues->resize(2 * values.size() + 1024);

    auto encodedValuesSize = encodedValues->size();
    codec.encodeArray(values.data(), values.size(), encodedValues->data(), encodedValuesSize);

    encodedValues->resize(encodedValuesSize);
    encodedValues->shrink_to_fit();

    if (segment_contains_null_values) {
       return std::make_shared<FastPFORSegment<T>>(encodedValues, std::move(null_values), codec_id, values.size());
    } else {
       return std::make_shared<FastPFORSegment<T>>(encodedValues, std::nullopt, codec_id, values.size());
     }
  }
};


}  // namespace opossum
