#include "MicroRosEyeNode.h"

#include "FeatureFlags.h"

#if ROSEYES_ENABLE_MICROROS

#include <Arduino.h>
#include <WiFi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void MicroRosEyeNode::runReconnectStep(uint32_t now_ms) {
  if (!ensureTransportConfigured()) {
    return;
  }

  if (entities_need_destroy_ || entities_init_depth_ != kStepNone) {
    destroyEntities();
    entities_need_destroy_ = false;
    next_entity_retry_ms_ = now_ms + kEntityRetryIntervalMs;
    return;
  }

  if (rmw_uros_ping_agent(kAgentPingTimeoutMs, kAgentPingAttempts) !=
      RMW_RET_OK) {
    next_entity_retry_ms_ = now_ms + kEntityRetryIntervalMs;
    return;
  }
  if (createEntities()) {
    session_state_ = MicroRosSessionState::Ready;
    last_agent_ping_ms_ = now_ms;
    agent_ping_fail_streak_ = 0;
    Serial.println("micro-ROS entities ready");
  } else {
    Serial.println("micro-ROS entity create failed; retry later");
    next_entity_retry_ms_ = now_ms + kEntityRetryIntervalMs;
  }
}

void MicroRosEyeNode::reconnectTaskEntry(void* arg) {
  static_cast<MicroRosEyeNode*>(arg)->reconnectTaskLoop();
}

void MicroRosEyeNode::reconnectTaskLoop() {
  for (;;) {
    const uint32_t now_ms = millis();

    // Alive-ping owns the same task as destroy/create — never on the eye thread.
    if (credentials_valid_ && !reconnect_busy_.load() &&
        session_state_ == MicroRosSessionState::Ready &&
        WiFi.status() == WL_CONNECTED &&
        (now_ms - last_agent_ping_ms_) >= kAgentAlivePingMs) {
      last_agent_ping_ms_ = now_ms;
      if (rmw_uros_ping_agent(kAgentPingTimeoutMs, kAgentPingAttempts) !=
          RMW_RET_OK) {
        if (agent_ping_fail_streak_ < 255) {
          ++agent_ping_fail_streak_;
        }
        if (agent_ping_fail_streak_ >= kAgentPingFailLimit) {
          dropSessionToConnecting(now_ms, "agent ping failed");
        }
      } else {
        agent_ping_fail_streak_ = 0;
      }
    }

    const bool due = credentials_valid_ &&
                     session_state_ == MicroRosSessionState::Connecting &&
                     WiFi.status() == WL_CONNECTED &&
                     now_ms >= next_entity_retry_ms_;
    if ((reconnect_kick_.load() || due) && !reconnect_busy_.load() &&
        session_state_ == MicroRosSessionState::Connecting) {
      reconnect_kick_.store(false);
      reconnect_busy_.store(true);
      runReconnectStep(millis());
      reconnect_busy_.store(false);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

#endif  // ROSEYES_ENABLE_MICROROS
