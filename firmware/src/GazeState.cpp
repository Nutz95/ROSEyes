#include "GazeState.h"

#include <cmath>

GazeState::GazeState() : x_(0.0f), y_(0.0f) {}

void GazeState::setNormalized(float x, float y) {
  x_ = clampUnit(x);
  y_ = clampUnit(y);
}

float GazeState::x() const { return x_; }

float GazeState::y() const { return y_; }

int GazeState::horizontalPixelOffset(int max_offset) const {
  if (max_offset < 0) {
    max_offset = 0;
  }
  return static_cast<int>(std::lround(x_ * static_cast<float>(max_offset)));
}

int GazeState::verticalPixelOffset(int max_offset) const {
  if (max_offset < 0) {
    max_offset = 0;
  }
  return static_cast<int>(std::lround(y_ * static_cast<float>(max_offset)));
}

float GazeState::clampUnit(float value) {
  if (value < -1.0f) {
    return -1.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}
