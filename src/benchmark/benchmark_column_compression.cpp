#include <memory>
#include <random>

//Important, otherwise the direct access functions from TurboPFOR dont get included
#define TURBOPFOR_DAC

#include "bitpack.h"
#include "benchmark/benchmark.h"

#include "benchmark_column_compression_turboPFOR.hpp"
#include "benchmark_column_compression_turboPFOR_block.hpp"
#include "benchmark_column_compression_turboPFOR_direct.hpp"
#include "benchmark_column_compression_unencoded.hpp"

#include "benchmark_column_compression_data.hpp"

#define COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(benchmarkName)                                    \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_small_numbers, benchmarkName##_benchmark_encoding,      \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_sequential_sorted_numbers, benchmarkName##_benchmark_encoding, \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_huge_numbers, benchmarkName##_benchmark_encoding,       \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_random_walk, benchmarkName##_benchmark_encoding,        \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_month_categorical_numbers, benchmarkName##_benchmark_encoding,        \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_year_categorical_numbers, benchmarkName##_benchmark_encoding,        \
                                                 benchmarkName##_benchmark_decoding);                            \
  COLUMN_COMPRESSION_BENCHMARK_DECODING_POINT(get_with_sequential_sorted_numbers, benchmarkName##_benchmark_decoding_points); \
  COLUMN_COMPRESSION_BENCHMARK_DECODING_POINT(get_with_sequential_sorted_numbers, benchmarkName##_benchmark_decoding_points_nocopy);

#define COLUMN_COMPRESSION_BENCHMARK_DECODING_POINT(setupMethod, benchmarkMethodDecodePoints)            \
  COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_10, benchmarkMethodDecodePoints);   \
  COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_100, benchmarkMethodDecodePoints);  \
  COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_1000, benchmarkMethodDecodePoints); \
  COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_10000, benchmarkMethodDecodePoints); \
COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_65000, benchmarkMethodDecodePoints); \
COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_10000_sorted, benchmarkMethodDecodePoints);

#define COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, poslistMethod, benchmarkMethod)      \
  BENCHMARK_F(BenchmarkColumnCompressionFixture, benchmarkMethod##_##setupMethod##_##poslistMethod) \
  (benchmark::State & state) {                                                                      \
    auto vec = setupMethod();                                                                       \
    auto poslist = poslistMethod();                                                                 \
    benchmarkMethod(vec, poslist, state);                                                           \
  }

#define COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(setupMethod, benchmarkMethodEncode, benchmarkMethodDecode) \
  COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodEncode);                                               \
  COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodDecode);
#define COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethod)                                            \
  BENCHMARK_F(BenchmarkColumnCompressionFixture, benchmarkMethod##_##setupMethod)(benchmark::State & state) { \
    auto vec = setupMethod();                                                                                 \
    benchmarkMethod(vec, state);                                                                              \
  }

#define CHUNK_SIZE 65'000

namespace opossum {

using ValueT = uint32_t;



// Analyse Compression

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:
 protected:
};

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(unencoded);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_direct);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_block);

}  // namespace opossum
