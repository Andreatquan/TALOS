#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "trafficLight.h"   // ANDREA: detectLightColor()
#include "stopSign.h"       // ANDREA: detectStopSign()

// =====================================================================
// CAMERA TASK: runs on core 0, forever, in the background.
//
//   Core 0 (this file)                 Core 1 (navigation.h)
//   ------------------                 ---------------------
//   grab frame                         read IR, steer, count stop lines
//   detect light + stop sign           ...never waits for the camera
//   save the answer  ───────────────►  getVision() reads the latest answer
//   repeat
//
// Navigation never touches the camera. It only reads the saved answer.
// =====================================================================

// ---------------------------------------------------------------------
// Settings (PLACEHOLDERS, tune on the real bot)
// ---------------------------------------------------------------------
// An answer older than this is ignored (treated as "unclear").
// Must be longer than one camera frame takes. Check visionFrameMs.
static const unsigned long VISION_MAX_AGE_MS = 200;

// ---------------------------------------------------------------------
// The latest answer, shared between the two cores
// ---------------------------------------------------------------------
struct VisionResult {
  LightColor    light;        // latest light color
  unsigned long greenSince;   // when green started (0 = not green)
  bool          stopSign;     // stop sign in the latest frame
  unsigned long updatedAt;    // when this answer was saved (0 = never)
};

static VisionResult  visionShared  = {NO_LIGHT, 0, false, 0};
static unsigned long visionFrameMs = 0;   // how long the last frame took (for tuning)

// Lock so one core never reads the answer while the other is halfway
// through writing it.
static portMUX_TYPE visionLock = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------------------
// For Navigation: read a copy of the latest answer
// ---------------------------------------------------------------------
inline VisionResult getVision() {
  portENTER_CRITICAL(&visionLock);
  VisionResult copy = visionShared;
  portEXIT_CRITICAL(&visionLock);
  return copy;
}

// Is this answer recent enough to trust?
inline bool visionFresh(const VisionResult &v) {
  return v.updatedAt != 0 && (millis() - v.updatedAt) <= VISION_MAX_AGE_MS;
}

// Fresh, green, and green has held for at least holdMs.
inline bool steadyGreen(const VisionResult &v, unsigned long holdMs) {
  return visionFresh(v)
      && v.light == GREEN_LIGHT
      && v.greenSince != 0
      && (millis() - v.greenSince) >= holdMs;
}

// For tuning: how long one frame takes to grab + detect.
inline unsigned long getVisionFrameMs() {
  portENTER_CRITICAL(&visionLock);
  unsigned long ms = visionFrameMs;
  portEXIT_CRITICAL(&visionLock);
  return ms;
}

// ---------------------------------------------------------------------
// The task itself: loops forever on core 0
// ---------------------------------------------------------------------
inline void cameraTask(void *) {
  for (;;) {
    unsigned long start = millis();

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {                        // capture failed: answer goes stale,
      vTaskDelay(pdMS_TO_TICKS(20));  // so Navigation treats it as unclear
      continue;
    }

    LightColor color = detectLightColor(fb);
    bool       sign  = detectStopSign(fb);
    esp_camera_fb_return(fb);         // always give the frame back

    unsigned long now = millis();

    portENTER_CRITICAL(&visionLock);
    visionShared.light = color;
    if (color == GREEN_LIGHT) {
      if (visionShared.greenSince == 0) visionShared.greenSince = now;
    } else {
      visionShared.greenSince = 0;    // any non-green resets the green timer
    }
    visionShared.stopSign  = sign;
    visionShared.updatedAt = now;
    visionFrameMs          = now - start;
    portEXIT_CRITICAL(&visionLock);

    vTaskDelay(1);   // short pause so core 0's system jobs can run
  }
}

// Call once from setup(), after the camera starts.
inline bool startCameraTask() {
  BaseType_t ok = xTaskCreatePinnedToCore(
      cameraTask,   // function to run
      "camera",     // name (for debugging)
      8192,         // stack size in bytes
      nullptr,      // no input
      1,            // priority
      nullptr,      // no handle needed
      0);           // core 0 (Navigation runs in loop() on core 1)
  return ok == pdPASS;
}