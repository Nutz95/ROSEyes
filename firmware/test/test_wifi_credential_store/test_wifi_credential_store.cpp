#include <unity.h>

#include <cstring>

#include "InMemoryKeyValueStore.h"
#include "WifiCredentialStore.h"

void setUp() {}
void tearDown() {}

void test_wifi_replace_then_load_or_seed() {
  InMemoryKeyValueStore memory_store;
  WifiCredentialStore store(memory_store);

  NetworkCredentials input;
  std::memset(&input, 0, sizeof(input));
  std::strncpy(input.wifi_ssid, "TestNet", sizeof(input.wifi_ssid) - 1);
  std::strncpy(input.wifi_password, "secret", sizeof(input.wifi_password) - 1);
  std::strncpy(input.agent_ip, "192.168.1.10", sizeof(input.agent_ip) - 1);
  input.agent_port = 8888;

  TEST_ASSERT_TRUE(store.replaceAll(input));

  NetworkCredentials output;
  TEST_ASSERT_TRUE(store.loadOrSeed(output));
  TEST_ASSERT_EQUAL_STRING("TestNet", output.wifi_ssid);
  TEST_ASSERT_EQUAL_STRING("secret", output.wifi_password);
  TEST_ASSERT_EQUAL_STRING("192.168.1.10", output.agent_ip);
  TEST_ASSERT_EQUAL_UINT16(8888, output.agent_port);
}

void test_wifi_is_complete_helper() {
  NetworkCredentials credentials;
  std::memset(&credentials, 0, sizeof(credentials));
  TEST_ASSERT_FALSE(WifiCredentialStore::isComplete(credentials));

  std::strncpy(credentials.wifi_ssid, "Net", sizeof(credentials.wifi_ssid) - 1);
  TEST_ASSERT_FALSE(WifiCredentialStore::isComplete(credentials));

  std::strncpy(credentials.agent_ip, "192.168.1.1",
               sizeof(credentials.agent_ip) - 1);
  TEST_ASSERT_TRUE(WifiCredentialStore::isComplete(credentials));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_wifi_replace_then_load_or_seed);
  RUN_TEST(test_wifi_is_complete_helper);
  return UNITY_END();
}
