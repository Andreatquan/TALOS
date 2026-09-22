#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "trafficLight.h"   // uses shared rgbToHsv()

// OV5640 camera capture one frame (RGB565)
//   ↓
// Take the stop-sign ROI (currently full frame)
//   ↓
// Read each RGB565 pixel
//   ↓
// Convert RGB565 → R,G,B → HSV
//   ↓
// Apply color thresholds
// (Is pixel bright enough?
// Is it saturated enough?
// Is hue red/yellow/green?)
//   ↓
// Count red pixels
//   ↓
// Compare red count to SIGN_MIN_PIXELS
//   ↓
// Enough red pixels?
//   ├─ YES → stop sign detected
//   └─ NO  → no stop sign yet
//   ↓
// Navigation decides what to do
//-------------------------------------------------------------------------------------------------------------------------------

// --- Stop sign HSV thresholds ----------------------------------------
// Separate from traffic-light values because the sign reflects light.
static const int SIGN_SAT_MIN = 90;   // minimum color saturation
static const int SIGN_VAL_MIN = 60;   // minimum brightness

// Red wraps around hue 0, so use two red ranges.
static const int SIGN_RED_HUE_LOW_MAX  = 20;
static const int SIGN_RED_HUE_HIGH_MIN = 340;

// Minimum red pixels needed to detect the sign.
// Higher value means the bot must get closer before stopping.
static const long SIGN_MIN_PIXELS = 4000;

// --- Camera crop ------------------------------------------------------
// Region of the frame checked for the stop sign.
// Currently scans the full 240x240 frame.
static const int SIGN_CROP_X0 = 0;
static const int SIGN_CROP_Y0 = 0;
static const int SIGN_CROP_X1 = 240;
static const int SIGN_CROP_Y1 = 240;

// Detects a large red region that represents the stop sign.
// outRed can return the red-pixel count for tuning.
inline bool detectStopSign(camera_fb_t *fb, long *outRed = nullptr) {

  // Only process RGB565 frames.
  if (fb->format != PIXFORMAT_RGB565) {
    if (outRed) *outRed = 0;
    return false;
  }

  uint16_t *px = (uint16_t *)fb->buf;
  long red = 0;

  // Keep crop inside the actual frame size.
  int x1 = min(SIGN_CROP_X1, (int)fb->width);
  int y1 = min(SIGN_CROP_Y1, (int)fb->height);

  for (int y = SIGN_CROP_Y0; y < y1; y++) {
    for (int x = SIGN_CROP_X0; x < x1; x++) {

      uint16_t p = px[y * fb->width + x];

      // Convert RGB565 pixel to 0-255 RGB.
      int r = ((p >> 11) & 0x1F) << 3;
      int g = ((p >>  5) & 0x3F) << 2;
      int b = ( p        & 0x1F) << 3;

      // Convert RGB to HSV.
      int h, s, v;
      rgbToHsv(r, g, b, h, s, v);

      // Ignore pixels that are too dim or unsaturated.
      if (s < SIGN_SAT_MIN || v < SIGN_VAL_MIN) continue;

      // Count red pixels.
      if (h <= SIGN_RED_HUE_LOW_MAX || h >= SIGN_RED_HUE_HIGH_MIN) {
        red++;
      }
    }
  }

  // Return raw pixel count if requested.
  if (outRed) *outRed = red;

  // Sign detected when enough red pixels are visible.
  return red >= SIGN_MIN_PIXELS;
}