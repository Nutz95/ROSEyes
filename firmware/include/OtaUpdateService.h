#pragma once

#include <stdint.h>

#include "NetworkCredentials.h"

/**
 * Non-blocking WiFi + ArduinoOTA helper (poll from the main loop).
 */
class OtaUpdateService {
 public:
  static constexpr uint32_t kWifiConnectTimeoutMs = 15000;
  static constexpr uint32_t kWifiRetryIntervalMs = 30000;
  static constexpr const char* kOtaHostname = "roseyes";

  /** Constructs an inactive OTA service. */
  OtaUpdateService();

  /**
   * Starts a non-blocking WiFi join; OTA begins once associated.
   * Safe to call once from setup(); progress happens in handle().
   * @return true when credentials were accepted and a join was started
   */
  bool begin(const NetworkCredentials& credentials);

  /** Advances WiFi/OTA state; must be called frequently from the main loop. */
  void handle();

  /** True when ArduinoOTA is listening. */
  bool isActive() const;

  /** True when station WiFi is associated. */
  bool isWifiConnected() const;

  /** Local IP string when WiFi is up, otherwise empty. */
  const char* localIpCStr() const;

 private:
  enum class Phase : uint8_t {
    Idle = 0,
    Connecting = 1,
    Ready = 2,
    FailedWaitingRetry = 3,
  };

  void startWifiJoin();
  void startArduinoOta();
  void rememberLocalIp();

  Phase phase_;
  uint32_t phase_started_ms_;
  uint32_t next_retry_ms_;
  bool credentials_valid_;
  char wifi_ssid_[33];
  char wifi_password_[65];
  char local_ip_[16];
};
