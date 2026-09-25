#include "TofRangeSensorStub.h"

TofRangeSensorStub::TofRangeSensorStub() : ready_(false) {}

bool TofRangeSensorStub::begin() {
  ready_ = false;
  return false;
}

bool TofRangeSensorStub::readDistanceMillimeters(uint16_t& distance_mm) const {
  distance_mm = 0;
  return false;
}

bool TofRangeSensorStub::isReady() const { return ready_; }
