#pragma once

#include <stdint.h>

#include "RangeObservation.h"
#include "SoftI2cBus.h"

/**
 * Waveshare TOF Laser Range Sensor Mini over bitbang I2C (NLink register map).
 * Avoids Wire/Wire1 so the camera can keep the hardware I2C controller.
 */
class TofRangeSensor {
 public:
  static constexpr uint8_t kDefaultI2cAddress = 0x08;
  static constexpr uint8_t kModeRegister = 0x0C;
  static constexpr uint8_t kBlockRegister = 0x20;
  static constexpr uint8_t kBlockBytes = 16;
  /** Interface mode nibble: 3 = IIC (NLink). */
  static constexpr uint8_t kInterfaceModeI2c = 3;
  /** Waveshare Mini typical span (mm); used when dis_status is non-zero. */
  static constexpr uint32_t kMinValidDistanceMm = 20;
  static constexpr uint32_t kMaxValidDistanceMm = 8000;
  /** Probe default addr + a few module-ID offsets. */
  static constexpr uint8_t kProbeCount = 8;

  /** Creates an unready sensor instance. */
  TofRangeSensor();

  /**
   * Opens soft-I2C on BoardPins SDA/SCL and probes a TOFSense slave.
   * @return true when a device ACKs
   */
  bool begin();

  /** True after a successful begin(). */
  bool isReady() const;

  /** 7-bit address that answered during begin() (0 if not ready). */
  uint8_t address() const;

  /**
   * Reads distance + status + signal strength into observation.
   * @return true when the I2C transfer succeeds (check observation.valid)
   */
  bool read(RangeObservation& observation);

 private:
  SoftI2cBus bus_;
  bool ready_;
  uint8_t address_;
};
