#pragma once

#include <stdint.h>

/**
 * One TOF range sample for telemetry and host display.
 */
struct RangeObservation {
  uint32_t distance_mm = 0;
  uint16_t status = 0;
  uint16_t signal_strength = 0;
  bool valid = false;
  uint32_t seq = 0;
};
