#pragma once

#include "BallObservationStore.h"
#include "IFreshGazeProvider.h"

/**
 * Adapts BallObservationStore into the same gaze contract as micro-ROS.
 */
class BallGazeProvider : public IFreshGazeProvider {
 public:
  static constexpr uint32_t kFreshTimeoutMs = 2500;

  /** Binds to the shared observation mailbox. */
  explicit BallGazeProvider(const BallObservationStore& store);

  /** True when a ball-derived gaze is still within the freshness window. */
  bool hasFreshGaze(uint32_t now_ms) const override;

  /** Copies the latest ball gaze into destination. */
  void copyGaze(GazeState& destination) const override;

 private:
  const BallObservationStore& store_;
};
