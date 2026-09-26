#pragma once

/**
 * Feature switches for optional firmware components.
 * PlatformIO envs set ROSEYES_ENABLE_MICROROS to 0 (eyes-only) or 1 (full).
 */
#ifndef ROSEYES_ENABLE_MICROROS
#define ROSEYES_ENABLE_MICROROS 0
#endif

/** Onboard OV2640 ball tracking (Sense). PlatformIO envs set 0 or 1. */
#ifndef ROSEYES_ENABLE_CAMERA_BALL
#define ROSEYES_ENABLE_CAMERA_BALL 0
#endif
