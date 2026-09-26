#pragma once

#include <stdint.h>

/**
 * Host-selected eye control mode (see /eyes/mode).
 */
enum class EyeControlMode : uint8_t {
  Autonomous = 0,
  Piloted = 1,
};
