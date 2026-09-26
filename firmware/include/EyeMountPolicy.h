#pragma once

#include "EyeId.h"
#include "GazeState.h"

/**
 * Per-eye panel mount: blit remap and gaze flip shared by renderer + display.
 */
class EyeMountPolicy {
 public:
  /** Software remap applied after composing an upright RGB565 frame. */
  enum class PanelTransform {
    /** 90° CW with L/R reversed (Eye1 / left mount). */
    ClockwiseGazeFlipped,
    /** 90° CW then image 180° (Eye2 / right mount). */
    ClockwiseThen180,
  };

  /**
   * Returns the blit transform for a mounted panel.
   * @param eye left or right panel
   */
  static PanelTransform panelTransform(EyeId eye);

  /**
   * Gaze used while composing one eye so both mounts look the same way.
   * Right eye negates horizontal gaze; both eyes negate vertical gaze so
   * shared +Y (look down, camera/ROS) matches the rotated GC9D01 mounts.
   * @param eye panel being composed
   * @param shared_gaze user/ROS/idle/ball gaze in shared coordinates
   */
  static GazeState gazeForComposition(EyeId eye, const GazeState& shared_gaze);
};
