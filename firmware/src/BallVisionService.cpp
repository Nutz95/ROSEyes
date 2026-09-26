#include "BallVisionService.h"

#include "BallVisionConfig.h"
#include "BoardPins.h"

#if ROSEYES_ENABLE_CAMERA_BALL

#include <Arduino.h>
#include <esp_camera.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <img_converters.h>

#include "BallColorDetector.h"

BallVisionService::BallVisionService(BallObservationStore& store,
                                     CameraJpegMailbox& jpeg_mailbox)
    : store_(store),
      jpeg_mailbox_(jpeg_mailbox),
      enabled_(false),
      started_(false),
      framesize_code_(BallVisionConfig::kFramesizeQvga),
      sticky_color_(BallColor::None) {}

bool BallVisionService::begin() {
  if (started_) {
    return true;
  }
  if (!initCamera()) {
    Serial.println("ball vision: camera init failed");
    return false;
  }
  started_ = true;
  enabled_ = true;
  const BaseType_t ok = xTaskCreatePinnedToCore(
      &BallVisionService::taskEntry, "ball_vision", 8192, this, 2, nullptr, 1);
  if (ok != pdPASS) {
    Serial.println("ball vision: task create failed");
    started_ = false;
    enabled_ = false;
    return false;
  }
  Serial.printf("ball vision: core1 started (QVGA, red%s, qqvga=%s)\n",
                BallVisionConfig::kEnableGreenFallback ? "+green" : "",
                BallVisionConfig::kEnableDynamicQqvga ? "on" : "off");
  return true;
}

void BallVisionService::setEnabled(bool enabled) {
  enabled_ = enabled;
}

bool BallVisionService::isEnabled() const {
  return enabled_;
}

bool BallVisionService::initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = BoardPins::kCamY2Pin;
  config.pin_d1 = BoardPins::kCamY3Pin;
  config.pin_d2 = BoardPins::kCamY4Pin;
  config.pin_d3 = BoardPins::kCamY5Pin;
  config.pin_d4 = BoardPins::kCamY6Pin;
  config.pin_d5 = BoardPins::kCamY7Pin;
  config.pin_d6 = BoardPins::kCamY8Pin;
  config.pin_d7 = BoardPins::kCamY9Pin;
  config.pin_xclk = BoardPins::kCamXclkPin;
  config.pin_pclk = BoardPins::kCamPclkPin;
  config.pin_vsync = BoardPins::kCamVsyncPin;
  config.pin_href = BoardPins::kCamHrefPin;
  config.pin_sccb_sda = BoardPins::kCamSiodPin;
  config.pin_sccb_scl = BoardPins::kCamSiocPin;
  config.pin_pwdn = BoardPins::kCamPwdnPin;
  config.pin_reset = BoardPins::kCamResetPin;
  // 16 MHz: better FPS; still usually OK with WiFi + step-2 detect.
  config.xclk_freq_hz = 16000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  return esp_camera_init(&config) == ESP_OK;
}

void BallVisionService::maybeAdjustFramesize(const BallObservation& observation) {
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    return;
  }
  uint8_t want = framesize_code_;
  if (!BallVisionConfig::kEnableDynamicQqvga) {
    want = BallVisionConfig::kFramesizeQvga;
  } else if (observation.found &&
             observation.diameter >= BallVisionConfig::kNearDiameterEnter) {
    want = BallVisionConfig::kFramesizeQqvga;
  } else if (!observation.found ||
             observation.diameter <= BallVisionConfig::kNearDiameterExit) {
    want = BallVisionConfig::kFramesizeQvga;
  }
  if (want == framesize_code_) {
    return;
  }
  const framesize_t size =
      (want == BallVisionConfig::kFramesizeQqvga) ? FRAMESIZE_QQVGA
                                                  : FRAMESIZE_QVGA;
  if (sensor->set_framesize(sensor, size) == 0) {
    framesize_code_ = want;
    Serial.printf("ball vision: framesize -> %s\n",
                  want == BallVisionConfig::kFramesizeQqvga ? "qqvga" : "qvga");
    // Drop a stale buffered frame after resolution change.
    camera_fb_t* stale = esp_camera_fb_get();
    if (stale != nullptr) {
      esp_camera_fb_return(stale);
    }
  }
}

