#pragma once

#include "NetworkCredentials.h"

/**
 * WiFi + ArduinoOTA helper so firmware can be updated over the LAN.
 */
class OtaUpdateService {
 public:
  static constexpr uint32_t kWifiConnectTimeoutMs = 15000;
  static constexpr uint32_t kWifiPollDelayMs = 200;
  static constexpr const char* kOtaHostname = "roseyes";

  /** Constructs an inactive OTA service. */
  OtaUpdateService();

  /**
   * Connects WiFi from credentials and starts ArduinoOTA.
   * @return true when WiFi is connected and OTA is listening
   */
  bool begin(const NetworkCredentials& credentials);

  /** Must be called frequently from the main loop. */
  void handle();

  /** True after a successful begin(). */
  bool isActive() const;

  /** Local IP string when WiFi is up, otherwise empty. */
  const char* localIpCStr() const;

 private:
  bool active_;
  char local_ip_[16];
};
