#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "img_converters.h"

#include "board_config.h"    // camera pin numbers
#include "trafficLight.h"    // ANDREA: detectLightColor()
#include "stopSign.h"        // ANDREA: detectStopSign()
#include "cameraTask.h"      // ANDREA: camera + detection on core 0
#include "navigation.h"      // ANDREA: navigationSetup(), navigationLoop()

// =====================================================================
// MODE SWITCH
//   1 = CALIBRATION: camera tuning. WiFi snapshot page + detection
//                    numbers in the Serial Monitor. Bot does NOT drive.
//   0 = RUN:         the real course. No WiFi. Navigation drives the bot.
// =====================================================================
#define CALIBRATION_MODE 0

// Set to 1 if the snapshot page shows the picture upside down.
// Both detectors crop by position, so the picture must be upright.
#define CAMERA_FLIP_VERTICAL 1

bool cameraReady = false;

// =====================================================================
// CAMERA SETUP (both modes)
// =====================================================================
bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;   // TODO: GPIO14 clashes with IR DS1 on the PCB
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 24000000;

  config.pixel_format = PIXFORMAT_RGB565;    // detectors read RGB565
  config.frame_size   = FRAMESIZE_240X240;   // detector thresholds assume 240x240
  config.jpeg_quality = 12;                  // unused for RGB565

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count    = 2;
    config.grab_mode   = CAMERA_GRAB_LATEST;      // always the newest frame
    Serial.println("PSRAM found");
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count    = 1;
    config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
    Serial.println("No PSRAM found - using DRAM frame buffer");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();

#if CAMERA_FLIP_VERTICAL
  s->set_vflip(s, 1);
#endif

  // Exposure lock for tuning. Uncomment once you calibrate on real lights.
  //s->set_whitebal(s, 0);       // auto white balance off
  //s->set_awb_gain(s, 0);
  //s->set_exposure_ctrl(s, 0);  // auto exposure off
  //s->set_aec2(s, 0);
  //s->set_gain_ctrl(s, 0);      // auto gain off
  //s->set_agc_gain(s, 0);
  //s->set_aec_value(s, 400);    // TUNE THIS. Lower = darker frame.

  Serial.println("Camera ready");
  return true;
}

// =====================================================================
// CALIBRATION MODE: WiFi snapshot + detection numbers
// =====================================================================
#if CALIBRATION_MODE

const char *ssid     = "Andrea_iPhone";
const char *password = "abcde12345";    

WebServer server(80);
bool serverRunning = false;

// Latest detection results, shown on the live page.
String lastInfo = "waiting for first frame...";

// Live page: reloads the picture and the detection numbers automatically.
// Open http://<board IP>/ once and leave it open.
void handleRoot() {
  server.send(200, "text/html",
    "<html><body style='background:#111;color:#eee;font-family:monospace'>"
    "<h3>TALOS camera</h3>"
    "<img id='cam' width='480' height='480' style='image-rendering:pixelated'><br>"
    "<pre id='info'></pre>"
    "<script>"
    "function tick(){"
    "  var img=new Image();"
    "  img.onload=function(){document.getElementById('cam').src=img.src;"
    "    fetch('/info').then(r=>r.text()).then(t=>{document.getElementById('info').textContent=t;});"
    "    setTimeout(tick,300);};"
    "  img.onerror=function(){setTimeout(tick,1000);};"
    "  img.src='/snap?t='+Date.now();"
    "}"
    "tick();"
    "</script></body></html>");
}

// Plain text of the latest detection numbers.
void handleInfo() {
  server.send(200, "text/plain", lastInfo);
}

// One fresh camera frame as a JPEG (the live page loads this over and over).
void handleSnapshot() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(503, "text/plain", "Capture failed");
    return;
  }

  uint8_t *jpg = NULL;
  size_t jpgLen = 0;
  bool ok = frame2jpg(fb, 80, &jpg, &jpgLen);
  esp_camera_fb_return(fb);

  if (!ok) {
    server.send(500, "text/plain", "JPEG conversion failed");
    return;
  }

  server.setContentLength(jpgLen);
  server.send(200, "image/jpeg", "");
  server.client().write(jpg, jpgLen);
  free(jpg);
}

