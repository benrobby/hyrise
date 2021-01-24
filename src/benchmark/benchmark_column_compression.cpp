#include <memory>
#include <random>

//Important, otherwise the direct access functions from TurboPFOR dont get included
#define TURBOPFOR_DAC

#include "bitpack.h"


#include "benchmark/benchmark.h"
#include "varintencode.h"

#include "benchmark_column_compression_SIMDCompressionAndIntersection.hpp"
#include "benchmark_column_compression_fastPFOR.hpp"
#include "benchmark_column_compression_maskedVByte.hpp"
#include "benchmark_column_compression_maskedVByteDelta.hpp"
#include "benchmark_column_compression_oroch_integerArray.hpp"
#include "benchmark_column_compression_oroch_varint.hpp"
#include "benchmark_column_compression_sdsl_lite_dac_vector.hpp"
#include "benchmark_column_compression_sdsl_lite_vlc_vector.hpp"
#include "benchmark_column_compression_streamVByte.hpp"
#include "benchmark_column_compression_streamVByte0124.hpp"
#include "benchmark_column_compression_streamVByteDelta.hpp"
#include "benchmark_column_compression_turboPFOR.hpp"
#include "benchmark_column_compression_turboPFOR_block.hpp"
#include "benchmark_column_compression_turboPFOR_direct.hpp"
#include "benchmark_column_compression_turboPFOR_direct_chunking.hpp"
#include "benchmark_column_compression_turboPFOR_bitcompression.hpp"
#include "benchmark_column_compression_dictionary.hpp"
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

