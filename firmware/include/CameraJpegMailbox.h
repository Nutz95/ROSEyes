#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * One-shot JPEG mailbox: core1 encodes, core0 publishes over micro-ROS.
 */
class CameraJpegMailbox {
 public:
  /** Max encoded JPEG bytes (also MicroRosEyeNode TX capacity). */
  static constexpr size_t kMaxBytes = 12288;

  /** Constructs an empty mailbox. */
  CameraJpegMailbox();

  /** Host/ROS asked for a snapshot (sticky until consumed by vision). */
  void requestCapture();

  /** True when vision should grab+encode a JPEG. */
  bool consumeCaptureRequest();

  /**
   * Vision writes an encoded JPEG (copied into the internal buffer).
   * @return false if too large
   */
  bool publishJpeg(const uint8_t* data, size_t length);

  /**
   * Core0 copies a pending JPEG if present and clears it.
   * @return length copied, or 0 if none
   */
  size_t takeJpeg(uint8_t* destination, size_t capacity);

  /** True when a JPEG is waiting to be published. */
  bool hasJpeg() const;

 private:
  volatile bool capture_requested_;
  volatile bool jpeg_ready_;
  volatile size_t jpeg_length_;
  uint8_t jpeg_buffer_[kMaxBytes];
};
