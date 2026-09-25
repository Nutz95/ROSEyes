#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "IClock.h"

/**
 * Arduino millis() backed IClock.
 */
class ArduinoClock : public IClock {
 public:
  /** Default-constructs an Arduino millis clock. */
  ArduinoClock() = default;

  /** Returns Arduino millis(). */
  uint32_t millis() const override;
};
