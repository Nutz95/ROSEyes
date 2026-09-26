#include "MicroRosEyeNode.h"

#include "FeatureFlags.h"

#if ROSEYES_ENABLE_MICROROS

#include <Arduino.h>
#include <WiFi.h>

#include <stdio.h>
#include <string.h>

#include "EyeControlModeParser.h"

MicroRosEyeNode* MicroRosEyeNode::active_instance_ = nullptr;

MicroRosEyeNode::MicroRosEyeNode()
    : allocator_(rcl_get_default_allocator()),
      last_gaze_ms_(0),
      last_status_ms_(0),
      last_agent_ping_ms_(0),
      next_entity_retry_ms_(0),
      first_publish_fail_ms_(0),
      publish_fail_count_(0),
      blink_requested_(false),
      transport_configured_(false),
      credentials_valid_(false),
      session_state_(MicroRosSessionState::IdleOnly),
      control_mode_(EyeControlMode::Autonomous),
      entities_init_depth_(kStepNone),
      agent_port_(8888),
      reported_gaze_x_(0.0f),
      reported_gaze_y_(0.0f),
      jpeg_mailbox_(nullptr) {
  memset(&support_, 0, sizeof(support_));
  memset(&node_, 0, sizeof(node_));
  memset(&gaze_subscription_, 0, sizeof(gaze_subscription_));
  memset(&blink_subscription_, 0, sizeof(blink_subscription_));
  memset(&mode_subscription_, 0, sizeof(mode_subscription_));
  memset(&snap_subscription_, 0, sizeof(snap_subscription_));
  memset(&status_publisher_, 0, sizeof(status_publisher_));
  memset(&ball_publisher_, 0, sizeof(ball_publisher_));
  memset(&jpeg_publisher_, 0, sizeof(jpeg_publisher_));
  memset(&perf_publisher_, 0, sizeof(perf_publisher_));
  memset(&executor_, 0, sizeof(executor_));
  memset(&gaze_message_, 0, sizeof(gaze_message_));
  memset(&blink_message_, 0, sizeof(blink_message_));
  memset(&snap_message_, 0, sizeof(snap_message_));
  memset(&mode_message_, 0, sizeof(mode_message_));
  memset(&status_message_, 0, sizeof(status_message_));
  memset(&ball_message_, 0, sizeof(ball_message_));
  memset(&perf_message_, 0, sizeof(perf_message_));
  memset(&jpeg_message_, 0, sizeof(jpeg_message_));
  memset(mode_buffer_, 0, sizeof(mode_buffer_));
  memset(status_buffer_, 0, sizeof(status_buffer_));
  memset(ball_buffer_, 0, sizeof(ball_buffer_));
  memset(perf_buffer_, 0, sizeof(perf_buffer_));
  memset(jpeg_tx_buffer_, 0, sizeof(jpeg_tx_buffer_));
  agent_ip_[0] = '\0';
  received_gaze_.setNormalized(0.0f, 0.0f);
  mode_message_.data.data = mode_buffer_;
  mode_message_.data.capacity = sizeof(mode_buffer_);
}

bool MicroRosEyeNode::begin(const NetworkCredentials& credentials) {
  active_instance_ = this;
  transport_configured_ = false;
  credentials_valid_ = false;
  control_mode_ = EyeControlMode::Autonomous;
  session_state_ = MicroRosSessionState::Connecting;

  IPAddress parsed_ip;
  if (!parsed_ip.fromString(credentials.agent_ip)) {
    Serial.printf("Invalid agent IP: %s\n", credentials.agent_ip);
    session_state_ = MicroRosSessionState::Faulted;
    return false;
  }

  strncpy(agent_ip_, credentials.agent_ip, sizeof(agent_ip_) - 1);
  agent_ip_[sizeof(agent_ip_) - 1] = '\0';
  agent_port_ = credentials.agent_port;
  credentials_valid_ = true;
  allocator_ = rcl_get_default_allocator();
  next_entity_retry_ms_ = 0;
  Serial.printf("micro-ROS: async connect %s:%u\n", agent_ip_, agent_port_);
  return true;
}

