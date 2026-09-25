#include <unity.h>

#include "EyeFrameBuffer.h"

void setUp() {}
void tearDown() {}

void test_fill_sets_all_pixels() {
  EyeFrameBuffer buffer;
  TEST_ASSERT_TRUE(buffer.begin());
  buffer.fill(0xF800);
  const uint16_t* pixels = buffer.data();
  TEST_ASSERT_NOT_NULL(pixels);
  TEST_ASSERT_EQUAL_HEX16(0xF800, pixels[0]);
  TEST_ASSERT_EQUAL_HEX16(0xF800, pixels[EyeFrameBuffer::kPixelCount / 2]);
  TEST_ASSERT_EQUAL_HEX16(0xF800,
                          pixels[EyeFrameBuffer::kPixelCount - 1]);
}

void test_fill_rect_clips_and_writes() {
  EyeFrameBuffer buffer;
  TEST_ASSERT_TRUE(buffer.begin());
  buffer.fill(0x0000);
  buffer.fillRect(-4, -2, 10, 6, 0x07E0);

  const uint16_t* pixels = buffer.data();
  // Clipped to x=0..5, y=0..3
  TEST_ASSERT_EQUAL_HEX16(0x07E0, pixels[0]);
  TEST_ASSERT_EQUAL_HEX16(0x07E0, pixels[5]);
  TEST_ASSERT_EQUAL_HEX16(0x0000, pixels[6]);
  TEST_ASSERT_EQUAL_HEX16(
      0x07E0, pixels[3 * EyeFrameBuffer::kWidth + 5]);
  TEST_ASSERT_EQUAL_HEX16(
      0x0000, pixels[4 * EyeFrameBuffer::kWidth]);
}

void test_apply_lid_closure_covers_upper_and_lower() {
  EyeFrameBuffer buffer;
  TEST_ASSERT_TRUE(buffer.begin());
  buffer.fill(0xFFFF);

  buffer.applyLidClosure(1.0f);
  const uint16_t* pixels = buffer.data();
  TEST_ASSERT_EQUAL_HEX16(0x0000, pixels[0]);
  TEST_ASSERT_EQUAL_HEX16(
      0x0000, pixels[(EyeFrameBuffer::kHeight - 1) * EyeFrameBuffer::kWidth]);

  const int expected_lower = static_cast<int>(
      EyeFrameBuffer::kLowerLidFraction *
      static_cast<float>(EyeFrameBuffer::kHeight));
  const int mid_open_row = EyeFrameBuffer::kHeight - expected_lower - 1;
  if (mid_open_row > 0 && mid_open_row < EyeFrameBuffer::kHeight) {
    // Full closure: upper covers entire height, so mid is also lid color.
    TEST_ASSERT_EQUAL_HEX16(
        0x0000, pixels[mid_open_row * EyeFrameBuffer::kWidth]);
  }
}

void test_apply_lid_closure_partial_upper() {
  EyeFrameBuffer buffer;
  TEST_ASSERT_TRUE(buffer.begin());
  buffer.fill(0xFFFF);
  buffer.applyLidClosure(0.25f);

  const uint16_t* pixels = buffer.data();
  const int upper_cover =
      static_cast<int>(0.25f * static_cast<float>(EyeFrameBuffer::kHeight));
  TEST_ASSERT_TRUE(upper_cover > 0);
  TEST_ASSERT_EQUAL_HEX16(0x0000, pixels[0]);
  TEST_ASSERT_EQUAL_HEX16(
      0xFFFF, pixels[upper_cover * EyeFrameBuffer::kWidth]);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_fill_sets_all_pixels);
  RUN_TEST(test_fill_rect_clips_and_writes);
  RUN_TEST(test_apply_lid_closure_covers_upper_and_lower);
  RUN_TEST(test_apply_lid_closure_partial_upper);
  return UNITY_END();
}
