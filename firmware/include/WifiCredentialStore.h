#pragma once

#include "IKeyValueStore.h"
#include "NetworkCredentials.h"

/**
 * Loads, seeds, and saves network credentials through an IKeyValueStore backend.
 */
class WifiCredentialStore {
 public:
  static constexpr const char* kNamespaceName = "roseyes";
  static constexpr const char* kKeyWifiSsid = "wifi_ssid";
  static constexpr const char* kKeyWifiPassword = "wifi_pass";
  static constexpr const char* kKeyAgentIp = "agent_ip";
  static constexpr const char* kKeyAgentPort = "agent_port";
  static constexpr uint16_t kDefaultAgentPort = 8888;

  /** Binds this store to a persistence backend. */
  explicit WifiCredentialStore(IKeyValueStore& key_value_store);

  /**
   * Loads credentials from storage, seeding from build flags when empty.
   * @return true when WiFi SSID and agent IP are both present
   */
  bool loadOrSeed(NetworkCredentials& credentials);

  /**
   * Overwrites persisted credentials.
   * @return true on successful write
   */
  bool replaceAll(const NetworkCredentials& credentials);

  /** Returns true when credentials contain the required fields. */
  static bool isComplete(const NetworkCredentials& credentials);

 private:
  bool load(NetworkCredentials& credentials) const;
  bool save(const NetworkCredentials& credentials);
  bool seedFromBuildFlags(NetworkCredentials& credentials);
  static bool isNonEmpty(const char* text);

  IKeyValueStore& key_value_store_;
};
