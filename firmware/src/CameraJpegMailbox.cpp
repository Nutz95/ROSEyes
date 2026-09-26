#include "CameraJpegMailbox.h"

#include <string.h>

CameraJpegMailbox::CameraJpegMailbox()
    : capture_requested_(false), jpeg_ready_(false), jpeg_length_(0) {
  memset(jpeg_buffer_, 0, sizeof(jpeg_buffer_));
}

void CameraJpegMailbox::requestCapture() {
  capture_requested_ = true;
}

bool CameraJpegMailbox::consumeCaptureRequest() {
  if (!capture_requested_) {
    return false;
  }
  capture_requested_ = false;
  return true;
}

bool CameraJpegMailbox::publishJpeg(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > sizeof(jpeg_buffer_)) {
    return false;
  }
  jpeg_ready_ = false;
  memcpy(jpeg_buffer_, data, length);
  jpeg_length_ = length;
  jpeg_ready_ = true;
  return true;
}

size_t CameraJpegMailbox::takeJpeg(uint8_t* destination, size_t capacity) {
  if (!jpeg_ready_ || destination == nullptr || capacity == 0) {
    return 0;
  }
  const size_t length = jpeg_length_;
  if (length > capacity) {
    jpeg_ready_ = false;
    jpeg_length_ = 0;
    return 0;
  }
  memcpy(destination, jpeg_buffer_, length);
  jpeg_ready_ = false;
  jpeg_length_ = 0;
  return length;
}

bool CameraJpegMailbox::hasJpeg() const {
  return jpeg_ready_;
}
