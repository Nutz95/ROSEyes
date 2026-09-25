#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Low-level GC9D01 command/data helper bound to a shared SPI bus.
 */
class Gc9d01Driver {
 public:
  /**
   * @param spi shared SPI peripheral
   * @param data_command_pin DC GPIO
   * @param reset_pin per-panel reset GPIO
   * @param chip_select_pin per-panel CS GPIO
   */
  Gc9d01Driver(SPIClass& spi, int data_command_pin, int reset_pin,
               int chip_select_pin);

  /** Configures GPIO directions for this panel. */
  void beginPins();

  /** Runs the GC9D01 dual-gate init sequence for 160x160 RGB565. */
  void initialize();

  /**
   * Sets MADCTL (memory access control) for panel orientation.
   * @param madctl GC9D01 register 0x36 value (MX/MY/MV/BGR bits)
   */
  void setMadctl(uint8_t madctl);

  /** Drives CS low for this panel (caller manages exclusivity). */
  void select();

  /** Drives CS high for this panel. */
  void deselect();

  /** Pulses the reset line. */
  void hardReset();

  /** Sends a command byte with DC low. */
  void writeCommand(uint8_t command);

  /** Sends a data byte with DC high. */
  void writeData(uint8_t data);

  /** Sends a data buffer with DC high. */
  void writeDataBuffer(const uint8_t* data, size_t length);

  /** Sets the GRAM address window then issues memory-write. */
  void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

  /** Streams pixel_count copies of an RGB565 color into the open window. */
  void writePixels(uint16_t color, uint32_t pixel_count);

  /** Streams an RGB565 buffer into the open address window. */
  void writeRgb565Buffer(const uint16_t* pixels, size_t pixel_count);

 private:
  SPIClass& spi_;
  int data_command_pin_;
  int reset_pin_;
  int chip_select_pin_;
};
