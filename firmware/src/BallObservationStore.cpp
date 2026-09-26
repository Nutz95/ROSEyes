#include "BallObservationStore.h"

BallObservationStore::BallObservationStore() : seq_(0) {
  observation_.found = false;
  observation_.x = 0.0f;
  observation_.y = 0.0f;
  observation_.diameter = 0.0f;
  observation_.fps = 0.0f;
  observation_.timestamp_ms = 0;
  observation_.seq = 0;
  observation_.color = BallColor::None;
  observation_.framesize_code = 0;
}

void BallObservationStore::publish(const BallObservation& observation) {
  ++seq_;
  observation_ = observation;
  observation_.seq = seq_;
  ++seq_;
}

BallObservation BallObservationStore::snapshot() const {
  BallObservation copy;
  for (;;) {
    const uint32_t before = seq_;
    if ((before & 1u) != 0u) {
      continue;
    }
    copy = observation_;
    const uint32_t after = seq_;
    if (before == after) {
      return copy;
    }
  }
}
