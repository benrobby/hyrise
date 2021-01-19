#pragma once

#include <memory>

#include "storage/base_segment_encoder.hpp"
#include "storage/turboPFOR_segment.hpp"
#include "vp4.h"
#include <fstream>

#define P4NENC_BOUND(n) ((n + 127) / 128 + (n + 32) * sizeof(uint32_t))
#define ROUND_UP(_n_, _a_) (((_n_) + ((_a_)-1)) & ~((_a_)-1))

namespace opossum {

class TurboPFOREncoder : public SegmentEncoder<TurboPFOREncoder> {

 public:
  static constexpr auto _encoding_type = enum_c<EncodingType, EncodingType::TurboPFOR>;
  static constexpr auto _uses_vector_compression = false;

  template <typename T>
  std::shared_ptr<AbstractEncodedSegment> _on_encode(const AnySegmentIterable<T> segment_iterable,
                                                     const PolymorphicAllocator<T>& allocator) {

    auto values = pmr_vector<uint32_t>(allocator); // destroy it when out of scope, only used to get values in continuous mem
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

    // Encrypt Init
    int n = values.size();
    values.resize(ROUND_UP(n, 32));
    auto outBuffer = std::make_shared<pmr_vector<unsigned char>>(allocator);
    outBuffer->resize(P4NENC_BOUND(n));
    p4encx32(values.data(), n, outBuffer->data());;

    if (segment_contains_null_values) {
      return std::make_shared<TurboPFORSegment<T>>(std::move(outBuffer), std::move(null_values), n);
    } else {
      return std::make_shared<TurboPFORSegment<T>>(std::move(outBuffer), std::nullopt, n);
    }
  }
};


}  // namespace opossum