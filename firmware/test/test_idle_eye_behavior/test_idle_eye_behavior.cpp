#include <unity.h>

#include "FakeClock.h"
#include "GazeState.h"
#include "IdleEyeBehavior.h"

void setUp() {}
void tearDown() {}

void test_idle_behavior_updates_gaze() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 4000, 0.45f);
  GazeState gaze;

  behavior.update(gaze);
  const float first_x = gaze.x();

  clock.advance(1000);
  behavior.update(gaze);
  TEST_ASSERT_TRUE(gaze.x() != first_x);
  TEST_ASSERT_TRUE(gaze.x() <= 0.45f);
  TEST_ASSERT_TRUE(gaze.x() >= -0.45f);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, gaze.y());
}

void test_idle_behavior_stays_in_sync_bounds() {
  FakeClock clock;
  IdleEyeBehavior behavior(clock, 5000, 0.35f);
  GazeState gaze;

  for (int step = 0; step < 20; ++step) {
    behavior.update(gaze);
    TEST_ASSERT_TRUE(gaze.x() <= 0.35f);
    TEST_ASSERT_TRUE(gaze.x() >= -0.35f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, gaze.y());
    clock.advance(250);
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_behavior_updates_gaze);
  RUN_TEST(test_idle_behavior_stays_in_sync_bounds);
  return UNITY_END();
}
