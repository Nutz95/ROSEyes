#include "RangeTelemetryFormatter.h"

#include <stdio.h>

int RangeTelemetryFormatter::format(const RangeObservation& observation,
                                    char* buffer, int capacity) {
  if (buffer == nullptr || capacity < 8) {
    return -1;
  }
  const int written = snprintf(
      buffer, static_cast<size_t>(capacity),
      "{\"mm\":%lu,\"ok\":%s,\"status\":%u,\"strength\":%u,\"seq\":%lu}",
      static_cast<unsigned long>(observation.distance_mm),
      observation.valid ? "true" : "false",
      static_cast<unsigned>(observation.status),
      static_cast<unsigned>(observation.signal_strength),
      static_cast<unsigned long>(observation.seq));
  if (written < 0 || written >= capacity) {
    return -1;
  }
  return written;
}
