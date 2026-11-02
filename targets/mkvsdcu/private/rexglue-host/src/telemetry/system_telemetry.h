#pragma once

#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Once-a-second process, thread, memory and GPU measurements for the
// performance overlay and the CSV log. Sampling runs on its own thread and
// only while something is watching.
namespace telemetry {

struct ThreadLoad {
  uint32_t id = 0;
  std::string name;
  double core_percent = 0;  // share of one logical core
};

struct SystemSnapshot {
  bool valid = false;
  uint32_t logical_cores = 0;
  double process_cpu_percent = 0;  // share of the whole CPU
  double system_cpu_percent = 0;
  uint32_t thread_count = 0;
  std::vector<ThreadLoad> busiest_threads;

  uint64_t working_set_bytes = 0;
  uint64_t private_bytes = 0;
  double page_faults_per_second = 0;

  std::string gpu_name;
  // Busiest GPU engine, as in Task Manager; -1 when unavailable.
  double gpu_3d_percent = -1;        // this process
  double gpu_3d_total_percent = -1;  // every process
  std::string gpu_engine;            // e.g. "3D" or "graphics_1"
  uint64_t vram_used_bytes = 0;
  uint64_t vram_budget_bytes = 0;
  uint64_t shared_used_bytes = 0;

  double vblank_hz = 0;  // guest vertical blank interrupts per second

  // Average per presented frame over the last second; -1 when unavailable.
  double swap_queue_ms = -1;   // guest VdSwap until the swap is issued
  double swap_pacing_ms = -1;  // part of that in the frame rate limiter
  double gpu_idle_ms = -1;      // GPU worker waiting for guest commands
  double wait_reg_mem_ms = -1;  // GPU worker blocked in PM4_WAIT_REG_MEM
  double frames_in_flight = -1; // guest swaps not yet issued, averaged at VdSwap
  uint32_t wait_target = 0;     // last WAIT_REG_MEM target; bit 31 = memory
  // Last WAIT_REG_MEM compare: function, reference, mask, value read, interval.
  uint32_t wait_function = 0, wait_ref = 0, wait_mask = 0, wait_value = 0, wait_interval = 0;
};

// Counters read from the guest GPU each sample. Totals since startup.
struct GpuCounters {
  uint32_t vblank = 0;
  uint64_t swap_samples = 0;
  uint64_t swap_queue_us = 0;
  uint64_t swap_pacing_us = 0;
  uint64_t gpu_idle_us = 0;
  uint64_t wait_reg_mem_us = 0;
  uint64_t backlog = 0, backlog_samples = 0;
  uint32_t wait_target = 0;
  uint32_t wait_function = 0, wait_ref = 0, wait_mask = 0, wait_value = 0, wait_interval = 0;
};

class SystemSampler {
 public:
  // `gpu_counters` reads the guest GPU's counters; may be empty.
  explicit SystemSampler(std::function<GpuCounters()> gpu_counters);
  ~SystemSampler();

  // Sampling runs while at least one reason is active.
  void SetWanted(const char* reason, bool wanted);
  SystemSnapshot Latest() const;

  // Starts or stops a once-a-second CSV row. Returns the file, or empty.
  std::filesystem::path SetCsvLogging(bool enabled, const std::filesystem::path& directory);
  std::filesystem::path csv_path() const;

 private:
  struct Impl;
  void Run();

  std::unique_ptr<Impl> impl_;
  std::function<GpuCounters()> gpu_counters_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  std::vector<std::string> reasons_;
  bool stop_ = false;
  SystemSnapshot latest_;
  std::thread thread_;
};

}  // namespace telemetry
