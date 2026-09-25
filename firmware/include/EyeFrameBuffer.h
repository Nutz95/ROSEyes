#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * RGB565 offscreen buffer used to compose one eye frame before a single blit.
 */
class EyeFrameBuffer {
 public:
  static constexpr int kWidth = 160;
  static constexpr int kHeight = 160;
  static constexpr size_t kPixelCount =
      static_cast<size_t>(kWidth) * static_cast<size_t>(kHeight);
  /** Lower lid rises this fraction of height at full closure. */
  static constexpr float kLowerLidFraction = 0.28f;
  /** Closures at or below this are treated as fully open (no wipe). */
  static constexpr float kLidClosureEpsilon = 0.001f;

  /** Creates an unallocated buffer. */
  EyeFrameBuffer();

  /** Releases pixel memory. */
  ~EyeFrameBuffer();

  EyeFrameBuffer(const EyeFrameBuffer&) = delete;
  EyeFrameBuffer& operator=(const EyeFrameBuffer&) = delete;

  /** Allocates PSRAM (or heap fallback). Returns false on failure. */
  bool begin();

  /** Fills the whole buffer with a solid color. */
  void fill(uint16_t color);

  /** Fills a clipped axis-aligned rectangle. */
  void fillRect(int x, int y, int width, int height, uint16_t color);

  /** Fills a circle using horizontal spans. */
  void fillCircle(int center_x, int center_y, int radius, uint16_t color);

  /**
   * Applies eyelid occlusion (upper lid dominant, slight lower lid).
   * @param closure 0 = open, 1 = fully closed
   */
  void applyLidClosure(float closure);

  /** Raw RGB565 pixels for DMA/SPI blit. */
  const uint16_t* data() const;

  /** Mutable pixels for composers that write into the buffer. */
  uint16_t* mutableData();

  /** True when begin() succeeded. */
  bool isReady() const;

 private:
  uint16_t* pixels_;
  bool ready_;
};
