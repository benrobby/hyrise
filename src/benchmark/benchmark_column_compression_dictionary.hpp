
#include <iostream>
#include "benchmark/benchmark.h"
#include <unordered_map>
#include <variant>


#include "vp4.h"


using ValueT = uint32_t;

enum VectorType {
  UINT_8,
  UINT_16,
  UINT_32
};

namespace opossum {

// I struggled on how to best implement the attribute vectors with the different
// uint sizes. For now, I created different instance variables, and touch only one of the
// variables. This is far from perfect, so I would be interested if there are better/smarter solutions.

class DictionaryEncoder {
 public:

  void encode(const std::vector<ValueT> &vec){
    // Fill Dictionary;
    for (const ValueT &value : vec){
      dictionary.push_back(value);
    }
    std::sort(dictionary.begin(), dictionary.end());
    dictionary.erase(unique(dictionary.begin(), dictionary.end()), dictionary.end());

    // Create AttributeVector
    size_t n = dictionary.size();
    if (n < pow(2,8)) {
      _setAttributeVector(attribute_vec_uint8, dictionary, vec);
      type = VectorType::UINT_8;
    } else if (n < pow(2, 16)) {
      _setAttributeVector(attribute_vec_uint16, dictionary, vec);
      type = VectorType::UINT_16;
    } else {
      _setAttributeVector(attribute_vec_uint32, dictionary, vec);
      type = VectorType::UINT_32;
    }
  }

  ValueT get(int i) {
    switch (type) {
      case VectorType::UINT_8: return _get(attribute_vec_uint8, i); break;
      case VectorType::UINT_16: return _get(attribute_vec_uint16, i); break;
      default: return _get(attribute_vec_uint32, i); break;
    }
  }

  void getAll(std::vector<ValueT>& valueAllocated) {
    switch (type) {
      case VectorType::UINT_8: _getAll(attribute_vec_uint8, valueAllocated); break;
      case VectorType::UINT_16: _getAll(attribute_vec_uint16, valueAllocated); break;
      default: _getAll(attribute_vec_uint32, valueAllocated); break;
    }
  }

  int size_in_bytes() {
    int sizeOfAttributeVector;
    switch (type) {
      case VectorType::UINT_8: sizeOfAttributeVector = attribute_vec_uint8.size() * 1; break;
      case VectorType::UINT_16: sizeOfAttributeVector = attribute_vec_uint16.size() * 2; break;
      default: sizeOfAttributeVector = attribute_vec_uint32.size() * 4; break;
    }

    return dictionary.size() * sizeof(ValueT) + sizeOfAttributeVector;
  }

  std::vector<ValueT> get_dictionary() {
    return dictionary;
  }
  std::vector<uint8_t> get_attribute_vec_uint8() {
    return attribute_vec_uint8;
  }
  std::vector<uint16_t> get_attribute_vec_uint16() {
    return attribute_vec_uint16;
  }
  std::vector<uint32_t> get_attribute_vec_uint32() {
    return attribute_vec_uint32;
  }

 private:
  std::vector<ValueT> dictionary;

  VectorType type;
  std::vector<uint8_t> attribute_vec_uint8;
  std::vector<uint16_t> attribute_vec_uint16;
  std::vector<uint32_t> attribute_vec_uint32;

  // Template Functions

  template <typename T>
  ValueT _get(const std::vector<T>& attributeVector, int i) {
    return dictionary[attributeVector[i]];
  }

  template <typename T>
  void _getAll(const std::vector<T>& attributeVector, std::vector<ValueT>& valueAllocated) {
    for (size_t i = 0; i < attributeVector.size(); i++) {
      ValueT value = dictionary[attributeVector[i]];
      valueAllocated[i] = value;
    }
  }

  template <typename T>
  void _setAttributeVector(std::vector<T>& attributeVector, const std::vector<ValueT>& dictionary,
                           const std::vector<ValueT> &vec) {
    for (const ValueT& value: vec) {
      int indexInDictionary = binary_search_find_index(dictionary, value);
      attributeVector.push_back(indexInDictionary);
    }
  }

  // Helper Functions

  int binary_search_find_index(const std::vector<ValueT>& v, int data) {
    auto it = std::lower_bound(v.begin(), v.end(), data);
    if (it == v.end() || *it != data) {
      return -1;
    } else {
      std::size_t index = std::distance(v.begin(), it);
      return index;
    }
  }
};

void dictionary_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  DictionaryEncoder encoder = DictionaryEncoder();
  benchmark::DoNotOptimize(encoder.get_dictionary().data());
  benchmark::DoNotOptimize(encoder.get_attribute_vec_uint8().data());
  benchmark::DoNotOptimize(encoder.get_attribute_vec_uint16().data());
  benchmark::DoNotOptimize(encoder.get_attribute_vec_uint32().data());
  for (auto _ : state) {
    encoder.encode(vec);
    benchmark::ClobberMemory();
  }
}

void dictionary_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  DictionaryEncoder encoder = DictionaryEncoder();
  encoder.encode(vec);
  std::vector<ValueT> result = std::vector<ValueT>(vec.size());

  benchmark::DoNotOptimize(result.data());

  for (auto _ : state) {
    encoder.getAll(result);
    benchmark::ClobberMemory();
  }
}

void _dictionary_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  DictionaryEncoder encoder = DictionaryEncoder();
  encoder.encode(vec);
  std::vector<ValueT> result = std::vector<ValueT>(vec.size());

  std::vector<ValueT> points {};
  points.resize(pointIndices.size());
  benchmark::DoNotOptimize(points.data());

  ValueT sum = 0;
  benchmark::DoNotOptimize(sum);

  for (auto _ : state) {
    if (nocopy) {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        sum += encoder.get(pointIndices[i]);
      }
    } else {
      for (size_t i = 0; i < pointIndices.size(); i++) {
        points[i] = encoder.get(pointIndices[i]);
      }
    }

    benchmark::ClobberMemory();
    sum = 0;
  }
}

void dictionary_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _dictionary_benchmark_decoding_points(vec, pointIndices, state, false);
}
void dictionary_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _dictionary_benchmark_decoding_points(vec, pointIndices, state, true);
}

float dictionary_compute_bitsPerInt(std::vector<ValueT>& vec) {
  DictionaryEncoder encoder = DictionaryEncoder();
  encoder.encode(vec);
  std::vector<ValueT> result = std::vector<ValueT>(vec.size());
  encoder.getAll(result);

  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i] != result[i]) {
      throw std::runtime_error("bug!");
    }
  }

  return encoder.size_in_bytes() * 8.0 / vec.size();
}

}  // namespace opossum