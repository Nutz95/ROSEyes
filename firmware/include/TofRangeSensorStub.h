#pragma once

#include <stdint.h>

/**
 * Placeholder for the Waveshare TOF Mini (I2C) — deferred to a later iteration.
 * Pins are reserved on BoardPins (SDA/SCL).
 */
class TofRangeSensorStub {
 public:
  /** Creates an unready stub instance. */
  TofRangeSensorStub();

  /** Reserved hook for future I2C bring-up. Currently a no-op that returns false. */
  bool begin();

  /** Reserved distance read. Returns false until the driver is implemented. */
  bool readDistanceMillimeters(uint16_t& distance_mm) const;

  /** Returns true once a real driver has successfully initialized. */
  bool isReady() const;

 private:
  bool ready_;
};