bool MicroRosEyeNode::ensureTransportConfigured() {
  if (transport_configured_) {
    return true;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  static micro_ros_agent_locator locator;
  if (!locator.address.fromString(agent_ip_)) {
    Serial.printf("micro-ROS: bad agent IP %s\n", agent_ip_);
    return false;
  }
  locator.port = static_cast<int>(agent_port_);
  rmw_uros_set_custom_transport(
      false, static_cast<void*>(&locator), platformio_transport_open,
      platformio_transport_close, platformio_transport_write,
      platformio_transport_read);
  transport_configured_ = true;
  Serial.println("micro-ROS: UDP transport ready");
  return true;
}

void MicroRosEyeNode::forceAutonomousMode() {
  if (control_mode_ != EyeControlMode::Autonomous) {
    control_mode_ = EyeControlMode::Autonomous;
    Serial.println("mode -> autonomous (session lost)");
  }
}

void MicroRosEyeNode::dropSessionToConnecting(const char* reason) {
  destroyEntities();
  session_state_ = MicroRosSessionState::Connecting;
  forceAutonomousMode();
  next_entity_retry_ms_ = 0;
  publish_fail_count_ = 0;
  first_publish_fail_ms_ = 0;
  if (reason != nullptr) {
    Serial.printf("micro-ROS: %s; will reconnect\n", reason);
  }
}

void MicroRosEyeNode::update(uint32_t now_ms) {
  if (!credentials_valid_ ||
      session_state_ == MicroRosSessionState::Faulted ||
      session_state_ == MicroRosSessionState::IdleOnly) {
    forceAutonomousMode();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (session_state_ == MicroRosSessionState::Ready) {
      dropSessionToConnecting("WiFi lost");
    }
    return;
  }

  if (!ensureTransportConfigured()) {
    return;
  }

  if (session_state_ != MicroRosSessionState::Ready) {
    if (now_ms < next_entity_retry_ms_) {
      return;
    }
    next_entity_retry_ms_ = now_ms + kEntityRetryIntervalMs;
    if (rmw_uros_ping_agent(kAgentPingTimeoutMs, kAgentPingAttempts) !=
        RMW_RET_OK) {
      return;
    }
    if (createEntities()) {
      session_state_ = MicroRosSessionState::Ready;
      last_agent_ping_ms_ = now_ms;
      Serial.println("micro-ROS entities ready");
    } else {
      Serial.println("micro-ROS entity create failed; retry later");
    }
    return;
  }

  if ((now_ms - last_agent_ping_ms_) >= kAgentAlivePingMs) {
    last_agent_ping_ms_ = now_ms;
    if (rmw_uros_ping_agent(kAgentPingTimeoutMs, kAgentPingAttempts) !=
        RMW_RET_OK) {
      dropSessionToConnecting("agent ping failed");
      return;
    }
  }

  rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(5));
  publishPendingJpeg();

  if (now_ms - last_status_ms_ < kStatusPeriodMs) {
    return;
  }
  last_status_ms_ = now_ms;
  snprintf(status_buffer_, sizeof(status_buffer_), "ok mode=%s gaze=%.2f,%.2f",
           EyeControlModeParser::toCString(control_mode_), reported_gaze_x_,
           reported_gaze_y_);
  status_message_.data.data = status_buffer_;
  status_message_.data.size = strlen(status_buffer_);
  status_message_.data.capacity = sizeof(status_buffer_);
  if (rcl_publish(&status_publisher_, &status_message_, nullptr) ==
      RCL_RET_OK) {
    publish_fail_count_ = 0;
    first_publish_fail_ms_ = 0;
    return;
  }
  if (publish_fail_count_ == 0) {
    first_publish_fail_ms_ = now_ms;
  }
  if (publish_fail_count_ < 255) {
    ++publish_fail_count_;
  }
  if (publish_fail_count_ >= kPublishFailLimit &&
      (now_ms - first_publish_fail_ms_) >= kPublishFailGraceMs) {
    dropSessionToConnecting("status publish failed");
  }
}

bool MicroRosEyeNode::hasFreshGaze(uint32_t now_ms) const {
  if (session_state_ != MicroRosSessionState::Ready || last_gaze_ms_ == 0) {
    return false;
  }
  return (now_ms - last_gaze_ms_) <= kGazeTimeoutMs;
}

void MicroRosEyeNode::copyGaze(GazeState& destination) const {
  destination.setNormalized(received_gaze_.x(), received_gaze_.y());
}

bool MicroRosEyeNode::consumeBlinkRequest() {
  if (!blink_requested_) {
    return false;
  }
  blink_requested_ = false;
  return true;
}

MicroRosSessionState MicroRosEyeNode::sessionState() const {
  return session_state_;
}

EyeControlMode MicroRosEyeNode::controlMode() const {
  if (session_state_ != MicroRosSessionState::Ready) {
    return EyeControlMode::Autonomous;
  }
  return control_mode_;
}

