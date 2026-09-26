#include "BallColorDetector.h"

#include <string.h>

#include "BallVisionConfig.h"

namespace {

constexpr int kFloodStackCap = 2048;
/** QVGA step-1 grid: 320×240. Single core1 writer — reuse across frames. */
constexpr int kMaxGridCells =
    BallVisionConfig::kFrameWidth * BallVisionConfig::kFrameHeight;
constexpr int kMaskBytes = (kMaxGridCells + 7) / 8;

struct FloodPoint {
  uint16_t col;
  uint16_t row;
};

uint8_t g_hits_red[kMaskBytes];
uint8_t g_hits_green[kMaskBytes];
uint8_t g_visited[kMaskBytes];
FloodPoint g_flood_stack[kFloodStackCap];

bool maskGet(const uint8_t* bits, int index) {
  return (bits[index >> 3] & static_cast<uint8_t>(1u << (index & 7))) != 0;
}

void maskSet(uint8_t* bits, int index) {
  bits[index >> 3] |= static_cast<uint8_t>(1u << (index & 7));
}

}  // namespace

uint16_t BallColorDetector::maybeSwap(uint16_t pixel, bool swap_bytes) {
  if (!swap_bytes) {
    return pixel;
  }
  return static_cast<uint16_t>((pixel >> 8) | (pixel << 8));
}

bool BallColorDetector::isRedRgb565(uint16_t pixel) {
  const int red = (pixel >> 11) & 0x1F;
  const int green = (pixel >> 5) & 0x3F;
  const int blue = pixel & 0x1F;
  const int green5 = green >> 1;
  if (red < BallVisionConfig::kRedRgbMin) {
    return false;
  }
  // Reject yellow / warm lamps (high G) and pale pinks (high B).
  if (green5 > BallVisionConfig::kRedMaxGreen5 ||
      blue > BallVisionConfig::kRedMaxBlue) {
    return false;
  }
  return red >= green5 + BallVisionConfig::kRedRgbDominate &&
         red >= blue + BallVisionConfig::kRedRgbDominate;
}

bool BallColorDetector::isGreenRgb565(uint16_t pixel) {
  const int red = (pixel >> 11) & 0x1F;
  const int green = (pixel >> 5) & 0x3F;
  const int blue = pixel & 0x1F;
  const int green5 = green >> 1;
  if (green5 < BallVisionConfig::kGreenRgbMin) {
    return false;
  }
  // Reject yellow lamps (R≈G) and cyan-ish whites.
  if (red > BallVisionConfig::kGreenMaxRed ||
      blue > BallVisionConfig::kGreenMaxBlue) {
    return false;
  }
  return green5 >= red + BallVisionConfig::kGreenRgbDominate &&
         green5 >= blue + BallVisionConfig::kGreenRgbDominate;
}

bool BallColorDetector::blobPasses(const BlobStats& blob, int width, int height,
                                   int step, int min_area, int min_w, int min_h,
                                   float min_fill, float* score_out) {
  if (blob.count < min_area || blob.max_x < blob.min_x ||
      blob.max_y < blob.min_y) {
    return false;
  }
  const int box_w = blob.max_x - blob.min_x + 1;
  const int box_h = blob.max_y - blob.min_y + 1;
  if (box_w < min_w || box_h < min_h) {
    return false;
  }
  const float aspect =
      static_cast<float>(box_w) / static_cast<float>(box_h);
  if (aspect < BallVisionConfig::kMinAspect ||
      aspect > BallVisionConfig::kMaxAspect) {
    return false;
  }
  const float diameter =
      static_cast<float>(box_w > box_h ? box_w : box_h) /
      static_cast<float>(width);
  if (diameter > BallVisionConfig::kMaxDiameter) {
    return false;
  }
  const int box_samples =
      ((box_w + step - 1) / step) * ((box_h + step - 1) / step);
  if (box_samples <= 0) {
    return false;
  }
  const float fill =
      static_cast<float>(blob.count) / static_cast<float>(box_samples);
  if (fill < min_fill) {
    return false;
  }
  (void)height;
  *score_out = fill * static_cast<float>(blob.count);
  return true;
}

