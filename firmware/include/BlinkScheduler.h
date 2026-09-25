#pragma once

#include <stdint.h>

#include "IClock.h"

/**
 * Schedules semi-random blinks and exposes a smooth lid-closure amount.
 */
class BlinkScheduler {
 public:
  /** Fraction of blink duration spent closing (progress 0 → kClosePhase). */
  static constexpr float kClosePhase = 0.35f;
  /** Progress at end of closed hold (close + hold = kHoldEnd). */
  static constexpr float kHoldEnd = 0.45f;
  /** Fraction of blink duration spent opening (= 1 - kHoldEnd). */
  static constexpr float kOpenPhase = 1.0f - kHoldEnd;

  /**
   * Constructs a scheduler using the provided clock.
   * @param clock monotonic millisecond clock
   * @param minimum_interval_ms lower bound between blinks
   * @param maximum_interval_ms upper bound between blinks
   * @param blink_duration_ms total close+open animation duration
   */
  BlinkScheduler(const IClock& clock, uint32_t minimum_interval_ms,
                 uint32_t maximum_interval_ms, uint32_t blink_duration_ms);

  /** Arms the next blink relative to now. */
  void reset();

  /** Requests an immediate blink on the next update. */
  void requestImmediateBlink();

  /** Advances timing; returns true while a blink animation is active. */
  bool update();

  /** Returns true while a blink animation is active. */
  bool isBlinking() const;

  /**
   * Lid closure amount for rendering.
   * @return 0 = fully open, 1 = fully closed (top-to-bottom motion)
   */
  float lidClosureAmount() const;

 private:
  uint32_t nextIntervalMs();
  uint32_t nextPseudoRandom();

  const IClock& clock_;
  uint32_t minimum_interval_ms_;
  uint32_t maximum_interval_ms_;
  uint32_t blink_duration_ms_;
  uint32_t next_blink_at_ms_;
  uint32_t blink_started_at_ms_;
  bool blinking_;
  uint32_t pseudo_random_state_;
};
