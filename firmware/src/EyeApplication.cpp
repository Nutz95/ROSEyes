#include "EyeApplication.h"

#include <Arduino.h>
#include <math.h>

#include "BoardPins.h"
#include "NetworkCredentials.h"

EyeApplication::EyeApplication()
    : credential_store_(nvs_store_),
      eye_renderer_(display_),
      blink_scheduler_(clock_, kBlinkMinIntervalMs, kBlinkMaxIntervalMs,
                       kBlinkDurationMs),
      idle_behavior_(clock_, kIdleMaxNormalizedOffset, kIdleSaccadeMinMs,
                     kIdleSaccadeMaxMs, kIdleFixationMinMs, kIdleFixationMaxMs),
#if ROSEYES_ENABLE_MICROROS
      gaze_source_(idle_behavior_, &micro_ros_node_),
#else
      gaze_source_(idle_behavior_, nullptr),
#endif
      force_redraw_(true),
      last_lid_closure_(-1.0f),
      last_drawn_gaze_x_(0.0f),
      last_drawn_gaze_y_(0.0f),
      last_heartbeat_ms_(0) {}

void EyeApplication::setup() {
  Serial.begin(BoardPins::kSerialBaudRate);
  delay(kSerialSettleMs);
  Serial.println("ROSEyes dual-eye firmware starting");
  Serial.flush();

  display_.begin();
  if (!eye_renderer_.begin()) {
    Serial.println("Eye framebuffer allocation failed");
  }
  force_redraw_ = true;
  renderIfDirty();
  Serial.println("Displays initialized (idle animation continues during WiFi/ROS)");

  NetworkCredentials credentials;
  const bool have_credentials = credential_store_.loadOrSeed(credentials);
  if (have_credentials) {
    // Non-blocking: association + OTA happen in loop via handle().
    ota_service_.begin(credentials);
  } else {
    Serial.println(
        "No WiFi credentials in NVS. Flash once with WIFI_SSID/WIFI_PASS "
        "(and MICROROS_AGENT_IP for micro-ROS) to enable OTA.");
  }

#if ROSEYES_ENABLE_MICROROS
  if (!have_credentials) {
    Serial.println("Continuing in idle-only mode (no micro-ROS).");
    return;
  }

  Serial.printf("Using agent %s:%u\n", credentials.agent_ip,
                credentials.agent_port);
  // Non-blocking: entity create waits for WiFi + agent ping in update().
  if (!micro_ros_node_.begin(credentials)) {
    Serial.println("micro-ROS begin rejected credentials; idle eyes active");
  }
#else
  Serial.println("Built without micro-ROS (eyes-only). Idle blink active.");
#endif
}

void EyeApplication::loop() {
  ota_service_.handle();
  uint32_t now_ms = clock_.millis();

#if ROSEYES_ENABLE_MICROROS
  micro_ros_node_.update(now_ms);
  // Refresh after executor callbacks so gaze freshness uses a current stamp.
  now_ms = clock_.millis();

  if (micro_ros_node_.consumeBlinkRequest()) {
    blink_scheduler_.requestImmediateBlink();
  }
#endif

  gaze_source_.selectGaze(now_ms, gaze_state_);
  blink_scheduler_.update();
  renderIfDirty();

  if ((now_ms - last_heartbeat_ms_) >= kHeartbeatPeriodMs) {
    last_heartbeat_ms_ = now_ms;
    Serial.printf(
        "hb wifi=%d ota=%d gaze=%.2f,%.2f lid=%.2f",
        ota_service_.isWifiConnected() ? 1 : 0,
        ota_service_.isActive() ? 1 : 0, gaze_state_.x(), gaze_state_.y(),
        blink_scheduler_.lidClosureAmount());
#if ROSEYES_ENABLE_MICROROS
    Serial.printf(" ros=%u ip=%s\n",
                  static_cast<unsigned>(micro_ros_node_.sessionState()),
                  ota_service_.localIpCStr());
#else
    Serial.printf(" ip=%s\n", ota_service_.localIpCStr());
#endif
    Serial.flush();
  }

  delay(kFramePeriodMs);
}

void EyeApplication::renderIfDirty() {
  const float lid_closure = blink_scheduler_.lidClosureAmount();
  const bool gaze_changed =
      fabsf(gaze_state_.x() - last_drawn_gaze_x_) > kGazeDirtyEpsilon ||
      fabsf(gaze_state_.y() - last_drawn_gaze_y_) > kGazeDirtyEpsilon;
  const bool lid_changed =
      fabsf(lid_closure - last_lid_closure_) > kLidDirtyEpsilon;

  if (!force_redraw_ && !gaze_changed && !lid_changed) {
    return;
  }

  eye_renderer_.drawEyes(gaze_state_, kIrisColor, lid_closure);

  last_lid_closure_ = lid_closure;
  last_drawn_gaze_x_ = gaze_state_.x();
  last_drawn_gaze_y_ = gaze_state_.y();
  force_redraw_ = false;
}
