#include "TofRangeSensor.h"

#if !defined(UNIT_TEST) && defined(ARDUINO)

#include <Arduino.h>

#include "BoardPins.h"

TofRangeSensor::TofRangeSensor() : ready_(false), address_(0) {}

bool TofRangeSensor::begin() {
  ready_ = false;
  address_ = 0;

  bus_.begin(BoardPins::kTofSdaPin, BoardPins::kTofSclPin);

  for (uint8_t i = 0; i < kProbeCount; ++i) {
    const uint8_t addr = static_cast<uint8_t>(kDefaultI2cAddress + i);
    if (!bus_.probe(addr)) {
      continue;
    }
    address_ = addr;
    Serial.printf("TOF: ACK at 0x%02X (soft-I2C SDA=%d SCL=%d)\n", address_,
                  BoardPins::kTofSdaPin, BoardPins::kTofSclPin);

    uint8_t mode_bytes[2] = {0xFF, 0xFF};
    if (bus_.writeRead(address_, kModeRegister, mode_bytes, 2)) {
      const uint8_t interface_mode = mode_bytes[0] & 0x07;
      const uint8_t module_id = mode_bytes[1];
      Serial.printf("TOF: mode=%u id=%u\n", interface_mode, module_id);
    }
    ready_ = true;
    return true;
  }

  Serial.printf("TOF: no device at 0x08..0x0F (soft-I2C SDA=%d SCL=%d)\n",
                BoardPins::kTofSdaPin, BoardPins::kTofSclPin);
  return false;
}

bool TofRangeSensor::isReady() const { return ready_; }

uint8_t TofRangeSensor::address() const { return address_; }

bool TofRangeSensor::read(RangeObservation& observation) {
  observation = RangeObservation{};
  if (!ready_) {
    return false;
  }

  uint8_t raw[kBlockBytes];
  if (!bus_.writeRead(address_, kBlockRegister, raw, kBlockBytes)) {
    return false;
  }

  observation.distance_mm = static_cast<uint32_t>(raw[4]) |
                            (static_cast<uint32_t>(raw[5]) << 8) |
                            (static_cast<uint32_t>(raw[6]) << 16) |
                            (static_cast<uint32_t>(raw[7]) << 24);
  observation.status = static_cast<uint16_t>(raw[8]) |
                       (static_cast<uint16_t>(raw[9]) << 8);
  observation.signal_strength = static_cast<uint16_t>(raw[10]) |
                                (static_cast<uint16_t>(raw[11]) << 8);
  // Mini often reports dis_status=1 while distance is good; trust span, not ==0.
  observation.valid = observation.distance_mm >= kMinValidDistanceMm &&
                      observation.distance_mm <= kMaxValidDistanceMm;
  return true;
}

#else

TofRangeSensor::TofRangeSensor() : ready_(false), address_(0) {}

bool TofRangeSensor::begin() {
  ready_ = false;
  address_ = 0;
  return false;
}

bool TofRangeSensor::isReady() const { return ready_; }

uint8_t TofRangeSensor::address() const { return address_; }

bool TofRangeSensor::read(RangeObservation& observation) {
  observation = RangeObservation{};
  return false;
}

#endif
