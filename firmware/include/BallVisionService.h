#pragma once

#include "BallObservationStore.h"
#include "CameraJpegMailbox.h"
#include "FeatureFlags.h"

#if ROSEYES_ENABLE_CAMERA_BALL
#include <esp_camera.h>
#endif

/**
 * OV2640 capture + detect task on FreeRTOS core 1.
 */
class BallVisionService {
 public:
  /**
   * @param store observation mailbox for gaze
   * @param jpeg_mailbox snapshot mailbox for on-demand JPEG
   */
  BallVisionService(BallObservationStore& store,
                    CameraJpegMailbox& jpeg_mailbox);

  /**
   * Initializes the camera (no-op when ROSEYES_ENABLE_CAMERA_BALL=0).
   * @return true when the camera started
   */
  bool begin();

  /** Enables or stops continuous detect (Piloted => false). Snap still works. */
  void setEnabled(bool enabled);

  /** True when continuous detect is enabled. */
  bool isEnabled() const;

 private:
#if ROSEYES_ENABLE_CAMERA_BALL
  static void taskEntry(void* self);
  void taskLoop();
  bool initCamera();
  void encodeSnapshot(camera_fb_t* frame);
  void maybeAdjustFramesize(const BallObservation& observation);
#endif

  BallObservationStore& store_;
  CameraJpegMailbox& jpeg_mailbox_;
  volatile bool enabled_;
  bool started_;
#if ROSEYES_ENABLE_CAMERA_BALL
  uint8_t framesize_code_;
  BallColor sticky_color_;
#endif
};
