#pragma once

#include <stdint.h>

#include "ArduinoClock.h"
#include "BallGazeProvider.h"
#include "BallObservationStore.h"
#include "BallVisionService.h"
#include "BlinkScheduler.h"
#include "CameraJpegMailbox.h"
#include "CpuLoadSampler.h"
#include "DualEyeDisplay.h"
#include "EyeRenderer.h"
#include "FeatureFlags.h"
#include "GazeSource.h"
#include "GazeState.h"
#include "IdleEyeBehavior.h"
#include "NvsKeyValueStore.h"
#include "OtaUpdateService.h"
#include "RangeObservation.h"
#include "Rgb565Colors.h"
#include "TofRangeService.h"
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
  /** How often to publish /eyes/ball telemetry. */
  static constexpr uint32_t kBallTelemetryPeriodMs = 200;
  /** How often to publish /eyes/perf telemetry. */
  static constexpr uint32_t kPerfTelemetryPeriodMs = 1000;
  /** How often to sample TOF and publish /eyes/range. */
  static constexpr uint32_t kRangeTelemetryPeriodMs = 100;

  /** Constructs the application with default blink and idle timings. */
  EyeApplication();

  /** Initializes serial, displays, NVS credentials, and micro-ROS. */
  void setup();

  /** Runs one animation / ROS service iteration. */
  void loop();

 private:
  void renderIfDirty();
  void publishBallTelemetry(uint32_t now_ms);
  void publishPerfTelemetry(uint32_t now_ms);
  void publishRangeTelemetry(uint32_t now_ms);

  ArduinoClock clock_;
  NvsKeyValueStore nvs_store_;
  WifiCredentialStore credential_store_;
  DualEyeDisplay display_;
  EyeRenderer eye_renderer_;
  GazeState gaze_state_;
  BlinkScheduler blink_scheduler_;
  IdleEyeBehavior idle_behavior_;
  OtaUpdateService ota_service_;
  CpuLoadSampler cpu_load_sampler_;
  BallObservationStore ball_observation_store_;
  CameraJpegMailbox camera_jpeg_mailbox_;
  BallVisionService ball_vision_;
  TofRangeService tof_range_;
#if ROSEYES_ENABLE_MICROROS
  MicroRosEyeNode micro_ros_node_;
#endif
  BallGazeProvider ball_gaze_provider_;
  GazeSource gaze_source_;
  bool force_redraw_;
  float last_lid_closure_;
  float last_drawn_gaze_x_;
  float last_drawn_gaze_y_;
  uint32_t last_heartbeat_ms_;
  uint32_t last_ball_telemetry_ms_;
  uint32_t last_perf_telemetry_ms_;
  uint32_t last_range_telemetry_ms_;
  uint32_t frames_since_perf_;
};
