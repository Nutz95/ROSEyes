#pragma once

#include <stdint.h>

/**
 * Common RGB565 color constants for the GC9D01 panels.
 */
class Rgb565Colors {
 public:
  static constexpr uint16_t kBlack = 0x0000;
  static constexpr uint16_t kWhite = 0xFFFF;
  static constexpr uint16_t kRed = 0xF800;
  static constexpr uint16_t kGreen = 0x07E0;
  static constexpr uint16_t kBlue = 0x001F;
  static constexpr uint16_t kCyan = 0x07FF;
  static constexpr uint16_t kYellow = 0xFFE0;
  static constexpr uint16_t kMagenta = 0xF81F;
  static constexpr uint16_t kOrange = 0xFD20;
};
