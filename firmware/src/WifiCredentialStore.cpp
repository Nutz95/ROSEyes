#include "WifiCredentialStore.h"

#include "NetworkSeedConfig.h"

#include <string.h>

WifiCredentialStore::WifiCredentialStore(IKeyValueStore& key_value_store)
    : key_value_store_(key_value_store) {}

bool WifiCredentialStore::loadOrSeed(NetworkCredentials& credentials) {
  if (!key_value_store_.begin(kNamespaceName, false)) {
    return false;
  }

  const bool loaded = load(credentials);
  if (loaded && isComplete(credentials)) {
    key_value_store_.end();
    return true;
  }

  if (!seedFromBuildFlags(credentials) || !isComplete(credentials)) {
    key_value_store_.end();
    return false;
  }

  const bool saved = save(credentials);
  key_value_store_.end();
  return saved;
}

bool WifiCredentialStore::replaceAll(const NetworkCredentials& credentials) {
  if (!isComplete(credentials)) {
    return false;
  }
  if (!key_value_store_.begin(kNamespaceName, false)) {
    return false;
  }
  const bool saved = save(credentials);
  key_value_store_.end();
  return saved;
}

bool WifiCredentialStore::isComplete(const NetworkCredentials& credentials) {
  return isNonEmpty(credentials.wifi_ssid) && isNonEmpty(credentials.agent_ip);
}

bool WifiCredentialStore::load(NetworkCredentials& credentials) const {
  memset(&credentials, 0, sizeof(credentials));
  credentials.agent_port = kDefaultAgentPort;

  if (!key_value_store_.getString(kKeyWifiSsid, credentials.wifi_ssid,
                                  sizeof(credentials.wifi_ssid))) {
    return false;
  }
  if (!isNonEmpty(credentials.wifi_ssid)) {
    return false;
  }

  key_value_store_.getString(kKeyWifiPassword, credentials.wifi_password,
                             sizeof(credentials.wifi_password));
  if (!key_value_store_.getString(kKeyAgentIp, credentials.agent_ip,
                                  sizeof(credentials.agent_ip))) {
    credentials.agent_ip[0] = '\0';
  }
  credentials.agent_port =
      key_value_store_.getUInt16(kKeyAgentPort, kDefaultAgentPort);
  return true;
}

bool WifiCredentialStore::save(const NetworkCredentials& credentials) {
  if (!key_value_store_.putString(kKeyWifiSsid, credentials.wifi_ssid)) {
    return false;
  }
  if (!key_value_store_.putString(kKeyWifiPassword, credentials.wifi_password)) {
    return false;
  }
  if (!key_value_store_.putString(kKeyAgentIp, credentials.agent_ip)) {
    return false;
  }
  return key_value_store_.putUInt16(kKeyAgentPort, credentials.agent_port);
}

bool WifiCredentialStore::seedFromBuildFlags(NetworkCredentials& credentials) {
  memset(&credentials, 0, sizeof(credentials));
  credentials.agent_port = static_cast<uint16_t>(MICROROS_AGENT_PORT_SEED);

  if (!isNonEmpty(WIFI_SSID_SEED) || !isNonEmpty(MICROROS_AGENT_IP_SEED)) {
    return false;
  }

  strncpy(credentials.wifi_ssid, WIFI_SSID_SEED,
          sizeof(credentials.wifi_ssid) - 1);
  strncpy(credentials.wifi_password, WIFI_PASS_SEED,
          sizeof(credentials.wifi_password) - 1);
  strncpy(credentials.agent_ip, MICROROS_AGENT_IP_SEED,
          sizeof(credentials.agent_ip) - 1);
  return true;
}

bool WifiCredentialStore::isNonEmpty(const char* text) {
  return text != nullptr && text[0] != '\0';
}
