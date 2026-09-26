#include "IdleEyeBehavior.h"

#include <math.h>

IdleEyeBehavior::IdleEyeBehavior(const IClock& clock,
                                 float maximum_normalized_offset,
                                 uint32_t saccade_min_ms,
                                 uint32_t saccade_max_ms,
                                 uint32_t fixation_min_ms,
                                 uint32_t fixation_max_ms)
    : clock_(clock),
      maximum_normalized_offset_(clampf(maximum_normalized_offset, 0.0f, 1.0f)),
      saccade_min_ms_(saccade_min_ms < 80u ? 80u : saccade_min_ms),
      saccade_max_ms_(saccade_max_ms < saccade_min_ms_ ? saccade_min_ms_
                                                       : saccade_max_ms),
      fixation_min_ms_(fixation_min_ms < 100u ? 100u : fixation_min_ms),
      fixation_max_ms_(fixation_max_ms < fixation_min_ms_ ? fixation_min_ms_
                                                         : fixation_max_ms),
      synced_to_gaze_(false),
      phase_(Phase::Fixating),
      phase_started_ms_(0),
      phase_duration_ms_(0),
      start_x_(0.0f),
      start_y_(0.0f),
      target_x_(0.0f),
      target_y_(0.0f),
      current_x_(0.0f),
      current_y_(0.0f),
      rng_state_(0xA5A5u) {
  reset();
}

void IdleEyeBehavior::reset() {
  synced_to_gaze_ = false;
  phase_ = Phase::Fixating;
  phase_started_ms_ = 0;
  phase_duration_ms_ = 0;
  rng_state_ ^= static_cast<uint32_t>(clock_.millis()) + 0x9E3779B9u;
  if (rng_state_ == 0) {
    rng_state_ = 0xA5A5u;
  }
}

void IdleEyeBehavior::update(GazeState& gaze_state) {
  const uint32_t now_ms = clock_.millis();

  if (!synced_to_gaze_) {
    current_x_ = clampf(gaze_state.x(), -maximum_normalized_offset_,
                        maximum_normalized_offset_);
    current_y_ = clampf(gaze_state.y(), -maximum_normalized_offset_,
                        maximum_normalized_offset_);
    target_x_ = current_x_;
    target_y_ = current_y_;
    synced_to_gaze_ = true;
    beginFixation(now_ms);
  }

  const uint32_t elapsed_ms = now_ms - phase_started_ms_;

  if (phase_ == Phase::Fixating) {
    if (elapsed_ms >= phase_duration_ms_) {
      beginSaccade(now_ms);
    }
  } else {
    const float duration =
        phase_duration_ms_ == 0
            ? 1.0f
            : static_cast<float>(phase_duration_ms_);
    float normalized = static_cast<float>(elapsed_ms) / duration;
    if (normalized >= 1.0f) {
      current_x_ = target_x_;
      current_y_ = target_y_;
      beginFixation(now_ms);
    } else {
      const float eased = easeInOutCubic(normalized);
      current_x_ = start_x_ + (target_x_ - start_x_) * eased;
      current_y_ = start_y_ + (target_y_ - start_y_) * eased;
    }
  }

  gaze_state.setNormalized(current_x_, current_y_);
}

void IdleEyeBehavior::beginFixation(uint32_t now_ms) {
  phase_ = Phase::Fixating;
  phase_started_ms_ = now_ms;
  phase_duration_ms_ = randomInRange(fixation_min_ms_, fixation_max_ms_);
}

void IdleEyeBehavior::beginSaccade(uint32_t now_ms) {
  start_x_ = current_x_;
  start_y_ = current_y_;
  pickNewTarget();

  const float delta_x = target_x_ - start_x_;
  const float delta_y = target_y_ - start_y_;
  const float distance = sqrtf(delta_x * delta_x + delta_y * delta_y);
  const float max_span = maximum_normalized_offset_ * 2.0f;
  const float distance_ratio =
      max_span <= 0.0001f ? 0.0f : clampf(distance / max_span, 0.0f, 1.0f);

  // Longer travels take a bit more time; still stays in the configured window.
  const float duration_span =
      static_cast<float>(saccade_max_ms_ - saccade_min_ms_);
  phase_duration_ms_ =
      saccade_min_ms_ +
      static_cast<uint32_t>(duration_span * (0.35f + 0.65f * distance_ratio));
  phase_ = Phase::Saccading;
  phase_started_ms_ = now_ms;
}

void IdleEyeBehavior::pickNewTarget() {
  const float max_offset = maximum_normalized_offset_;
  const float min_jump = max_offset * 0.25f;

  for (uint8_t attempt = 0; attempt < 12u; ++attempt) {
    const float candidate_x = randomFloat(-max_offset, max_offset);
    const float candidate_y = randomFloat(-max_offset, max_offset);
    const float delta_x = candidate_x - current_x_;
    const float delta_y = candidate_y - current_y_;
    const float distance = sqrtf(delta_x * delta_x + delta_y * delta_y);
    if (distance >= min_jump) {
      target_x_ = candidate_x;
      target_y_ = candidate_y;
      return;
    }
  }

  // Fallback: flip toward the opposite quadrant.
  target_x_ = clampf(-current_x_ * 0.8f + randomFloat(-0.1f, 0.1f), -max_offset,
                     max_offset);
  target_y_ = clampf(-current_y_ * 0.8f + randomFloat(-0.1f, 0.1f), -max_offset,
                     max_offset);
}

uint32_t IdleEyeBehavior::nextRandom() {
  rng_state_ = rng_state_ * 1664525u + 1013904223u;
  return rng_state_;
}

uint32_t IdleEyeBehavior::randomInRange(uint32_t minimum_inclusive,
                                        uint32_t maximum_inclusive) {
  if (maximum_inclusive <= minimum_inclusive) {
    return minimum_inclusive;
  }
  const uint32_t span = maximum_inclusive - minimum_inclusive + 1u;
  return minimum_inclusive + (nextRandom() % span);
}

float IdleEyeBehavior::randomFloat(float minimum_inclusive,
                                   float maximum_inclusive) {
  const float unit = static_cast<float>(nextRandom() & 0x00FFFFFFu) /
                     static_cast<float>(0x01000000u);
  return minimum_inclusive +
         (maximum_inclusive - minimum_inclusive) * unit;
}

float IdleEyeBehavior::easeInOutCubic(float normalized_time) {
  const float time_value = clampf(normalized_time, 0.0f, 1.0f);
  if (time_value < 0.5f) {
    return 4.0f * time_value * time_value * time_value;
  }
  const float inverted = -2.0f * time_value + 2.0f;
  return 1.0f - (inverted * inverted * inverted) * 0.5f;
}

float IdleEyeBehavior::clampf(float value, float minimum_value,
                              float maximum_value) {
  if (value < minimum_value) {
    return minimum_value;
  }
  if (value > maximum_value) {
    return maximum_value;
  }
  return value;
}
