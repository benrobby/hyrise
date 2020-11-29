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
#include "benchmark_column_compression_sdsl_lite_vlc_vector.hpp"
#include "benchmark_column_compression_sdsl_lite_dac_vector.hpp"
#include "benchmark_column_compression_turboPFOR.hpp"

#include "benchmark_column_compression_streamVByte.hpp"

#define BENCHMARK_NAMES 

#define COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(benchmarkName)                                   \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_small_numbers, benchmarkName##_benchmark_encoding,      \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_sequential_numbers, benchmarkName##_benchmark_encoding, \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_huge_numbers, benchmarkName##_benchmark_encoding,       \
                                                 benchmarkName##_benchmark_decoding);                             \
  COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_random_walk, benchmarkName##_benchmark_encoding,        \
                                                 benchmarkName##_benchmark_decoding);

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
      maskedVByte_compute_bitsPerInt, maskedVByteDelta_compute_bitsPerInt, fastPFOR_fastpfor256_compute_bitsPerInt, streamVByte_compute_bitsPerInt,
      oroch_varint_compute_bitsPerInt, oroch_integerArray_compute_bitsPerInt,
      sdsl_lite_vlc_vector_compute_bitsPerInt, sdsl_lite_dac_vector_compute_bitsPerInt, turboPFOR_compute_bitsPerInt};
  std::vector<std::string> functionNames = {"maskedVByte", "maskedVByteDelta", "fastPFOR_fastpfor256", "streamVByte", "oroch_varint",
                                            "oroch_integerArray", "sdsl_lite_vlc_vector", "sdsl_lite_dac_vector",
                                              "turboPFOR"};
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

COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByte);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByteDelta);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastpfor256);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(streamVByte);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_varint);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_integerArray);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_vlc_vector);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_dac_vector);
COLUMNN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR);





// comment in to run all encodings, ensure that they are correct and write out their compression ratio (bits per integer)
BENCHMARK_F(BenchmarkColumnCompressionFixture, write_BitsPerInt)(benchmark::State& state) { writeBitsPerInt(); }

}  // namespace opossum
