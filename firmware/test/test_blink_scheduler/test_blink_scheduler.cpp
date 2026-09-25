#include <unity.h>

#include "BlinkScheduler.h"
#include "FakeClock.h"

void setUp() {}
void tearDown() {}

void test_blink_triggers_after_interval() {
  FakeClock clock;
  BlinkScheduler scheduler(clock, 2000, 2000, 100);
  TEST_ASSERT_FALSE(scheduler.update());

  clock.advance(1999);
  TEST_ASSERT_FALSE(scheduler.update());

  clock.advance(1);
  TEST_ASSERT_TRUE(scheduler.update());
  TEST_ASSERT_TRUE(scheduler.isBlinking());

  clock.advance(100);
  TEST_ASSERT_FALSE(scheduler.update());
  TEST_ASSERT_FALSE(scheduler.isBlinking());
}

void test_immediate_blink_request() {
  FakeClock clock;
  BlinkScheduler scheduler(clock, 5000, 5000, 50);
  scheduler.requestImmediateBlink();
  TEST_ASSERT_TRUE(scheduler.update());
}

void test_lid_closure_peaks_mid_blink() {
  FakeClock clock;
  constexpr uint32_t kDurationMs = 200;
  BlinkScheduler scheduler(clock, 2000, 2000, kDurationMs);
  scheduler.requestImmediateBlink();
  TEST_ASSERT_TRUE(scheduler.update());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, scheduler.lidClosureAmount());

  // Mid close phase → nearly closed.
  const uint32_t mid_close_ms = static_cast<uint32_t>(
      BlinkScheduler::kClosePhase * 0.5f * static_cast<float>(kDurationMs));
  clock.advance(mid_close_ms > 0 ? mid_close_ms : 1);
  TEST_ASSERT_TRUE(scheduler.update());
  TEST_ASSERT_TRUE(scheduler.lidClosureAmount() > 0.4f);

  // Inside hold window → fully closed.
  const uint32_t hold_sample_ms = static_cast<uint32_t>(
      ((BlinkScheduler::kClosePhase + BlinkScheduler::kHoldEnd) * 0.5f) *
      static_cast<float>(kDurationMs));
  clock.advance(hold_sample_ms > mid_close_ms ? hold_sample_ms - mid_close_ms
                                              : 1);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scheduler.lidClosureAmount());

  // Past blink duration → open again.
  clock.advance(kDurationMs);
  TEST_ASSERT_FALSE(scheduler.update());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, scheduler.lidClosureAmount());
}

void test_lid_phases_match_named_constants() {
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.35f, BlinkScheduler::kClosePhase);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.45f, BlinkScheduler::kHoldEnd);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.55f, BlinkScheduler::kOpenPhase);
  TEST_ASSERT_FLOAT_WITHIN(
      0.0001f, 1.0f,
      BlinkScheduler::kClosePhase + (BlinkScheduler::kHoldEnd -
                                     BlinkScheduler::kClosePhase) +
          BlinkScheduler::kOpenPhase);

  constexpr uint32_t kDurationMs = 200;
  FakeClock clock;
  BlinkScheduler scheduler(clock, 2000, 2000, kDurationMs);
  scheduler.requestImmediateBlink();
  TEST_ASSERT_TRUE(scheduler.update());

  // End of close phase: fully closed.
  const uint32_t close_end_ms = static_cast<uint32_t>(
      BlinkScheduler::kClosePhase * static_cast<float>(kDurationMs));
  clock.advance(close_end_ms);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, scheduler.lidClosureAmount());

  // Mid hold: still closed.
  const uint32_t hold_mid_ms =
      static_cast<uint32_t>(
          ((BlinkScheduler::kClosePhase + BlinkScheduler::kHoldEnd) * 0.5f) *
          static_cast<float>(kDurationMs)) -
      close_end_ms;
  clock.advance(hold_mid_ms);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, scheduler.lidClosureAmount());

  // Mid open phase: partially open.
  const uint32_t hold_end_ms = static_cast<uint32_t>(
      BlinkScheduler::kHoldEnd * static_cast<float>(kDurationMs));
  const uint32_t already = close_end_ms + hold_mid_ms;
  clock.advance(hold_end_ms > already ? hold_end_ms - already : 0);
  const uint32_t open_mid_ms = static_cast<uint32_t>(
      BlinkScheduler::kOpenPhase * 0.5f * static_cast<float>(kDurationMs));
  clock.advance(open_mid_ms);
  const float mid_open = scheduler.lidClosureAmount();
  TEST_ASSERT_TRUE(mid_open > 0.2f && mid_open < 0.8f);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_blink_triggers_after_interval);
  RUN_TEST(test_immediate_blink_request);
  RUN_TEST(test_lid_closure_peaks_mid_blink);
  RUN_TEST(test_lid_phases_match_named_constants);
  return UNITY_END();
}
