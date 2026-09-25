#include "Gc9d01Driver.h"

namespace {

struct Gc9d01InitStep {
  uint8_t command;
  const uint8_t* data;
  uint8_t data_length;
};

void writeInitSteps(Gc9d01Driver& driver, const Gc9d01InitStep* steps,
                    size_t step_count) {
  for (size_t index = 0; index < step_count; ++index) {
    const Gc9d01InitStep& step = steps[index];
    driver.writeCommand(step.command);
    if (step.data != nullptr && step.data_length > 0) {
      driver.writeDataBuffer(step.data, step.data_length);
    }
  }
}

}  // namespace

Gc9d01Driver::Gc9d01Driver(SPIClass& spi, int data_command_pin, int reset_pin,
                           int chip_select_pin)
    : spi_(spi),
      data_command_pin_(data_command_pin),
      reset_pin_(reset_pin),
      chip_select_pin_(chip_select_pin) {}

void Gc9d01Driver::beginPins() {
  pinMode(reset_pin_, OUTPUT);
  pinMode(chip_select_pin_, OUTPUT);
  pinMode(data_command_pin_, OUTPUT);
  digitalWrite(chip_select_pin_, HIGH);
  digitalWrite(data_command_pin_, HIGH);
  digitalWrite(reset_pin_, HIGH);
}

void Gc9d01Driver::select() { digitalWrite(chip_select_pin_, LOW); }

void Gc9d01Driver::deselect() { digitalWrite(chip_select_pin_, HIGH); }

void Gc9d01Driver::hardReset() {
  digitalWrite(reset_pin_, HIGH);
  delay(10);
  digitalWrite(reset_pin_, LOW);
  delay(20);
  digitalWrite(reset_pin_, HIGH);
  delay(120);
}

void Gc9d01Driver::writeCommand(uint8_t command) {
  digitalWrite(data_command_pin_, LOW);
  spi_.transfer(command);
}

void Gc9d01Driver::writeData(uint8_t data) {
  digitalWrite(data_command_pin_, HIGH);
  spi_.transfer(data);
}

void Gc9d01Driver::writeDataBuffer(const uint8_t* data, size_t length) {
  digitalWrite(data_command_pin_, HIGH);
  spi_.writeBytes(data, length);
}

void Gc9d01Driver::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                                    uint16_t y1) {
  select();
  writeCommand(0x2A);
  writeData(static_cast<uint8_t>(x0 >> 8));
  writeData(static_cast<uint8_t>(x0 & 0xFF));
  writeData(static_cast<uint8_t>(x1 >> 8));
  writeData(static_cast<uint8_t>(x1 & 0xFF));
  writeCommand(0x2B);
  writeData(static_cast<uint8_t>(y0 >> 8));
  writeData(static_cast<uint8_t>(y0 & 0xFF));
  writeData(static_cast<uint8_t>(y1 >> 8));
  writeData(static_cast<uint8_t>(y1 & 0xFF));
  writeCommand(0x2C);
}

void Gc9d01Driver::writePixels(uint16_t color, uint32_t pixel_count) {
  digitalWrite(data_command_pin_, HIGH);
  // Chunked solid fill: fewer SPI transactions than per-pixel writeBytes.
  static constexpr uint32_t kChunkPixels = 64;
  uint16_t chunk[kChunkPixels];
  for (uint32_t index = 0; index < kChunkPixels; ++index) {
    chunk[index] = color;
  }
  uint32_t remaining = pixel_count;
  while (remaining > 0) {
    const uint32_t count =
        remaining < kChunkPixels ? remaining : kChunkPixels;
    spi_.writeBytes(reinterpret_cast<const uint8_t*>(chunk),
                    count * sizeof(uint16_t));
    remaining -= count;
  }
}

void Gc9d01Driver::writeRgb565Buffer(const uint16_t* pixels,
                                     size_t pixel_count) {
  digitalWrite(data_command_pin_, HIGH);
  // Native uint16_t memory dump == TFT_eSPI pushPixels(!_swapBytes) / pushImage.
  // Per-byte HI-first transfers were swapping colors and creating tramé.
  spi_.writeBytes(reinterpret_cast<const uint8_t*>(pixels),
                  pixel_count * sizeof(uint16_t));
}

