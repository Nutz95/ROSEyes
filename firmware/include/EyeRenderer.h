#pragma once

#include <stdint.h>

#include "DualEyeDisplay.h"
#include "EyeFrameBuffer.h"
#include "GazeState.h"

/**
 * Composes synchronized eyes (procedural / robot bitmap / Uncanny) then blits.
 * Active pack is selected in EyeModelConfig.h.
 * Per-eye gaze flip and panel remap live in EyeMountPolicy.
 */
class EyeRenderer {
 public:
  static constexpr int kMaxGazePixelOffset = 16;
  static constexpr int kIrisRadius = 44;
  static constexpr int kPupilRadius = 18;
  static constexpr int kHighlightRadius = 6;
  static constexpr int kHighlightOffsetX = -10;
  static constexpr int kHighlightOffsetY = -12;
  /** UncannyEyes lid map: 0 open .. 254 fully closed (contract of upper/lower). */
  static constexpr uint8_t kUncannyLidClosedThreshold = 254;
  /** UncannyEyes mid pupil size (iris polar scale divisor companion). */
  static constexpr uint32_t kUncannyIrisScale = 128;

  /** Binds rendering to the dual display. */
  explicit EyeRenderer(DualEyeDisplay& display);

  /** Allocates the offscreen buffer (PSRAM preferred). */
  bool begin();

  /**
   * Draws both eyes with shared lid closure.
   * Mount policy (EyeMountPolicy) negates right-eye horizontal gaze and
   * chooses the per-panel blit remap.
   * @param gaze normalized look direction
   * @param iris_color RGB565 iris fill (procedural pack only)
   * @param lid_closure 0 open .. 1 closed (top-to-bottom lids)
   */
  void drawEyes(const GazeState& gaze, uint16_t iris_color, float lid_closure);

 private:
  /** Composes the active eye pack into the offscreen buffer. */
  void composeFrame(const GazeState& gaze, uint16_t iris_color,
                    float lid_closure);
  void composeProcedural(const GazeState& gaze, uint16_t iris_color,
                         float lid_closure);
  void composeRobotBitmap(const GazeState& gaze, float lid_closure);
  void composeUncanny(const GazeState& gaze, float lid_closure);

  DualEyeDisplay& display_;
  EyeFrameBuffer frame_buffer_;
};
