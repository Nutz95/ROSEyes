#include "EyeApplication.h"

#include <Arduino.h>
#include <WiFi.h>
#include <math.h>

#include "BallTelemetryFormatter.h"
#include "BoardPins.h"
#include "EyeControlMode.h"
#include "EyeControlModeParser.h"
#include "FeatureFlags.h"
#include "NetworkCredentials.h"
#include "PerfSnapshot.h"
#include "PerfTelemetryFormatter.h"
#include "RangeTelemetryFormatter.h"

EyeApplication::EyeApplication()
    : credential_store_(nvs_store_),
      eye_renderer_(display_),
      blink_scheduler_(clock_, kBlinkMinIntervalMs, kBlinkMaxIntervalMs,
                       kBlinkDurationMs),
      idle_behavior_(clock_, kIdleMaxNormalizedOffset, kIdleSaccadeMinMs,
                     kIdleSaccadeMaxMs, kIdleFixationMinMs, kIdleFixationMaxMs),
      ball_vision_(ball_observation_store_, camera_jpeg_mailbox_),
      ball_gaze_provider_(ball_observation_store_),
#if ROSEYES_ENABLE_MICROROS
      gaze_source_(idle_behavior_, &micro_ros_node_, &ball_gaze_provider_),
#else
      gaze_source_(idle_behavior_, nullptr, &ball_gaze_provider_),
#endif
      force_redraw_(true),
      last_lid_closure_(-1.0f),
      last_drawn_gaze_x_(0.0f),
      last_drawn_gaze_y_(0.0f),
      last_heartbeat_ms_(0),
      last_ball_telemetry_ms_(0),
      last_perf_telemetry_ms_(0),
      last_range_telemetry_ms_(0),
      frames_since_perf_(0) {}

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
  cpu_load_sampler_.begin();
  Serial.println("Displays initialized (idle animation continues during WiFi/ROS)");

  // Start WiFi/OTA before camera/TOF so a stuck peripheral cannot brick updates.
  NetworkCredentials credentials;
  const bool have_credentials = credential_store_.loadOrSeed(credentials);
  if (have_credentials) {
    ota_service_.begin(credentials);
  } else {
    Serial.println(
        "No WiFi credentials in NVS. Flash once with WIFI_SSID/WIFI_PASS "
        "(and MICROROS_AGENT_IP for micro-ROS) to enable OTA.");
  }

#if ROSEYES_ENABLE_CAMERA_BALL
  if (ball_vision_.begin()) {
    Serial.println("Ball vision enabled (Autonomous tracks red ball)");
  } else {
    Serial.println("Ball vision unavailable; Autonomous uses idle only");
  }
#endif

  // TOF I2C runs on its own FreeRTOS task — never blocks eyes/OTA if the bus hangs.
#if ROSEYES_ENABLE_TOF
  if (!tof_range_.start()) {
    Serial.println("TOF Mini background start failed");
  }
#else
  Serial.println("TOF disabled (ROSEYES_ENABLE_TOF=0)");
#endif

#if ROSEYES_ENABLE_MICROROS
  if (!have_credentials) {
    Serial.println("Continuing in idle-only mode (no micro-ROS).");
    return;
  }

  Serial.printf("Using agent %s:%u\n", credentials.agent_ip,
                credentials.agent_port);
  if (!micro_ros_node_.begin(credentials)) {
    Serial.println("micro-ROS begin rejected credentials; idle eyes active");
  } else {
    micro_ros_node_.setJpegMailbox(&camera_jpeg_mailbox_);
  }
#else
  Serial.println("Built without micro-ROS (eyes-only). Idle blink active.");
#endif
}

void EyeApplication::publishBallTelemetry(uint32_t now_ms) {
  if ((now_ms - last_ball_telemetry_ms_) < kBallTelemetryPeriodMs) {
    return;
  }
  last_ball_telemetry_ms_ = now_ms;
  const BallObservation observation = ball_observation_store_.snapshot();
  char json[192];
  if (BallTelemetryFormatter::format(observation, json, sizeof(json)) < 0) {
    return;
  }
#if ROSEYES_ENABLE_MICROROS
  (void)micro_ros_node_.tryPublishBallJson(json);
#endif
}

