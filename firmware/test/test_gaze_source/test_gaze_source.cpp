#include <math.h>
#include <unity.h>

#include "FakeClock.h"
#include "GazeSource.h"
#include "GazeState.h"
#include "IFreshGazeProvider.h"
#include "IdleEyeBehavior.h"

namespace {

class StubFreshGazeProvider : public IFreshGazeProvider {
 public:
  StubFreshGazeProvider() : fresh_(false) {
    gaze_.setNormalized(0.5f, -0.25f);
  }

  void setFresh(bool fresh) { fresh_ = fresh; }

  bool hasFreshGaze(uint32_t now_ms) const override {
    (void)now_ms;
    return fresh_;
  }

  void copyGaze(GazeState& destination) const override {
    destination.setNormalized(gaze_.x(), gaze_.y());
  }

 private:
  bool fresh_;
  GazeState gaze_;
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_gaze_source_uses_idle_without_provider() {
  FakeClock clock;
  IdleEyeBehavior idle(clock, 1.0f, 350, 780, 650, 2400);
  GazeSource source(idle, nullptr);
  GazeState gaze;
  source.selectGaze(0, gaze);
  TEST_ASSERT_TRUE(fabsf(gaze.x()) <= 1.0f + 0.001f);
  TEST_ASSERT_TRUE(fabsf(gaze.y()) <= 1.0f + 0.001f);
}

void test_gaze_source_prefers_fresh_external() {
  FakeClock clock;
  IdleEyeBehavior idle(clock, 1.0f, 350, 780, 650, 2400);
  StubFreshGazeProvider provider;
  provider.setFresh(true);
  GazeSource source(idle, &provider);
  GazeState gaze;
  source.selectGaze(100, gaze);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, gaze.x());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.25f, gaze.y());
}

void test_gaze_source_falls_back_when_stale() {
  FakeClock clock;
  IdleEyeBehavior idle(clock, 1.0f, 350, 780, 650, 2400);
  StubFreshGazeProvider provider;
  provider.setFresh(false);
  GazeSource source(idle, &provider);
  GazeState gaze;
  gaze.setNormalized(0.9f, 0.9f);
  source.selectGaze(100, gaze);
  // Idle syncs from the previous pose within the full normalized roam radius.
  TEST_ASSERT_TRUE(fabsf(gaze.x()) <= 1.0f + 0.001f);
  TEST_ASSERT_TRUE(fabsf(gaze.y()) <= 1.0f + 0.001f);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_gaze_source_uses_idle_without_provider);
  RUN_TEST(test_gaze_source_prefers_fresh_external);
  RUN_TEST(test_gaze_source_falls_back_when_stale);
  return UNITY_END();
}
