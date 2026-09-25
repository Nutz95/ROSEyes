#pragma once

#include <stdint.h>

#include "IClock.h"

/**
 * Mutable clock for deterministic unit tests.
 */
class FakeClock : public IClock {
 public:
  /** Starts at time zero. */
  FakeClock();

  /** Returns the configured fake time. */
  uint32_t millis() const override;

  /** Sets absolute fake time in milliseconds. */
  void setMillis(uint32_t value);

  /** Advances fake time by delta_ms. */
  void advance(uint32_t delta_ms);

 private:
  uint32_t current_ms_;
};