bool MicroRosEyeNode::tryPublishString(rcl_publisher_t& publisher,
                                       std_msgs__msg__String& message,
                                       char* buffer, size_t capacity,
                                       const char* json) {
  if (session_state_ != MicroRosSessionState::Ready || json == nullptr ||
      buffer == nullptr || capacity == 0) {
    return false;
  }
  const size_t len = strlen(json);
  if (len >= capacity) {
    return false;
  }
  memcpy(buffer, json, len + 1);
  message.data.data = buffer;
  message.data.size = len;
  message.data.capacity = capacity;
  return rcl_publish(&publisher, &message, nullptr) == RCL_RET_OK;
}

bool MicroRosEyeNode::tryPublishBallJson(const char* json) {
  return tryPublishString(ball_publisher_, ball_message_, ball_buffer_,
                          sizeof(ball_buffer_), json);
}

bool MicroRosEyeNode::tryPublishPerfJson(const char* json) {
  return tryPublishString(perf_publisher_, perf_message_, perf_buffer_,
                          sizeof(perf_buffer_), json);
}

void MicroRosEyeNode::setReportedGaze(float x, float y) {
  reported_gaze_x_ = x;
  reported_gaze_y_ = y;
}

void MicroRosEyeNode::setJpegMailbox(CameraJpegMailbox* mailbox) {
  jpeg_mailbox_ = mailbox;
}

void MicroRosEyeNode::publishPendingJpeg() {
  if (session_state_ != MicroRosSessionState::Ready || jpeg_mailbox_ == nullptr ||
      !jpeg_mailbox_->hasJpeg()) {
    return;
  }
  const size_t length =
      jpeg_mailbox_->takeJpeg(jpeg_tx_buffer_, sizeof(jpeg_tx_buffer_));
  if (length == 0) {
    return;
  }
  jpeg_message_.layout.dim.data = nullptr;
  jpeg_message_.layout.dim.size = 0;
  jpeg_message_.layout.dim.capacity = 0;
  jpeg_message_.layout.data_offset = 0;
  jpeg_message_.data.data = jpeg_tx_buffer_;
  jpeg_message_.data.size = length;
  jpeg_message_.data.capacity = sizeof(jpeg_tx_buffer_);
  if (rcl_publish(&jpeg_publisher_, &jpeg_message_, nullptr) != RCL_RET_OK) {
    Serial.println("micro-ROS: jpeg publish failed");
  } else {
    Serial.printf("micro-ROS: jpeg published (%u bytes)\n",
                  static_cast<unsigned>(length));
  }
}

void MicroRosEyeNode::onGazeMessage(const void* message_void) {
  if (active_instance_ == nullptr || message_void == nullptr) {
    return;
  }
  const auto* message =
      static_cast<const geometry_msgs__msg__Vector3*>(message_void);
  active_instance_->received_gaze_.setNormalized(
      static_cast<float>(message->x), static_cast<float>(message->y));
  active_instance_->last_gaze_ms_ = ::millis();
}

void MicroRosEyeNode::onBlinkMessage(const void* /*message_void*/) {
  if (active_instance_ != nullptr) {
    active_instance_->blink_requested_ = true;
  }
}

void MicroRosEyeNode::onModeMessage(const void* message_void) {
  if (active_instance_ == nullptr || message_void == nullptr) {
    return;
  }
  const auto* message =
      static_cast<const std_msgs__msg__String*>(message_void);
  if (message->data.data == nullptr || message->data.size == 0) {
    return;
  }
  char token[32];
  const size_t copy_len = message->data.size < sizeof(token) - 1
                              ? message->data.size
                              : sizeof(token) - 1;
  memcpy(token, message->data.data, copy_len);
  token[copy_len] = '\0';
  EyeControlMode parsed = EyeControlMode::Autonomous;
  if (!EyeControlModeParser::tryParse(token, parsed)) {
    Serial.printf("mode ignored: '%s'\n", token);
    return;
  }
  if (active_instance_->control_mode_ != parsed) {
    active_instance_->control_mode_ = parsed;
    Serial.printf("mode -> %s\n", EyeControlModeParser::toCString(parsed));
  }
}

void MicroRosEyeNode::onSnapMessage(const void* /*message_void*/) {
  if (active_instance_ == nullptr || active_instance_->jpeg_mailbox_ == nullptr) {
    return;
  }
  active_instance_->jpeg_mailbox_->requestCapture();
  Serial.println("camera snap requested");
}

#endif  // ROSEYES_ENABLE_MICROROS
