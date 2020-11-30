#include <memory>
#include <memory>

#include "benchmark/benchmark.h"
#include "headers/codecfactory.h"
#include "varintdecode.h"
#include "varintencode.h"

#include "benchmark_column_compression_fastPFOR.hpp"
#include "benchmark_column_compression_maskedVByte.hpp"
#include "benchmark_column_compression_maskedVByteDelta.hpp"
#include "benchmark_column_compression_oroch_integerArray.hpp"
#include "benchmark_column_compression_oroch_varint.hpp"
#include "benchmark_column_compression_sdsl_lite_dac_vector.hpp"
#include "benchmark_column_compression_sdsl_lite_vlc_vector.hpp"
#include "benchmark_column_compression_streamVByte.hpp"
#include "benchmark_column_compression_turboPFOR.hpp"

#define BENCHMARK_NAMES

#define COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(benchmarkName)                                    \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_small_numbers, benchmarkName##_benchmark_encoding,      \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_sequential_numbers, benchmarkName##_benchmark_encoding, \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_huge_numbers, benchmarkName##_benchmark_encoding,       \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_random_walk, benchmarkName##_benchmark_encoding,        \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_DECODING_POINT(get_with_small_numbers, benchmarkName##_benchmark_decoding_points);

#define COLUMN_COMPRESSION_BENCHMARK_DECODING_POINT(setupMethod, benchmarkMethodDecodePoints) \
  COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, get_poslist_10, benchmarkMethodDecodePoints);

#define COLUMN_COMPRESSION_BENCHMARK_WITH_POSLIST(setupMethod, poslistMethod, benchmarkMethod)                \
  BENCHMARK_F(BenchmarkColumnCompressionFixture, benchmarkMethod##_##setupMethod)(benchmark::State & state) { \
    auto vec = setupMethod();                                                                                 \
    auto poslist = poslistMethod();                                                                           \
    benchmarkMethod(vec, poslist, state);                                                                     \
  }

#define COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(setupMethod, benchmarkMethodEncode, benchmarkMethodDecode) \
  COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodEncode);                                               \
  COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodDecode);
#define COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethod)                                            \
  BENCHMARK_F(BenchmarkColumnCompressionFixture, benchmarkMethod##_##setupMethod)(benchmark::State & state) { \
    auto vec = setupMethod();                                                                                 \
    benchmarkMethod(vec, state);                                                                              \
  }

using namespace FastPForLib;

