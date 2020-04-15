#pragma once

#include "utils/meta_tables/abstract_meta_table.hpp"

namespace opossum {

/**
 * This is a class for showing dynamic system information such as CPU and RAM load.
 */
class MetaSystemUtilizationTable : public AbstractMetaTable {
 public:
  MetaSystemUtilizationTable();

  const std::string& name() const final;

  void init();

  struct SystemMemoryUsage {
    int64_t total_ram;
    int64_t total_swap;
    int64_t total_memory;
    int64_t free_ram;
    int64_t free_swap;
    int64_t free_memory;
  };

 protected:
  std::shared_ptr<Table> _on_generate() const final;

  struct LoadAvg {
    float load_1_min;
    float load_5_min;
    float load_15_min;
  };

  struct ProcessMemoryUsage {
    int64_t virtual_memory;
    int64_t physical_memory;
  };

  LoadAvg _get_load_avg() const;
  int _get_cpu_count() const;
  float _get_system_cpu_usage() const;
  float _get_process_cpu_usage() const;
  SystemMemoryUsage _get_system_memory_usage() const;
  int64_t _int_from_string(std::string input_string) const;
  ProcessMemoryUsage _get_process_memory_usage() const;
};

}  // namespace opossum