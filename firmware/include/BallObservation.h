#pragma once

#include <stdint.h>

#include "BallColor.h"

/**
 * One vision snapshot shared core1 → core0 (POD, no heap).
 */
struct BallObservation {
  bool found;
  float x;
  float y;
  float diameter;
  float fps;
  uint32_t timestamp_ms;
  uint32_t seq;
  BallColor color;
  uint8_t framesize_code;
};
