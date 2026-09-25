#include "OtaUpdateService.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include <string.h>

OtaUpdateService::OtaUpdateService() : active_(false) {
  local_ip_[0] = '\0';
}

bool OtaUpdateService::begin(const NetworkCredentials& credentials) {
  active_ = false;
  local_ip_[0] = '\0';

  WiFi.mode(WIFI_STA);
  WiFi.begin(credentials.wifi_ssid, credentials.wifi_password);

  const uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - start_ms) < kWifiConnectTimeoutMs) {
    delay(kWifiPollDelayMs);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA: WiFi connect failed");
    return false;
  }

  const String ip = WiFi.localIP().toString();
  strncpy(local_ip_, ip.c_str(), sizeof(local_ip_) - 1);

  ArduinoOTA.setHostname(kOtaHostname);
  ArduinoOTA.onStart([]() { Serial.println("OTA: update starting"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA: update finished"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA: error %u\n", static_cast<unsigned>(error));
  });
  ArduinoOTA.begin();

  active_ = true;
  Serial.printf("OTA: ready at %s (use -OtaIp %s)\n", local_ip_, local_ip_);
  return true;
}

void OtaUpdateService::handle() {
  if (active_) {
    ArduinoOTA.handle();
  }
}

bool OtaUpdateService::isActive() const { return active_; }

const char* OtaUpdateService::localIpCStr() const { return local_ip_; }
