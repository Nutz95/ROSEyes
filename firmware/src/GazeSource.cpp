#include "GazeSource.h"

GazeSource::GazeSource(IdleEyeBehavior& idle_behavior,
                       const IFreshGazeProvider* external_provider)
    : idle_behavior_(idle_behavior),
      external_provider_(external_provider),
      was_using_external_(false) {}

void GazeSource::selectGaze(uint32_t now_ms, GazeState& destination) {
  if (external_provider_ != nullptr &&
      external_provider_->hasFreshGaze(now_ms)) {
    external_provider_->copyGaze(destination);
    was_using_external_ = true;
    return;
  }
  if (was_using_external_) {
    // Resume idle from the last ROS pose instead of an old mid-saccade point.
    idle_behavior_.reset();
    was_using_external_ = false;
  }
  idle_behavior_.update(destination);
}
