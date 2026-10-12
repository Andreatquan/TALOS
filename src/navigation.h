#pragma once
#include <Arduino.h>
#include "route.h"          // ANDREA: the list of turns
#include "cameraTask.h"     // ANDREA: getVision(), steadyGreen(), visionFresh()
// #include "laneReading.h"    // what the IR sensors can report
#include "MotorControls.h"  // LUKE
// #include "sensors.h"     // ARTURO (uncomment when his file is in src/)

// Luke's encoder pulse counters (they live in MotorControls.cpp).
extern volatile uint32_t pulseCountR;
extern volatile uint32_t pulseCountL;

// ---- Motor settings (Luke: tune on the real bot) ----
const int PULSES_INTO_INTERSECTION = 60;    // drive this far past the line before pivoting
const int PULSES_STRAIGHT          = 250;   // drive this far to cross going straight
const int STEER_SLOW               = 40;    // how much to slow one wheel to steer back
const int LOST_SPEED               = 120;   // slower speed when the lane is lost
const unsigned long TURN_TIMEOUT_MS = 8000; // encoder unplugged? don't turn forever

MotorControls *motors = nullptr;   // Luke's motor object

// Call once from setup(): runs Luke's setup (pins, PWM, encoders).
void setupMotors() {
  motors = new MotorControls();
}

void stopMotors() {
  motors->stopMotorR();
  motors->stopMotorL();
}

void driveForward() {
  motors->startBothMotors(PWM_SPEED);
}

// Stay in the lane: slow one wheel to curve back.
void followLane(LaneReading lane) {
  if (lane == LANE_DRIFT_LEFT) {            // too far left: curve right
    motors->startMotorL(PWM_SPEED);
    motors->startMotorR(PWM_SPEED - STEER_SLOW);
  } else if (lane == LANE_DRIFT_RIGHT) {    // too far right: curve left
    motors->startMotorL(PWM_SPEED - STEER_SLOW);
    motors->startMotorR(PWM_SPEED);
  } else if (lane == LANE_LOST) {           // lost: slow down, go straight
    motors->startBothMotors(LOST_SPEED);
  } else {                                  // centered
    driveForward();
  }
}

// ---- Turns: start once, then ask isTurnComplete() every loop ----
RouteStep     turnDir       = GO_STRAIGHT;
bool          pivoting      = false;   // false = driving into the intersection
unsigned long turnStartedAt = 0;

// Start the move for this intersection, then return right away.
void startTurn(RouteStep dir) {
  turnDir       = dir;
  pivoting      = false;
  turnStartedAt = millis();
  motors->resetEncoders();
  driveForward();
}

// Is the move done? Restarts the wheels each call, so a turn
// picks up again after an obstacle pause.
bool isTurnComplete() {
  if (millis() - turnStartedAt > TURN_TIMEOUT_MS) {   // safety
    Serial.println("Turn timeout - check encoders");
    stopMotors();
    return true;
  }

  uint32_t r = pulseCountR;
  uint32_t l = pulseCountL;

  // Going straight: drive until far enough.
  if (turnDir == GO_STRAIGHT) {
    if ((r + l) / 2 >= PULSES_STRAIGHT) { stopMotors(); return true; }
    driveForward();
    return false;
  }

  // Turning, part 1: drive into the intersection.
  if (!pivoting) {
    if ((r + l) / 2 >= PULSES_INTO_INTERSECTION) {
      motors->resetEncoders();
      pivoting = true;
    } else {
      driveForward();
    }
    return false;
  }

  // Turning, part 2: pivot. One wheel drives, the other stays still.
  if (turnDir == TURN_RIGHT) {              // right turn: LEFT wheel drives
    if (l >= PULSES_PER_TURN) { stopMotors(); return true; }
    motors->startMotorL(PWM_SPEED);
    motors->stopMotorR();
  } else {                                  // left turn: RIGHT wheel drives
    if (r >= PULSES_PER_TURN) { stopMotors(); return true; }
    motors->startMotorR(PWM_SPEED);
    motors->stopMotorL();
  }
  return false;
}

