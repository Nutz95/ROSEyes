#include "GazeSource.h"

GazeSource::GazeSource(IdleEyeBehavior& idle_behavior,
                       const IFreshGazeProvider* external_provider)
    : idle_behavior_(idle_behavior), external_provider_(external_provider) {}

void GazeSource::selectGaze(uint32_t now_ms, GazeState& destination) {
  if (external_provider_ != nullptr &&
      external_provider_->hasFreshGaze(now_ms)) {
    external_provider_->copyGaze(destination);
    return;
  }
  idle_behavior_.update(destination);
}
