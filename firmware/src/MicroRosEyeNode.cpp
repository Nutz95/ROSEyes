#include "MicroRosEyeNode.h"

#include "FeatureFlags.h"

#if ROSEYES_ENABLE_MICROROS

#include <Arduino.h>
#include <WiFi.h>

#include <stdio.h>
#include <string.h>

MicroRosEyeNode* MicroRosEyeNode::active_instance_ = nullptr;

namespace {

enum EntityInitStep : uint8_t {
  kStepNone = 0,
  kStepSupport = 1,
  kStepNode = 2,
  kStepGazeSub = 3,
  kStepBlinkSub = 4,
  kStepStatusPub = 5,
  kStepExecutor = 6,
  kStepGazeCallback = 7,
  kStepBlinkCallback = 8,
};

}  // namespace

MicroRosEyeNode::MicroRosEyeNode()
    : allocator_(rcl_get_default_allocator()),
      last_gaze_ms_(0),
      last_status_ms_(0),
      next_entity_retry_ms_(0),
      blink_requested_(false),
      transport_started_(false),
      session_state_(MicroRosSessionState::IdleOnly),
      entities_init_depth_(kStepNone) {
  memset(&support_, 0, sizeof(support_));
  memset(&node_, 0, sizeof(node_));
  memset(&gaze_subscription_, 0, sizeof(gaze_subscription_));
  memset(&blink_subscription_, 0, sizeof(blink_subscription_));
  memset(&status_publisher_, 0, sizeof(status_publisher_));
  memset(&executor_, 0, sizeof(executor_));
  memset(&gaze_message_, 0, sizeof(gaze_message_));
  memset(&blink_message_, 0, sizeof(blink_message_));
  memset(&status_message_, 0, sizeof(status_message_));
  memset(status_buffer_, 0, sizeof(status_buffer_));
  received_gaze_.setNormalized(0.0f, 0.0f);
}

bool MicroRosEyeNode::begin(const NetworkCredentials& credentials) {
  active_instance_ = this;
  session_state_ = MicroRosSessionState::Connecting;

  IPAddress agent_ip;
  if (!agent_ip.fromString(credentials.agent_ip)) {
    Serial.printf("Invalid agent IP: %s\n", credentials.agent_ip);
    session_state_ = MicroRosSessionState::Faulted;
    return false;
  }

  Serial.printf("Connecting WiFi SSID=%s ...\n", credentials.wifi_ssid);
  set_microros_wifi_transports(const_cast<char*>(credentials.wifi_ssid),
                               const_cast<char*>(credentials.wifi_password),
                               agent_ip, credentials.agent_port);
  delay(2000);
  transport_started_ = true;
  allocator_ = rcl_get_default_allocator();

  if (!createEntities()) {
    Serial.println("micro-ROS entity creation failed; will retry in update()");
    next_entity_retry_ms_ = millis() + kEntityRetryIntervalMs;
    return false;
  }

  session_state_ = MicroRosSessionState::Ready;
  Serial.println("micro-ROS entities ready");
  return true;
}

void MicroRosEyeNode::update(uint32_t now_ms) {
  if (!transport_started_) {
    return;
  }

  if (session_state_ != MicroRosSessionState::Ready) {
    if (now_ms < next_entity_retry_ms_) {
      return;
    }
    next_entity_retry_ms_ = now_ms + kEntityRetryIntervalMs;
    if (createEntities()) {
      session_state_ = MicroRosSessionState::Ready;
      Serial.println("micro-ROS entities ready (retry)");
    }
    return;
  }

  rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(10));

  if (now_ms - last_status_ms_ >= kStatusPeriodMs) {
    last_status_ms_ = now_ms;
    snprintf(status_buffer_, sizeof(status_buffer_), "ok gaze=%.2f,%.2f",
             received_gaze_.x(), received_gaze_.y());
    status_message_.data.data = status_buffer_;
    status_message_.data.size = strlen(status_buffer_);
    status_message_.data.capacity = sizeof(status_buffer_);
    rcl_publish(&status_publisher_, &status_message_, nullptr);
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

bool MicroRosEyeNode::createEntities() {
  if (session_state_ == MicroRosSessionState::Ready &&
      entities_init_depth_ == kStepBlinkCallback) {
    return true;
  }

  destroyEntities();
  entities_init_depth_ = kStepNone;

  if (rclc_support_init(&support_, 0, nullptr, &allocator_) != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepSupport;

  if (rclc_node_init_default(&node_, "roseyes_eye_node", "", &support_) !=
      RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepNode;

  if (rclc_subscription_init_default(
          &gaze_subscription_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Vector3),
          "eyes/gaze") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepGazeSub;

  if (rclc_subscription_init_default(
          &blink_subscription_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Empty),
          "eyes/blink") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepBlinkSub;

  if (rclc_publisher_init_default(
          &status_publisher_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
          "eyes/status") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepStatusPub;

  if (rclc_executor_init(&executor_, &support_.context, 2, &allocator_) !=
      RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepExecutor;

  if (rclc_executor_add_subscription(&executor_, &gaze_subscription_,
                                     &gaze_message_, &onGazeMessage,
                                     ON_NEW_DATA) != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepGazeCallback;

  if (rclc_executor_add_subscription(&executor_, &blink_subscription_,
                                     &blink_message_, &onBlinkMessage,
                                     ON_NEW_DATA) != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepBlinkCallback;

  memset(&status_message_, 0, sizeof(status_message_));
  return true;
}

void MicroRosEyeNode::destroyEntities() {
  if (entities_init_depth_ >= kStepBlinkCallback ||
      entities_init_depth_ >= kStepGazeCallback ||
      entities_init_depth_ >= kStepExecutor) {
    rclc_executor_fini(&executor_);
  }
  if (entities_init_depth_ >= kStepStatusPub) {
    rcl_publisher_fini(&status_publisher_, &node_);
  }
  if (entities_init_depth_ >= kStepBlinkSub) {
    rcl_subscription_fini(&blink_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepGazeSub) {
    rcl_subscription_fini(&gaze_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepNode) {
    rcl_node_fini(&node_);
  }
  if (entities_init_depth_ >= kStepSupport) {
    rclc_support_fini(&support_);
  }
  entities_init_depth_ = kStepNone;
  if (session_state_ == MicroRosSessionState::Ready) {
    session_state_ = MicroRosSessionState::Connecting;
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
  if (active_instance_ == nullptr) {
    return;
  }
  active_instance_->blink_requested_ = true;
}

#endif  // ROSEYES_ENABLE_MICROROS
