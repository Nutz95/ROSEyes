#include <unity.h>

#include "EyeControlMode.h"
#include "EyeControlModeParser.h"

void setUp() {}
void tearDown() {}

void test_parse_autonomous_aliases() {
  EyeControlMode mode = EyeControlMode::Piloted;
  TEST_ASSERT_TRUE(EyeControlModeParser::tryParse("autonomous", mode));
  TEST_ASSERT_EQUAL(static_cast<int>(EyeControlMode::Autonomous),
                    static_cast<int>(mode));
  TEST_ASSERT_TRUE(EyeControlModeParser::tryParse("AUTO", mode));
  TEST_ASSERT_EQUAL(static_cast<int>(EyeControlMode::Autonomous),
                    static_cast<int>(mode));
}

void test_parse_piloted_aliases() {
  EyeControlMode mode = EyeControlMode::Autonomous;
  TEST_ASSERT_TRUE(EyeControlModeParser::tryParse("piloted", mode));
  TEST_ASSERT_EQUAL(static_cast<int>(EyeControlMode::Piloted),
                    static_cast<int>(mode));
  TEST_ASSERT_TRUE(EyeControlModeParser::tryParse("Pilot", mode));
  TEST_ASSERT_EQUAL(static_cast<int>(EyeControlMode::Piloted),
                    static_cast<int>(mode));
}

void test_parse_rejects_unknown() {
  EyeControlMode mode = EyeControlMode::Autonomous;
  TEST_ASSERT_FALSE(EyeControlModeParser::tryParse("flying", mode));
  TEST_ASSERT_FALSE(EyeControlModeParser::tryParse(nullptr, mode));
}

void test_to_cstring() {
  TEST_ASSERT_EQUAL_STRING(
      "autonomous",
      EyeControlModeParser::toCString(EyeControlMode::Autonomous));
  TEST_ASSERT_EQUAL_STRING(
      "piloted", EyeControlModeParser::toCString(EyeControlMode::Piloted));
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parse_autonomous_aliases);
  RUN_TEST(test_parse_piloted_aliases);
  RUN_TEST(test_parse_rejects_unknown);
  RUN_TEST(test_to_cstring);
  return UNITY_END();
}
