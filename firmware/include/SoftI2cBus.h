#pragma once

#include <stdint.h>

/**
 * Minimal open-drain bitbang I2C master (no ESP-IDF i2c driver / Wire).
 * Used for TOF so OV2640 SCCB can keep the hardware I2C controller.
 */
class SoftI2cBus {
 public:
  static constexpr uint32_t kHalfBitUs = 5;

  /** Creates an unconfigured bus. */
  SoftI2cBus();

  /**
   * Claims SDA/SCL with internal pull-ups (open-drain style).
   * @param sda_pin GPIO for SDA
   * @param scl_pin GPIO for SCL
   */
  void begin(int sda_pin, int scl_pin);

  /**
   * Writes register address then reads length bytes (repeated start).
   * @return true on full transfer with ACKs
   */
  bool writeRead(uint8_t address7, uint8_t reg, uint8_t* buffer, uint8_t length);

  /**
   * Probes a 7-bit address (START + addr+W + ACK? + STOP).
   * @return true when the slave ACKs
   */
  bool probe(uint8_t address7);

 private:
  void delayHalf() const;
  void sdaRelease();
  void sdaDriveLow();
  void sclRelease();
  void sclDriveLow();
  bool sclWaitHigh();
  bool readSda() const;
  void startCond();
  void stopCond();
  bool writeByte(uint8_t value);
  /** Reads one byte; returns false on clock-stretch timeout (fail-closed). */
  bool readByte(uint8_t& value, bool ack);

  int sda_pin_;
  int scl_pin_;
  bool begun_;
};
