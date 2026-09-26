#include "CpuLoadSampler.h"

#include <Arduino.h>
#include <esp_freertos_hooks.h>

namespace {

volatile uint32_t g_idle_hits0 = 0;
volatile uint32_t g_idle_hits1 = 0;

bool onIdleCore0() {
  ++g_idle_hits0;
  return true;
}

bool onIdleCore1() {
  ++g_idle_hits1;
  return true;
}

float busyFromIdle(uint32_t idle_per_s, uint32_t& peak_idle_per_s) {
  if (idle_per_s > peak_idle_per_s) {
    peak_idle_per_s = idle_per_s;
  }
  if (peak_idle_per_s == 0) {
    return 0.0f;
  }
  float busy = 100.0f * (1.0f - static_cast<float>(idle_per_s) /
                                     static_cast<float>(peak_idle_per_s));
  if (busy < 0.0f) {
    busy = 0.0f;
  }
  if (busy > 100.0f) {
    busy = 100.0f;
  }
  return busy;
}

}  // namespace

CpuLoadSampler::CpuLoadSampler()
    : last_hits0_(0),
      last_hits1_(0),
      last_ms_(0),
      peak_idle_per_s0_(0),
      peak_idle_per_s1_(0),
      primed_(false) {}

void CpuLoadSampler::begin() {
  esp_register_freertos_idle_hook_for_cpu(onIdleCore0, 0);
  esp_register_freertos_idle_hook_for_cpu(onIdleCore1, 1);
  last_hits0_ = g_idle_hits0;
  last_hits1_ = g_idle_hits1;
  last_ms_ = millis();
  primed_ = false;
}

void CpuLoadSampler::sample(float& core0_pct, float& core1_pct) {
  const uint32_t now_ms = millis();
  const uint32_t hits0 = g_idle_hits0;
  const uint32_t hits1 = g_idle_hits1;
  const uint32_t dt_ms = now_ms - last_ms_;
  if (dt_ms < 50) {
    core0_pct = 0.0f;
    core1_pct = 0.0f;
    return;
  }
  const uint32_t d0 = hits0 - last_hits0_;
  const uint32_t d1 = hits1 - last_hits1_;
  last_hits0_ = hits0;
  last_hits1_ = hits1;
  last_ms_ = now_ms;

  const uint32_t idle0_per_s = d0 * 1000u / dt_ms;
  const uint32_t idle1_per_s = d1 * 1000u / dt_ms;
  if (!primed_) {
    peak_idle_per_s0_ = idle0_per_s;
    peak_idle_per_s1_ = idle1_per_s;
    primed_ = true;
    core0_pct = 0.0f;
    core1_pct = 0.0f;
    return;
  }
  core0_pct = busyFromIdle(idle0_per_s, peak_idle_per_s0_);
  core1_pct = busyFromIdle(idle1_per_s, peak_idle_per_s1_);
}
