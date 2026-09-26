#include <unity.h>

#include "FakeClock.h"
#include "GazeState.h"
#include "IdleEyeBehavior.h"

void setUp() {}
void tearDown() {}

void test_idle_behavior_moves_over_time() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 0.45f, 200, 400, 100, 200);
  GazeState gaze;
  gaze.setNormalized(0.0f, 0.0f);

  behavior.update(gaze);
  const float first_x = gaze.x();
  const float first_y = gaze.y();

  bool changed = false;
  for (int step = 0; step < 40; ++step) {
    clock.advance(50);
    behavior.update(gaze);
    if (gaze.x() != first_x || gaze.y() != first_y) {
      changed = true;
      break;
    }
  }
  TEST_ASSERT_TRUE(changed);
}

void test_idle_behavior_stays_in_bounds() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 0.35f, 150, 300, 80, 160);
  GazeState gaze;
  gaze.setNormalized(0.0f, 0.0f);

  for (int step = 0; step < 80; ++step) {
    behavior.update(gaze);
    TEST_ASSERT_TRUE(gaze.x() <= 0.35f + 0.0001f);
    TEST_ASSERT_TRUE(gaze.x() >= -0.35f - 0.0001f);
    TEST_ASSERT_TRUE(gaze.y() <= 0.35f + 0.0001f);
    TEST_ASSERT_TRUE(gaze.y() >= -0.35f - 0.0001f);
    clock.advance(40);
  }
}

void test_idle_behavior_can_look_vertical() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 0.45f, 100, 200, 50, 80);
  GazeState gaze;
  gaze.setNormalized(0.0f, 0.0f);

  bool saw_vertical = false;
  for (int step = 0; step < 200; ++step) {
    behavior.update(gaze);
    if (fabsf(gaze.y()) > 0.05f) {
      saw_vertical = true;
      break;
    }
    clock.advance(30);
  }
  TEST_ASSERT_TRUE(saw_vertical);
}

void test_idle_saccade_is_smooth() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 0.45f, 300, 300, 1000, 1000);
  GazeState gaze;
  gaze.setNormalized(0.0f, 0.0f);

  // Leave fixation quickly by using short fixation in a separate instance is
  // hard; advance far enough to enter a saccade then sample consecutive frames.
  behavior.update(gaze);
  clock.advance(1100);
  behavior.update(gaze);

  float previous_x = gaze.x();
  float previous_y = gaze.y();
  for (int step = 0; step < 12; ++step) {
    clock.advance(20);
    behavior.update(gaze);
    const float jump =
        fabsf(gaze.x() - previous_x) + fabsf(gaze.y() - previous_y);
    TEST_ASSERT_TRUE(jump < 0.25f);
    previous_x = gaze.x();
    previous_y = gaze.y();
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_behavior_moves_over_time);
  RUN_TEST(test_idle_behavior_stays_in_bounds);
  RUN_TEST(test_idle_behavior_can_look_vertical);
  RUN_TEST(test_idle_saccade_is_smooth);
  return UNITY_END();
}
