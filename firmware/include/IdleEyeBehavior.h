#pragma once

#include <stdint.h>

#include "GazeState.h"
#include "IClock.h"

/**
 * Natural idle gaze: random 2D saccades with ease-in-out motion and fixations.
 */
class IdleEyeBehavior {
 public:
  /**
   * @param clock monotonic clock
   * @param maximum_normalized_offset peak |x|/|y| while idling (within 0..1)
   * @param saccade_min_ms shortest move duration
   * @param saccade_max_ms longest move duration
   * @param fixation_min_ms shortest pause after arriving
   * @param fixation_max_ms longest pause after arriving
   */
  IdleEyeBehavior(const IClock& clock, float maximum_normalized_offset,
                  uint32_t saccade_min_ms, uint32_t saccade_max_ms,
                  uint32_t fixation_min_ms, uint32_t fixation_max_ms);

  /** Clears phase so the next update resumes from the live gaze pose. */
  void reset();

  /** Advances idle motion and writes the shared gaze into gaze_state. */
  void update(GazeState& gaze_state);

 private:
  enum class Phase : uint8_t { Fixating = 0, Saccading = 1 };

  void beginFixation(uint32_t now_ms);
  void beginSaccade(uint32_t now_ms);
  void pickNewTarget();
  uint32_t nextRandom();
  uint32_t randomInRange(uint32_t minimum_inclusive, uint32_t maximum_inclusive);
  float randomFloat(float minimum_inclusive, float maximum_inclusive);
  static float easeInOutCubic(float normalized_time);
  static float clampf(float value, float minimum_value, float maximum_value);

  const IClock& clock_;
  float maximum_normalized_offset_;
  uint32_t saccade_min_ms_;
  uint32_t saccade_max_ms_;
  uint32_t fixation_min_ms_;
  uint32_t fixation_max_ms_;

  bool synced_to_gaze_;
  Phase phase_;
  uint32_t phase_started_ms_;
  uint32_t phase_duration_ms_;
  float start_x_;
  float start_y_;
  float target_x_;
  float target_y_;
  float current_x_;
  float current_y_;
  uint32_t rng_state_;
};
