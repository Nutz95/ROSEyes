#include "TofRangeService.h"

#include "FeatureFlags.h"

#if !defined(UNIT_TEST) && defined(ARDUINO) && ROSEYES_ENABLE_TOF

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TofRangeService::TofRangeService()
    : lock_(portMUX_INITIALIZER_UNLOCKED), ready_(false), started_(false) {}

bool TofRangeService::start() {
  if (started_) {
    return true;
  }
  started_ = true;
  const BaseType_t ok = xTaskCreatePinnedToCore(
      &TofRangeService::taskEntry, "tof_range", kStackWords, this,
      kTaskPriority, nullptr, kTaskCore);
  if (ok != pdPASS) {
    started_ = false;
    Serial.println("TOF: task create failed");
    return false;
  }
  Serial.println("TOF: background task started (soft-I2C, isolated from eyes)");
  return true;
}

bool TofRangeService::isReady() const { return ready_; }

RangeObservation TofRangeService::snapshot() const {
  RangeObservation copy;
  portENTER_CRITICAL(&lock_);
  copy = latest_;
  portEXIT_CRITICAL(&lock_);
  return copy;
}

void TofRangeService::store(const RangeObservation& observation) {
  portENTER_CRITICAL(&lock_);
  latest_ = observation;
  portEXIT_CRITICAL(&lock_);
}

void TofRangeService::taskEntry(void* arg) {
  static_cast<TofRangeService*>(arg)->run();
}

void TofRangeService::run() {
  if (!sensor_.begin()) {
    Serial.println("TOF: probe failed in background task");
    vTaskDelete(nullptr);
    return;
  }
  ready_ = true;
  Serial.printf("TOF: ready addr=0x%02X\n", sensor_.address());

  uint32_t seq = 0;
  for (;;) {
    RangeObservation sample;
    if (sensor_.read(sample)) {
      ++seq;
      sample.seq = seq;
      store(sample);
    }
    vTaskDelay(pdMS_TO_TICKS(kSamplePeriodMs));
  }
}

#else

TofRangeService::TofRangeService() : ready_(false), started_(false) {
#if defined(ARDUINO)
  lock_ = portMUX_INITIALIZER_UNLOCKED;
#endif
}

bool TofRangeService::start() {
  started_ = false;
  return false;
}

bool TofRangeService::isReady() const { return false; }

RangeObservation TofRangeService::snapshot() const {
  return RangeObservation{};
}

void TofRangeService::taskEntry(void* /*arg*/) {}

void TofRangeService::run() {}

void TofRangeService::store(const RangeObservation& /*observation*/) {}

#endif
