#include "EyeFrameBuffer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO) && defined(BOARD_HAS_PSRAM)
#include <esp_heap_caps.h>
#endif

namespace {

void fillRow(uint16_t* row, int width, uint16_t color) {
  for (int column = 0; column < width; ++column) {
    row[column] = color;
  }
}

}  // namespace

EyeFrameBuffer::EyeFrameBuffer() : pixels_(nullptr), ready_(false) {}

EyeFrameBuffer::~EyeFrameBuffer() {
  if (pixels_ != nullptr) {
    free(pixels_);
    pixels_ = nullptr;
  }
}

bool EyeFrameBuffer::begin() {
  if (ready_) {
    return true;
  }
#if defined(ARDUINO) && defined(BOARD_HAS_PSRAM)
  pixels_ = static_cast<uint16_t*>(heap_caps_malloc(
      kPixelCount * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#endif
  if (pixels_ == nullptr) {
    pixels_ = static_cast<uint16_t*>(malloc(kPixelCount * sizeof(uint16_t)));
  }
  ready_ = pixels_ != nullptr;
  return ready_;
}

void EyeFrameBuffer::fill(uint16_t color) {
  if (!ready_) {
    return;
  }
  const uint8_t low = static_cast<uint8_t>(color & 0xFF);
  const uint8_t high = static_cast<uint8_t>(color >> 8);
  if (low == high) {
    memset(pixels_, static_cast<int>(low), kPixelCount * sizeof(uint16_t));
    return;
  }
  fillRow(pixels_, kWidth, color);
  for (int row = 1; row < kHeight; ++row) {
    memcpy(pixels_ + static_cast<size_t>(row) * kWidth, pixels_,
           static_cast<size_t>(kWidth) * sizeof(uint16_t));
  }
}

void EyeFrameBuffer::fillRect(int x, int y, int width, int height,
                              uint16_t color) {
  if (!ready_ || width <= 0 || height <= 0) {
    return;
  }
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (x + width > kWidth) {
    width = kWidth - x;
  }
  if (y + height > kHeight) {
    height = kHeight - y;
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  uint16_t* first_row =
      pixels_ + static_cast<size_t>(y) * kWidth + static_cast<size_t>(x);
  fillRow(first_row, width, color);
  const size_t row_bytes = static_cast<size_t>(width) * sizeof(uint16_t);
  for (int row = 1; row < height; ++row) {
    memcpy(pixels_ + static_cast<size_t>(y + row) * kWidth +
               static_cast<size_t>(x),
           first_row, row_bytes);
  }
}

void EyeFrameBuffer::fillCircle(int center_x, int center_y, int radius,
                                uint16_t color) {
  for (int delta_y = -radius; delta_y <= radius; ++delta_y) {
    const int delta_x = static_cast<int>(
        sqrtf(static_cast<float>(radius * radius - delta_y * delta_y)));
    fillRect(center_x - delta_x, center_y + delta_y, 2 * delta_x + 1, 1, color);
  }
}

void EyeFrameBuffer::applyLidClosure(float closure) {
  if (!ready_) {
    return;
  }
  if (closure < 0.0f) {
    closure = 0.0f;
  }
  if (closure > 1.0f) {
    closure = 1.0f;
  }
  if (closure <= kLidClosureEpsilon) {
    return;
  }

  // Match background so blink is a lid wipe, not a flash.
  static constexpr uint16_t kLidColor = 0x0000;

  // Upper lid travels top -> bottom (main blink motion).
  const int upper_cover =
      static_cast<int>(closure * static_cast<float>(kHeight));
  if (upper_cover > 0) {
    fillRect(0, 0, kWidth, upper_cover, kLidColor);
  }

  // Slight lower lid rising for a more natural close.
  const int lower_cover = static_cast<int>(
      closure * kLowerLidFraction * static_cast<float>(kHeight));
  if (lower_cover > 0) {
    fillRect(0, kHeight - lower_cover, kWidth, lower_cover, kLidColor);
  }
}

const uint16_t* EyeFrameBuffer::data() const { return pixels_; }

uint16_t* EyeFrameBuffer::mutableData() { return pixels_; }

bool EyeFrameBuffer::isReady() const { return ready_; }