void EyeApplication::publishPerfTelemetry(uint32_t now_ms) {
  if (last_perf_telemetry_ms_ == 0) {
    last_perf_telemetry_ms_ = now_ms;
    frames_since_perf_ = 0;
    return;
  }
  if ((now_ms - last_perf_telemetry_ms_) < kPerfTelemetryPeriodMs) {
    return;
  }
  const uint32_t elapsed_ms = now_ms - last_perf_telemetry_ms_;
  const float loop_hz =
      elapsed_ms > 0
          ? (static_cast<float>(frames_since_perf_) * 1000.0f /
             static_cast<float>(elapsed_ms))
          : 0.0f;
  last_perf_telemetry_ms_ = now_ms;
  frames_since_perf_ = 0;

  const BallObservation ball = ball_observation_store_.snapshot();
  float cpu0 = 0.0f;
  float cpu1 = 0.0f;
  cpu_load_sampler_.sample(cpu0, cpu1);
  PerfSnapshot snapshot{};
  snapshot.heap_free = ESP.getFreeHeap();
  snapshot.heap_size = ESP.getHeapSize();
  snapshot.heap_min = ESP.getMinFreeHeap();
  snapshot.psram_free = ESP.getFreePsram();
  snapshot.psram_size = ESP.getPsramSize();
  snapshot.loop_hz = loop_hz;
  snapshot.cpu0_pct = cpu0;
  snapshot.cpu1_pct = cpu1;
  snapshot.wifi_rssi = ota_service_.isWifiConnected() ? WiFi.RSSI() : 0;
  snapshot.ball_fps = ball.fps;
  snapshot.uptime_s = now_ms / 1000u;
  char json[256];
  if (PerfTelemetryFormatter::format(json, sizeof(json), snapshot) < 0) {
    return;
  }
#if ROSEYES_ENABLE_MICROROS
  (void)micro_ros_node_.tryPublishPerfJson(json);
#else
  (void)json;
#endif
}

void EyeApplication::publishRangeTelemetry(uint32_t now_ms) {
#if !ROSEYES_ENABLE_TOF
  (void)now_ms;
  return;
#else
  if (!tof_range_.isReady()) {
    return;
  }
  if ((now_ms - last_range_telemetry_ms_) < kRangeTelemetryPeriodMs) {
    return;
  }
  last_range_telemetry_ms_ = now_ms;

  const RangeObservation sample = tof_range_.snapshot();
  char json[96];
  if (RangeTelemetryFormatter::format(sample, json, sizeof(json)) < 0) {
    return;
  }
#if ROSEYES_ENABLE_MICROROS
  (void)micro_ros_node_.tryPublishRangeJson(json);
#else
  (void)json;
#endif
#endif
}

void EyeApplication::loop() {
  const uint32_t frame_start_ms = clock_.millis();
  ota_service_.handle();
  uint32_t now_ms = clock_.millis();

#if ROSEYES_ENABLE_MICROROS
  // Fast path only: ingest gaze/blink/mode before render (no reconnect).
  micro_ros_node_.spinIncoming(now_ms);
  if (micro_ros_node_.consumeBlinkRequest()) {
    blink_scheduler_.requestImmediateBlink();
  }
  const EyeControlMode mode = micro_ros_node_.controlMode();
  gaze_source_.setControlMode(mode);
  ball_vision_.setEnabled(mode == EyeControlMode::Autonomous);
#else
  gaze_source_.setControlMode(EyeControlMode::Autonomous);
  ball_vision_.setEnabled(true);
#endif

  gaze_source_.selectGaze(now_ms, gaze_state_);
#if ROSEYES_ENABLE_MICROROS
  micro_ros_node_.setReportedGaze(gaze_state_.x(), gaze_state_.y());
#endif
  blink_scheduler_.update();
  renderIfDirty();
  ++frames_since_perf_;

#if ROSEYES_ENABLE_MICROROS
  // Heavy XRCE work after eyes so agent-down / createEntities cannot freeze lids.
  now_ms = clock_.millis();
  micro_ros_node_.maintainSession(now_ms);
#endif

  now_ms = clock_.millis();
  publishBallTelemetry(now_ms);
  publishPerfTelemetry(now_ms);
  publishRangeTelemetry(now_ms);

  if ((now_ms - last_heartbeat_ms_) >= kHeartbeatPeriodMs) {
    last_heartbeat_ms_ = now_ms;
    const BallObservation ball = ball_observation_store_.snapshot();
    Serial.printf(
        "hb wifi=%d ota=%d mode=%s gaze=%.2f,%.2f lid=%.2f ball=%d",
        ota_service_.isWifiConnected() ? 1 : 0,
        ota_service_.isActive() ? 1 : 0,
        EyeControlModeParser::toCString(gaze_source_.controlMode()),
        gaze_state_.x(), gaze_state_.y(),
        blink_scheduler_.lidClosureAmount(), ball.found ? 1 : 0);
    if (tof_range_.isReady()) {
      const RangeObservation range = tof_range_.snapshot();
      Serial.printf(" tof=%lumm st=%u ok=%d",
                    static_cast<unsigned long>(range.distance_mm),
                    static_cast<unsigned>(range.status),
                    range.valid ? 1 : 0);
    } else {
      Serial.print(" tof=-");
    }
#if ROSEYES_ENABLE_MICROROS
    Serial.printf(" ros=%u rx=%d ip=%s\n",
                  static_cast<unsigned>(micro_ros_node_.sessionState()),
                  micro_ros_node_.hasFreshGaze(now_ms) ? 1 : 0,
                  ota_service_.localIpCStr());
#else
    Serial.printf(" ip=%s\n", ota_service_.localIpCStr());
#endif
    Serial.flush();
  }

  const uint32_t elapsed_ms = clock_.millis() - frame_start_ms;
  if (elapsed_ms < kFramePeriodMs) {
    delay(kFramePeriodMs - elapsed_ms);
  }
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
