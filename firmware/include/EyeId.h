#pragma once

#include <stdint.h>

/**
 * Identifies which round display is addressed on the shared SPI bus.
 */
enum class EyeId : uint8_t {
  Left = 0,
  Right = 1,
};
