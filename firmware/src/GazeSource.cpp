#include "GazeSource.h"

GazeSource::GazeSource(IdleEyeBehavior& idle_behavior,
                       const IFreshGazeProvider* ros_provider,
                       const IFreshGazeProvider* ball_provider)
    : idle_behavior_(idle_behavior),
      ros_provider_(ros_provider),
      ball_provider_(ball_provider),
      control_mode_(EyeControlMode::Autonomous),
      was_using_external_(false) {}

void GazeSource::setControlMode(EyeControlMode mode) {
  control_mode_ = mode;
}

EyeControlMode GazeSource::controlMode() const {
  return control_mode_;
}

void GazeSource::selectGaze(uint32_t now_ms, GazeState& destination) {
  const IFreshGazeProvider* chosen = nullptr;

  if (control_mode_ == EyeControlMode::Piloted) {
    if (ros_provider_ != nullptr && ros_provider_->hasFreshGaze(now_ms)) {
      chosen = ros_provider_;
    }
  } else if (ball_provider_ != nullptr &&
             ball_provider_->hasFreshGaze(now_ms)) {
    chosen = ball_provider_;
  }

  if (chosen != nullptr) {
    chosen->copyGaze(destination);
    was_using_external_ = true;
    return;
  }

  if (was_using_external_) {
    idle_behavior_.reset();
    was_using_external_ = false;
  }
  idle_behavior_.update(destination);
}