void writeBitsPerInt() {
  std::ofstream csvFile("bits_per_int.csv");
  csvFile << "name,dataName,bitsPerInt" << std::endl;

  // keep both in sync
  std::vector<pair<float (*)(std::vector<ValueT> & vec), string>> functions = {
      make_pair(dictionary_compute_bitsPerInt, "dictionary"),

      make_pair(SIMDCompressionAndIntersection_fastpfor_compute_bitsPerInt, "SIMDCompressionAndIntersection_fastpfor"),
      make_pair(SIMDCompressionAndIntersection_varint_compute_bitsPerInt, "SIMDCompressionAndIntersection_varint"),
      make_pair(SIMDCompressionAndIntersection_vbyte_compute_bitsPerInt, "SIMDCompressionAndIntersection_vbyte"),
      make_pair(SIMDCompressionAndIntersection_maskedvbyte_compute_bitsPerInt, "SIMDCompressionAndIntersection_maskedvbyte"),
      make_pair(SIMDCompressionAndIntersection_streamvbyte_compute_bitsPerInt, "SIMDCompressionAndIntersection_streamvbyte"),
      make_pair(SIMDCompressionAndIntersection_frameofreference_compute_bitsPerInt, "SIMDCompressionAndIntersection_frameofreference"),
      make_pair(SIMDCompressionAndIntersection_simdframeofreference_compute_bitsPerInt, "SIMDCompressionAndIntersection_simdframeofreference"),
      make_pair(SIMDCompressionAndIntersection_varintgb_compute_bitsPerInt, "SIMDCompressionAndIntersection_varintgb"),
      make_pair(SIMDCompressionAndIntersection_s4fastpford4_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4fastpford4"),
      make_pair(SIMDCompressionAndIntersection_s4fastpfordm_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4fastpfordm"),
      make_pair(SIMDCompressionAndIntersection_s4fastpford1_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4fastpford1"),
      make_pair(SIMDCompressionAndIntersection_s4fastpford2_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4fastpford2"),
      make_pair(SIMDCompressionAndIntersection_bp32_compute_bitsPerInt, "SIMDCompressionAndIntersection_bp32"),
      make_pair(SIMDCompressionAndIntersection_ibp32_compute_bitsPerInt, "SIMDCompressionAndIntersection_ibp32"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d1ni_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d1ni"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d2ni_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d2ni"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d4ni_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d4ni"),
      make_pair(SIMDCompressionAndIntersection_s4bp128dmni_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128dmni"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d1_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d1"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d2_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d2"),
      make_pair(SIMDCompressionAndIntersection_s4bp128d4_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128d4"),
      make_pair(SIMDCompressionAndIntersection_s4bp128dm_compute_bitsPerInt, "SIMDCompressionAndIntersection_s4bp128dm"),
      make_pair(SIMDCompressionAndIntersection_for_compute_bitsPerInt, "SIMDCompressionAndIntersection_for"),

      make_pair(SIMDCompressionAndIntersection_simdframeofreference_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_simdframeofreference_with_select"),
      make_pair(SIMDCompressionAndIntersection_for_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_for_with_select"),
      make_pair(SIMDCompressionAndIntersection_maskedvbyte_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_maskedvbyte_with_select"),
      make_pair(SIMDCompressionAndIntersection_streamvbyte_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_streamvbyte_with_select"),
      make_pair(SIMDCompressionAndIntersection_varint_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_varint_with_select"),
      make_pair(SIMDCompressionAndIntersection_vbyte_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_vbyte_with_select"),
      make_pair(SIMDCompressionAndIntersection_varintgb_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_varintgb_with_select"),

      make_pair(maskedVByte_compute_bitsPerInt, "maskedVByte"),
      make_pair(maskedVByteDelta_compute_bitsPerInt, "maskedVByteDelta"),

      make_pair(streamVByte_compute_bitsPerInt, "streamVByte"),
      make_pair(streamVByte0124_compute_bitsPerInt, "streamVByte0124"),
      make_pair(streamVByteDelta_compute_bitsPerInt, "streamVByteDelta"),

      make_pair(oroch_varint_compute_bitsPerInt, "oroch_varint"),
      make_pair(oroch_integerArray_compute_bitsPerInt, "oroch_integerArray"),

      make_pair(sdsl_lite_vlc_vector_compute_bitsPerInt, "sdsl_lite_vlc_vector"),
      make_pair(sdsl_lite_dac_vector_compute_bitsPerInt, "sdsl_lite_dac_vector"),

      make_pair(turboPFOR_compute_bitsPerInt, "turboPFOR"),
      make_pair(turboPFOR_block_compute_bitsPerInt, "turboPFOR_block"),
      make_pair(turboPFOR_direct_compute_bitsPerInt, "turboPFOR_direct"),
      make_pair(turboPFOR_direct_chunking_compute_bitsPerInt, "turboPFOR_direct_chunking"),
      make_pair(turboPFOR_bitcompression_compute_bitsPerInt, "turboPFOR_bitcompression"),

      make_pair(fastPFOR_fastpfor256_compute_bitsPerInt, "fastPFOR_fastpfor256"),

      // Fast PFOR Codecs
      make_pair(fastPFOR_fastbinarypacking8_compute_bitsPerInt, "fastPFOR_fastbinarypacking8"),
      make_pair(fastPFOR_fastbinarypacking16_compute_bitsPerInt, "fastPFOR_fastbinarypacking16"),
      make_pair(fastPFOR_fastbinarypacking32_compute_bitsPerInt, "fastPFOR_fastbinarypacking32"),
      make_pair(fastPFOR_BP32_compute_bitsPerInt, "fastPFOR_BP32"),
      make_pair(fastPFOR_vsencoding_compute_bitsPerInt, "fastPFOR_vsencoding"),
      make_pair(fastPFOR_fastpfor128_compute_bitsPerInt, "fastPFOR_fastpfor128"),
      make_pair(fastPFOR_simdfastpfor128_compute_bitsPerInt, "fastPFOR_simdfastpfor128"),
      make_pair(fastPFOR_simdfastpfor256_compute_bitsPerInt, "fastPFOR_simdfastpfor256"),
      make_pair(fastPFOR_simplepfor_compute_bitsPerInt, "fastPFOR_simplepfor"),
      make_pair(fastPFOR_simdsimplepfor_compute_bitsPerInt, "fastPFOR_simdsimplepfor"),
      make_pair(fastPFOR_pfor_compute_bitsPerInt, "fastPFOR_pfor"),
      make_pair(fastPFOR_simdpfor_compute_bitsPerInt, "fastPFOR_simdpfor"),
      make_pair(fastPFOR_pfor2008_compute_bitsPerInt, "fastPFOR_pfor2008"),
      make_pair(fastPFOR_simdnewpfor_compute_bitsPerInt, "fastPFOR_simdnewpfor"),
      make_pair(fastPFOR_newpfor_compute_bitsPerInt, "fastPFOR_newpfor"),
      make_pair(fastPFOR_optpfor_compute_bitsPerInt, "fastPFOR_optpfor"),
      make_pair(fastPFOR_simdoptpfor_compute_bitsPerInt, "fastPFOR_simdoptpfor"),
      make_pair(fastPFOR_varint_compute_bitsPerInt, "fastPFOR_varint"),
      make_pair(fastPFOR_vbyte_compute_bitsPerInt, "fastPFOR_vbyte"),
      make_pair(fastPFOR_maskedvbyte_compute_bitsPerInt, "fastPFOR_maskedvbyte"),
      make_pair(fastPFOR_streamvbyte_compute_bitsPerInt, "fastPFOR_streamvbyte"),
      make_pair(fastPFOR_varintgb_compute_bitsPerInt, "fastPFOR_varintgb"),

      make_pair(fastPFOR_simple8b_compute_bitsPerInt, "fastPFOR_simple8b"),
      make_pair(fastPFOR_simdbinarypacking_compute_bitsPerInt, "fastPFOR_simdbinarypacking"),
      make_pair(fastPFOR_simdgroupsimple_compute_bitsPerInt, "fastPFOR_simdgroupsimple"),
      make_pair(fastPFOR_simdgroupsimple_ringbuf_compute_bitsPerInt, "fastPFOR_simdgroupsimple_ringbuf"),
      make_pair(fastPFOR_copy_compute_bitsPerInt, "fastPFOR_copy"),

      // Broken
      // make_pair(fastPFOR_varintg8iu_compute_bitsPerInt, "fastPFOR_varintg8iu"),
      //make_pair(fastPFOR_simple16_compute_bitsPerInt, "fastPFOR_simple16"),
      //make_pair(fastPFOR_simple9_compute_bitsPerInt, "fastPFOR_simple9"),
      // make_pair(fastPFOR_simple9_rle_compute_bitsPerInt, "fastPFOR_simple9_rle"), // bug
      // make_pair(fastPFOR_simple8b_rle_compute_bitsPerInt, "fastPFOR_simple8b_rle"), // bug
      // make_pair(fastPFOR_snappy_compute_bitsPerInt, "fastPFOR_snappy"),  // todo compile with snappy
      // make_pair(SIMDCompressionAndIntersection_frameofreference_with_select_compute_bitsPerInt, "SIMDCompressionAndIntersection_frameofreference_with_select"), // bug

  };

  for (size_t j = 0; j < functions.size(); j++) {
    std::vector<std::vector<ValueT>> inputs = {get_with_small_numbers(), get_with_sequential_sorted_numbers(),
                                               get_with_huge_numbers(), get_with_random_walk(),
                                               get_with_month_categorical_numbers(), get_with_year_categorical_numbers()};
    std::vector<std::string> names = {"small_numbers", "sequential_numbers", "huge_numbers", "random_walk", "categorical_numbers", "year_numbers"};
    for (int i = 0; i < static_cast<int>(inputs.size()); i++) {
      csvFile << functions[j].second << "," << names[i] << "," << functions[j].first(inputs[i]) << std::endl;
      cout << functions[j].second << "," << names[i] << "," << functions[j].first(inputs[i]) << std::endl;
    }
  }

  csvFile << std::endl;
  csvFile.close();
}

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:
 protected:
};

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(unencoded);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(dictionary);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_direct);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_direct_chunking);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_bitcompression);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(turboPFOR_block);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(maskedVByteDelta);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(streamVByte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(streamVByte0124);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(streamVByteDelta);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_varint);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(oroch_integerArray);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_vlc_vector);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(sdsl_lite_dac_vector);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_fastpfor256);

//FastPFOR Codecs
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
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_maskedvbyte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_streamvbyte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_varintgb);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple8b);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdbinarypacking);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdgroupsimple);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simdgroupsimple_ringbuf);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_copy);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_fastpfor);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_varint);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_vbyte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_maskedvbyte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_streamvbyte);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_frameofreference);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_simdframeofreference);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_varintgb);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4fastpford4);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4fastpfordm);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4fastpford1);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4fastpford2);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_bp32);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_ibp32);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d1ni);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d2ni);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d4ni);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128dmni);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d1);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d2);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128d4);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_s4bp128dm);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_for);

// for the ones that support point access, also benchmark this
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_simdframeofreference_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_for_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_maskedvbyte_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_streamvbyte_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_varint_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_vbyte_with_select);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_varintgb_with_select);

// Broken
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(SIMDCompressionAndIntersection_frameofreference_with_select); // bug
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_varintg8iu);
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple16);
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple9);
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_snappy);  // todo compile with snappy
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple8b_rle);
// COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING_ALL_DATA(fastPFOR_simple9_rle); // bug

// comment in to run all encodings, ensure that they are correct and write out their compression ratio (bits per integer)
BENCHMARK_F(BenchmarkColumnCompressionFixture, writeBitsPerInt)(benchmark::State& state) { writeBitsPerInt(); }

}  // namespace opossum