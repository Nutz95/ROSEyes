#include <unity.h>

#include "GazeState.h"

void setUp() {}
void tearDown() {}

void test_gaze_defaults_to_center() {
  GazeState gaze;
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, gaze.x());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, gaze.y());
}

void test_gaze_clamps_out_of_range() {
  GazeState gaze;
  gaze.setNormalized(2.5f, -4.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, gaze.x());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, gaze.y());
}

void test_gaze_maps_to_pixel_offsets() {
  GazeState gaze;
  gaze.setNormalized(0.5f, -0.5f);
  TEST_ASSERT_EQUAL_INT(9, gaze.horizontalPixelOffset(18));
  TEST_ASSERT_EQUAL_INT(-9, gaze.verticalPixelOffset(18));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_gaze_defaults_to_center);
  RUN_TEST(test_gaze_clamps_out_of_range);
  RUN_TEST(test_gaze_maps_to_pixel_offsets);
  return UNITY_END();
}