void BallVisionService::encodeSnapshot(camera_fb_t* frame) {
  if (frame == nullptr) {
    return;
  }
  uint8_t* jpg = nullptr;
  size_t jpg_len = 0;
  if (!frame2jpg(frame, BallVisionConfig::kJpegQuality, &jpg, &jpg_len) ||
      jpg == nullptr) {
    Serial.println("ball vision: JPEG encode failed");
    return;
  }
  if (!jpeg_mailbox_.publishJpeg(jpg, jpg_len)) {
    Serial.printf("ball vision: JPEG too large (%u > %u)\n",
                  static_cast<unsigned>(jpg_len),
                  static_cast<unsigned>(CameraJpegMailbox::kMaxBytes));
  } else {
    Serial.printf("ball vision: snapshot ready (%u bytes)\n",
                  static_cast<unsigned>(jpg_len));
  }
  free(jpg);
}

void BallVisionService::taskEntry(void* self) {
  static_cast<BallVisionService*>(self)->taskLoop();
}

void BallVisionService::taskLoop() {
  BallColorDetector detector;
  uint32_t window_start_ms = millis();
  uint32_t frames_in_window = 0;
  float fps = 0.0f;
  const uint32_t min_period_ms = 1000u / BallVisionConfig::kTargetFps;

  for (;;) {
    const bool want_snap = jpeg_mailbox_.consumeCaptureRequest();
    if (!enabled_ && !want_snap) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    const uint32_t frame_start_ms = millis();
    camera_fb_t* frame = esp_camera_fb_get();
    if (frame == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    if (want_snap) {
      encodeSnapshot(frame);
    }

    BallObservation observation;
    observation.found = false;
    observation.x = 0.0f;
    observation.y = 0.0f;
    observation.diameter = 0.0f;
    observation.fps = fps;
    observation.timestamp_ms = frame_start_ms;
    observation.seq = 0;
    observation.color = BallColor::None;
    observation.framesize_code = framesize_code_;

    const size_t expected =
        static_cast<size_t>(frame->width) * frame->height * 2u;
    if (enabled_ && frame->format == PIXFORMAT_RGB565 &&
        frame->len >= expected) {
      detector.detect(reinterpret_cast<const uint16_t*>(frame->buf),
                      static_cast<int>(frame->width),
                      static_cast<int>(frame->height), observation,
                      BallVisionConfig::kSwapRgb565Bytes, sticky_color_);
      if (observation.found) {
        sticky_color_ = observation.color;
      }
      observation.framesize_code = framesize_code_;
      maybeAdjustFramesize(observation);
    }
    esp_camera_fb_return(frame);

    if (enabled_) {
      ++frames_in_window;
      const uint32_t now_ms = millis();
      if ((now_ms - window_start_ms) >= 1000u) {
        fps = static_cast<float>(frames_in_window) * 1000.0f /
              static_cast<float>(now_ms - window_start_ms);
        frames_in_window = 0;
        window_start_ms = now_ms;
      }
      observation.fps = fps;
      observation.timestamp_ms = now_ms;
      store_.publish(observation);
    }

    const uint32_t elapsed_ms = millis() - frame_start_ms;
    if (elapsed_ms < min_period_ms) {
      vTaskDelay(pdMS_TO_TICKS(min_period_ms - elapsed_ms));
    } else {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}

#else  // !ROSEYES_ENABLE_CAMERA_BALL

BallVisionService::BallVisionService(BallObservationStore& store,
                                     CameraJpegMailbox& jpeg_mailbox)
    : store_(store),
      jpeg_mailbox_(jpeg_mailbox),
      enabled_(false),
      started_(false) {
  (void)store_;
  (void)jpeg_mailbox_;
}

bool BallVisionService::begin() {
  return false;
}

void BallVisionService::setEnabled(bool enabled) {
  enabled_ = enabled;
}

bool BallVisionService::isEnabled() const {
  return false;
}

#endif
