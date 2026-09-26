#include "EyeMountPolicy.h"

EyeMountPolicy::PanelTransform EyeMountPolicy::panelTransform(EyeId eye) {
  if (eye == EyeId::Left) {
    return PanelTransform::ClockwiseGazeFlipped;
  }
  return PanelTransform::ClockwiseThen180;
}

GazeState EyeMountPolicy::gazeForComposition(EyeId eye,
                                             const GazeState& shared_gaze) {
  GazeState eye_gaze;
  // Shared +Y = look down (ROS / camera). Panels are CW-mounted: flip Y here
  // only so blit rotations stay unchanged.
  const float y = -shared_gaze.y();
  if (eye == EyeId::Right) {
    eye_gaze.setNormalized(-shared_gaze.x(), y);
  } else {
    eye_gaze.setNormalized(shared_gaze.x(), y);
  }
  return eye_gaze;
}