void Gc9d01Driver::initialize() {
  hardReset();
  select();

  writeCommand(0xFE);
  writeCommand(0xEF);

  for (uint8_t command = 0x80; command <= 0x8F; ++command) {
    writeCommand(command);
    writeData(0xFF);
  }

  static const uint8_t kPayload74[] = {0x02, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t kPayload60[] = {0x38, 0x0F, 0x79, 0x67};
  static const uint8_t kPayload61[] = {0x38, 0x11, 0x79, 0x67};
  static const uint8_t kPayload64[] = {0x38, 0x17, 0x71, 0x5F, 0x79, 0x67};
  static const uint8_t kPayload65[] = {0x38, 0x13, 0x71, 0x5B, 0x79, 0x67};
  static const uint8_t kPayload6C[] = {0x22, 0x02, 0x22, 0x02, 0x22, 0x22, 0x50};
  static const uint8_t kPayload6E[] = {
      0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x0F, 0x0F, 0x0D, 0x0D, 0x0B,
      0x0B, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0C, 0x0C,
      0x0E, 0x0E, 0x10, 0x10, 0x00, 0x00, 0x02, 0x02, 0x04, 0x04};
  static const uint8_t kPayload93[] = {0x33, 0x7F, 0x00};
  static const uint8_t kPayload70[] = {0x0D, 0x02, 0x08, 0x0D, 0x02, 0x08};
  static const uint8_t kPayload71[] = {0x0D, 0x02, 0x08};
  static const uint8_t kPayloadF0[] = {0x53, 0x15, 0x0A, 0x04, 0x00, 0x3E};
  static const uint8_t kPayloadF2[] = {0x53, 0x15, 0x0A, 0x04, 0x00, 0x3A};
  static const uint8_t kPayloadF1[] = {0x56, 0xA8, 0x7F, 0x33, 0x34, 0x5F};
  static const uint8_t kPayloadF3[] = {0x52, 0xA4, 0x7F, 0x33, 0x34, 0xDF};

  static const uint8_t kOne3A[] = {0x05};
  static const uint8_t kOneEC[] = {0x01};
  static const uint8_t kOne98[] = {0x3E};
  static const uint8_t kOne99[] = {0x3E};
  static const uint8_t kTwoB5[] = {0x0D, 0x0D};
  static const uint8_t kTwo6A[] = {0x00, 0x00};
  static const uint8_t kOneBF[] = {0x01};
  static const uint8_t kOneF9[] = {0x40};
  static const uint8_t kOne9B[] = {0x3B};
  static const uint8_t kOne7E[] = {0x30};
  static const uint8_t kTwo91[] = {0x0E, 0x09};
  static const uint8_t kOneC3[] = {0x19};
  static const uint8_t kOneC4[] = {0x19};
  static const uint8_t kOneC9[] = {0x3C};
  // MADCTL applied separately per eye after init (mirrored ±90° mount).
  static const uint8_t kOne36[] = {0x00};

  static const Gc9d01InitStep kInitSteps[] = {
      {0x3A, kOne3A, 1},
      {0xEC, kOneEC, 1},
      {0x74, kPayload74, sizeof(kPayload74)},
      {0x98, kOne98, 1},
      {0x99, kOne99, 1},
      {0xB5, kTwoB5, 2},
      {0x60, kPayload60, sizeof(kPayload60)},
      {0x61, kPayload61, sizeof(kPayload61)},
      {0x64, kPayload64, sizeof(kPayload64)},
      {0x65, kPayload65, sizeof(kPayload65)},
      {0x6A, kTwo6A, 2},
      {0x6C, kPayload6C, sizeof(kPayload6C)},
      {0x6E, kPayload6E, sizeof(kPayload6E)},
      {0xBF, kOneBF, 1},
      {0xF9, kOneF9, 1},
      {0x9B, kOne9B, 1},
      {0x93, kPayload93, sizeof(kPayload93)},
      {0x7E, kOne7E, 1},
      {0x70, kPayload70, sizeof(kPayload70)},
      {0x71, kPayload71, sizeof(kPayload71)},
      {0x91, kTwo91, 2},
      {0xC3, kOneC3, 1},
      {0xC4, kOneC4, 1},
      {0xC9, kOneC9, 1},
      {0xF0, kPayloadF0, sizeof(kPayloadF0)},
      {0xF2, kPayloadF2, sizeof(kPayloadF2)},
      {0xF1, kPayloadF1, sizeof(kPayloadF1)},
      {0xF3, kPayloadF3, sizeof(kPayloadF3)},
      {0x36, kOne36, 1},
  };

  writeInitSteps(*this, kInitSteps,
               sizeof(kInitSteps) / sizeof(kInitSteps[0]));

  writeCommand(0x11);
  deselect();
  delay(200);

  select();
  writeCommand(0x29);
  writeCommand(0x2C);
  deselect();
  delay(10);
}

void Gc9d01Driver::setMadctl(uint8_t madctl) {
  select();
  writeCommand(0x36);
  writeData(madctl);
  deselect();
}
