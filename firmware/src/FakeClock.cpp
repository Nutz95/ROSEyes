#include "FakeClock.h"

FakeClock::FakeClock() : current_ms_(0) {}

uint32_t FakeClock::millis() const { return current_ms_; }

void FakeClock::setMillis(uint32_t value) { current_ms_ = value; }

void FakeClock::advance(uint32_t delta_ms) { current_ms_ += delta_ms; }
