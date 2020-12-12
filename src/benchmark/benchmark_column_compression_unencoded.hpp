
#include <iostream>
#include "benchmark/benchmark.h"
#include <unordered_map>
#include <variant>


#include "vp4.h"


using ValueT = uint32_t;

namespace opossum {

// I struggled on how to best implement the attribute vectors with the different
// uint sizes. For now, I created different instance variables, and touch only one of the
// variables. This is far from perfect, so I would be interested if there are better/smarter solutions.

class UnencodedEncoder {
 public:

  void encode(const std::vector<ValueT> &v){
    vec = v;
  }

  ValueT get(int i) {
    return vec[i];
  }

  void getAll(std::vector<ValueT>& valueAllocated) {
    std::copy(vec.begin(), vec.end(), std::back_inserter(valueAllocated));
  }

  int size_in_bytes() {
    return vec.size() * sizeof(ValueT);
  }

  std::vector<ValueT> getVec() {
    return vec;
  }

 private:
  std::vector<ValueT> vec;

};

void unencoded_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  UnencodedEncoder encoder = UnencodedEncoder();
  benchmark::DoNotOptimize(encoder.getVec().data());
  for (auto _ : state) {
    encoder.encode(vec);
    benchmark::ClobberMemory();
  }
}

void unencoded_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  UnencodedEncoder encoder = UnencodedEncoder();
  encoder.encode(vec);
  std::vector<ValueT> result = std::vector<ValueT>(vec.size());

  benchmark::DoNotOptimize(result.data());

  for (auto _ : state) {
    encoder.getAll(result);
    benchmark::ClobberMemory();
  }
}

void _unencoded_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state, bool nocopy) {
  UnencodedEncoder encoder = UnencodedEncoder();
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

void unencoded_benchmark_decoding_points(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _unencoded_benchmark_decoding_points(vec, pointIndices, state, false);
}
void unencoded_benchmark_decoding_points_nocopy(const std::vector<ValueT>& vec, const std::vector<size_t>& pointIndices, benchmark::State& state) {
  return _unencoded_benchmark_decoding_points(vec, pointIndices, state, true);
}

float unencoded_compute_bitsPerInt(std::vector<ValueT>& vec) {
  UnencodedEncoder encoder = UnencodedEncoder();
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