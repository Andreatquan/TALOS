#pragma once
#include <Arduino.h>
#include "route.h"
#include "cameraTask.h"     // ANDREA: getVision(), steadyGreen(), visionFresh()
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
  LOOKING_FOR_LIGHT,      // stopped at a stop line, checking if there's a light
  WAITING_AT_LIGHT,       // stopped at a lit intersection, waiting for green
  CROSSING_INTERSECTION,  // turning or going straight (Luke's code moves the bot)
  EXITING_COURSE,         // route done, looking for the stop sign
  STOP                    // run is over (finished or failed)
};

// ---------------------------------------------------------------------
// Settings (all PLACEHOLDERS, tune on the real course)
// ---------------------------------------------------------------------
static const unsigned long GREEN_CONFIRM_MS     = 250;    // green must hold this long
static const unsigned long LIGHT_SEARCH_MS      = 1000;   // look this long before deciding "no light"
static const unsigned long LOCKOUT_HOLD_MS      = 1000;   // ignore stop lines after a crossing
static const unsigned long STOP_SIGN_TIMEOUT_MS = 15000;  // give up looking for stop sign

// ---------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------
static NavMode       mode          = DRIVING_ROUTE;
static int           routeIndex    = 0;      // which intersection is next
static unsigned long lookingSince  = 0;      // when LOOKING_FOR_LIGHT started
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
  startTurn(ROUTE[routeIndex]);          // LUKE: also handles GO_STRAIGHT
  mode = CROSSING_INTERSECTION;
}

// ---------------------------------------------------------------------
// Setup: reset everything, wait for start, start the timer
// ---------------------------------------------------------------------
inline void navigationSetup() {
  mode          = DRIVING_ROUTE;
  routeIndex    = 0;
  lookingSince  = 0;
  exitingSince  = 0;
  lockoutEndsAt = 0;
  wasBlocked    = false;

  stopMotors();
  waitForStartSignal();   // ARTURO: button + 5 s countdown
  startTimer();           // ARTURO: runs until the stop sign
}

// ---------------------------------------------------------------------
// Loop: called every loop from main.cpp (core 1).
// Never touches the camera. Reads the camera task's latest answer.
// ---------------------------------------------------------------------
inline void navigationLoop() {
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
        VisionResult v = getVision();

        if (steadyGreen(v, GREEN_CONFIRM_MS)) {
          Serial.println("Steady green at the line - no stop");
          enterIntersection();                         // green right now, keep going
        } else if (visionFresh(v) && v.light != NO_LIGHT) {
          Serial.println("Light seen, not steady green - stopping");
          stopMotors();                                // red, yellow, or green just started
          mode = WAITING_AT_LIGHT;
        } else {
          Serial.println("No light seen - stopping to look");
          stopMotors();                                // no light, or camera answer too old
          lookingSince = millis();
          mode = LOOKING_FOR_LIGHT;
        }
      } else {
        followLane(lane);                              // LUKE: steering + lane-lost handling
      }
      break;
    }

    // 2. Stopped at a stop line. Is there a light here or not?
    //    Any color seen      -> it's a lit intersection, wait for green.
    //    Nothing for a while -> no light here, go.
    //    Camera answer stale -> keep waiting (never go on a guess).
    case LOOKING_FOR_LIGHT: {
      stopMotors();
      VisionResult v = getVision();

      if (steadyGreen(v, GREEN_CONFIRM_MS)) {
        enterIntersection();
      } else if (visionFresh(v) && v.light != NO_LIGHT) {
        Serial.println("Light found - waiting for green");
        mode = WAITING_AT_LIGHT;
      } else if (!visionFresh(v)) {
        lookingSince = millis();   // camera answer too old: restart the search clock
      } else if (millis() - lookingSince >= LIGHT_SEARCH_MS) {
        Serial.println("No light here - going");
        enterIntersection();
      }
      break;
    }

    // 3. Stopped at a lit intersection. Go only on steady, fresh green.
    //    Red, yellow, unclear, and old answers all mean wait.
    case WAITING_AT_LIGHT:
      stopMotors();
      if (steadyGreen(getVision(), GREEN_CONFIRM_MS)) {
        enterIntersection();
      }
      break;

    // 4. Luke's code is moving the bot through the intersection
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

    // 5. Route finished. Drive until the stop sign.
    //    Stop sign is only acted on here, so nothing earlier can end the run.
    case EXITING_COURSE: {
      VisionResult v = getVision();
      if (visionFresh(v) && v.stopSign) {
        endRun("STOP SIGN - course complete");
      } else if (millis() - exitingSince > STOP_SIGN_TIMEOUT_MS) {
        endRun("STOP SIGN NOT FOUND - stopping");
      } else {
        driveForward();   // TODO: use followLane() if the exit has lane tape
      }
      break;
    }

    case STOP:
      break;   // handled at the top
  }
}