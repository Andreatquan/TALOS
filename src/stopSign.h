#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "trafficLight.h"   // uses shared rgbToHsv()

// OV5640 camera captures one frame (RGB565)
//   ↓
// Scan the stop-sign crop (currently full frame)
//   ↓
// For each pixel: RGB565 → R,G,B → HSV → is it red?
//   ↓
// Count red pixels, per row and per column
//   ↓
// COLOR check: enough red pixels?
//   ↓
// SHAPE checks on the red area:
//   1. Draw a box around the red (ignoring stray pixels)
//   2. Box not cut off by the edge of the frame
//   3. Box roughly square          (octagon is as wide as tall)
//   4. Red fills enough of the box (one solid shape, not scattered red)
//   5. Flat edges                  (octagon has flat sides, a round
//                                   red traffic light does not)
//   ↓
// All pass → stop sign detected
//   ↓
// Navigation decides what to do
//-------------------------------------------------------------------------------------------------------------------------------

// --- Color: stop sign HSV thresholds --------------------------------
// Separate from traffic-light values because the sign reflects light.
static const int SIGN_SAT_MIN = 90;   // minimum color saturation
static const int SIGN_VAL_MIN = 60;   // minimum brightness

// Red wraps around hue 0, so use two red ranges.
static const int SIGN_RED_HUE_LOW_MAX  = 20;
static const int SIGN_RED_HUE_HIGH_MIN = 340;

// --- Size ------------------------------------------------------------
// Minimum red pixels needed. Higher = bot must get closer before stopping.
static const long SIGN_MIN_PIXELS = 4000;

// --- Shape (all PLACEHOLDERS, tune with the real sign) ---------------
// A row or column needs this many red pixels to count as part of the
// shape. Stops a few stray red pixels from stretching the box.
static const int   SIGN_EDGE_MIN_PIXELS = 3;

// Box width / height. 1.0 = perfect square.
static const float SIGN_ASPECT_MIN = 0.75f;
static const float SIGN_ASPECT_MAX = 1.33f;

// Red pixels / box area. The white "STOP" letters lower this.
static const float SIGN_FILL_MIN = 0.40f;

// How much of an edge must be red to count as flat.
// Octagon: about 0.41 of the box. Circle: much less.
static const float SIGN_FLAT_MIN = 0.30f;

// Flat edges needed out of 4. 3 allows one edge to be blocked or blurry.
static const int SIGN_FLAT_EDGES_NEEDED = 3;

// --- Camera crop -----------------------------------------------------
// Region of the frame checked for the stop sign. Max 240 x 240.
static const int SIGN_CROP_X0 = 0;
static const int SIGN_CROP_Y0 = 0;
static const int SIGN_CROP_X1 = 240;
static const int SIGN_CROP_Y1 = 240;

// --- Tuning info -----------------------------------------------------
// Pass a SignDebug to detectStopSign() in calibration mode to see why
// a frame passed or failed.
struct SignDebug {
  long  red;         // red pixels found
  int   width;       // box width  (0 = no box)
  int   height;      // box height (0 = no box)
  float fill;        // red / box area
  int   flatEdges;   // how many of 4 edges were flat
  const char *result;  // which check decided the answer
};

// Detects a red octagon (stop sign).
inline bool detectStopSign(camera_fb_t *fb, SignDebug *dbg = nullptr) {

  SignDebug d = {0, 0, 0, 0.0f, 0, ""};

  // Only process real RGB565 frames.
  if (fb == nullptr || fb->format != PIXFORMAT_RGB565) {
    d.result = "no frame";
    if (dbg) *dbg = d;
    return false;
  }

  uint16_t *px = (uint16_t *)fb->buf;

  // Keep crop inside the actual frame size.
  int x0 = SIGN_CROP_X0, y0 = SIGN_CROP_Y0;
  int x1 = min(SIGN_CROP_X1, (int)fb->width);
  int y1 = min(SIGN_CROP_Y1, (int)fb->height);

  // Red pixels in each row and each column.
  uint16_t rowRed[240] = {0};
  uint16_t colRed[240] = {0};

  // ---- Pass over the image: find and count red pixels ----
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {

      uint16_t p = px[y * fb->width + x];
      p = (uint16_t)((p >> 8) | (p << 8));   // camera sends bytes swapped: fix order


      int r = ((p >> 11) & 0x1F) << 3;
      int g = ((p >>  5) & 0x3F) << 2;
      int b = ( p        & 0x1F) << 3;

      int h, s, v;
      rgbToHsv(r, g, b, h, s, v);

      if (s < SIGN_SAT_MIN || v < SIGN_VAL_MIN) continue;

      if (h <= SIGN_RED_HUE_LOW_MAX || h >= SIGN_RED_HUE_HIGH_MIN) {
        d.red++;
        rowRed[y]++;
        colRed[x]++;
      }
    }
  }

  // ---- COLOR check: enough red? ----
  if (d.red < SIGN_MIN_PIXELS) {
    d.result = "not enough red";
    if (dbg) *dbg = d;
    return false;
  }

  // ---- SHAPE 1: box around the red ----
  int top = -1, bottom = -1, left = -1, right = -1;
  for (int y = y0; y < y1; y++) {
    if (rowRed[y] >= SIGN_EDGE_MIN_PIXELS) {
      if (top < 0) top = y;
      bottom = y;
    }
  }
  for (int x = x0; x < x1; x++) {
    if (colRed[x] >= SIGN_EDGE_MIN_PIXELS) {
      if (left < 0) left = x;
      right = x;
    }
  }
  if (top < 0 || left < 0) {
    d.result = "no shape";
    if (dbg) *dbg = d;
    return false;
  }

  d.width  = right - left + 1;
  d.height = bottom - top + 1;
  d.fill   = (float)d.red / (d.width * d.height);

  // ---- SHAPE 2: not cut off by the frame edge ----
  // A cut-off shape has a fake flat edge where the frame ends.
  if (top <= y0 || left <= x0 || bottom >= y1 - 1 || right >= x1 - 1) {
    d.result = "touches frame edge";
    if (dbg) *dbg = d;
    return false;
  }

  // ---- SHAPE 3: roughly square ----
  float aspect = (float)d.width / d.height;
  if (aspect < SIGN_ASPECT_MIN || aspect > SIGN_ASPECT_MAX) {
    d.result = "not square";
    if (dbg) *dbg = d;
    return false;
  }

  // ---- SHAPE 4: red fills enough of the box ----
  if (d.fill < SIGN_FILL_MIN) {
    d.result = "too hollow / scattered";
    if (dbg) *dbg = d;
    return false;
  }

  // ---- SHAPE 5: flat edges ----
  // Check the outermost row/column on each side. An octagon's edge is a
  // long flat line; a circle only touches the box at a short point.
  if (rowRed[top]    >= SIGN_FLAT_MIN * d.width)  d.flatEdges++;
  if (rowRed[bottom] >= SIGN_FLAT_MIN * d.width)  d.flatEdges++;
  if (colRed[left]   >= SIGN_FLAT_MIN * d.height) d.flatEdges++;
  if (colRed[right]  >= SIGN_FLAT_MIN * d.height) d.flatEdges++;

  if (d.flatEdges < SIGN_FLAT_EDGES_NEEDED) {
    d.result = "edges not flat (round?)";
    if (dbg) *dbg = d;
    return false;
  }

  d.result = "STOP SIGN";
  if (dbg) *dbg = d;
  return true;
}