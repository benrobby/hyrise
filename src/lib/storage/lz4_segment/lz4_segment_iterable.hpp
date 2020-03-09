#pragma once

#include <type_traits>

#include "storage/segment_iterables.hpp"

#include "storage/lz4_segment.hpp"
#include "storage/vector_compression/resolve_compressed_vector_type.hpp"

namespace opossum {

template <typename T>
class LZ4SegmentIterable : public PointAccessibleSegmentIterable<LZ4SegmentIterable<T>> {
 public:
  using ValueType = T;

  explicit LZ4SegmentIterable(const LZ4Segment<T>& segment) : _segment{segment} {}

  template <typename Functor>
  void _on_with_iterators(const Functor& functor) const {
    using ValueIterator = typename std::vector<T>::const_iterator;

    const auto decompressed_segment = _segment.decompress();
    _segment.access_counter[SegmentAccessCounter::AccessType::Sequential] += decompressed_segment.size();

    auto begin =
        Iterator<ValueIterator>{&decompressed_segment, &_segment.null_values(), ChunkOffset{0u}};
    auto end = Iterator<ValueIterator>{&decompressed_segment, &_segment.null_values(),
                                       static_cast<ChunkOffset>(decompressed_segment.size())};
    functor(begin, end);
  }

  /**
   * For the point access, we first retrieve the values for all chunk offsets in the position list and then save
   * the decompressed values in a vector. The first value in that vector (index 0) is the value for the chunk offset
   * at index 0 in the position list.
   */
  template <typename Functor>
  void _on_with_iterators(const std::shared_ptr<const PosList>& position_filter, const Functor& functor) const {
    using ValueIterator = typename std::vector<T>::const_iterator;

    const auto position_filter_size = position_filter->size();
    _segment.access_counter[SegmentAccessCounter::access_type(*position_filter)] += position_filter_size;

    // vector storing the uncompressed values
    auto decompressed_filtered_segment = std::vector<ValueType>(position_filter_size);

    // _segment.decompress() takes the currently cached block (reference) and its id in addition to the requested
    // element. If the requested element is not within that block, the next block will be decompressed and written to
    // `_cached_block` while the value and the new block id are returned. In case the requested element is within the
    // cached block, the value and the input block id are returned.
    for (auto index = size_t{0u}; index < position_filter_size; ++index) {
      const auto& position = (*position_filter)[index];
      // NOLINTNEXTLINE
      auto [value, block_index] = _segment.decompress(position.chunk_offset, _cached_block_index, _cached_block);
      decompressed_filtered_segment[index] = std::move(value);
      _cached_block_index = block_index;
    }

    auto begin =
        PointAccessIterator<ValueIterator>{&decompressed_filtered_segment, &_segment.null_values(),
                                           position_filter->cbegin(), position_filter->cbegin()};
    auto end =
        PointAccessIterator<ValueIterator>{&decompressed_filtered_segment, &_segment.null_values(),
                                           position_filter->cbegin(), position_filter->cend()};
    functor(begin, end);
  }

  size_t _on_size() const { return _segment.size(); }

 private:
  const LZ4Segment<T>& _segment;
  mutable std::vector<char> _cached_block;
  mutable std::optional<size_t> _cached_block_index = std::nullopt;

 private:
  template <typename ValueIterator>
  class Iterator : public BaseSegmentIterator<Iterator<ValueIterator>, SegmentPosition<T>> {
   public:
    using ValueType = T;
    using IterableType = LZ4SegmentIterable<T>;

   public:
    // Begin and End Iterator
    explicit Iterator(const std::vector<T>* data, const std::optional<pmr_vector<bool>>* null_values, ChunkOffset chunk_offset)
        : _data{data}, _null_values{null_values}, _chunk_offset{chunk_offset} {}

   private:
    friend class boost::iterator_core_access;  // grants the boost::iterator_facade access to the private interface

    void increment() {
      ++_chunk_offset;
    }

    void decrement() {
      --_chunk_offset;
    }

    void advance(std::ptrdiff_t n) {
      _chunk_offset += n;
    }

    bool equal(const Iterator& other) const { return _chunk_offset == other._chunk_offset; }

    std::ptrdiff_t distance_to(const Iterator& other) const {
      return static_cast<std::ptrdiff_t>(other._chunk_offset) - _chunk_offset;
    }

    SegmentPosition<T> dereference() const {
      const auto value = (*_data)[_chunk_offset];
      const auto is_null = *_null_values ? (*(*_null_values))[_chunk_offset] : false;
      return SegmentPosition<T>{std::move(value), std::move(is_null), _chunk_offset};
    }

   private:
    const std::vector<T>* _data;
    const std::optional<pmr_vector<bool>>* _null_values;

    ChunkOffset _chunk_offset;
  };

  template <typename ValueIterator>
  class PointAccessIterator
      : public BasePointAccessSegmentIterator<PointAccessIterator<ValueIterator>, SegmentPosition<T>> {
   public:
    using ValueType = T;
    using IterableType = LZ4SegmentIterable<T>;

    // Begin Iterator
    PointAccessIterator(const std::vector<T>* data, const std::optional<pmr_vector<bool>>* null_values,
                        PosList::const_iterator position_filter_begin, PosList::const_iterator position_filter_it)
        : BasePointAccessSegmentIterator<PointAccessIterator<ValueIterator>,
                                         SegmentPosition<T>>{std::move(position_filter_begin),
                                                             std::move(position_filter_it)},
          _data{data}, _null_values{null_values} {}

   private:
    friend class boost::iterator_core_access;  // grants the boost::iterator_facade access to the private interface

    SegmentPosition<T> dereference() const {
      const auto& chunk_offsets = this->chunk_offsets();
      const auto& value = (*_data)[chunk_offsets.offset_in_referenced_chunk];
      const auto is_null = *_null_values && (*(*_null_values))[chunk_offsets.offset_in_referenced_chunk];
      return SegmentPosition<T>{value, is_null, chunk_offsets.offset_in_poslist};
    }

   private:
    const std::vector<T>* _data;
    const std::optional<pmr_vector<bool>>* _null_values;
  };
};

}  // namespace opossum
