#pragma once

#include <stdint.h>

#include "ArduinoClock.h"
#include "BlinkScheduler.h"
#include "DualEyeDisplay.h"
#include "EyeRenderer.h"
#include "FeatureFlags.h"
#include "GazeSource.h"
#include "GazeState.h"
#include "IdleEyeBehavior.h"
#include "NvsKeyValueStore.h"
#include "OtaUpdateService.h"
#include "Rgb565Colors.h"
#include "WifiCredentialStore.h"

#if ROSEYES_ENABLE_MICROROS
#include "MicroRosEyeNode.h"
#endif

/**
 * Top-level firmware orchestrator for display, idle behavior, and micro-ROS.
 */
class EyeApplication {
 public:
  static constexpr uint16_t kIrisColor = Rgb565Colors::kOrange;
  static constexpr uint32_t kFramePeriodMs = 40;
  static constexpr uint32_t kBlinkMinIntervalMs = 2200;
  static constexpr uint32_t kBlinkMaxIntervalMs = 3800;
  static constexpr uint32_t kBlinkDurationMs = 260;
  static constexpr uint32_t kIdleSaccadeMinMs = 350;
  static constexpr uint32_t kIdleSaccadeMaxMs = 780;
  static constexpr uint32_t kIdleFixationMinMs = 650;
  static constexpr uint32_t kIdleFixationMaxMs = 2400;
  /** Idle roam radius — same [-1,1] envelope as joystick / ROS gaze. */
  static constexpr float kIdleMaxNormalizedOffset = 1.0f;
  static constexpr float kGazeDirtyEpsilon = 0.008f;
  static constexpr float kLidDirtyEpsilon = 0.015f;
  /** Serial CDC settle delay before the first boot banner. */
  static constexpr uint32_t kSerialSettleMs = 500;
  /** Period for the wifi/ros/gaze heartbeat line on Serial. */
  static constexpr uint32_t kHeartbeatPeriodMs = 2000;

  /** Constructs the application with default blink and idle timings. */
  EyeApplication();

  /** Initializes serial, displays, NVS credentials, and micro-ROS. */
  void setup();

  /** Runs one animation / ROS service iteration. */
  void loop();

 private:
  void renderIfDirty();

  ArduinoClock clock_;
  NvsKeyValueStore nvs_store_;
  WifiCredentialStore credential_store_;
  DualEyeDisplay display_;
  EyeRenderer eye_renderer_;
  GazeState gaze_state_;
  BlinkScheduler blink_scheduler_;
  IdleEyeBehavior idle_behavior_;
  OtaUpdateService ota_service_;
#if ROSEYES_ENABLE_MICROROS
  MicroRosEyeNode micro_ros_node_;
#endif
  GazeSource gaze_source_;
  bool force_redraw_;
  float last_lid_closure_;
  float last_drawn_gaze_x_;
  float last_drawn_gaze_y_;
  uint32_t last_heartbeat_ms_;
};
