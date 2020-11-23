#include <memory>

#include "benchmark/benchmark.h"

#include "headers/codecfactory.h"


namespace opossum {

using ValueT = uint32_t;

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
  std::vector<ValueT> _enc;
  std::vector<ValueT> _dec;
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

BENCHMARK_F(BenchmarkColumnCompressionFixture, BM_Column_Compression_FastPFOR_Encoding)(benchmark::State& state) {
  // Add some benchmark-specific setup here

  using namespace FastPForLib;
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");

  for (auto _ : state) {

    _enc = std::vector<uint32_t>(_vec.size() + 1024);

    size_t compressedsize = _enc.size();
    codec.encodeArray(_vec.data(), _vec.size(), _enc.data(),
                      compressedsize);
    // if desired, shrink back the array:
    _enc.resize(compressedsize);
    _enc.shrink_to_fit();
  }

  std::cout << std::setprecision(3);
  std::cout << "You are using "
            << 32.0 * static_cast<double>(_enc.size()) /
                   static_cast<double>(_vec.size())
            << " bits per integer. " << std::endl;
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, BM_Column_Compression_FastPFOR_Decoding)(benchmark::State& state) {
  using namespace FastPForLib;
  IntegerCODEC &codec = *CODECFactory::getFromName("fastpfor256");
  _enc = std::vector<uint32_t>(_vec.size() + 1024);

  size_t compressedsize = _enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), _enc.data(),
                    compressedsize);
  // if desired, shrink back the array:
  _enc.resize(compressedsize);
  _enc.shrink_to_fit();

  for (auto _ : state) {

    _dec = std::vector<uint32_t>(_vec.size());
    size_t recoveredsize = _dec.size();
    codec.decodeArray(_enc.data(), _enc.size(),
                      _dec.data(), recoveredsize);
    _dec.resize(recoveredsize);
  }
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, BM_Column_Compression_FastPFOR_Simple_Encoding)(benchmark::State& state) {
  // Add some benchmark-specific setup here

  using namespace FastPForLib;
  IntegerCODEC &codec = *CODECFactory::getFromName("simplepfor");

  for (auto _ : state) {

    _enc = std::vector<uint32_t>(_vec.size() + 1024);

    size_t compressedsize = _enc.size();
    codec.encodeArray(_vec.data(), _vec.size(), _enc.data(),
                      compressedsize);
    // if desired, shrink back the array:
    _enc.resize(compressedsize);
    _enc.shrink_to_fit();
  }

  std::cout << std::setprecision(3);
  std::cout << "You are using "
            << 32.0 * static_cast<double>(_enc.size()) /
                   static_cast<double>(_vec.size())
            << " bits per integer. " << std::endl;
}

BENCHMARK_F(BenchmarkColumnCompressionFixture, BM_Column_Compression_FastPFOR_Simple_Decoding)(benchmark::State& state) {
  using namespace FastPForLib;
  IntegerCODEC &codec = *CODECFactory::getFromName("simplepfor");
  _enc = std::vector<uint32_t>(_vec.size() + 1024);

  size_t compressedsize = _enc.size();
  codec.encodeArray(_vec.data(), _vec.size(), _enc.data(),
                    compressedsize);
  // if desired, shrink back the array:
  _enc.resize(compressedsize);
  _enc.shrink_to_fit();

  for (auto _ : state) {

    _dec = std::vector<uint32_t>(_vec.size());
    size_t recoveredsize = _dec.size();
    codec.decodeArray(_enc.data(), _enc.size(),
                      _dec.data(), recoveredsize);
    _dec.resize(recoveredsize);
  }
}


}  // namespace opossum
