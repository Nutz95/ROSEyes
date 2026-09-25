#pragma once

#include "GazeState.h"
#include "IClock.h"

/**
 * Smooth synchronized idle gaze (shared by both eyes) when ROS is silent.
 */
class IdleEyeBehavior {
 public:
  /**
   * @param clock monotonic clock
   * @param horizontal_period_ms full left-right cycle duration
   * @param maximum_normalized_offset peak |x| while idling (within 0..1)
   */
  IdleEyeBehavior(const IClock& clock, uint32_t horizontal_period_ms,
                  float maximum_normalized_offset);

  /** Resets phase so motion restarts smoothly from center. */
  void reset();

  /** Writes a smooth shared gaze into gaze_state. */
  void update(GazeState& gaze_state);

 private:
  const IClock& clock_;
  uint32_t horizontal_period_ms_;
  float maximum_normalized_offset_;
  uint32_t phase_origin_ms_;
};
