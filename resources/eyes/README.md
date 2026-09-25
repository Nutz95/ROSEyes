# Eye models (selectable assets)

Firmware picks the active eye pack in
[`firmware/include/EyeModelConfig.h`](../../firmware/include/EyeModelConfig.h)
by commenting / uncommenting defines and includes (same pattern as Spotpear
`Animated_Eyes_2/config.h`).

## Layout

| Path | Origin | Format |
|------|--------|--------|
| `models/uncanny/*.h` | Spotpear / Adafruit UncannyEyes | Layered sclera + iris map + eyelids |
| `models/robot_bitmap/robot_eyes_XX.h` | `dual_lcd_robot_eyes` AI set | 160×160 RGB565 full frames (~50 KB flash each) |
| `models/robot_bitmap/selected_robot_frames.h` | ROSEyes | Which robot frames are linked |

## How to switch

1. Open `firmware/include/EyeModelConfig.h`.
2. Leave **exactly one** of:
   - `ROSEYES_EYE_MODEL_PROCEDURAL`
   - `ROSEYES_EYE_MODEL_UNCANNY`
   - `ROSEYES_EYE_MODEL_ROBOT_BITMAP`
3. For Uncanny: uncomment one `#include "uncanny/....h"`.
4. For robot bitmaps: edit `selected_robot_frames.h` (includes + `kFrames` + `frameCount()`).
5. Rebuild / OTA.

## Flash budget (XIAO ESP32-S3, 8 MB)

- Procedural: negligible
- One Uncanny pack: ~150–400 KB
- Each robot frame: ~50 KB (all 30 ≈ 1.5 MB — fine, but slower compiles)

Upstream demos (not compiled into ROSEyes): `0.71inch EYE Board/`.
