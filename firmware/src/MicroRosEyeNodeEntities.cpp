#include "MicroRosEyeNode.h"

#include "FeatureFlags.h"

#if ROSEYES_ENABLE_MICROROS

#include <string.h>

bool MicroRosEyeNode::createEntities() {
  if (session_state_ == MicroRosSessionState::Ready &&
      entities_init_depth_ == kStepSnapCallback) {
    return true;
  }
  destroyEntities();
  entities_init_depth_ = kStepNone;

  if (rclc_support_init(&support_, 0, nullptr, &allocator_) != RCL_RET_OK) {
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
  mode_message_.data.data = mode_buffer_;
  mode_message_.data.size = 0;
  mode_message_.data.capacity = sizeof(mode_buffer_);
  if (rclc_subscription_init_default(
          &mode_subscription_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
          "eyes/mode") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepModeSub;
  if (rclc_subscription_init_default(
          &snap_subscription_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Empty),
          "eyes/camera/snap") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepSnapSub;
  if (rclc_publisher_init_default(
          &status_publisher_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
          "eyes/status") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepStatusPub;
  if (rclc_publisher_init_default(
          &ball_publisher_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
          "eyes/ball") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepBallPub;
  if (rclc_publisher_init_default(
          &jpeg_publisher_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8MultiArray),
          "eyes/camera/jpeg") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepJpegPub;
  if (rclc_publisher_init_default(
          &perf_publisher_, &node_,
          ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
          "eyes/perf") != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepPerfPub;
  if (rclc_executor_init(&executor_, &support_.context, 4, &allocator_) !=
      RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepExecutor;
  if (rclc_executor_add_subscription(&executor_, &gaze_subscription_,
                                     &gaze_message_, &onGazeMessage,
                                     ON_NEW_DATA) != RCL_RET_OK ||
      rclc_executor_add_subscription(&executor_, &blink_subscription_,
                                     &blink_message_, &onBlinkMessage,
                                     ON_NEW_DATA) != RCL_RET_OK ||
      rclc_executor_add_subscription(&executor_, &mode_subscription_,
                                     &mode_message_, &onModeMessage,
                                     ON_NEW_DATA) != RCL_RET_OK ||
      rclc_executor_add_subscription(&executor_, &snap_subscription_,
                                     &snap_message_, &onSnapMessage,
                                     ON_NEW_DATA) != RCL_RET_OK) {
    destroyEntities();
    return false;
  }
  entities_init_depth_ = kStepSnapCallback;
  memset(&status_message_, 0, sizeof(status_message_));
  memset(&ball_message_, 0, sizeof(ball_message_));
  memset(&perf_message_, 0, sizeof(perf_message_));
  memset(&jpeg_message_, 0, sizeof(jpeg_message_));
  publish_fail_count_ = 0;
  first_publish_fail_ms_ = 0;
  return true;
}

void MicroRosEyeNode::destroyEntities() {
  if (entities_init_depth_ >= kStepExecutor) {
    rclc_executor_fini(&executor_);
  }
  if (entities_init_depth_ >= kStepPerfPub) {
    (void)rcl_publisher_fini(&perf_publisher_, &node_);
  }
  if (entities_init_depth_ >= kStepJpegPub) {
    (void)rcl_publisher_fini(&jpeg_publisher_, &node_);
  }
  if (entities_init_depth_ >= kStepBallPub) {
    (void)rcl_publisher_fini(&ball_publisher_, &node_);
  }
  if (entities_init_depth_ >= kStepStatusPub) {
    (void)rcl_publisher_fini(&status_publisher_, &node_);
  }
  if (entities_init_depth_ >= kStepSnapSub) {
    (void)rcl_subscription_fini(&snap_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepModeSub) {
    (void)rcl_subscription_fini(&mode_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepBlinkSub) {
    (void)rcl_subscription_fini(&blink_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepGazeSub) {
    (void)rcl_subscription_fini(&gaze_subscription_, &node_);
  }
  if (entities_init_depth_ >= kStepNode) {
    (void)rcl_node_fini(&node_);
  }
  if (entities_init_depth_ >= kStepSupport) {
    rclc_support_fini(&support_);
  }
  entities_init_depth_ = kStepNone;
}

#endif
