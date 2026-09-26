#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "CameraJpegMailbox.h"

void setUp() {}
void tearDown() {}

void test_request_then_consume() {
  CameraJpegMailbox mailbox;
  TEST_ASSERT_FALSE(mailbox.consumeCaptureRequest());
  mailbox.requestCapture();
  TEST_ASSERT_TRUE(mailbox.consumeCaptureRequest());
  TEST_ASSERT_FALSE(mailbox.consumeCaptureRequest());
}

void test_publish_then_take() {
  CameraJpegMailbox mailbox;
  const uint8_t payload[] = {0xFF, 0xD8, 0xFF, 0xD9};
  TEST_ASSERT_TRUE(mailbox.publishJpeg(payload, sizeof(payload)));
  TEST_ASSERT_TRUE(mailbox.hasJpeg());

  uint8_t out[8] = {};
  const size_t length = mailbox.takeJpeg(out, sizeof(out));
  TEST_ASSERT_EQUAL_UINT(sizeof(payload), length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, out, sizeof(payload));
  TEST_ASSERT_FALSE(mailbox.hasJpeg());
  TEST_ASSERT_EQUAL_UINT(0, mailbox.takeJpeg(out, sizeof(out)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_request_then_consume);
  RUN_TEST(test_publish_then_take);
  return UNITY_END();
}
