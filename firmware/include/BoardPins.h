#pragma once

/**
 * XIAO ESP32-S3 Sense pin map used by ROSEyes (GPIO numbers, not Dx labels).
 *
 * Eyes (SPI):
 *   RST1=D0/GPIO1, RST2=D1/GPIO2, CS1=D2/GPIO3, CS2=D3/GPIO4,
 *   SCK=D8/GPIO7, DC=D9/GPIO8, MOSI=D10/GPIO9
 *
 * Camera (OV2640 DVP on Sense FPC):
 *   XCLK=10, SIOD=40, SIOC=39, Y2..Y9, VSYNC=38, HREF=47, PCLK=13
 *
 * TOF reserved (I2C, not driven in this iteration):
 *   SDA=D4/GPIO5, SCL=D5/GPIO6
 */
class BoardPins {
 public:
  static constexpr int kResetLeftPin = 1;
  static constexpr int kResetRightPin = 2;
  static constexpr int kChipSelectLeftPin = 3;
  static constexpr int kChipSelectRightPin = 4;
  static constexpr int kTofSdaPin = 5;
  static constexpr int kTofSclPin = 6;
  static constexpr int kSpiClockPin = 7;
  static constexpr int kDataCommandPin = 8;
  static constexpr int kSpiMosiPin = 9;

  static constexpr int kCamPwdnPin = -1;
  static constexpr int kCamResetPin = -1;
  static constexpr int kCamXclkPin = 10;
  static constexpr int kCamSiodPin = 40;
  static constexpr int kCamSiocPin = 39;
  static constexpr int kCamY9Pin = 48;
  static constexpr int kCamY8Pin = 11;
  static constexpr int kCamY7Pin = 12;
  static constexpr int kCamY6Pin = 14;
  static constexpr int kCamY5Pin = 16;
  static constexpr int kCamY4Pin = 18;
  static constexpr int kCamY3Pin = 17;
  static constexpr int kCamY2Pin = 15;
  static constexpr int kCamVsyncPin = 38;
  static constexpr int kCamHrefPin = 47;
  static constexpr int kCamPclkPin = 13;

  static constexpr int kDisplayWidthPixels = 160;
  static constexpr int kDisplayHeightPixels = 160;
  /**
   * Keep MADCTL at 0x00 (native scan). Hardware MV rotation caused tramé /
   * jagged edges; ±90° panel mount is corrected by software blit rotation.
   */
  static constexpr uint8_t kMadctlNative = 0x00;
  static constexpr unsigned long kSpiClockHz = 40000000UL;
  static constexpr unsigned long kSerialBaudRate = 115200UL;
};
