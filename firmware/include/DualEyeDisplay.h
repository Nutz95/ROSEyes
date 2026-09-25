#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>
#include <stdint.h>

#include "BoardPins.h"
#include "EyeId.h"
#include "EyeMountPolicy.h"
#include "Gc9d01Driver.h"

/**
 * Dual GC9D01 panel manager sharing one SPI bus with independent CS/RST.
 */
class DualEyeDisplay {
 public:
  /** Constructs drivers for left and right panels on FSPI. */
  DualEyeDisplay();

  /** Initializes SPI and both GC9D01 panels. */
  void begin();

  /** Fills one panel with a solid RGB565 color. */
  void fillScreen(EyeId eye, uint16_t color);

  /** Fills a rectangle clipped to the panel. */
  void fillRect(EyeId eye, int16_t x, int16_t y, int16_t width, int16_t height,
                uint16_t color);

  /**
   * Blits a full-screen upright RGB565 buffer to one panel.
   * Applies the per-eye software remap from EyeMountPolicy (MADCTL native).
   */
  void blitRgb565(EyeId eye, const uint16_t* pixels, size_t pixel_count);

 private:
  using SourceIndexFn = size_t (*)(int row, int column, int size);

  Gc9d01Driver& driverFor(EyeId eye);
  void deselectBoth();
  void lockBus();
  void blitWithRemap(Gc9d01Driver& driver, const uint16_t* source,
                     SourceIndexFn index_fn);
  static SourceIndexFn indexFnFor(EyeMountPolicy::PanelTransform transform);
  static size_t indexClockwiseGazeFlipped(int row, int column, int size);
  static size_t indexClockwiseThen180(int row, int column, int size);

  SPIClass spi_;
  Gc9d01Driver left_driver_;
  Gc9d01Driver right_driver_;
  bool bus_locked_;
};
