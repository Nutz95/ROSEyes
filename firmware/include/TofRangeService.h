#pragma once

#include <stdint.h>

#include "RangeObservation.h"
#include "TofRangeSensor.h"

#if defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
struct portMUX_TYPE {};
#endif

/**
 * Background TOF sampler on FreeRTOS — I2C never runs on the eye/OTA thread.
 */
class TofRangeService {
 public:
  static constexpr uint32_t kSamplePeriodMs = 100;
  static constexpr uint32_t kStackWords = 3072;
  static constexpr uint8_t kTaskPriority = 1;
  /** Pin away from ball-vision core1. */
  static constexpr uint8_t kTaskCore = 0;

  /** Creates an idle service (no task yet). */
  TofRangeService();

  /**
   * Spawns the sampler task and returns immediately (never blocks on I2C).
   * @return false if the task could not be created
   */
  bool start();

  /** True once the background task has probed the sensor successfully. */
  bool isReady() const;

  /** Copies the latest sample (zeroed if not ready). */
  RangeObservation snapshot() const;

 private:
  static void taskEntry(void* arg);
  void run();
  void store(const RangeObservation& observation);

  TofRangeSensor sensor_;
  RangeObservation latest_;
  mutable portMUX_TYPE lock_;
  volatile bool ready_;
  bool started_;
};
