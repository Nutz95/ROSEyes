#include "PerfTelemetryFormatter.h"

#include <stdio.h>

int PerfTelemetryFormatter::format(char* buffer, int capacity,
                                   const PerfSnapshot& snapshot) {
  if (buffer == nullptr || capacity < 8) {
    return -1;
  }
  const float heap_pct =
      snapshot.heap_size > 0
          ? (100.0f * static_cast<float>(snapshot.heap_free) /
             static_cast<float>(snapshot.heap_size))
          : 0.0f;
  const float psram_pct =
      snapshot.psram_size > 0
          ? (100.0f * static_cast<float>(snapshot.psram_free) /
             static_cast<float>(snapshot.psram_size))
          : 0.0f;
  const int written = snprintf(
      buffer, static_cast<size_t>(capacity),
      "{\"heap_free\":%lu,\"heap_size\":%lu,\"heap_pct\":%.1f,"
      "\"heap_min\":%lu,\"psram_free\":%lu,\"psram_size\":%lu,"
      "\"psram_pct\":%.1f,\"loop_hz\":%.1f,\"cpu0_pct\":%.0f,"
      "\"cpu1_pct\":%.0f,\"wifi_rssi\":%d,\"ball_fps\":%.1f,"
      "\"uptime_s\":%lu}",
      static_cast<unsigned long>(snapshot.heap_free),
      static_cast<unsigned long>(snapshot.heap_size), heap_pct,
      static_cast<unsigned long>(snapshot.heap_min),
      static_cast<unsigned long>(snapshot.psram_free),
      static_cast<unsigned long>(snapshot.psram_size), psram_pct,
      snapshot.loop_hz, snapshot.cpu0_pct, snapshot.cpu1_pct,
      snapshot.wifi_rssi, snapshot.ball_fps,
      static_cast<unsigned long>(snapshot.uptime_s));
  if (written < 0 || written >= capacity) {
    return -1;
  }
  return written;
}
