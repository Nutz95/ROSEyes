#pragma once

#include "BallObservation.h"

/**
 * Formats BallObservation as compact JSON for /eyes/ball.
 */
class BallTelemetryFormatter {
 public:
  /**
   * Writes JSON into buffer (NUL-terminated).
   * @return bytes written excluding NUL, or -1 on truncation
   */
  static int format(const BallObservation& observation, char* buffer,
                    int capacity);

 private:
  static const char* colorName(BallColor color);
  static const char* framesizeName(uint8_t code);
};
