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
    auto null_values = std::make_shared<pmr_vector<bool>>(allocator);

    // todo: can we get a pointer so we don't have to copy everything?
    segment_iterable.with_iterators([&](auto it, auto end) {
      for (; it != end; ++it) {
        values.push_back(it->is_null() ? 0u : static_cast<uint32_t>(it->value())); // todo: zig zag encode int to uint?
        null_values->push_back(it->is_null());
      }
    });

    // The resize method of the vector might have overallocated memory - hand that memory back to the system
    values.shrink_to_fit();
    null_values->shrink_to_fit();

    auto codec_name = "simdbinarypacking";
    auto codec = *FastPForLib::CODECFactory::getFromName(codec_name);

    auto encodedValues = std::make_shared<pmr_vector<uint32_t>>(allocator);
    encodedValues->resize(2 * values.size() + 1024);

    auto encodedValuesSize = encodedValues->size();
    codec.encodeArray(values.data(), values.size(), encodedValues->data(), encodedValuesSize);

    encodedValues->resize(encodedValuesSize);
    encodedValues->shrink_to_fit();

    return std::make_shared<FastPFORSegment<T>>(encodedValues, null_values, 0);
  }
};


}  // namespace opossum
