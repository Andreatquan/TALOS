#pragma once
#include <Arduino.h>
#include "esp_camera.h"

// OV5640 capture frame (RGB565)
//  ↓
// Take ROI (top half of the frame)
//  ↓
// Read each RGB565 pixel
//  ↓
// Convert RGB565 → R,G,B → HSV
//  ↓
// Apply color thresholds
// (Is pixel bright enough?
//  Is it saturated enough?
//  Is hue red/yellow/green?)
//  ↓
// Count & compare red, yellow, green pixels
//  ↓
// Return RED_LIGHT / YELLOW_LIGHT / GREEN_LIGHT / NO_LIGHT
//  ↓
// cameraTask writes it on the note → Navigation decides what to do
//---------------------------------------------------------------------------------------------------------------------------------------

// Possible traffic light results
enum LightColor { NO_LIGHT, RED_LIGHT, YELLOW_LIGHT, GREEN_LIGHT };

// Answer into print text
inline const char* lightName(LightColor c) {
  switch (c) {
    case RED_LIGHT:    return "RED";
    case YELLOW_LIGHT: return "YELLOW";
    case GREEN_LIGHT:  return "GREEN";
    default:           return "NONE";
  }
}

// --- ROI --------------------------------------------------------------
// Scan only the top half of the frame.
static const float ROI_Y_START = 0.0f;
static const float ROI_Y_END   = 0.5f;
static const float ROI_X_START = 0.0f;
static const float ROI_X_END   = 1.0f;

// --- HSV thresholds ---------------------------------------------------
// Minimum saturation and brightness for a valid LED pixel.
static const int SAT_MIN = 120;
static const int VAL_MIN = 120;

// Hue ranges in degrees.
static const int RED_HUE_LOW_MAX   = 15;
static const int RED_HUE_HIGH_MIN  = 345;
static const int YELLOW_HUE_MIN    = 35;
static const int YELLOW_HUE_MAX    = 70;
static const int GREEN_HUE_MIN     = 80;
static const int GREEN_HUE_MAX     = 165;

// Minimum pixels needed to trust a color (tuned for 240x240).
static const int MIN_PIXELS = 40;

// Converts RGB to HSV. Shared with stopSign.h.
inline void rgbToHsv(int r, int g, int b, int &h, int &s, int &v) {
  int maxC = max(r, max(g, b));
  int minC = min(r, min(g, b));
  int delta = maxC - minC;

  v = maxC;
  s = (maxC == 0) ? 0 : (delta * 255) / maxC;

  if (delta == 0) {
    h = 0;
  } else if (maxC == r) {
    h = 60 * (g - b) / delta;
  } else if (maxC == g) {
    h = 60 * (b - r) / delta + 120;
  } else {
    h = 60 * (r - g) / delta + 240;
  }

  if (h < 0) h += 360;
}

// Detects red, yellow, or green within the ROI.
inline LightColor detectLightColor(camera_fb_t *fb,
                                   long *outRed    = nullptr,
                                   long *outYellow = nullptr,
                                   long *outGreen  = nullptr) {

  // Only process real RGB565 frames.
  if (fb == nullptr || fb->format != PIXFORMAT_RGB565) {
    if (outRed)    *outRed    = 0;
    if (outYellow) *outYellow = 0;
    if (outGreen)  *outGreen  = 0;
    return NO_LIGHT;
  }

  uint16_t *px = (uint16_t *)fb->buf;
  long red = 0, yellow = 0, green = 0;

  // Convert ROI percentages to pixel positions.
  int y0 = (int)(fb->height * ROI_Y_START);
  int y1 = (int)(fb->height * ROI_Y_END);
  int x0 = (int)(fb->width  * ROI_X_START);
  int x1 = (int)(fb->width  * ROI_X_END);

  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {

      uint16_t p = px[y * fb->width + x];
      p = (uint16_t)((p >> 8) | (p << 8));   // camera sends bytes swapped: fix order


      // Convert RGB565 to RGB.
      int r = ((p >> 11) & 0x1F) << 3;
      int g = ((p >>  5) & 0x3F) << 2;
      int b = ( p        & 0x1F) << 3;

      int h, s, v;
      rgbToHsv(r, g, b, h, s, v);

      // Ignore dull or dark pixels.
      if (s < SAT_MIN || v < VAL_MIN) continue;

      // Count matching colors.
      if (h <= RED_HUE_LOW_MAX || h >= RED_HUE_HIGH_MIN) {
        red++;
      } else if (h >= YELLOW_HUE_MIN && h <= YELLOW_HUE_MAX) {
        yellow++;
      } else if (h >= GREEN_HUE_MIN && h <= GREEN_HUE_MAX) {
        green++;
      }
    }
  }

  // Return raw counts if requested.
  if (outRed)    *outRed    = red;
  if (outYellow) *outYellow = yellow;
  if (outGreen)  *outGreen  = green;

  // Return the strongest valid color. Ties = NO_LIGHT (wait).
  if (red >= MIN_PIXELS && red > yellow && red > green)       return RED_LIGHT;
  if (yellow >= MIN_PIXELS && yellow > red && yellow > green) return YELLOW_LIGHT;
  if (green >= MIN_PIXELS && green > red && green > yellow)   return GREEN_LIGHT;

  return NO_LIGHT;
}