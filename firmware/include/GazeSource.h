#pragma once

#include <stdint.h>

#include "GazeState.h"
#include "IFreshGazeProvider.h"
#include "IdleEyeBehavior.h"

/**
 * Selects the active gaze: fresh external provider when present, else idle.
 */
class GazeSource {
 public:
  /**
   * @param idle_behavior fallback gaze when no external sample is fresh
   * @param external_provider optional ROS (or other) gaze; nullptr = idle only
   */
  GazeSource(IdleEyeBehavior& idle_behavior,
             const IFreshGazeProvider* external_provider);

  /**
   * Writes the preferred gaze into destination for this frame.
   * @param now_ms monotonic time used for freshness
   * @param destination output gaze state
   */
  void selectGaze(uint32_t now_ms, GazeState& destination);

 private:
  IdleEyeBehavior& idle_behavior_;
  const IFreshGazeProvider* external_provider_;
};
