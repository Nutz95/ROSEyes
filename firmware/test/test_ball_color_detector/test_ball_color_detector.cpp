#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unity.h>

#include "BallColorDetector.h"
#include "BallObservation.h"
#include "BallVisionConfig.h"

namespace {

uint16_t rgb888To565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) |
                               (b >> 3));
}

void fillSolid(uint16_t* pixels, int width, int height, uint16_t color) {
  const int count = width * height;
  for (int i = 0; i < count; ++i) {
    pixels[i] = color;
  }
}

void fillCircle(uint16_t* pixels, int width, int height, int cx, int cy,
                int radius, uint16_t color) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int dx = x - cx;
      const int dy = y - cy;
      if (dx * dx + dy * dy <= radius * radius) {
        pixels[y * width + x] = color;
      }
    }
  }
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_detector_finds_red_blob() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 60, 20, 12, rgb888To565(220, 30, 30));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_EQUAL(static_cast<int>(BallColor::Red),
                    static_cast<int>(observation.color));
  TEST_ASSERT_TRUE(observation.x > 0.2f);
  TEST_ASSERT_TRUE(observation.y < 0.0f);
  TEST_ASSERT_TRUE(observation.diameter > 0.1f);
}

void test_detector_rejects_empty_frame() {
  constexpr int kWidth = 40;
  constexpr int kHeight = 30;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(40, 40, 40));

  BallColorDetector detector;
  BallObservation observation{};
  observation.found = true;
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_FALSE(observation.found);
}

void test_detector_rejects_tiny_blob() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(10, 10, 10));
  fillCircle(pixels, kWidth, kHeight, 40, 30, 2, rgb888To565(255, 0, 0));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_FALSE(observation.found);
}

void test_detector_finds_small_qvga_blob() {
  constexpr int kWidth = 320;
  constexpr int kHeight = 240;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 240, 80, 5, rgb888To565(200, 40, 40));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_TRUE(observation.x > 0.2f);
  TEST_ASSERT_TRUE(observation.diameter > 0.02f);
}

void test_detector_green_fallback() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 20, 40, 12, rgb888To565(30, 220, 30));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_EQUAL(static_cast<int>(BallColor::Green),
                    static_cast<int>(observation.color));
  TEST_ASSERT_TRUE(observation.x < -0.2f);
}

void test_detector_prefers_red_over_green() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 20, 40, 12, rgb888To565(30, 220, 30));
  fillCircle(pixels, kWidth, kHeight, 60, 20, 12, rgb888To565(220, 30, 30));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_EQUAL(static_cast<int>(BallColor::Red),
                    static_cast<int>(observation.color));
  TEST_ASSERT_TRUE(observation.x > 0.2f);
}

void test_detector_sticky_green_beats_red() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 20, 40, 12, rgb888To565(30, 220, 30));
  fillCircle(pixels, kWidth, kHeight, 60, 20, 12, rgb888To565(220, 30, 30));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation, false, BallColor::Green);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_EQUAL(static_cast<int>(BallColor::Green),
                    static_cast<int>(observation.color));
  TEST_ASSERT_TRUE(observation.x < -0.2f);
}

void test_detector_rejects_yellow_lamp() {
  constexpr int kWidth = 80;
  constexpr int kHeight = 60;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(20, 20, 20));
  fillCircle(pixels, kWidth, kHeight, 60, 45, 18, rgb888To565(255, 200, 40));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_FALSE(observation.found);
}

void test_detector_prefers_compact_ball_over_huge_blob() {
  constexpr int kWidth = 160;
  constexpr int kHeight = 120;
  uint16_t* pixels =
      static_cast<uint16_t*>(malloc(sizeof(uint16_t) * kWidth * kHeight));
  TEST_ASSERT_NOT_NULL(pixels);
  fillSolid(pixels, kWidth, kHeight, rgb888To565(10, 10, 10));
  // Huge saturated-red region (would be d≈0.7) — must be rejected.
  for (int y = 10; y < 100; ++y) {
    for (int x = 10; x < 120; ++x) {
      pixels[y * kWidth + x] = rgb888To565(230, 20, 20);
    }
  }
  fillCircle(pixels, kWidth, kHeight, 140, 20, 10, rgb888To565(230, 20, 20));

  BallColorDetector detector;
  BallObservation observation{};
  detector.detect(pixels, kWidth, kHeight, observation);
  free(pixels);

  TEST_ASSERT_TRUE(observation.found);
  TEST_ASSERT_EQUAL(static_cast<int>(BallColor::Red),
                    static_cast<int>(observation.color));
  TEST_ASSERT_TRUE(observation.x > 0.3f);
  TEST_ASSERT_TRUE(observation.diameter < 0.30f);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_detector_finds_red_blob);
  RUN_TEST(test_detector_rejects_empty_frame);
  RUN_TEST(test_detector_rejects_tiny_blob);
  RUN_TEST(test_detector_finds_small_qvga_blob);
  RUN_TEST(test_detector_green_fallback);
  RUN_TEST(test_detector_prefers_red_over_green);
  RUN_TEST(test_detector_sticky_green_beats_red);
  RUN_TEST(test_detector_rejects_yellow_lamp);
  RUN_TEST(test_detector_prefers_compact_ball_over_huge_blob);
  return UNITY_END();
}
