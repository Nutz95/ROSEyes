#include "BlinkScheduler.h"

BlinkScheduler::BlinkScheduler(const IClock& clock, uint32_t minimum_interval_ms,
                               uint32_t maximum_interval_ms,
                               uint32_t blink_duration_ms)
    : clock_(clock),
      minimum_interval_ms_(minimum_interval_ms),
      maximum_interval_ms_(maximum_interval_ms < minimum_interval_ms
                               ? minimum_interval_ms
                               : maximum_interval_ms),
      blink_duration_ms_(blink_duration_ms == 0 ? 1u : blink_duration_ms),
      next_blink_at_ms_(0),
      blink_started_at_ms_(0),
      blinking_(false),
      pseudo_random_state_(0xA5A5u) {
  reset();
}

void BlinkScheduler::reset() {
  blinking_ = false;
  blink_started_at_ms_ = 0;
  next_blink_at_ms_ = clock_.millis() + nextIntervalMs();
}

void BlinkScheduler::requestImmediateBlink() {
  next_blink_at_ms_ = clock_.millis();
}

bool BlinkScheduler::update() {
  const uint32_t now_ms = clock_.millis();

  if (blinking_) {
    if (now_ms >= blink_started_at_ms_ + blink_duration_ms_) {
      blinking_ = false;
      next_blink_at_ms_ = now_ms + nextIntervalMs();
    }
    return blinking_;
  }

  if (now_ms >= next_blink_at_ms_) {
    blinking_ = true;
    blink_started_at_ms_ = now_ms;
  }
  return blinking_;
}

bool BlinkScheduler::isBlinking() const { return blinking_; }

float BlinkScheduler::lidClosureAmount() const {
  if (!blinking_) {
    return 0.0f;
  }
  const uint32_t now_ms = clock_.millis();
  if (now_ms <= blink_started_at_ms_) {
    return 0.0f;
  }
  const uint32_t elapsed = now_ms - blink_started_at_ms_;
  if (elapsed >= blink_duration_ms_) {
    return 0.0f;
  }

  const float progress =
      static_cast<float>(elapsed) / static_cast<float>(blink_duration_ms_);
  // Close quickly, short hold, open (most of the duration).
  if (progress < kClosePhase) {
    return progress / kClosePhase;
  }
  if (progress < kHoldEnd) {
    return 1.0f;
  }
  return 1.0f - ((progress - kHoldEnd) / kOpenPhase);
}

uint32_t BlinkScheduler::nextIntervalMs() {
  const uint32_t span = maximum_interval_ms_ - minimum_interval_ms_;
  if (span == 0) {
    return minimum_interval_ms_;
  }
  return minimum_interval_ms_ + (nextPseudoRandom() % (span + 1u));
}

uint32_t BlinkScheduler::nextPseudoRandom() {
  uint32_t value = pseudo_random_state_;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  if (value == 0) {
    value = 0xA5A5u;
  }
  pseudo_random_state_ = value;
  return value;
}
