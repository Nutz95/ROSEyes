#include "EyeRenderer.h"

#include <Arduino.h>
#include <pgmspace.h>

#include "BoardPins.h"
#include "EyeId.h"
#include "EyeModelConfig.h"
#include "EyeMountPolicy.h"
#include "Rgb565Colors.h"

EyeRenderer::EyeRenderer(DualEyeDisplay& display) : display_(display) {}

bool EyeRenderer::begin() { return frame_buffer_.begin(); }

void EyeRenderer::drawEyes(const GazeState& gaze, uint16_t iris_color,
                           float lid_closure) {
  if (!frame_buffer_.isReady()) {
    return;
  }

  const GazeState left_gaze =
      EyeMountPolicy::gazeForComposition(EyeId::Left, gaze);
  composeFrame(left_gaze, iris_color, lid_closure);
  display_.blitRgb565(EyeId::Left, frame_buffer_.data(),
                      EyeFrameBuffer::kPixelCount);

  const GazeState right_gaze =
      EyeMountPolicy::gazeForComposition(EyeId::Right, gaze);
  composeFrame(right_gaze, iris_color, lid_closure);
  display_.blitRgb565(EyeId::Right, frame_buffer_.data(),
                      EyeFrameBuffer::kPixelCount);
}

void EyeRenderer::composeFrame(const GazeState& gaze, uint16_t iris_color,
                               float lid_closure) {
#if defined(ROSEYES_EYE_MODEL_PROCEDURAL)
  composeProcedural(gaze, iris_color, lid_closure);
#elif defined(ROSEYES_EYE_MODEL_ROBOT_BITMAP)
  (void)iris_color;
  composeRobotBitmap(gaze, lid_closure);
#elif defined(ROSEYES_EYE_MODEL_UNCANNY)
  (void)iris_color;
  composeUncanny(gaze, lid_closure);
#endif
}

void EyeRenderer::composeProcedural(const GazeState& gaze, uint16_t iris_color,
                                    float lid_closure) {
  frame_buffer_.fill(Rgb565Colors::kBlack);

  const int center_x = BoardPins::kDisplayWidthPixels / 2 +
                       gaze.horizontalPixelOffset(kMaxGazePixelOffset);
  const int center_y = BoardPins::kDisplayHeightPixels / 2 +
                       gaze.verticalPixelOffset(kMaxGazePixelOffset);

  frame_buffer_.fillCircle(center_x, center_y, kIrisRadius, iris_color);
  frame_buffer_.fillCircle(center_x, center_y, kPupilRadius, Rgb565Colors::kBlack);
  frame_buffer_.fillCircle(center_x + kHighlightOffsetX,
                           center_y + kHighlightOffsetY, kHighlightRadius,
                           Rgb565Colors::kWhite);
  frame_buffer_.applyLidClosure(lid_closure);
}

void EyeRenderer::composeRobotBitmap(const GazeState& gaze,
                                     float lid_closure) {
#if defined(ROSEYES_EYE_MODEL_ROBOT_BITMAP)
  frame_buffer_.fill(Rgb565Colors::kBlack);

  const int offset_x = gaze.horizontalPixelOffset(kMaxGazePixelOffset);
  const int offset_y = gaze.verticalPixelOffset(kMaxGazePixelOffset);
  const uint16_t* source = SelectedRobotFrames::frame(0);
  if (source == nullptr || !frame_buffer_.isReady()) {
    return;
  }

  uint16_t* dest = frame_buffer_.mutableData();
  const int src_width = SelectedRobotFrames::kWidth;
  const int src_height = SelectedRobotFrames::kHeight;

  int src_y0 = 0;
  int dest_y0 = offset_y;
  int copy_height = src_height;
  if (dest_y0 < 0) {
    src_y0 = -dest_y0;
    copy_height += dest_y0;
    dest_y0 = 0;
  }
  if (dest_y0 + copy_height > EyeFrameBuffer::kHeight) {
    copy_height = EyeFrameBuffer::kHeight - dest_y0;
  }
  if (copy_height <= 0) {
    frame_buffer_.applyLidClosure(lid_closure);
    return;
  }

  int src_x0 = 0;
  int dest_x0 = offset_x;
  int copy_width = src_width;
  if (dest_x0 < 0) {
    src_x0 = -dest_x0;
    copy_width += dest_x0;
    dest_x0 = 0;
  }
  if (dest_x0 + copy_width > EyeFrameBuffer::kWidth) {
    copy_width = EyeFrameBuffer::kWidth - dest_x0;
  }
  if (copy_width <= 0) {
    frame_buffer_.applyLidClosure(lid_closure);
    return;
  }

  for (int row = 0; row < copy_height; ++row) {
    const size_t source_row =
        static_cast<size_t>(src_y0 + row) * static_cast<size_t>(src_width) +
        static_cast<size_t>(src_x0);
    const size_t dest_row =
        static_cast<size_t>(dest_y0 + row) * EyeFrameBuffer::kWidth +
        static_cast<size_t>(dest_x0);
    for (int column = 0; column < copy_width; ++column) {
      dest[dest_row + static_cast<size_t>(column)] =
          pgm_read_word(&source[source_row + static_cast<size_t>(column)]);
    }
  }
  frame_buffer_.applyLidClosure(lid_closure);
#else
  (void)gaze;
  (void)lid_closure;
#endif
}

