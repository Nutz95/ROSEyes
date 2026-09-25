#pragma once

/**
 * Select which 160x160 robot eye frames are linked into the firmware.
 * Comment/uncomment includes, and keep the kFrames table in sync.
 *
 * Each included frame costs ~50 KB flash. Default: one neutral frame.
 */
#include <stddef.h>
#include <stdint.h>

#include "robot_eyes_01.h"
// #include "robot_eyes_02.h"
// #include "robot_eyes_03.h"
// #include "robot_eyes_04.h"
// #include "robot_eyes_05.h"
// #include "robot_eyes_06.h"
// #include "robot_eyes_07.h"
// #include "robot_eyes_08.h"
// #include "robot_eyes_09.h"
// #include "robot_eyes_10.h"
// #include "robot_eyes_11.h"
// #include "robot_eyes_12.h"
// #include "robot_eyes_13.h"
// #include "robot_eyes_14.h"
// #include "robot_eyes_15.h"
// #include "robot_eyes_16.h"
// #include "robot_eyes_17.h"
// #include "robot_eyes_18.h"
// #include "robot_eyes_19.h"
// #include "robot_eyes_20.h"
// #include "robot_eyes_21.h"
// #include "robot_eyes_22.h"
// #include "robot_eyes_23.h"
// #include "robot_eyes_24.h"
// #include "robot_eyes_25.h"
// #include "robot_eyes_26.h"
// #include "robot_eyes_27.h"
// #include "robot_eyes_28.h"
// #include "robot_eyes_29.h"
// #include "robot_eyes_30.h"

struct SelectedRobotFrames {
  static constexpr int kWidth = 160;
  static constexpr int kHeight = 160;
  static constexpr size_t kPixelCount = 25600;

  static const uint16_t* const* table(size_t* frame_count) {
    static const uint16_t* const kFrames[] = {
        image_data_robot_eyes_01,
        // image_data_robot_eyes_02,
        // image_data_robot_eyes_03,
        // image_data_robot_eyes_04,
        // image_data_robot_eyes_05,
        // image_data_robot_eyes_06,
        // image_data_robot_eyes_07,
        // image_data_robot_eyes_08,
        // image_data_robot_eyes_09,
        // image_data_robot_eyes_10,
        // image_data_robot_eyes_11,
        // image_data_robot_eyes_12,
        // image_data_robot_eyes_13,
        // image_data_robot_eyes_14,
        // image_data_robot_eyes_15,
        // image_data_robot_eyes_16,
        // image_data_robot_eyes_17,
        // image_data_robot_eyes_18,
        // image_data_robot_eyes_19,
        // image_data_robot_eyes_20,
        // image_data_robot_eyes_21,
        // image_data_robot_eyes_22,
        // image_data_robot_eyes_23,
        // image_data_robot_eyes_24,
        // image_data_robot_eyes_25,
        // image_data_robot_eyes_26,
        // image_data_robot_eyes_27,
        // image_data_robot_eyes_28,
        // image_data_robot_eyes_29,
        // image_data_robot_eyes_30,
    };
    *frame_count = sizeof(kFrames) / sizeof(kFrames[0]);
    return kFrames;
  }

  static const uint16_t* frame(size_t index) {
    size_t count = 0;
    const uint16_t* const* frames = table(&count);
    if (count == 0) {
      return nullptr;
    }
    return frames[index % count];
  }

  static size_t frameCount() {
    size_t count = 0;
    (void)table(&count);
    return count;
  }
};
