#include "OtaUpdateService.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include <string.h>

OtaUpdateService::OtaUpdateService()
    : phase_(Phase::Idle),
      phase_started_ms_(0),
      next_retry_ms_(0),
      credentials_valid_(false) {
  wifi_ssid_[0] = '\0';
  wifi_password_[0] = '\0';
  local_ip_[0] = '\0';
}

bool OtaUpdateService::begin(const NetworkCredentials& credentials) {
  phase_ = Phase::Idle;
  local_ip_[0] = '\0';
  credentials_valid_ = false;

  if (credentials.wifi_ssid[0] == '\0') {
    Serial.println("OTA: empty SSID, skipping WiFi");
    return false;
  }

  strncpy(wifi_ssid_, credentials.wifi_ssid, sizeof(wifi_ssid_) - 1);
  wifi_ssid_[sizeof(wifi_ssid_) - 1] = '\0';
  strncpy(wifi_password_, credentials.wifi_password, sizeof(wifi_password_) - 1);
  wifi_password_[sizeof(wifi_password_) - 1] = '\0';
  credentials_valid_ = true;

  WiFi.mode(WIFI_STA);
  startWifiJoin();
  return true;
}

void OtaUpdateService::startWifiJoin() {
  phase_ = Phase::Connecting;
  phase_started_ms_ = millis();
  Serial.printf("OTA: connecting WiFi SSID=%s (non-blocking)...\n", wifi_ssid_);
  WiFi.begin(wifi_ssid_, wifi_password_);
}

void OtaUpdateService::rememberLocalIp() {
  const String ip = WiFi.localIP().toString();
  strncpy(local_ip_, ip.c_str(), sizeof(local_ip_) - 1);
  local_ip_[sizeof(local_ip_) - 1] = '\0';
}

void OtaUpdateService::startArduinoOta() {
  rememberLocalIp();
  ArduinoOTA.setHostname(kOtaHostname);
  ArduinoOTA.onStart([]() { Serial.println("OTA: update starting"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA: update finished"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA: error %u\n", static_cast<unsigned>(error));
  });
  ArduinoOTA.begin();
  phase_ = Phase::Ready;
  Serial.printf("OTA: ready at %s (use -OtaIp %s)\n", local_ip_, local_ip_);
}

void OtaUpdateService::handle() {
  const uint32_t now_ms = millis();

  switch (phase_) {
    case Phase::Idle:
      break;

    case Phase::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        startArduinoOta();
        break;
      }
      if ((now_ms - phase_started_ms_) >= kWifiConnectTimeoutMs) {
        Serial.println("OTA: WiFi connect timed out; will retry later");
        WiFi.disconnect(false);
        phase_ = Phase::FailedWaitingRetry;
        next_retry_ms_ = now_ms + kWifiRetryIntervalMs;
      }
      break;

    case Phase::Ready:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("OTA: WiFi lost; reconnecting...");
        local_ip_[0] = '\0';
        startWifiJoin();
        break;
      }
      ArduinoOTA.handle();
      break;

    case Phase::FailedWaitingRetry:
      if (!credentials_valid_) {
        break;
      }
      if (now_ms >= next_retry_ms_) {
        startWifiJoin();
      }
      break;
  }
}

bool OtaUpdateService::isActive() const { return phase_ == Phase::Ready; }

bool OtaUpdateService::isWifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

const char* OtaUpdateService::localIpCStr() const { return local_ip_; }
