#include "BallTelemetryFormatter.h"

#include <stdio.h>

const char* BallTelemetryFormatter::colorName(BallColor color) {
  switch (color) {
    case BallColor::Red:
      return "red";
    case BallColor::Green:
      return "green";
    case BallColor::None:
    default:
      return "none";
  }
}

const char* BallTelemetryFormatter::framesizeName(uint8_t code) {
  switch (code) {
    case 1:
      return "qqvga";
    case 0:
    default:
      return "qvga";
  }
}

int BallTelemetryFormatter::format(const BallObservation& observation,
                                   char* buffer, int capacity) {
  if (buffer == nullptr || capacity < 8) {
    return -1;
  }
  const int written = snprintf(
      buffer, static_cast<size_t>(capacity),
      "{\"found\":%s,\"x\":%.3f,\"y\":%.3f,\"diameter\":%.3f,\"fps\":%.1f,"
      "\"framesize\":\"%s\",\"color\":\"%s\",\"seq\":%lu}",
      observation.found ? "true" : "false", observation.x, observation.y,
      observation.diameter, observation.fps,
      framesizeName(observation.framesize_code), colorName(observation.color),
      static_cast<unsigned long>(observation.seq));
  if (written < 0 || written >= capacity) {
    return -1;
  }
  return written;
}