void BallColorDetector::writeObservation(const BlobStats& blob, int width,
                                         int height, BallColor color,
                                         BallObservation& out) {
  const int box_w = blob.max_x - blob.min_x + 1;
  const int box_h = blob.max_y - blob.min_y + 1;
  const float center_x =
      static_cast<float>(blob.sum_x) / static_cast<float>(blob.count);
  const float center_y =
      static_cast<float>(blob.sum_y) / static_cast<float>(blob.count);
  float gaze_x = (center_x / static_cast<float>(width - 1)) * 2.0f - 1.0f;
  float gaze_y = (center_y / static_cast<float>(height - 1)) * 2.0f - 1.0f;
  if (gaze_x > -BallVisionConfig::kGazeDeadzone &&
      gaze_x < BallVisionConfig::kGazeDeadzone) {
    gaze_x = 0.0f;
  }
  if (gaze_y > -BallVisionConfig::kGazeDeadzone &&
      gaze_y < BallVisionConfig::kGazeDeadzone) {
    gaze_y = 0.0f;
  }
  out.found = true;
  out.x = gaze_x;
  out.y = gaze_y;
  out.diameter =
      static_cast<float>(box_w > box_h ? box_w : box_h) /
      static_cast<float>(width);
  out.color = color;
}

void BallColorDetector::findBestBlob(const uint8_t* hits, int width, int height,
                                     int step, int grid_w, int grid_h,
                                     int min_area, int min_w, int min_h,
                                     float min_fill, BallColor color,
                                     BallObservation& out) const {
  out.found = false;
  out.x = 0.0f;
  out.y = 0.0f;
  out.diameter = 0.0f;
  out.color = BallColor::None;
  if (hits == nullptr || color == BallColor::None) {
    return;
  }

  const int cells = grid_w * grid_h;
  const size_t bytes = static_cast<size_t>((cells + 7) / 8);
  memset(g_visited, 0, bytes);

  BlobStats best{};
  float best_score = -1.0f;
  FloodPoint* stack = g_flood_stack;

  for (int gy = 0; gy < grid_h; ++gy) {
    for (int gx = 0; gx < grid_w; ++gx) {
      const int seed = gy * grid_w + gx;
      if (!maskGet(hits, seed) || maskGet(g_visited, seed)) {
        continue;
      }

      BlobStats blob{};
      blob.count = 0;
      blob.sum_x = 0;
      blob.sum_y = 0;
      blob.min_x = width;
      blob.min_y = height;
      blob.max_x = -1;
      blob.max_y = -1;
      bool truncated = false;

      int sp = 0;
      stack[sp++] = {static_cast<uint16_t>(gx), static_cast<uint16_t>(gy)};
      maskSet(g_visited, seed);

      while (sp > 0) {
        const FloodPoint p = stack[--sp];
        const int px = static_cast<int>(p.col) * step;
        const int py = static_cast<int>(p.row) * step;
        ++blob.count;
        blob.sum_x += px;
        blob.sum_y += py;
        if (px < blob.min_x) {
          blob.min_x = px;
        }
        if (px > blob.max_x) {
          blob.max_x = px;
        }
        if (py < blob.min_y) {
          blob.min_y = py;
        }
        if (py > blob.max_y) {
          blob.max_y = py;
        }

        static const int kDx[4] = {1, -1, 0, 0};
        static const int kDy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
          const int nx = static_cast<int>(p.col) + kDx[i];
          const int ny = static_cast<int>(p.row) + kDy[i];
          if (nx < 0 || ny < 0 || nx >= grid_w || ny >= grid_h) {
            continue;
          }
          const int ni = ny * grid_w + nx;
          if (!maskGet(hits, ni) || maskGet(g_visited, ni)) {
            continue;
          }
          maskSet(g_visited, ni);
          if (sp >= kFloodStackCap) {
            // Ceiling: huge components (lamps) — discard rather than under-grow.
            truncated = true;
            continue;
          }
          stack[sp++] = {static_cast<uint16_t>(nx), static_cast<uint16_t>(ny)};
        }
      }

      if (truncated) {
        continue;
      }
      float score = 0.0f;
      if (blobPasses(blob, width, height, step, min_area, min_w, min_h,
                     min_fill, &score) &&
          score > best_score) {
        best_score = score;
        best = blob;
      }
    }
  }

  if (best_score < 0.0f) {
    return;
  }
  writeObservation(best, width, height, color, out);
}

