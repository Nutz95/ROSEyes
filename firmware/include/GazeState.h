#pragma once

/**
 * Holds normalized gaze coordinates and maps them to pixel offsets.
 *
 * Coordinates use the ROS contract: x/y in [-1, 1].
 * Positive x looks right; positive y looks down (display space).
 */
class GazeState {
 public:
  /** Creates a centered gaze (0, 0). */
  GazeState();

  /** Clamps and stores normalized gaze in [-1, 1]. */
  void setNormalized(float x, float y);

  /** Returns the current normalized horizontal gaze. */
  float x() const;

  /** Returns the current normalized vertical gaze. */
  float y() const;

  /** Maps normalized x to a horizontal pixel offset within max_offset. */
  int horizontalPixelOffset(int max_offset) const;

  /** Maps normalized y to a vertical pixel offset within max_offset. */
  int verticalPixelOffset(int max_offset) const;

 private:
  static float clampUnit(float value);

  float x_;
  float y_;
};
