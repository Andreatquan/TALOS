#pragma once
#include <Arduino.h>
#include "route.h"

// =====================================================================
// STUBS: fake versions of Luke's and Arturo's functions so Navigation
// runs today. Type letters in the Serial Monitor to fake events:
//
//   p = press the start button
//   s = stop line under the front IR (one reading)
//   l = lane lost (one reading)
//   o = obstacle on/off
//   b = battery low on/off
//
// When a teammate's real code arrives: delete their section below and
// uncomment their #include in navigation.h. Keep function names the same.
// =====================================================================

// Shared type: what the IR sensors report. (Moves to Arturo's sensors.h.)
enum LaneReading {
  LANE_CENTERED,
  LANE_DRIFT_LEFT,
  LANE_DRIFT_RIGHT,
  LANE_STOP_LINE,
  LANE_LOST
};

// ---- Fake input state (STUB-ONLY) ----
static bool fakeStartPressed = false;
static bool fakeStopLine     = false;
static bool fakeLaneLost     = false;
static bool fakeObstacle     = false;
static bool fakeBatteryLow   = false;

// Reads Serial Monitor letters. Called at the top of navigationLoop().
inline void readFakeInputs() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'p': fakeStartPressed = true;            Serial.println("[fake] start pressed");  break;
      case 's': fakeStopLine     = true;            Serial.println("[fake] stop line");      break;
      case 'l': fakeLaneLost     = true;            Serial.println("[fake] lane lost");      break;
      case 'o': fakeObstacle     = !fakeObstacle;   Serial.printf("[fake] obstacle %s\n", fakeObstacle ? "ON" : "OFF");  break;
      case 'b': fakeBatteryLow   = !fakeBatteryLow; Serial.printf("[fake] battery low %s\n", fakeBatteryLow ? "ON" : "OFF"); break;
      default: break;
    }
  }
}

// =====================================================================
// STUB(ARTURO): sensors.h
// =====================================================================
inline LaneReading readIRSensors() {
  if (fakeStopLine) { fakeStopLine = false; return LANE_STOP_LINE; }
  if (fakeLaneLost) { fakeLaneLost = false; return LANE_LOST; }
  return LANE_CENTERED;
}

inline bool detectObstacle() { return fakeObstacle; }

// =====================================================================
// STUB(ARTURO): power.h
// =====================================================================
inline bool isBatteryLow() { return fakeBatteryLow; }

// =====================================================================
// STUB(ARTURO): startButton.h
// Blocks until start is pressed, then counts down 5 seconds.
// =====================================================================
inline void waitForStartSignal() {
  Serial.println("[stub] waiting for start button - type p");
  while (!fakeStartPressed) {
    readFakeInputs();
    delay(10);
  }
  for (int i = 5; i > 0; i--) {
    Serial.printf("[stub] %d...\n", i);
    delay(1000);
  }
  Serial.println("[stub] GO");
}

// =====================================================================
// STUB(ARTURO): timerDisplay.h
// =====================================================================
static unsigned long fakeTimerStart = 0;

inline void startTimer() {
  fakeTimerStart = millis();
  Serial.println("[stub] timer started");
}

inline void stopTimer() {
  unsigned long tenths = (millis() - fakeTimerStart) / 100;
  Serial.printf("[stub] timer stopped: %lu.%lu s\n", tenths / 10, tenths % 10);
}

// =====================================================================
// STUB(LUKE): motorDrive.h
// =====================================================================
static const unsigned long FAKE_TURN_MS = 1500;   // pretend a turn takes 1.5 s
static unsigned long fakeTurnStartedAt = 0;

inline void stopMotors()   { /* real code: both motors off */ }
inline void driveForward() { /* real code: both motors forward */ }

// Keeps the bot in its lane. Also handles LANE_LOST.
inline void followLane(LaneReading lane) {
  if (lane == LANE_LOST) Serial.println("[stub] lane lost - Luke's code handles this");
}

// Starts a turn (or straight crossing) through the intersection.
inline void startTurn(RouteStep dir) {
  const char *name = (dir == TURN_LEFT) ? "LEFT" : (dir == TURN_RIGHT) ? "RIGHT" : "STRAIGHT";
  Serial.printf("[stub] turn started: %s\n", name);
  fakeTurnStartedAt = millis();
}

inline bool isTurnComplete() {
  if (millis() - fakeTurnStartedAt >= FAKE_TURN_MS) {
    Serial.println("[stub] turn complete");
    return true;
  }
  return false;
}