#pragma once

#include "BallObservation.h"

/**
 * Seqlocked observation mailbox (1 writer core1 / 1 reader core0).
 */
class BallObservationStore {
 public:
  /** Constructs an empty (not found) observation. */
  BallObservationStore();

  /** Writer: publish a new snapshot (core1). */
  void publish(const BallObservation& observation);

  /** Reader: copy the latest consistent snapshot (core0). */
  BallObservation snapshot() const;

 private:
  volatile uint32_t seq_;
  BallObservation observation_;
};
