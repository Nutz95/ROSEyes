#include "SoftI2cBus.h"

#if !defined(UNIT_TEST) && defined(ARDUINO)

#include <Arduino.h>

SoftI2cBus::SoftI2cBus() : sda_pin_(-1), scl_pin_(-1), begun_(false) {}

void SoftI2cBus::begin(int sda_pin, int scl_pin) {
  sda_pin_ = sda_pin;
  scl_pin_ = scl_pin;
  pinMode(sda_pin_, INPUT_PULLUP);
  pinMode(scl_pin_, INPUT_PULLUP);
  begun_ = true;
}

void SoftI2cBus::delayHalf() const { delayMicroseconds(kHalfBitUs); }

void SoftI2cBus::sdaRelease() {
  pinMode(sda_pin_, INPUT_PULLUP);
}

void SoftI2cBus::sdaDriveLow() {
  pinMode(sda_pin_, OUTPUT);
  digitalWrite(sda_pin_, LOW);
}

void SoftI2cBus::sclRelease() {
  pinMode(scl_pin_, INPUT_PULLUP);
}

void SoftI2cBus::sclDriveLow() {
  pinMode(scl_pin_, OUTPUT);
  digitalWrite(scl_pin_, LOW);
}

bool SoftI2cBus::readSda() const {
  return digitalRead(sda_pin_) != LOW;
}

bool SoftI2cBus::sclWaitHigh() {
  sclRelease();
  for (uint16_t i = 0; i < 200; ++i) {
    if (digitalRead(scl_pin_) != LOW) {
      delayHalf();
      return true;
    }
    delayMicroseconds(5);
  }
  return false;
}

void SoftI2cBus::startCond() {
  sdaRelease();
  sclRelease();
  delayHalf();
  sdaDriveLow();
  delayHalf();
  sclDriveLow();
}

void SoftI2cBus::stopCond() {
  sdaDriveLow();
  delayHalf();
  if (!sclWaitHigh()) {
    return;
  }
  sdaRelease();
  delayHalf();
}

bool SoftI2cBus::writeByte(uint8_t value) {
  for (int bit = 7; bit >= 0; --bit) {
    if ((value >> bit) & 0x01) {
      sdaRelease();
    } else {
      sdaDriveLow();
    }
    delayHalf();
    if (!sclWaitHigh()) {
      return false;
    }
    sclDriveLow();
  }
  sdaRelease();
  delayHalf();
  if (!sclWaitHigh()) {
    return false;
  }
  const bool ack = !readSda();
  sclDriveLow();
  return ack;
}

bool SoftI2cBus::readByte(uint8_t& value, bool ack) {
  value = 0;
  sdaRelease();
  for (int bit = 7; bit >= 0; --bit) {
    delayHalf();
    if (!sclWaitHigh()) {
      return false;
    }
    if (readSda()) {
      value |= static_cast<uint8_t>(1u << bit);
    }
    sclDriveLow();
  }
  if (ack) {
    sdaDriveLow();
  } else {
    sdaRelease();
  }
  delayHalf();
  if (!sclWaitHigh()) {
    return false;
  }
  sclDriveLow();
  sdaRelease();
  return true;
}

bool SoftI2cBus::probe(uint8_t address7) {
  if (!begun_) {
    return false;
  }
  startCond();
  const bool ack = writeByte(static_cast<uint8_t>(address7 << 1));
  stopCond();
  return ack;
}

bool SoftI2cBus::writeRead(uint8_t address7, uint8_t reg, uint8_t* buffer,
                           uint8_t length) {
  if (!begun_ || buffer == nullptr || length == 0) {
    return false;
  }
  startCond();
  if (!writeByte(static_cast<uint8_t>(address7 << 1)) || !writeByte(reg)) {
    stopCond();
    return false;
  }
  startCond();
  if (!writeByte(static_cast<uint8_t>((address7 << 1) | 0x01))) {
    stopCond();
    return false;
  }
  for (uint8_t i = 0; i < length; ++i) {
    if (!readByte(buffer[i], i + 1 < length)) {
      stopCond();
      return false;
    }
  }
  stopCond();
  return true;
}

#else

SoftI2cBus::SoftI2cBus() : sda_pin_(-1), scl_pin_(-1), begun_(false) {}

void SoftI2cBus::begin(int /*sda_pin*/, int /*scl_pin*/) { begun_ = false; }

bool SoftI2cBus::writeRead(uint8_t /*address7*/, uint8_t /*reg*/,
                           uint8_t* /*buffer*/, uint8_t /*length*/) {
  return false;
}

bool SoftI2cBus::probe(uint8_t /*address7*/) { return false; }

void SoftI2cBus::delayHalf() const {}
void SoftI2cBus::sdaRelease() {}
void SoftI2cBus::sdaDriveLow() {}
void SoftI2cBus::sclRelease() {}
void SoftI2cBus::sclDriveLow() {}
bool SoftI2cBus::sclWaitHigh() { return false; }
bool SoftI2cBus::readSda() const { return true; }
void SoftI2cBus::startCond() {}
void SoftI2cBus::stopCond() {}
bool SoftI2cBus::writeByte(uint8_t /*value*/) { return false; }
bool SoftI2cBus::readByte(uint8_t& value, bool /*ack*/) {
  value = 0;
  return false;
}

#endif
