#pragma once

#include <stdint.h>

/**
 * Lifecycle of the micro-ROS eye session.
 */
enum class MicroRosSessionState : uint8_t {
  IdleOnly = 0,
  Connecting = 1,
  Ready = 2,
  Faulted = 3,
};
