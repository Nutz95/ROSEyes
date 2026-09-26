#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "PerfSnapshot.h"
#include "PerfTelemetryFormatter.h"

void setUp() {}
void tearDown() {}

void test_perf_format_contains_keys() {
  char buffer[256];
  PerfSnapshot snapshot{};
  snapshot.heap_free = 100000;
  snapshot.heap_size = 327680;
  snapshot.heap_min = 80000;
  snapshot.psram_free = 4000000;
  snapshot.psram_size = 8388608;
  snapshot.loop_hz = 24.5f;
  snapshot.cpu0_pct = 35.0f;
  snapshot.cpu1_pct = 60.0f;
  snapshot.wifi_rssi = -55;
  snapshot.ball_fps = 8.0f;
  snapshot.uptime_s = 12;
  const int written =
      PerfTelemetryFormatter::format(buffer, sizeof(buffer), snapshot);
  TEST_ASSERT_TRUE(written > 0);
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"heap_free\":100000"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"heap_pct\":"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"psram_pct\":"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"cpu0_pct\":35"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"cpu1_pct\":60"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"loop_hz\":24.5"));
}

void test_perf_format_rejects_tiny_buffer() {
  char buffer[4];
  PerfSnapshot snapshot{};
  TEST_ASSERT_EQUAL_INT(
      -1, PerfTelemetryFormatter::format(buffer, sizeof(buffer), snapshot));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_perf_format_contains_keys);
  RUN_TEST(test_perf_format_rejects_tiny_buffer);
  return UNITY_END();
}
