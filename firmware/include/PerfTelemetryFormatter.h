#pragma once

#include "PerfSnapshot.h"

/**
 * Formats /eyes/perf JSON (heap/PSRAM %, CPU load, loop, ball fps).
 */
class PerfTelemetryFormatter {
 public:
  /**
   * Writes a compact JSON payload into buffer.
   * @return bytes written, or -1 on failure
   */
  static int format(char* buffer, int capacity, const PerfSnapshot& snapshot);
};
