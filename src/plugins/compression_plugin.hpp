#pragma once

#include <algorithm>
#include <mutex>
#include <numeric>
#include <queue>
#include <thread>

#include "gtest/gtest_prod.h"
#include "hyrise.hpp"
#include "storage/chunk.hpp"
#include "utils/abstract_plugin.hpp"
#include "utils/pausable_loop_thread.hpp"
#include "utils/settings/abstract_setting.hpp"
#include "utils/singleton.hpp"

namespace opossum {

class CompressionPlugin : public AbstractPlugin {
 public:
  const std::string description() const final;

  void start() final;

  void stop() final;

 private:
  // Budget in megabyte
  class MemoryBudgetSetting : public AbstractSetting {
   public:
    explicit MemoryBudgetSetting() : AbstractSetting("Plugin::Compression::MemoryBudget") {}
    const std::string& description() const final {
      static const auto description = std::string{"The memory budget to target for the CompressionPlugin."};
      return description;
    }
    const std::string& get() { return _value; }
    void set(const std::string& value) final { _value = value; }

    std::string _value = "9999999999";
  };

  constexpr static std::chrono::milliseconds THREAD_INTERVAL = std::chrono::milliseconds(7'500);

  std::unique_ptr<PausableLoopThread> _loop_thread;

  bool _stop_requested = false;

  int64_t _compress_column(const std::string table_name, const std::string column_name, const std::string encoding_name,
                           const bool column_was_accessed, const int64_t desired_memory_usage_reduction);

  void _optimize_compression();

  std::shared_ptr<MemoryBudgetSetting> _memory_budget_setting;

  std::vector<std::shared_ptr<BaseSegment>> _keep_alive_stash;

  // Hand tuned, determined by an greedy "Microsoft-like heuristic". Adapted the use of FSBA and changed it almost
  // everywhere to SIMDBP128 (long story).
  // Update: temporary change to FSBA due to encoding crashes.
  std::vector<std::vector<std::string>> _static_compression_config = {
      // {"UNACCESSED", "lineitem", "l_shipinstruct", "FixedStringDictionarySIMDBP128"},
      // {"UNACCESSED", "lineitem", "l_comment", "LZ4SIMDBP128"},
      // {"UNACCESSED", "nation", "n_comment", "FixedStringDictionarySIMDBP128"},
      // {"UNACCESSED", "part", "p_comment", "LZ4SIMDBP128"},
      // {"UNACCESSED", "lineitem", "l_linenumber", "FrameOfReferenceSIMDBP128"},
      // {"UNACCESSED", "orders", "o_clerk", "LZ4SIMDBP128"},
      // {"UNACCESSED", "part", "p_retailsize", "DictionarySIMDBP128"},
      // {"UNACCESSED", "partsupp", "ps_comment", "LZ4SIMDBP128"},
      // {"UNACCESSED", "region", "r_comment", "LZ4SIMDBP128"},
      // {"ACCESSED", "customer", "c_comment", "DictionaryFSBA"},
      // {"ACCESSED", "customer", "c_address", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "customer", "c_phone", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "customer", "c_name", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_commitdate", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_receiptdate", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_extendedprice", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_shipdate", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_orderkey", "DictionaryFSBA"},
      // {"ACCESSED", "orders", "o_orderkey", "DictionaryFSBA"},
      // {"ACCESSED", "part", "p_partkey", "DictionaryFSBA"},
      // {"ACCESSED", "customer", "c_custkey", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_suppkey", "DictionaryFSBA"},
      // {"ACCESSED", "partsupp", "ps_suppkey", "DictionaryFSBA"},
      // {"ACCESSED", "customer", "c_nationkey", "DictionaryFSBA"},
      // {"ACCESSED", "partsupp", "ps_partkey", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_partkey", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_discount", "DictionaryFSBA"},
      // {"ACCESSED", "partsupp", "ps_supplycost", "DictionaryFSBA"},
      // {"ACCESSED", "part", "p_name", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "orders", "o_custkey", "DictionaryFSBA"},
      // {"ACCESSED", "customer", "c_acctbal", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_quantity", "DictionaryFSBA"},
      // {"ACCESSED", "supplier", "s_comment", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "supplier", "s_phone", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "partsupp", "ps_availqty", "DictionaryFSBA"},
      // {"ACCESSED", "supplier", "s_address", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "orders", "o_comment", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "supplier", "s_suppkey", "DictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_tax", "DictionaryFSBA"},
      // {"ACCESSED", "supplier", "s_name", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "orders", "o_totalprice", "DictionaryFSBA"},
      // {"ACCESSED", "orders", "o_shippriority", "DictionaryFSBA"},
      // {"ACCESSED", "part", "p_size", "DictionaryFSBA"},
      // {"ACCESSED", "orders", "o_orderdate", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "nation", "n_nationkey", "FrameOfReferenceFSBA"},
      // {"ACCESSED", "part", "p_type", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "nation", "n_name", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "part", "p_container", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_shipmode", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_returnflag", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "part", "p_brand", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "lineitem", "l_linestatus", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "nation", "n_regionkey", "FrameOfReferenceFSBA"},
      // {"ACCESSED", "region", "r_regionkey", "FrameOfReferenceFSBA"},
      // {"ACCESSED", "region", "r_name", "DictionaryFSBA"},
      // {"ACCESSED", "orders", "o_orderstatus", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "part", "p_mfgr", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "customer", "c_mktsegment", "FixedStringDictionaryFSBA"},
      // {"ACCESSED", "orders", "o_orderpriority", "FixedStringDictionaryFSBA"},
      {"UNACCESSED", "lineitem", "l_shipinstruct", "FixedStringDictionarySIMDBP128"},
      {"UNACCESSED", "lineitem", "l_comment", "LZ4SIMDBP128"},
      {"UNACCESSED", "nation", "n_comment", "FixedStringDictionarySIMDBP128"},
      {"UNACCESSED", "part", "p_comment", "LZ4SIMDBP128"},
      {"UNACCESSED", "lineitem", "l_linenumber", "FrameOfReferenceSIMDBP128"},
      {"UNACCESSED", "orders", "o_clerk", "LZ4SIMDBP128"},
      {"UNACCESSED", "part", "p_retailsize", "DictionarySIMDBP128"},
      {"UNACCESSED", "partsupp", "ps_comment", "LZ4SIMDBP128"},
      {"UNACCESSED", "region", "r_comment", "LZ4SIMDBP128"},
      {"ACCESSED", "customer", "c_comment", "DictionaryFSBA"},
      {"ACCESSED", "customer", "c_address", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "customer", "c_phone", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "customer", "c_name", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_commitdate", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_receiptdate", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_extendedprice", "DictionaryFSBA"},
      {"ACCESSED", "lineitem", "l_shipdate", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_orderkey", "DictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_orderkey", "DictionarySIMDBP128"},
      {"ACCESSED", "part", "p_partkey", "DictionaryFSBA"},
      {"ACCESSED", "customer", "c_custkey", "DictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_suppkey", "DictionarySIMDBP128"},
      {"ACCESSED", "partsupp", "ps_suppkey", "DictionarySIMDBP128"},
      {"ACCESSED", "customer", "c_nationkey", "DictionarySIMDBP128"},
      {"ACCESSED", "partsupp", "ps_partkey", "DictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_partkey", "DictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_discount", "DictionarySIMDBP128"},
      {"ACCESSED", "partsupp", "ps_supplycost", "DictionarySIMDBP128"},
      {"ACCESSED", "part", "p_name", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_custkey", "DictionarySIMDBP128"},
      {"ACCESSED", "customer", "c_acctbal", "DictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_quantity", "DictionarySIMDBP128"},
      {"ACCESSED", "supplier", "s_comment", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "supplier", "s_phone", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "partsupp", "ps_availqty", "DictionarySIMDBP128"},
      {"ACCESSED", "supplier", "s_address", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_comment", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "supplier", "s_suppkey", "DictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_tax", "DictionarySIMDBP128"},
      {"ACCESSED", "supplier", "s_name", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_totalprice", "DictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_shippriority", "DictionarySIMDBP128"},
      {"ACCESSED", "part", "p_size", "DictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_orderdate", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "nation", "n_nationkey", "FrameOfReferenceSIMDBP128"},
      {"ACCESSED", "part", "p_type", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "nation", "n_name", "LZ4SIMDBP128"},
      {"ACCESSED", "part", "p_container", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_shipmode", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_returnflag", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "part", "p_brand", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "lineitem", "l_linestatus", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "nation", "n_regionkey", "FrameOfReferenceSIMDBP128"},
      {"ACCESSED", "region", "r_regionkey", "FrameOfReferenceSIMDBP128"},
      {"ACCESSED", "region", "r_name", "DictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_orderstatus", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "part", "p_mfgr", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "customer", "c_mktsegment", "FixedStringDictionarySIMDBP128"},
      {"ACCESSED", "orders", "o_orderpriority", "FixedStringDictionarySIMDBP128"},
  };
};

}  // namespace opossum
