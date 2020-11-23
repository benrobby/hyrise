#include <memory>

#include "benchmark/benchmark.h"
#include "varintdecode.h"
#include "varintencode.h"
#include "headers/codecfactory.h"

#define COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(setupMethod, benchmarkMethodEncode, benchmarkMethodDecode) COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodEncode); COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethodDecode);
#define COLUMN_COMPRESSION_BENCHMARK(setupMethod, benchmarkMethod) BENCHMARK_F(BenchmarkColumnCompressionFixture, benchmarkMethod ## _ ## setupMethod)(benchmark::State& state) { auto vec = setupMethod(); benchmarkMethod(vec, state); }

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


// FastPFOR


void fastPFOR_benchmark_encoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() + 1024);

  for (auto _ : state) {
    size_t compressedsize = enc.size();
    codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
    // if desired, shrink back the array:
    enc.resize(compressedsize);
    enc.shrink_to_fit();
    benchmark::ClobberMemory();
  }
}

void fastPFOR_256_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  fastPFOR_benchmark_encoding(vec, *CODECFactory::getFromName("fastpfor256"), state);
}

void fastPFOR_benchmark_decoding(const std::vector<ValueT>& vec, IntegerCODEC& codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(vec.data(), vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(vec.size());
  size_t recoveredsize = dec.size();

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
    dec.resize(recoveredsize);
    benchmark::ClobberMemory();
  }
}

void fastPFOR_256_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  fastPFOR_benchmark_decoding(vec, *CODECFactory::getFromName("fastpfor256"), state);
}

float fastPFOR_compute_bitsPerInt(std::vector<ValueT>& _vec, IntegerCODEC& codec) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(_vec.size());
  size_t recoveredsize = dec.size();

  codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
  dec.resize(recoveredsize);

  if (_vec != dec)
    throw std::runtime_error("bug!");

  // # bits (encoded) / # elements to encode
  return 32.0 * static_cast<double>(enc.size()) / static_cast<double>(_vec.size());
}

 void write_bitsPerInt() {
    std::ofstream csvFile("bits_per_int.csv");

    csvFile << "name,bitsPerInt" << std::endl;
    float bitsPerInt = 0.0;

    IntegerCODEC& codec = *CODECFactory::getFromName("fastpfor256");
    std::vector<ValueT> vec = get_with_small_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(vec, codec);
    csvFile << "FastPFOR_Small_Numbers," << bitsPerInt << std::endl;
    vec = get_with_sequential_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(vec, codec);
    csvFile << "FastPFOR_Sequential_Numbers," << bitsPerInt << std::endl;
    vec = get_with_huge_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(vec, codec);
    csvFile << "FastPFOR_Huge_Numbers," << bitsPerInt << std::endl;
    vec = get_with_random_walk();
    bitsPerInt = fastPFOR_compute_bitsPerInt(vec, codec);
    csvFile << "FastPFOR_Random_Walk," << bitsPerInt << std::endl;

    csvFile << std::endl;
    csvFile.close();
  }


// MaskedVByte


void maskedVbyte_benchmark_encoding(const std::vector<ValueT>& vec, benchmark::State& state) { 
  int N = vec.size();
	ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);

	uint8_t* compressedbuffer = (uint8_t*) malloc(N * sizeof(ValueT));

  for (auto _ : state){
    vbyte_encode(datain, vec.size(), compressedbuffer);
  }
}

void maskedVbyte_benchmark_decoding(const std::vector<ValueT>& vec, benchmark::State& state) {
  // Encode
  int N = vec.size();
	ValueT* datain = new ValueT[N];
  std::copy(vec.begin(), vec.end(), datain);
  uint8_t* compressedbuffer = (uint8_t*) malloc(N * sizeof(ValueT));
  vbyte_encode(datain, vec.size(), compressedbuffer);

  // Decode
	ValueT* data_recovered = new ValueT[N];

  for (auto _ : state) {
    masked_vbyte_decode(compressedbuffer, data_recovered, N);
  }
}

float maskedVByte_compute_bitsPerInt(std::vector<ValueT>& vec) {
    // Encode
    int N = vec.size();
    ValueT* datain = new ValueT[N];
    std::copy(vec.begin(), vec.end(), datain);
    uint8_t* compressedbuffer = (uint8_t*) malloc(N * sizeof(ValueT));
    vbyte_encode(datain, vec.size(), compressedbuffer);

    // Decode
	  ValueT* data_recovered = new ValueT[N];
    size_t computed_size = masked_vbyte_decode(compressedbuffer, data_recovered, N);

    return computed_size / N * 4; // since computed size is givenout in byte
}

  void maskedVByte_writeBitsPerInt() {
    std::ofstream csvFile("masked_vbyte_bits_per_int.csv");
    csvFile << "name,bitsPerInt" << std::endl;


    std::vector<std::vector<ValueT>> inputs = {get_with_small_numbers(), get_with_sequential_numbers(),
        get_with_huge_numbers(), get_with_random_walk()};
    std::vector<std::string> names = {"small_numbers", "sequential_numbers", "huge_numbers","random_walk"};
    for (int i = 0; i < (int) inputs.size(); i++){
        float bits_per_int = maskedVByte_compute_bitsPerInt(inputs[i]);
        csvFile << names[i] << bits_per_int << std::endl;
    }
    csvFile << std::endl;
    csvFile.close();
}


class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:

 protected:
};

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_small_numbers, fastPFOR_256_benchmark_encoding, fastPFOR_256_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_sequential_numbers, fastPFOR_256_benchmark_encoding, fastPFOR_256_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_huge_numbers, fastPFOR_256_benchmark_encoding, fastPFOR_256_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_random_walk, fastPFOR_256_benchmark_encoding, fastPFOR_256_benchmark_decoding);

COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_sequential_numbers, maskedVbyte_benchmark_encoding, maskedVbyte_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_small_numbers, maskedVbyte_benchmark_encoding, maskedVbyte_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_huge_numbers, maskedVbyte_benchmark_encoding, maskedVbyte_benchmark_decoding);
COLUMN_COMPRESSION_BENCHMARK_ENCODING_DECODING(get_with_random_walk, maskedVbyte_benchmark_encoding, maskedVbyte_benchmark_decoding);


// comment in to run all encodings, ensure that they are correct and write out their compression ratio (bits per integer)
BENCHMARK_F(BenchmarkColumnCompressionFixture, write_BitsPerInt)(benchmark::State& state) { write_bitsPerInt(); }
BENCHMARK_F(BenchmarkColumnCompressionFixture, maskedVByte_writeBitsPerInt)(benchmark::State& state) { maskedVByte_writeBitsPerInt(); }


}  // namespace opossum