namespace opossum {

using ValueT = uint32_t;

// poslists

std::vector<size_t> get_poslist_10() {
  std::vector<size_t> vec(10);
  std::generate(vec.begin(), vec.end(), []() {
    static size_t v = 0;
    v = (v + 1) % 4;  // todo think about distribution
    return v;
  });
  return vec;
}

// Data

std::vector<ValueT> get_with_small_numbers() {
  std::vector<ValueT> vec(65'000);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 0;
    v = (v + 1) % 4;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_sequential_numbers() {
  std::vector<ValueT> vec(65'000);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 0;
    v = v + 5;
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_huge_numbers() {
  std::vector<ValueT> vec(65'000);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 331;
    v = 1'000'000 + ((v * 7) % 1'000'000);
    return v;
  });
  return vec;
}

std::vector<ValueT> get_with_random_walk() {
  std::vector<ValueT> vec(65'000);
  std::generate(vec.begin(), vec.end(), []() {
    static ValueT v = 945713;
    if ((v * 13 % 7) > 3) {
      v = v + 13;
    } else {
      v = v - 11;
    }
    return v;
  });
  return vec;
}

void writeBitsPerInt() {
  std::ofstream csvFile("bits_per_int.csv");
  csvFile << "name,dataName,bitsPerInt" << std::endl;

  // keep both in sync
  std::vector<float (*)(std::vector<ValueT> & vec)> functions = {
      maskedVByte_compute_bitsPerInt, maskedVByteDelta_compute_bitsPerInt,

      fastPFOR_fastbinarypacking8_compute_bitsPerInt, fastPFOR_fastbinarypacking16_compute_bitsPerInt,
      fastPFOR_fastbinarypacking32_compute_bitsPerInt, fastPFOR_BP32_compute_bitsPerInt,
      fastPFOR_vsencoding_compute_bitsPerInt, fastPFOR_fastpfor128_compute_bitsPerInt,
      fastPFOR_fastpfor256_compute_bitsPerInt, fastPFOR_simdfastpfor128_compute_bitsPerInt,
      fastPFOR_simdfastpfor256_compute_bitsPerInt, fastPFOR_simplepfor_compute_bitsPerInt,
      fastPFOR_simdsimplepfor_compute_bitsPerInt, fastPFOR_pfor_compute_bitsPerInt,
      fastPFOR_simdpfor_compute_bitsPerInt, fastPFOR_pfor2008_compute_bitsPerInt,
      fastPFOR_simdnewpfor_compute_bitsPerInt, fastPFOR_newpfor_compute_bitsPerInt, fastPFOR_optpfor_compute_bitsPerInt,
      fastPFOR_simdoptpfor_compute_bitsPerInt, fastPFOR_varint_compute_bitsPerInt, fastPFOR_vbyte_compute_bitsPerInt,
      //fastPFOR_maskedvbyte_compute_bitsPerInt,
      //fastPFOR_streamvbyte_compute_bitsPerInt,
      //fastPFOR_varintgb_compute_bitsPerInt,
      //fastPFOR_simple16_compute_bitsPerInt,
      //fastPFOR_simple9_compute_bitsPerInt,
      //fastPFOR_simple9_rle_compute_bitsPerInt,
      // fastPFOR_simple8b_compute_bitsPerInt,
      // fastPFOR_simple8b_rle_compute_bitsPerInt,
      // fastPFOR_varintg8iu_compute_bitsPerInt,
      // fastPFOR_snappy_compute_bitsPerInt, // todo compile with snappy
      // fastPFOR_simdbinarypacking_compute_bitsPerInt,
      // fastPFOR_simdgroupsimple_compute_bitsPerInt,
      // fastPFOR_simdgroupsimple_ringbuf_compute_bitsPerInt,
      // fastPFOR_copy_compute_bitsPerInt,

      streamVByte_compute_bitsPerInt,

      oroch_varint_compute_bitsPerInt, oroch_integerArray_compute_bitsPerInt,

      sdsl_lite_vlc_vector_compute_bitsPerInt, sdsl_lite_dac_vector_compute_bitsPerInt,

      turboPFOR_compute_bitsPerInt};
  std::vector<std::string> functionNames = {
      "maskedVByte", "maskedVByteDelta",

      "fastPFOR_fastbinarypacking8", "fastPFOR_fastbinarypacking16", "fastPFOR_fastbinarypacking32", "fastPFOR_BP32",
      "fastPFOR_vsencoding", "fastPFOR_fastpfor128", "fastPFOR_fastpfor256", "fastPFOR_simdfastpfor128",
      "fastPFOR_simdfastpfor256", "fastPFOR_simplepfor", "fastPFOR_simdsimplepfor", "fastPFOR_pfor",
      "fastPFOR_simdpfor", "fastPFOR_pfor2008", "fastPFOR_simdnewpfor", "fastPFOR_newpfor", "fastPFOR_optpfor",
      "fastPFOR_simdoptpfor", "fastPFOR_varint", "fastPFOR_vbyte",
      // "fastPFOR_maskedvbyte", // todo debug why the decoded output is different from the input
      // "fastPFOR_streamvbyte",
      // "fastPFOR_varintgb",
      // "fastPFOR_simple16",
      // "fastPFOR_simple9",
      // "fastPFOR_simple9_rle",
      // "fastPFOR_simple8b",
      // "fastPFOR_simple8b_rle",
      // "fastPFOR_varintg8iu",
      // "fastPFOR_snappy", // todo compile with snappy
      // "fastPFOR_simdbinarypacking",
      // "fastPFOR_simdgroupsimple",
      // "fastPFOR_simdgroupsimple_ringbuf",
      // "fastPFOR_copy",

      "streamVByte",

      "oroch_varint", "oroch_integerArray",

      "sdsl_lite_vlc_vector", "sdsl_lite_dac_vector", "turboPFOR"};

  for (size_t j = 0; j < functions.size(); j++) {
    std::vector<std::vector<ValueT>> inputs = {get_with_small_numbers(), get_with_sequential_numbers(),
                                               get_with_huge_numbers(), get_with_random_walk()};
    std::vector<std::string> names = {"small_numbers", "sequential_numbers", "huge_numbers", "random_walk"};
    for (int i = 0; i < (int)inputs.size(); i++) {
      csvFile << functionNames[j] << "," << names[i] << "," << functions[j](inputs[i]) << std::endl;
    }
  }

  csvFile << std::endl;
  csvFile.close();
}

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:
 protected:
};

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByteDelta);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastbinarypacking8);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastbinarypacking16);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastbinarypacking32);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_BP32);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_vsencoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastpfor128);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastpfor256);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdfastpfor128);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdfastpfor256);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simplepfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdsimplepfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_pfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_pfor2008);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdnewpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_newpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_optpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdoptpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_varint);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_vbyte);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_maskedvbyte);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_streamvbyte);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_varintgb);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple16);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple9);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple9_rle);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple8b);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple8b_rle);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_varintg8iu);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_snappy);  // todo compile with snappy
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdbinarypacking);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdgroupsimple);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdgroupsimple_ringbuf);
 // COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_copy);

// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(streamVByte);

// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_varint);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_integerArray);

// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_vlc_vector);
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_dac_vector);

// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR);

// comment in to run all encodings, ensure that they are correct and write out their compression ratio (bits per integer)
BENCHMARK_F(BenchmarkColumnCompressionFixture, write_BitsPerInt)(benchmark::State& state) { writeBitsPerInt(); }

}  // namespace opossum
