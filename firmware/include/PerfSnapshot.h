#pragma once

#include <stdint.h>

/**
 * One /eyes/perf sample (heap, PSRAM, loop, CPU, radio, ball).
 */
struct PerfSnapshot {
  uint32_t heap_free;
  uint32_t heap_size;
  uint32_t heap_min;
  uint32_t psram_free;
  uint32_t psram_size;
  float loop_hz;
  float cpu0_pct;
  float cpu1_pct;
  int wifi_rssi;
  float ball_fps;
  uint32_t uptime_s;
};
