#pragma once

#include <stdint.h>

/**
 * Millisecond clock abstraction so schedulers stay unit-testable.
 */
class IClock {
 public:
  virtual ~IClock() = default;

  /** Returns monotonic milliseconds since an arbitrary epoch. */
  virtual uint32_t millis() const = 0;
};
