#pragma once

#include "RangeObservation.h"

/**
 * Formats /eyes/range JSON (mm, status, strength, seq).
 */
class RangeTelemetryFormatter {
 public:
  /**
   * Writes a compact JSON payload into buffer.
   * @return bytes written, or -1 on failure
   */
  static int format(const RangeObservation& observation, char* buffer,
                    int capacity);
};
