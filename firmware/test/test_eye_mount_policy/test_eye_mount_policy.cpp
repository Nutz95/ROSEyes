#include <unity.h>

#include "EyeId.h"
#include "EyeMountPolicy.h"
#include "GazeState.h"

void setUp() {}
void tearDown() {}

void test_left_gaze_flips_y_keeps_x() {
  GazeState shared;
  shared.setNormalized(0.4f, -0.2f);
  const GazeState left =
      EyeMountPolicy::gazeForComposition(EyeId::Left, shared);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.4f, left.x());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.2f, left.y());
}

void test_right_gaze_negates_x_and_y() {
  GazeState shared;
  shared.setNormalized(0.4f, -0.2f);
  const GazeState right =
      EyeMountPolicy::gazeForComposition(EyeId::Right, shared);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.4f, right.x());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.2f, right.y());
}

void test_panel_transforms_differ_per_eye() {
  TEST_ASSERT_EQUAL(
      static_cast<int>(EyeMountPolicy::PanelTransform::ClockwiseGazeFlipped),
      static_cast<int>(EyeMountPolicy::panelTransform(EyeId::Left)));
  TEST_ASSERT_EQUAL(
      static_cast<int>(EyeMountPolicy::PanelTransform::ClockwiseThen180),
      static_cast<int>(EyeMountPolicy::panelTransform(EyeId::Right)));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_left_gaze_flips_y_keeps_x);
  RUN_TEST(test_right_gaze_negates_x_and_y);
  RUN_TEST(test_panel_transforms_differ_per_eye);
  return UNITY_END();
}
