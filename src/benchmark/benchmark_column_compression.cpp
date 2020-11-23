#include <memory>

#include "benchmark/benchmark.h"

#include "headers/codecfactory.h"

using namespace FastPForLib;

namespace opossum {

using ValueT = uint32_t;

void fastPFOR_benchmark_encoding(const std::vector<ValueT>& _vec, IntegerCODEC& codec, benchmark::State& state) {
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);

  for (auto _ : state) {
    size_t compressedsize = enc.size();
    codec.encodeArray(_vec.data(), _vec.size(), enc.data(), compressedsize);
    // if desired, shrink back the array:
    enc.resize(compressedsize);
    enc.shrink_to_fit();
    benchmark::ClobberMemory();
  }
}

void fastPFOR_benchmark_decoding(const std::vector<ValueT>& _vec, IntegerCODEC& codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), enc.data(), compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(_vec.size());
  size_t recoveredsize = dec.size();

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(), dec.data(), recoveredsize);
    dec.resize(recoveredsize);
    benchmark::ClobberMemory();
  }
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

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:

  void write_bitsPerInt() {
    std::ofstream csvFile("bits_per_int.csv");

    csvFile << "name,bitsPerInt" << std::endl;
    float bitsPerInt = 0.0;

    IntegerCODEC& codec = *CODECFactory::getFromName("fastpfor256");
    set_up_with_small_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(_vec, codec);
    csvFile << "FastPFOR_Small_Numbers," << bitsPerInt << std::endl;
    set_up_with_sequential_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(_vec, codec);
    csvFile << "FastPFOR_Sequential_Numbers," << bitsPerInt << std::endl;
    set_up_with_huge_numbers();
    bitsPerInt = fastPFOR_compute_bitsPerInt(_vec, codec);
    csvFile << "FastPFOR_Huge_Numbers," << bitsPerInt << std::endl;
    set_up_with_random_walk();
    bitsPerInt = fastPFOR_compute_bitsPerInt(_vec, codec);
    csvFile << "FastPFOR_Random_Walk," << bitsPerInt << std::endl;

    csvFile << std::endl;
    csvFile.close();
  }

  void set_up_with_small_numbers() {
    _vec.resize(65'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 0;
      v = (v + 1) % 4;
      return v;
    });
  }

  void set_up_with_sequential_numbers() {
    _vec.resize(65'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 0;
      v = v + 5;
      return v;
    });
  }

  void set_up_with_huge_numbers() {
    _vec.resize(65'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 0;
      v = 1'000'000 + ((v * 7) % 1'000'000);
      return v;
    });
  }

  void set_up_with_random_walk() {
    _vec.resize(65'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 1'000'000;
      if ((v * 13 % 7) > 3) {
        v = v + 5;
      } else {
        v = v - 5;
      }
      return v;
    });
  }

 protected:
  std::vector<ValueT> _vec;
};

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Small_Numbers_Encoding)(benchmark::State& state) {
  set_up_with_small_numbers();
  fastPFOR_benchmark_encoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Small_Numbers_Decoding)(benchmark::State& state) {
  set_up_with_small_numbers();
  fastPFOR_benchmark_decoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Sequential_Numbers_Encoding)(benchmark::State& state) {
  set_up_with_sequential_numbers();
  fastPFOR_benchmark_encoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Sequential_Numbers_Decoding)(benchmark::State& state) {
  set_up_with_sequential_numbers();
  fastPFOR_benchmark_decoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Huge_Numbers_Encoding)(benchmark::State& state) {
  set_up_with_huge_numbers();
  fastPFOR_benchmark_encoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Huge_Numbers_Decoding)(benchmark::State& state) {
  set_up_with_huge_numbers();
  fastPFOR_benchmark_decoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Random_Walk_Encoding)(benchmark::State& state) {
  set_up_with_random_walk();
  fastPFOR_benchmark_encoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Random_Walk_Decoding)(benchmark::State& state) {
  set_up_with_random_walk();
  fastPFOR_benchmark_decoding(_vec, *CODECFactory::getFromName("fastpfor256"), state);
}

// comment in to run all encodings, ensure that they are correct and write out their compression ratio (bits per integer)
BENCHMARK_F(BenchmarkColumnCompressionFixture, write_BitsPerInt)(benchmark::State& state) { write_bitsPerInt(); }

}  // namespace opossum
