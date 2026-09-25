#pragma once

#include <stdint.h>

#include "GazeState.h"

/**
 * Optional external gaze feed (e.g. micro-ROS) with freshness semantics.
 */
class IFreshGazeProvider {
 public:
  /** Virtual destructor for polymorphic providers. */
  virtual ~IFreshGazeProvider() = default;

  /** True when a gaze sample is still within the freshness window. */
  virtual bool hasFreshGaze(uint32_t now_ms) const = 0;

  /** Copies the latest received gaze into destination. */
  virtual void copyGaze(GazeState& destination) const = 0;
};
