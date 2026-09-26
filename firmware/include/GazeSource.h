#pragma once

#include <stdint.h>

#include "EyeControlMode.h"
#include "GazeState.h"
#include "IFreshGazeProvider.h"
#include "IdleEyeBehavior.h"

/**
 * Muxes ROS / ball gaze providers by control mode, else idle.
 */
class GazeSource {
 public:
  /**
   * @param idle_behavior fallback when neither provider is fresh
   * @param ros_provider optional micro-ROS gaze (used in Piloted)
   * @param ball_provider optional onboard ball gaze (used in Autonomous)
   */
  GazeSource(IdleEyeBehavior& idle_behavior,
             const IFreshGazeProvider* ros_provider,
             const IFreshGazeProvider* ball_provider);

  /** Updates which provider is eligible (from /eyes/mode). */
  void setControlMode(EyeControlMode mode);

  /** Returns the mode last set via setControlMode. */
  EyeControlMode controlMode() const;

  /**
   * Writes the preferred gaze into destination for this frame.
   * @param now_ms monotonic time used for freshness
   * @param destination output gaze state
   */
  void selectGaze(uint32_t now_ms, GazeState& destination);

 private:
  IdleEyeBehavior& idle_behavior_;
  const IFreshGazeProvider* ros_provider_;
  const IFreshGazeProvider* ball_provider_;
  EyeControlMode control_mode_;
  bool was_using_external_;
};