void EyeRenderer::composeUncanny(const GazeState& gaze, float lid_closure) {
#if defined(ROSEYES_EYE_MODEL_UNCANNY)
  frame_buffer_.fill(Rgb565Colors::kBlack);
  if (!frame_buffer_.isReady()) {
    return;
  }

  // Map normalized gaze [-1,1] onto UncannyEyes sclera pan window.
  const int32_t max_x = SCLERA_WIDTH - SCREEN_WIDTH;
  const int32_t max_y = SCLERA_HEIGHT - SCREEN_HEIGHT;
  int32_t sclera_x =
      static_cast<int32_t>((gaze.x() + 1.0f) * 0.5f * static_cast<float>(max_x));
  int32_t sclera_y =
      static_cast<int32_t>((gaze.y() + 1.0f) * 0.5f * static_cast<float>(max_y));
  if (sclera_x < 0) {
    sclera_x = 0;
  }
  if (sclera_y < 0) {
    sclera_y = 0;
  }
  if (sclera_x > max_x) {
    sclera_x = max_x;
  }
  if (sclera_y > max_y) {
    sclera_y = max_y;
  }

  // Lid maps: higher threshold => more eyelid coverage (0 open .. closed).
  // Previous mapping inverted this, so blinks looked like "open on close".
  const uint8_t lid =
      static_cast<uint8_t>(
          lid_closure <= 0.0f
              ? 0.0f
              : (lid_closure >= 1.0f
                     ? static_cast<float>(kUncannyLidClosedThreshold)
                     : lid_closure *
                           static_cast<float>(kUncannyLidClosedThreshold)));
  const uint8_t u_threshold = lid;
  const uint8_t l_threshold = lid;

  const uint32_t iris_scale = kUncannyIrisScale;
  const int origin_x = (EyeFrameBuffer::kWidth - SCREEN_WIDTH) / 2;
  const int origin_y = (EyeFrameBuffer::kHeight - SCREEN_HEIGHT) / 2;

  uint16_t* dest = frame_buffer_.mutableData();
  int32_t iris_y = sclera_y - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
  const int32_t sclera_x_save = sclera_x;

  for (uint32_t screen_y = 0; screen_y < SCREEN_HEIGHT;
       ++screen_y, ++sclera_y, ++iris_y) {
    sclera_x = sclera_x_save;
    int32_t iris_x = sclera_x_save - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
    for (uint32_t screen_x = 0; screen_x < SCREEN_WIDTH;
         ++screen_x, ++sclera_x, ++iris_x) {
      uint16_t pixel = 0;
      const uint8_t upper_v =
          pgm_read_byte(upper + screen_y * SCREEN_WIDTH + screen_x);
      const uint8_t lower_v =
          pgm_read_byte(lower + screen_y * SCREEN_WIDTH + screen_x);
      if (lower_v <= l_threshold || upper_v <= u_threshold) {
        pixel = 0;
      } else if (iris_y < 0 || iris_y >= IRIS_HEIGHT || iris_x < 0 ||
                 iris_x >= IRIS_WIDTH) {
        pixel = pgm_read_word(sclera + sclera_y * SCLERA_WIDTH + sclera_x);
      } else {
        const uint16_t polar_sample =
            pgm_read_word(polar + iris_y * IRIS_WIDTH + iris_x);
        const uint32_t distance =
            (iris_scale * (polar_sample & 0x7F)) / 128;
        if (distance < IRIS_MAP_HEIGHT) {
          const uint32_t angle =
              (IRIS_MAP_WIDTH * (polar_sample >> 7)) / 512;
          pixel = pgm_read_word(iris + distance * IRIS_MAP_WIDTH + angle);
        } else {
          pixel = pgm_read_word(sclera + sclera_y * SCLERA_WIDTH + sclera_x);
        }
      }

      // UncannyEyes stores a byte-swapped pixel then LE-dumps it (pushPixels).
      // With our writeBytes LE dump, apply the same swap so colors match demos.
      pixel = static_cast<uint16_t>((pixel << 8) | (pixel >> 8));

      const int dest_x = origin_x + static_cast<int>(screen_x);
      const int dest_y = origin_y + static_cast<int>(screen_y);
      dest[static_cast<size_t>(dest_y) * EyeFrameBuffer::kWidth +
           static_cast<size_t>(dest_x)] = pixel;
    }
  }
#else
  (void)gaze;
  (void)lid_closure;
#endif
}
