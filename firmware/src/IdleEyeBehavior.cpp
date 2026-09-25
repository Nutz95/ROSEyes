#include "IdleEyeBehavior.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

IdleEyeBehavior::IdleEyeBehavior(const IClock& clock,
                                 uint32_t horizontal_period_ms,
                                 float maximum_normalized_offset)
    : clock_(clock),
      horizontal_period_ms_(horizontal_period_ms < 1000u ? 1000u
                                                         : horizontal_period_ms),
      maximum_normalized_offset_(maximum_normalized_offset < 0.0f
                                     ? 0.0f
                                     : (maximum_normalized_offset > 1.0f
                                            ? 1.0f
                                            : maximum_normalized_offset)),
      phase_origin_ms_(0) {
  reset();
}

void IdleEyeBehavior::reset() { phase_origin_ms_ = clock_.millis(); }

void IdleEyeBehavior::update(GazeState& gaze_state) {
  const uint32_t elapsed = clock_.millis() - phase_origin_ms_;
  const float phase = (2.0f * static_cast<float>(M_PI) *
                       static_cast<float>(elapsed)) /
                      static_cast<float>(horizontal_period_ms_);

  // Smooth shared horizontal sweep (same gaze for left and right).
  const float offset_x = maximum_normalized_offset_ * sinf(phase);
  gaze_state.setNormalized(offset_x, 0.0f);
}
