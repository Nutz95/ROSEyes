#pragma once

#include <stdint.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <geometry_msgs/msg/vector3.h>
#include <std_msgs/msg/empty.h>
#include <std_msgs/msg/string.h>

#include "GazeState.h"
#include "IFreshGazeProvider.h"
#include "MicroRosSessionState.h"
#include "NetworkCredentials.h"

/**
 * micro-ROS node: WiFi XRCE transport, gaze subscription, status publisher.
 * Init/reconnect is non-blocking so idle eye animation keeps running.
 */
class MicroRosEyeNode : public IFreshGazeProvider {
 public:
  static constexpr uint32_t kGazeTimeoutMs = 500;
  static constexpr uint32_t kStatusPeriodMs = 1000;
  /** Backoff between agent ping / entity create attempts. */
  static constexpr uint32_t kEntityRetryIntervalMs = 8000;
  /** Short ping so a missing agent does not stall the render loop. */
  static constexpr int kAgentPingTimeoutMs = 50;
  static constexpr uint8_t kAgentPingAttempts = 1;

  /** Constructs an idle micro-ROS eye node. */
  MicroRosEyeNode();

  /**
   * Stores agent credentials and arms async connect (no blocking WiFi wait).
   * Requires station WiFi already joining/joined (e.g. via OtaUpdateService).
   * @return true when credentials look usable
   */
  bool begin(const NetworkCredentials& credentials);

  /** Spins / retries without long stalls when agent or WiFi is absent. */
  void update(uint32_t now_ms);

  /** True when a gaze message arrived within the timeout window. */
  bool hasFreshGaze(uint32_t now_ms) const override;

  /** Copies the latest received gaze into destination. */
  void copyGaze(GazeState& destination) const override;

  /** True when an immediate blink was requested via /eyes/blink. */
  bool consumeBlinkRequest();

  /** Returns the current session lifecycle state. */
  MicroRosSessionState sessionState() const;

 private:
  bool ensureTransportConfigured();
  bool createEntities();
  void destroyEntities();
  static void onGazeMessage(const void* message_void);
  static void onBlinkMessage(const void* message_void);

  static MicroRosEyeNode* active_instance_;

  rcl_allocator_t allocator_;
  rclc_support_t support_;
  rcl_node_t node_;
  rcl_subscription_t gaze_subscription_;
  rcl_subscription_t blink_subscription_;
  rcl_publisher_t status_publisher_;
  rclc_executor_t executor_;
  geometry_msgs__msg__Vector3 gaze_message_;
  std_msgs__msg__Empty blink_message_;
  std_msgs__msg__String status_message_;
  char status_buffer_[64];

  GazeState received_gaze_;
  uint32_t last_gaze_ms_;
  uint32_t last_status_ms_;
  uint32_t next_entity_retry_ms_;
  bool blink_requested_;
  bool transport_configured_;
  bool credentials_valid_;
  MicroRosSessionState session_state_;
  uint8_t entities_init_depth_;
  uint16_t agent_port_;
  char agent_ip_[16];
};
