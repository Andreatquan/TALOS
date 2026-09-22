#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "route.h"
// #include "trafficLight.h"   // ANDREA: detectLightColor() 
// #include "stopSign.h"       // ANDREA: detectStopSign()    

// Teammates' code. Right now it all comes from stubs.h (fake versions).
// When real code arrives: delete that section of stubs.h, uncomment the real file.
#include "stubs.h"
// #include "sensors.h"       // ARTURO
// #include "power.h"         // ARTURO
// #include "startButton.h"   // ARTURO
// #include "timerDisplay.h"  // ARTURO
// #include "motorDrive.h"    // LUKE


// ---------------------------------------------------------------------
// States
// ---------------------------------------------------------------------
enum NavMode {
  DRIVING_ROUTE,          // following the lane between intersections
  WAITING_AT_LIGHT,       // stopped at a stop line, waiting for green
  CROSSING_INTERSECTION,  // turning or going straight (Luke's code moves the bot)
  EXITING_COURSE,         // route done, looking for the stop sign
  STOP                    // run is over (finished or failed)
};

// ---------------------------------------------------------------------
// Settings (all PLACEHOLDERS, tune on the real course)
// ---------------------------------------------------------------------
static const unsigned long GREEN_CONFIRM_MS     = 250;    // green must hold this long
static const unsigned long LOCKOUT_HOLD_MS      = 1000;   // ignore stop lines after a crossing
static const unsigned long STOP_SIGN_TIMEOUT_MS = 15000;  // give up looking for stop sign

// ---------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------
static NavMode       mode          = DRIVING_ROUTE;
static int           routeIndex    = 0;      // which intersection is next
static unsigned long greenSince    = 0;      // when green was first seen (0 = not green)
static unsigned long exitingSince  = 0;      // when the route finished
static unsigned long lockoutEndsAt = 0;      // ignore stop lines until this time
static bool          wasBlocked    = false;  // obstacle seen last loop

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------

// Ends the run: stop motors, stop timer, go to STOP.
inline void endRun(const char *reason) {
  Serial.println(reason);
  stopMotors();
  stopTimer();
  mode = STOP;
}

// Hands the intersection to Luke's code.
inline void enterIntersection() {
  Serial.println("Entering intersection");
  startTurn(ROUTE[routeIndex].action);   // LUKE: also handles GO_STRAIGHT
  mode = CROSSING_INTERSECTION;
}

// main.cpp can use this to skip grabbing a camera frame when Navigation
// doesn't need one. Skipping frames keeps lane-keeping fast.
inline bool navigationNeedsCamera() {
  return mode == WAITING_AT_LIGHT || mode == EXITING_COURSE;
}

// ---------------------------------------------------------------------
// Setup: reset everything, wait for start, start the timer
// ---------------------------------------------------------------------
inline void navigationSetup() {
  mode          = DRIVING_ROUTE;
  routeIndex    = 0;
  greenSince    = 0;
  exitingSince  = 0;
  lockoutEndsAt = 0;
  wasBlocked    = false;

  stopMotors();
  waitForStartSignal();   // ARTURO: button + 5 s countdown
  startTimer();           // ARTURO: runs until the stop sign
}

// ---------------------------------------------------------------------
// Loop: called every loop from main.cpp
// fb can be nullptr if main.cpp skipped the camera this loop.
// ---------------------------------------------------------------------
inline void navigationLoop(camera_fb_t *fb) {
  readFakeInputs();   // STUB-ONLY: delete this line when real sensors are in

  if (mode == STOP) {
    stopMotors();
    return;
  }

  // ---- Safety checks, every state ----
  // TODO: if battery goes low mid-intersection, finish the turn first? (open)
  if (isBatteryLow()) {
    endRun("BATTERY LOW - stopping");
    return;
  }

  // Pause while blocked, carry on when clear.
  // TODO: may false-trigger on poles or the stop sign (decisions doc section 4).
  // TODO: ask Luke whether a turn can be paused and resumed.
  if (detectObstacle()) {
    if (!wasBlocked) Serial.println("OBSTACLE - paused");
    wasBlocked = true;
    stopMotors();
    return;
  }
  if (wasBlocked) {
    Serial.println("Obstacle cleared - continuing");
    wasBlocked = false;
  }

  // ---- State machine ----
  switch (mode) {

    // 1. Follow the lane until a stop line
    case DRIVING_ROUTE: {
      LaneReading lane = readIRSensors();
      bool lockedOut = millis() < lockoutEndsAt;

      if (lane == LANE_STOP_LINE && !lockedOut) {
        Serial.printf("Stop line: intersection %d of %d\n", routeIndex + 1, ROUTE_LENGTH);
        if (ROUTE[routeIndex].hasLight) {
          stopMotors();                  // stop, then check the light
          greenSince = 0;
          mode = WAITING_AT_LIGHT;
        } else {
          enterIntersection();           // no light, just go
        }
      } else {
        followLane(lane);                // LUKE: steering + lane-lost handling
      }
      break;
    }

    // 2. Stopped at a lit intersection. Go only on steady green.
    //    Red, yellow, and unclear all mean wait.
    //    TODO (later): check the light on the approach instead of always stopping.
    case WAITING_AT_LIGHT: {
      stopMotors();
      bool green = (fb != nullptr) && (detectLightColor(fb) == GREEN_LIGHT);

      if (!green) {
        greenSince = 0;
        break;
      }
      if (greenSince == 0) greenSince = millis();
      if (millis() - greenSince >= GREEN_CONFIRM_MS) {
        greenSince = 0;
        enterIntersection();
      }
      break;
    }

    // 3. Luke's code is moving the bot through the intersection
    case CROSSING_INTERSECTION:
      if (isTurnComplete()) {
        routeIndex++;
        lockoutEndsAt = millis() + LOCKOUT_HOLD_MS;   // don't double-count

        if (routeIndex >= ROUTE_LENGTH) {
          Serial.println("Route done - looking for stop sign");
          exitingSince = millis();
          mode = EXITING_COURSE;
        } else {
          mode = DRIVING_ROUTE;   // LUKE's followLane() finds the line again
        }
      }
      break;

    // 4. Route finished. Drive until the stop sign.
    //    Stop sign is only checked here, so a red light can't end the run early.
    case EXITING_COURSE:
      if (fb != nullptr && detectStopSign(fb)) {
        endRun("STOP SIGN - course complete");
      } else if (millis() - exitingSince > STOP_SIGN_TIMEOUT_MS) {
        endRun("STOP SIGN NOT FOUND - stopping");
      } else {
        driveForward();   // TODO: use followLane() if the exit has lane tape
      }
      break;

    case STOP:
      break;   // handled at the top
  }
}