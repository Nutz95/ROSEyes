#pragma once

#include <stddef.h>
#include <stdint.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <geometry_msgs/msg/vector3.h>
#include <std_msgs/msg/empty.h>
#include <std_msgs/msg/string.h>
#include <std_msgs/msg/u_int8_multi_array.h>

#include "CameraJpegMailbox.h"
#include "EyeControlMode.h"
#include "GazeState.h"
#include "IFreshGazeProvider.h"
#include "MicroRosSessionState.h"
#include "NetworkCredentials.h"

/**
 * micro-ROS node: WiFi XRCE transport, gaze/mode/blink/snap, status/ball/jpeg.
 */
class MicroRosEyeNode : public IFreshGazeProvider {
 public:
  static constexpr uint32_t kGazeTimeoutMs = 2500;
  static constexpr uint32_t kStatusPeriodMs = 2000;
  static constexpr uint32_t kEntityRetryIntervalMs = 2000;
  static constexpr uint32_t kAgentAlivePingMs = 2000;
  static constexpr uint32_t kPublishFailGraceMs = 3000;
  static constexpr uint8_t kPublishFailLimit = 3;
  static constexpr int kAgentPingTimeoutMs = 50;
  static constexpr uint8_t kAgentPingAttempts = 1;
  static constexpr size_t kJpegTxCapacity = CameraJpegMailbox::kMaxBytes;

  /** Constructs an idle micro-ROS eye node. */
  MicroRosEyeNode();

  /**
   * Stores agent credentials and arms async connect (no blocking WiFi wait).
   * @return true when credentials look usable
   */
  bool begin(const NetworkCredentials& credentials);

  /** Optional JPEG mailbox for /eyes/camera/snap → /eyes/camera/jpeg. */
  void setJpegMailbox(CameraJpegMailbox* mailbox);

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

  /**
   * Host-selected control mode (/eyes/mode). Forces Autonomous when the
   * XRCE session is not Ready.
   */
  EyeControlMode controlMode() const;

  /**
   * Publishes JSON on /eyes/ball when the session is Ready.
   * @return true on successful publish
   */
  bool tryPublishBallJson(const char* json);

  /**
   * Publishes JSON on /eyes/perf when the session is Ready.
   * @return true on successful publish
   */
  bool tryPublishPerfJson(const char* json);

  /**
   * Gaze shown in /eyes/status (active mux: ROS / ball / idle), not only last
   * host /eyes/gaze sample.
   */
  void setReportedGaze(float x, float y);

 private:
  enum EntityInitStep : uint8_t {
    kStepNone = 0,
    kStepSupport = 1,
    kStepNode = 2,
    kStepGazeSub = 3,
    kStepBlinkSub = 4,
    kStepModeSub = 5,
    kStepSnapSub = 6,
    kStepStatusPub = 7,
    kStepBallPub = 8,
    kStepJpegPub = 9,
    kStepPerfPub = 10,
    kStepExecutor = 11,
    kStepGazeCallback = 12,
    kStepBlinkCallback = 13,
    kStepModeCallback = 14,
    kStepSnapCallback = 15,
  };

  bool ensureTransportConfigured();
  bool createEntities();
  void destroyEntities();
  void forceAutonomousMode();
  void dropSessionToConnecting(const char* reason);
  void publishPendingJpeg();
  bool tryPublishString(rcl_publisher_t& publisher,
                        std_msgs__msg__String& message, char* buffer,
                        size_t capacity, const char* json);
  static void onGazeMessage(const void* message_void);
  static void onBlinkMessage(const void* message_void);
  static void onModeMessage(const void* message_void);
  static void onSnapMessage(const void* message_void);

  static MicroRosEyeNode* active_instance_;

  rcl_allocator_t allocator_;
  rclc_support_t support_;
  rcl_node_t node_;
  rcl_subscription_t gaze_subscription_;
  rcl_subscription_t blink_subscription_;
  rcl_subscription_t mode_subscription_;
  rcl_subscription_t snap_subscription_;
  rcl_publisher_t status_publisher_;
  rcl_publisher_t ball_publisher_;
  rcl_publisher_t jpeg_publisher_;
  rcl_publisher_t perf_publisher_;
  rclc_executor_t executor_;
  geometry_msgs__msg__Vector3 gaze_message_;
  std_msgs__msg__Empty blink_message_;
  std_msgs__msg__Empty snap_message_;
  std_msgs__msg__String mode_message_;
  std_msgs__msg__String status_message_;
  std_msgs__msg__String ball_message_;
  std_msgs__msg__String perf_message_;
  std_msgs__msg__UInt8MultiArray jpeg_message_;
  char mode_buffer_[32];
  char status_buffer_[80];
  char ball_buffer_[192];
  char perf_buffer_[256];
  uint8_t jpeg_tx_buffer_[kJpegTxCapacity];

  GazeState received_gaze_;
  float reported_gaze_x_;
  float reported_gaze_y_;
  CameraJpegMailbox* jpeg_mailbox_;
  uint32_t last_gaze_ms_;
  uint32_t last_status_ms_;
  uint32_t last_agent_ping_ms_;
  uint32_t next_entity_retry_ms_;
  uint32_t first_publish_fail_ms_;
  uint8_t publish_fail_count_;
  bool blink_requested_;
  bool transport_configured_;
  bool credentials_valid_;
  MicroRosSessionState session_state_;
  EyeControlMode control_mode_;
  uint8_t entities_init_depth_;
  uint16_t agent_port_;
  char agent_ip_[16];
};
