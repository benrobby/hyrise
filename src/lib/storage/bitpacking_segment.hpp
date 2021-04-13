#pragma once

#define TURBOPFOR_DAC

#include <memory>
#include <type_traits>

#include <boost/hana/contains.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/type.hpp>

#include "abstract_encoded_segment.hpp"
#include "types.hpp"

#include "bitpacking_segment/vector_types.hpp"

namespace opossum {

class BaseCompressedVector;

/**
 * @brief Segment for encodings from the SIMDCompressionAndIntersection lib.
 *
 * todo: docs
 *
 * std::enable_if_t must be used here and cannot be replaced by a
 * static_assert in order to prevent instantiation of
 * FrameOfReferenceSegment<T> with T other than int32_t. Otherwise,
 * the compiler might instantiate FrameOfReferenceSegment with other
 * types even if they are never actually needed.
 * "If the function selected by overload resolution can be determined
 * without instantiating a class template definition, it is unspecified
 * whether that instantiation actually takes place." Draft Std. N4800 12.8.1.8
 */
template <typename T, typename = std::enable_if_t<encoding_supports_data_type(
    enum_c<EncodingType, EncodingType::Bitpacking>, hana::type_c<T>)>>
class BitpackingSegment : public AbstractEncodedSegment {
 public:
  explicit BitpackingSegment(const std::shared_ptr<pmr_bitpacking_vector<uint32_t>> encoded_values,
                           std::optional<pmr_vector<bool>> null_values);

  const std::shared_ptr<pmr_bitpacking_vector<uint32_t>> encoded_values() const;
  const std::optional<pmr_vector<bool>>& null_values() const;
  ChunkOffset size() const final;

  /**
   * @defgroup AbstractSegment interface
   * @{
   */

  AllTypeVariant operator[](const ChunkOffset chunk_offset) const final;

  std::optional<T> get_typed_value(const ChunkOffset chunk_offset) const {
    // performance critical - not in cpp to help with inlining

    if (_null_values && (*_null_values)[chunk_offset]) {
      return std::nullopt;
    }

    return (*_encoded_values)[chunk_offset];
  }

  std::shared_ptr<AbstractSegment> copy_using_allocator(const PolymorphicAllocator<size_t>& alloc) const final;

  size_t memory_usage(const MemoryUsageCalculationMode mode) const final;

  /**@}*/

  /**
   * @defgroup AbstractEncodedSegment interface
   * @{
   */

  EncodingType encoding_type() const final;
  std::optional<CompressedVectorType> compressed_vector_type() const final;

  /**@}*/

 protected:
  const std::shared_ptr<pmr_bitpacking_vector<uint32_t>> _encoded_values;
  const std::optional<pmr_vector<bool>> _null_values;};

}  // namespace opossum