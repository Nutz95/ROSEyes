#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "RangeObservation.h"
#include "RangeTelemetryFormatter.h"

void setUp() {}
void tearDown() {}

void test_range_format_contains_keys() {
  char buffer[96];
  RangeObservation observation{};
  observation.distance_mm = 1234;
  observation.valid = true;
  observation.status = 0;
  observation.signal_strength = 42;
  observation.seq = 7;
  const int written =
      RangeTelemetryFormatter::format(observation, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(written > 0);
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"mm\":1234"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"ok\":true"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"status\":0"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"strength\":42"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"seq\":7"));
}

void test_range_format_rejects_tiny_buffer() {
  char buffer[4];
  RangeObservation observation{};
  TEST_ASSERT_EQUAL_INT(
      -1, RangeTelemetryFormatter::format(observation, buffer, sizeof(buffer)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_range_format_contains_keys);
  RUN_TEST(test_range_format_rejects_tiny_buffer);
  return UNITY_END();
}
