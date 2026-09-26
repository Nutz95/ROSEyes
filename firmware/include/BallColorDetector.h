#pragma once

#include <stddef.h>
#include <stdint.h>

#include "BallColor.h"
#include "BallObservation.h"

/**
 * Fast RGB565 blob detector for a solid-colored ball.
 */
class BallColorDetector {
 public:
  /**
   * One pixel pass builds red/green masks; flood prefers sticky color first.
   * @param pixels row-major RGB565 (host endian unless swap_bytes)
   * @param width frame width
   * @param height frame height
   * @param out filled when a blob passes filters; found=false otherwise
   * @param swap_bytes swap each uint16 (ESP cam RGB565 is often byte-swapped)
   * @param preferred last valid color to try first (None => red)
   */
  void detect(const uint16_t* pixels, int width, int height,
              BallObservation& out, bool swap_bytes = false,
              BallColor preferred = BallColor::None) const;

 private:
  struct BlobStats {
    int count;
    int64_t sum_x;
    int64_t sum_y;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
  };

  static uint16_t maybeSwap(uint16_t pixel, bool swap_bytes);
  /** Saturated red (rejects yellow lamps / warm whites). */
  static bool isRedRgb565(uint16_t pixel);
  /** Saturated green (rejects yellow lamps). */
  static bool isGreenRgb565(uint16_t pixel);
  /** Flood a pre-filled hit mask; fail-closed if BFS stack overflows. */
  void findBestBlob(const uint8_t* hits, int width, int height, int step,
                    int grid_w, int grid_h, int min_area, int min_w, int min_h,
                    float min_fill, BallColor color,
                    BallObservation& out) const;
  static bool blobPasses(const BlobStats& blob, int width, int height,
                         int step, int min_area, int min_w, int min_h,
                         float min_fill, float* score_out);
  static void writeObservation(const BlobStats& blob, int width, int height,
                               BallColor color, BallObservation& out);
};
