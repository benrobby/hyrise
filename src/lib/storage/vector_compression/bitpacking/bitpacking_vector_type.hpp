#pragma once

#include "compact_vector.hpp"
#include "types.hpp"

namespace opossum {

/**
 * @brief Template arguments of compact::vector<typename IDX, unsigned BITS, typename W, typename Allocator>
 *
 * IDX: right now, compact_vector is only used in the BitpackingVector where the value type uint32_t is sufficient.
 * BITS: 0 means that the bit width is determined at runtime
 * W: the compact::vector allocates its memory in chunks of the specified word size.
 * Allocator: the allocator should allocate the same value_type as W
 */

using pmr_compact_vector = compact::vector<uint32_t, 0u, uint64_t, PolymorphicAllocator<uint64_t>>;

}  // namespace opossum