void setupWifi() {
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    server.on("/", handleRoot);                  // live page
    server.on("/snap", handleSnapshot);          // one JPEG frame
    server.on("/info", handleInfo);              // detection numbers
    server.on("/favicon.ico", []() { server.send(204); });   // silences the browser icon error
    server.begin();
    serverRunning = true;
    Serial.print("Live view: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed - serial output only");
  }
}

// Runs both detectors on every frame and prints the numbers
// whenever either answer changes.
void calibrationLoop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    delay(500);
    return;
  }

  unsigned long detectStart = millis();

  // Traffic light
  long redCount = 0, yellowCount = 0, greenCount = 0;
  LightColor color = detectLightColor(fb, &redCount, &yellowCount, &greenCount);

  // Stop sign
  SignDebug sd;
  detectStopSign(fb, &sd);

  unsigned long detectMs = millis() - detectStart;   // time for both detectors

  // Save the numbers for the live page (every frame).
  char buf[200];
  snprintf(buf, sizeof(buf),
           "light  R:%5ld  Y:%5ld  G:%5ld  -> %s\n"
           "sign   red:%5ld  box:%dx%d  fill:%.2f  flat:%d  -> %s\n"
           "time   %lu ms",
           redCount, yellowCount, greenCount, lightName(color),
           sd.red, sd.width, sd.height, sd.fill, sd.flatEdges, sd.result,
           detectMs);
  lastInfo = buf;

  static LightColor  lastColor = NO_LIGHT;
  static const char *lastSign  = nullptr;
  if (color != lastColor || sd.result != lastSign) {
    Serial.printf("light  R:%5ld  Y:%5ld  G:%5ld  -> %s\n",
                  redCount, yellowCount, greenCount, lightName(color));
    Serial.printf("sign   red:%5ld  box:%dx%d  fill:%.2f  flat:%d  -> %s\n",
                  sd.red, sd.width, sd.height, sd.fill, sd.flatEdges, sd.result);
    Serial.printf("time   both detectors: %lu ms\n", detectMs);
    lastColor = color;
    lastSign  = sd.result;
  }

  esp_camera_fb_return(fb);

  if (serverRunning) server.handleClient();

  delay(100);   // fast enough to aim the camera by refreshing the browser
}

#endif

// =====================================================================
// RUN MODE: camera task on core 0, Navigation here on core 1
// =====================================================================
// Set to 1 to print how long each camera frame takes, once a second.
// VISION_MAX_AGE_MS in cameraTask.h must be longer than this number.
#define PRINT_CAMERA_SPEED 1

void runLoop() {
  navigationLoop();   // reads IR, steers, reads the camera task's latest answer

#if PRINT_CAMERA_SPEED
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    Serial.printf("[camera] %lu ms per frame\n", getVisionFrameMs());
    lastPrint = millis();
  }
#endif
}

// =====================================================================
// SETUP + LOOP
// =====================================================================
void setup() {
  Serial.begin(115200);
  Serial.println();

  cameraReady = setupCamera();

#if CALIBRATION_MODE
  Serial.println("=== CALIBRATION MODE (bot will not drive) ===");
  setupWifi();
#else
  Serial.println("=== RUN MODE ===");
  if (!cameraReady) {
    Serial.println("Camera failed - bot will NOT drive");
    return;                      // no camera = can't see lights, so never start
  }
  if (!startCameraTask()) {      // camera + detection now run on core 0
    Serial.println("Camera task failed to start - bot will NOT drive");
    cameraReady = false;
    return;
  }
  navigationSetup();             // waits for start button + countdown, starts timer
#endif
}

void loop() {
  if (!cameraReady) {
    delay(1000);
    return;
  }

#if CALIBRATION_MODE
  calibrationLoop();
#else
  runLoop();
#endif
}