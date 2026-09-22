#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "img_converters.h"

#include "board_config.h"
#include "trafficLight.h"
#include "stopSign.h"

const char *ssid = "Andrea_iPhone";
const char *password = "abcde12345";

void startCameraServer();
void setupLedFlash();

WebServer server(80);

bool serverRunning = false;

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

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Sensor subsystem pulled out while calibrating the camera.
  // Restore once Sensors.cpp / Ultrasonic.cpp are back as modules.
  // setupObstacleSensor();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 24000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Frame size depends on format. The RGB565 path is what this project uses:
  // 240x240 = 57,600 pixels, which is what the detector thresholds assume.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    config.frame_size = FRAMESIZE_UXGA;
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  if (!psramFound()) {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
    Serial.println("No PSRAM found - using DRAM frame buffer");
  } else {
    Serial.println("PSRAM found");
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  //s->set_whitebal(s, 0);       // auto white balance off
  //s->set_awb_gain(s, 0);
  //s->set_exposure_ctrl(s, 0);  // auto exposure off
  //s->set_aec2(s, 0);
  //s->set_gain_ctrl(s, 0);      // auto gain off
  //s->set_agc_gain(s, 0);
  //s->set_aec_value(s, 400);    // TUNE THIS. Lower = darker frame.
                                // LED reads white w/ near-zero counts -> lower it.
                               // Frame too dark to register -> raise it.

  // Dead code on this board (OV5640, not OV3660). Left for reference.
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

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
    server.on("/", handleSnapshot);
    server.begin();
    serverRunning = true;
    Serial.print("Snapshot ready: http://");
    Serial.println(WiFi.localIP());
    Serial.println("Refresh the page for a fresh frame.");
  } else {
    Serial.println("WiFi failed - continuing with serial output only");
  }

  Serial.println("Camera ready - starting calibration output");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    delay(500);
    return;
  }

  if (fb->format == PIXFORMAT_RGB565) {
    uint16_t p = ((uint16_t *)fb->buf)[(fb->height / 2) * fb->width + (fb->width / 2)];
    uint16_t sw = (p >> 8) | (p << 8);

    Serial.printf("center  raw r=%3d g=%3d b=%3d  |  swap r=%3d g=%3d b=%3d\n",
      ((p  >> 11) & 0x1F) << 3, ((p  >> 5) & 0x3F) << 2, (p  & 0x1F) << 3,
      ((sw >> 11) & 0x1F) << 3, ((sw >> 5) & 0x3F) << 2, (sw & 0x1F) << 3);
  }

  long redCount = 0, yellowCount = 0, greenCount = 0;
  LightColor color = detectLightColor(fb, &redCount, &yellowCount, &greenCount);

  // Only print when the verdict changes, to keep the monitor readable.
  static LightColor lastColor = NO_LIGHT;
  if (color != lastColor) {
    Serial.printf("light   R:%5ld  Y:%5ld  G:%5ld  ->  %s\n",
                  redCount, yellowCount, greenCount, lightName(color));
    lastColor = color;
  }

  esp_camera_fb_return(fb);

  if (serverRunning) {
    server.handleClient();
  }

  delay(100);   // fast enough to aim the camera by refreshing the browser
}