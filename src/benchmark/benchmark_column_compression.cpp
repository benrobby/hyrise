#include <memory>

#include "benchmark/benchmark.h"

#include "headers/codecfactory.h"

using namespace FastPForLib;

namespace opossum {

  using ValueT = uint32_t;

  void benchmark_encoding_fastPFOR(const std::vector<ValueT>& _vec, IntegerCODEC& codec, benchmark::State& state) {
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);

  for (auto _ : state) {
    size_t compressedsize = enc.size();
    codec.encodeArray(_vec.data(), _vec.size(), enc.data(),
                      compressedsize);
    // if desired, shrink back the array:
    enc.resize(compressedsize);
    enc.shrink_to_fit();
    benchmark::ClobberMemory();
  }
}

void benchmark_decoding_fastPFOR(const std::vector<ValueT>& _vec, IntegerCODEC& codec, benchmark::State& state) {
  // Encode
  std::vector<ValueT> enc = std::vector<uint32_t>(_vec.size() + 1024);
  size_t compressedsize = enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), enc.data(),
                    compressedsize);
  enc.resize(compressedsize);
  enc.shrink_to_fit();

  // Decode
  std::vector<ValueT> dec = std::vector<uint32_t>(_vec.size());
  size_t recoveredsize = dec.size();

  for (auto _ : state) {
    codec.decodeArray(enc.data(), enc.size(),
                      dec.data(), recoveredsize);
    dec.resize(recoveredsize);
    benchmark::ClobberMemory();
  }
}

class BenchmarkColumnCompressionFixture : public benchmark::Fixture {
 public:
  void SetUp(::benchmark::State& state) override {
    // Fill the vector with 1M values in the pattern 0, 1, 2, 3, 0, 1, 2, 3, ...
    // The "TableScan" will scan for one value (2), so it will select 25%.
  }
  void TearDown(::benchmark::State& state) override { 
    // empty
   }

  void set_up_vector_with_small_numbers() {
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

  void set_up_with_huge_numbers(){
     _vec.resize(65'000);
    std::generate(_vec.begin(), _vec.end(), []() {
      static ValueT v = 0;
      v = 1'000'000 + ((v * 7) % 1'000'000);
      return v;
    });
  }

    void set_up_with_random_walk(){
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
  // Add some benchmark-specific setup here
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_vector_with_small_numbers();
  benchmark_encoding_fastPFOR(_vec, codec, state);
  
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Small_Numbers_Decoding)(benchmark::State& state) {
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_vector_with_small_numbers();
  benchmark_decoding_fastPFOR(_vec, codec, state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Sequential_Numbers_Encoding)(benchmark::State& state) {
  // Add some benchmark-specific setup here
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_sequential_numbers();
  benchmark_encoding_fastPFOR(_vec, codec, state);
  
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Sequential_Numbers_Decoding)(benchmark::State& state) {
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_sequential_numbers();
  benchmark_decoding_fastPFOR(_vec, codec, state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Huge_Numbers_Encoding)(benchmark::State& state) {
  // Add some benchmark-specific setup here
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_huge_numbers();
  benchmark_encoding_fastPFOR(_vec, codec, state);
  
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Huge_Numbers_Decoding)(benchmark::State& state) {
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_huge_numbers();
  benchmark_decoding_fastPFOR(_vec, codec, state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Random_Walk_Encoding)(benchmark::State& state) {
  // Add some benchmark-specific setup here
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_random_walk();
  benchmark_encoding_fastPFOR(_vec, codec, state);
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, FastPFOR_Random_Walk_Decoding)(benchmark::State& state) {
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  set_up_with_random_walk();
  benchmark_decoding_fastPFOR(_vec, codec, state);
}
}  // namespace opossum