void BallColorDetector::detect(const uint16_t* pixels, int width, int height,
                               BallObservation& out, bool swap_bytes,
                               BallColor preferred) const {
  out.found = false;
  out.x = 0.0f;
  out.y = 0.0f;
  out.diameter = 0.0f;
  out.color = BallColor::None;
  if (pixels == nullptr || width <= 1 || height <= 1) {
    return;
  }

  const bool qvga = width >= BallVisionConfig::kQvgaWidthThreshold;
  const int step =
      qvga ? BallVisionConfig::kDetectStepQvga : BallVisionConfig::kDetectStepQqvga;
  const int min_area_cfg =
      qvga ? BallVisionConfig::kMinAreaQvga : BallVisionConfig::kMinAreaQqvga;
  const int min_w =
      qvga ? BallVisionConfig::kMinWidthQvga : BallVisionConfig::kMinWidthQqvga;
  const int min_h =
      qvga ? BallVisionConfig::kMinHeightQvga : BallVisionConfig::kMinHeightQqvga;
  const float min_fill = qvga ? BallVisionConfig::kMinFillRatioQvga
                              : BallVisionConfig::kMinFillRatioQqvga;
  const int min_area = min_area_cfg / (step * step);

  const int grid_w = (width + step - 1) / step;
  const int grid_h = (height + step - 1) / step;
  const int cells = grid_w * grid_h;
  if (cells <= 0 || cells > kMaxGridCells) {
    return;
  }
  const size_t bytes = static_cast<size_t>((cells + 7) / 8);
  const bool want_green = BallVisionConfig::kEnableGreenFallback;
  memset(g_hits_red, 0, bytes);
  if (want_green) {
    memset(g_hits_green, 0, bytes);
  }

  // Single frame scan → red (+ optional green) hit masks.
  for (int row = 0, gy = 0; row < height; row += step, ++gy) {
    const uint16_t* line = pixels + static_cast<size_t>(row) * width;
    for (int col = 0, gx = 0; col < width; col += step, ++gx) {
      const uint16_t pixel = maybeSwap(line[col], swap_bytes);
      const int index = gy * grid_w + gx;
      if (isRedRgb565(pixel)) {
        maskSet(g_hits_red, index);
      } else if (want_green && isGreenRgb565(pixel)) {
        maskSet(g_hits_green, index);
      }
    }
  }

  const BallColor primary =
      (preferred == BallColor::Green) ? BallColor::Green : BallColor::Red;
  const BallColor secondary =
      (primary == BallColor::Red) ? BallColor::Green : BallColor::Red;
  const uint8_t* primary_hits =
      (primary == BallColor::Red) ? g_hits_red : g_hits_green;
  const uint8_t* secondary_hits =
      (secondary == BallColor::Red) ? g_hits_red : g_hits_green;

  findBestBlob(primary_hits, width, height, step, grid_w, grid_h, min_area,
               min_w, min_h, min_fill, primary, out);
  if (out.found || !want_green) {
    return;
  }
  findBestBlob(secondary_hits, width, height, step, grid_w, grid_h, min_area,
               min_w, min_h, min_fill, secondary, out);
}
