#include "BallGazeProvider.h"

BallGazeProvider::BallGazeProvider(const BallObservationStore& store)
    : store_(store) {}

bool BallGazeProvider::hasFreshGaze(uint32_t now_ms) const {
  const BallObservation observation = store_.snapshot();
  if (!observation.found || observation.timestamp_ms == 0) {
    return false;
  }
  return (now_ms - observation.timestamp_ms) <= kFreshTimeoutMs;
}

void BallGazeProvider::copyGaze(GazeState& destination) const {
  const BallObservation observation = store_.snapshot();
  destination.setNormalized(observation.x, observation.y);
}
