#include "DualEyeDisplay.h"

DualEyeDisplay::DualEyeDisplay()
    : spi_(FSPI),
      left_driver_(spi_, BoardPins::kDataCommandPin, BoardPins::kResetLeftPin,
                   BoardPins::kChipSelectLeftPin),
      right_driver_(spi_, BoardPins::kDataCommandPin, BoardPins::kResetRightPin,
                    BoardPins::kChipSelectRightPin),
      bus_locked_(false) {}

void DualEyeDisplay::begin() {
  left_driver_.beginPins();
  right_driver_.beginPins();
  deselectBoth();

  spi_.begin(BoardPins::kSpiClockPin, -1, BoardPins::kSpiMosiPin, -1);
  lockBus();

  left_driver_.initialize();
  right_driver_.initialize();
  // Native MADCTL — sharp scan. Orientation fixed in blitRgb565.
  left_driver_.setMadctl(BoardPins::kMadctlNative);
  right_driver_.setMadctl(BoardPins::kMadctlNative);
}

void DualEyeDisplay::fillScreen(EyeId eye, uint16_t color) {
  fillRect(eye, 0, 0, BoardPins::kDisplayWidthPixels,
           BoardPins::kDisplayHeightPixels, color);
}

void DualEyeDisplay::fillRect(EyeId eye, int16_t x, int16_t y, int16_t width,
                              int16_t height, uint16_t color) {
  if (width <= 0 || height <= 0) {
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
  if (x + width > BoardPins::kDisplayWidthPixels) {
    width = static_cast<int16_t>(BoardPins::kDisplayWidthPixels - x);
  }
  if (y + height > BoardPins::kDisplayHeightPixels) {
    height = static_cast<int16_t>(BoardPins::kDisplayHeightPixels - y);
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  lockBus();
  Gc9d01Driver& driver = driverFor(eye);
  driver.setAddressWindow(
      static_cast<uint16_t>(x), static_cast<uint16_t>(y),
      static_cast<uint16_t>(x + width - 1),
      static_cast<uint16_t>(y + height - 1));
  const uint32_t pixel_count =
      static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
  driver.writePixels(color, pixel_count);
  deselectBoth();
}

void DualEyeDisplay::blitRgb565(EyeId eye, const uint16_t* pixels,
                                size_t pixel_count) {
  if (pixels == nullptr ||
      pixel_count < static_cast<size_t>(BoardPins::kDisplayWidthPixels) *
                        static_cast<size_t>(BoardPins::kDisplayHeightPixels)) {
    return;
  }
  lockBus();
  Gc9d01Driver& driver = driverFor(eye);
  driver.setAddressWindow(
      0, 0,
      static_cast<uint16_t>(BoardPins::kDisplayWidthPixels - 1),
      static_cast<uint16_t>(BoardPins::kDisplayHeightPixels - 1));
  blitWithRemap(driver, pixels,
                indexFnFor(EyeMountPolicy::panelTransform(eye)));
  deselectBoth();
}

void DualEyeDisplay::blitWithRemap(Gc9d01Driver& driver,
                                   const uint16_t* source,
                                   SourceIndexFn index_fn) {
  constexpr int kSize = BoardPins::kDisplayWidthPixels;
  uint16_t line[kSize];
  for (int row = 0; row < kSize; ++row) {
    for (int column = 0; column < kSize; ++column) {
      line[column] = source[index_fn(row, column, kSize)];
    }
    driver.writeRgb565Buffer(line, static_cast<size_t>(kSize));
  }
}

DualEyeDisplay::SourceIndexFn DualEyeDisplay::indexFnFor(
    EyeMountPolicy::PanelTransform transform) {
  switch (transform) {
    case EyeMountPolicy::PanelTransform::ClockwiseGazeFlipped:
      return &DualEyeDisplay::indexClockwiseGazeFlipped;
    case EyeMountPolicy::PanelTransform::ClockwiseThen180:
      return &DualEyeDisplay::indexClockwiseThen180;
  }
  return &DualEyeDisplay::indexClockwiseGazeFlipped;
}

size_t DualEyeDisplay::indexClockwiseGazeFlipped(int row, int column,
                                                 int size) {
  return static_cast<size_t>((size - 1 - column) * size + (size - 1 - row));
}

size_t DualEyeDisplay::indexClockwiseThen180(int row, int column, int size) {
  return static_cast<size_t>(column * size + (size - 1 - row));
}

Gc9d01Driver& DualEyeDisplay::driverFor(EyeId eye) {
  if (eye == EyeId::Left) {
    return left_driver_;
  }
  return right_driver_;
}

void DualEyeDisplay::deselectBoth() {
  left_driver_.deselect();
  right_driver_.deselect();
}

void DualEyeDisplay::lockBus() {
  if (bus_locked_) {
    return;
  }
  spi_.beginTransaction(
      SPISettings(BoardPins::kSpiClockHz, MSBFIRST, SPI_MODE0));
  bus_locked_ = true;
}
