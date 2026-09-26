#pragma once

#include <stdint.h>

/**
 * Estimates per-core CPU load from FreeRTOS idle-hook hit rates.
 */
class CpuLoadSampler {
 public:
  /** Constructs an unprimed sampler. */
  CpuLoadSampler();

  /** Registers idle hooks on both cores (call once from setup). */
  void begin();

  /**
   * Samples idle counters (~call at 1 Hz).
   * @param core0_pct busy percent for CPU0 [0..100]
   * @param core1_pct busy percent for CPU1 [0..100]
   */
  void sample(float& core0_pct, float& core1_pct);

 private:
  uint32_t last_hits0_;
  uint32_t last_hits1_;
  uint32_t last_ms_;
  uint32_t peak_idle_per_s0_;
  uint32_t peak_idle_per_s1_;
  bool primed_;
};