// =====================================================================
// NAVIGATION: decides what the bot does. Runs on core 1, over and over.
// =====================================================================

enum NavMode { DRIVING, AT_STOP_LINE, CROSSING, EXITING, DONE };

// ---- Settings (tune on the real course) ----
const unsigned long GREEN_HOLD_MS  = 250;    // green must last this long
const unsigned long NO_LIGHT_MS    = 1000;   // see nothing this long = no light here
const unsigned long LOCKOUT_MS     = 1000;   // ignore stop lines after a crossing
const unsigned long SIGN_GIVEUP_MS = 15000;  // stop anyway if no stop sign

NavMode       mode         = DRIVING;
int           routeIndex   = 0;      // which intersection is next
bool          sawLight     = false;  // saw a light color at this stop line
unsigned long timer        = 0;      // start time for the current wait
unsigned long lockoutUntil = 0;      // ignore stop lines until this time

// Start the turn for this intersection (Luke's code does the moving).
void go() {
  Serial.println("Go");
  startTurn(ROUTE[routeIndex]);
  mode = CROSSING;
}

// End the run.
void finish(const char *why) {
  Serial.println(why);
  stopMotors();
  stopTimer();
  mode = DONE;
}

// ---------------------------------------------------------------------
// Runs once: wait for the start button, then start the timer.
// ---------------------------------------------------------------------
void navigationSetup() {
  stopMotors();
  waitForStartSignal();   // ARTURO: button + 5 s countdown
  startTimer();           // ARTURO
}

// ---------------------------------------------------------------------
// Runs over and over. Each call makes one quick decision and returns.
// ---------------------------------------------------------------------
void navigationLoop() {

  // ---- Safety, checked first every time ----
  if (mode == DONE)      { stopMotors(); return; }
  if (isBatteryLow())    { finish("Battery low"); return; }
  if (detectObstacle())  { stopMotors(); return; }   // wait until it clears

  VisionResult v = getVision();   // camera's latest answer

  switch (mode) {

    // 1. Stay in the lane until a new stop line.
    case DRIVING: {
      LaneReading lane = readIRSensors();
      bool newStopLine = (lane == LANE_STOP_LINE) && (millis() > lockoutUntil);

      if (!newStopLine) {
        followLane(lane);                      // LUKE: steer
      } else if (steadyGreen(v, GREEN_HOLD_MS)) {
        go();                                  // green: don't even stop
      } else {
        stopMotors();                          // anything else: stop and check
        sawLight = false;
        timer = millis();
        mode = AT_STOP_LINE;
      }
      break;
    }

    // 2. Stopped at the line. Go on steady green, or if there's no light.
    case AT_STOP_LINE:
      stopMotors();

      if (steadyGreen(v, GREEN_HOLD_MS)) {
        go();                                  // green: go
      } else if (!visionFresh(v)) {
        timer = millis();                      // camera answer too old: start over
      } else if (v.light != NO_LIGHT) {
        sawLight = true;                       // red / yellow / new green: wait
      } else if (!sawLight && millis() - timer >= NO_LIGHT_MS) {
        go();                                  // 1 s of nothing: no light here
      }
      break;

    // 3. Luke's code is turning. When done, next intersection.
    case CROSSING:
      if (isTurnComplete()) {
        routeIndex++;
        lockoutUntil = millis() + LOCKOUT_MS;  // don't count the same line twice

        if (routeIndex >= ROUTE_LENGTH) {
          timer = millis();
          mode = EXITING;                      // last turn done
        } else {
          mode = DRIVING;
        }
      }
      break;

    // 4. After the last turn: drive until the stop sign.
    case EXITING:
      if (visionFresh(v) && v.stopSign)           finish("Stop sign - done");
      else if (millis() - timer > SIGN_GIVEUP_MS) finish("No stop sign found");
      else                                        driveForward();
      break;

    case DONE:
      break;
  }
}