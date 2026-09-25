#pragma once

/**
 * Eye model selection for ROSEyes.
 *
 * Edit this file to choose which eyes are compiled into the firmware.
 * Uncomment exactly ONE model family. For Uncanny / robot packs, also
 * uncomment the matching include(s) below.
 *
 * Asset sources live in:
 *   resources/eyes/models/uncanny/       (Adafruit/Spotpear Animated Eyes)
 *   resources/eyes/models/robot_bitmap/  (dual_lcd_robot_eyes AI 160x160)
 */

// =============================================================================
// Pick exactly ONE of the three families:
// =============================================================================

/** Small procedural cartoon iris (no large flash tables). */
// #define ROSEYES_EYE_MODEL_PROCEDURAL

/** UncannyEyes layered textures (sclera/iris/lids). Uncomment ONE .h below. */
#define ROSEYES_EYE_MODEL_UNCANNY

/** Full-frame 160x160 RGB565 robot bitmaps. Edit selected_robot_frames.h. */
// #define ROSEYES_EYE_MODEL_ROBOT_BITMAP

// -----------------------------------------------------------------------------
// UncannyEyes packs — enable ROSEYES_EYE_MODEL_UNCANNY, then uncomment ONE:
// -----------------------------------------------------------------------------
#if defined(ROSEYES_EYE_MODEL_UNCANNY)
#include <Arduino.h>
#include <pgmspace.h>
// #include "uncanny/dragonEye.h"
//  #include "uncanny/noScleraEye.h"
// #include "uncanny/goatEye.h"
// //#include "uncanny/newtEye.h"
// #include "uncanny/terminatorEye.h"
// #include "uncanny/catEye.h"
// #include "uncanny/owlEye.h"
// #include "uncanny/naugaEye.h"
// #include "uncanny/doeEye.h"
#include "uncanny/defaultEye.h"
#endif

// -----------------------------------------------------------------------------
// Robot bitmap frames — enable ROSEYES_EYE_MODEL_ROBOT_BITMAP, then edit:
//   resources/eyes/models/robot_bitmap/selected_robot_frames.h
// -----------------------------------------------------------------------------
#if defined(ROSEYES_EYE_MODEL_ROBOT_BITMAP)
#include "robot_bitmap/selected_robot_frames.h"
#endif

#if defined(ROSEYES_EYE_MODEL_PROCEDURAL) + defined(ROSEYES_EYE_MODEL_UNCANNY) + defined(ROSEYES_EYE_MODEL_ROBOT_BITMAP) != 1
#error "EyeModelConfig.h: enable exactly one of PROCEDURAL / UNCANNY / ROBOT_BITMAP"
#endif
