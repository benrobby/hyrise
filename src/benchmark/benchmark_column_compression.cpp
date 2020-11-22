#include <memory>

#include "benchmark/benchmark.h"

namespace opossum {

using ValueT = int32_t;

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:
  void SetUp(::benchmark::State& state) override {
    // Fill the vector with 1M values in the pattern 0, 1, 2, 3, 0, 1, 2, 3, ...
    // The "TableScan" will scan for one value (2), so it will select 25%.
    _vec.resize(1'000'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 0;
      v = (v + 1) % 4;
      return v;
    });
  }
  void TearDown(::benchmark::State& state) override { 
    // empty
   }

 protected:
  std::vector<ValueT> _vec;
};

/**
 * Reference implementation, growing the vector on demand
 */
BENCHMARK_F(BenchmarkColumnCompressionFixture, BM_Column_Compression_Dummy)(benchmark::State& state) {
  // Add some benchmark-specific setup here

  for (auto _ : state) {
    std::vector<size_t> result;
    benchmark::DoNotOptimize(result.data());  // Do not optimize out the vector
    const auto size = _vec.size();
    for (size_t i = 0; i < size; ++i) {
      if (_vec[i] == 2) {
        result.push_back(i);
        benchmark::ClobberMemory();  // Force that record to be written to memory
      }
    }
  }

}

}  // namespace opossum
