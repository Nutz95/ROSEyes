#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Tunables for Sense OV2640 ball tracking (thresholds / framesize policy).
 * Camera DVP pins live in BoardPins; JPEG mailbox size in CameraJpegMailbox.
 */
class BallVisionConfig {
 public:
  static constexpr int kFrameWidth = 320;
  static constexpr int kFrameHeight = 240;
  static constexpr uint32_t kTargetFps = 20;
  static constexpr uint32_t kGazeTimeoutMs = 2500;
  static constexpr float kGazeDeadzone = 0.08f;
  /** ESP cam RGB565 is typically byte-swapped vs native uint16. */
  static constexpr bool kSwapRgb565Bytes = true;
  /** Detect every Nth pixel — QVGA uses 1 (far/small blobs); QQVGA uses 2. */
  static constexpr int kDetectStepQvga = 1;
  static constexpr int kDetectStepQqvga = 2;

  /** Min blob area in pixels (scaled by 1/step^2 in detector). */
  static constexpr int kMinAreaQvga = 18;
  static constexpr int kMinAreaQqvga = 48;
  static constexpr int kMinWidthQvga = 3;
  static constexpr int kMinHeightQvga = 3;
  static constexpr int kMinWidthQqvga = 6;
  static constexpr int kMinHeightQqvga = 6;
  static constexpr float kMinAspect = 0.55f;
  static constexpr float kMaxAspect = 1.80f;
  /** Compact balls only — rejects hand+ball / lamp bloom unions. */
  static constexpr float kMinFillRatioQvga = 0.22f;
  static constexpr float kMinFillRatioQqvga = 0.25f;
  /** Reject ceiling-lamp sized “blobs”. */
  static constexpr float kMaxDiameter = 0.42f;

  /** Saturated red (5-bit R/B, 6-bit G halved). Caps reject yellow lamps. */
  static constexpr int kRedRgbMin = 8;
  static constexpr int kRedRgbDominate = 4;
  static constexpr int kRedMaxGreen5 = 12;
  static constexpr int kRedMaxBlue = 10;
  /** Saturated green; caps reject yellow (high R) lamps. */
  static constexpr int kGreenRgbMin = 8;
  static constexpr int kGreenRgbDominate = 4;
  static constexpr int kGreenMaxRed = 12;
  static constexpr int kGreenMaxBlue = 12;
  /** If preferred misses, scan the other color. */
  static constexpr bool kEnableGreenFallback = true;

  /**
   * Near-ball QQVGA downscale for FPS. Off by default: QVGA alone is ~8–9 fps
   * after the fast path; set true to re-enable diameter hysteresis.
   */
  static constexpr bool kEnableDynamicQqvga = false;
  static constexpr float kNearDiameterEnter = 0.40f;
  static constexpr float kNearDiameterExit = 0.22f;
  static constexpr uint8_t kFramesizeQvga = 0;
  static constexpr uint8_t kFramesizeQqvga = 1;

  static constexpr int kJpegQuality = 12;
};
