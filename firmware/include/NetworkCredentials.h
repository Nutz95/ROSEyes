#pragma once

#include <stdint.h>

/**
 * WiFi and micro-ROS agent settings persisted in NVS / key-value storage.
 */
struct NetworkCredentials {
  char wifi_ssid[33];
  char wifi_password[65];
  char agent_ip[16];
  uint16_t agent_port;
};
