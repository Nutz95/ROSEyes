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
  if (eye == EyeId::Right) {
    eye_gaze.setNormalized(-shared_gaze.x(), shared_gaze.y());
  } else {
    eye_gaze.setNormalized(shared_gaze.x(), shared_gaze.y());
  }
  return eye_gaze;
}
