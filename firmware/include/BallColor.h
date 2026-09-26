#pragma once

#include <stdint.h>

/**
 * Detected ball color token for telemetry and sticky tracking.
 */
enum class BallColor : uint8_t {
  None = 0,
  Red = 1,
  Green = 2,
};
