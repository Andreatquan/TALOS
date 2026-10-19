#pragma once
#include <Arduino.h>
#include "route.h"          // ANDREA: the list of turns
#include "cameraTask.h"     // ANDREA: getVision(), steadyGreen(), visionFresh()
// #include "laneReading.h"    // what the IR sensors can report
#include "MotorControls.h"  // LUKE: motors
// #include "sensors.h"     // ARTURO (uncomment when his file is in src/)

enum NavMode { DRIVING, AT_STOP_LINE, CROSSING, EXITING, DONE };

// ---- Navigation settings (tune on the real course) ----
const unsigned long GREEN_HOLD_MS  = 250;    // green must last this long
const unsigned long NO_LIGHT_MS    = 1000;   // see nothing this long = no light here
const unsigned long LOCKOUT_MS     = 1000;   // ignore stop lines after a crossing
const unsigned long SIGN_GIVEUP_MS = 15000;  // stop anyway if no stop sign

// ---- Motor settings (tune with Luke on the real bot) ----
const int PULSES_INTO_INTERSECTION = 60;     // drive this far past the line before pivoting
const int PULSES_STRAIGHT          = 250;    // drive this far to cross going straight
const int STEER_SLOW               = 40;     // slow one wheel this much to steer back
const int LOST_SPEED               = 120;    // slower speed when the lane is lost
const unsigned long CROSS_TIMEOUT_MS = 8000; // encoder broken? stop the turn after this long

// ---- Luke's motor object and wheel counters ----
MotorControls motors;                        // LUKE: motor object (his setup runs here)
extern volatile uint32_t pulseCountR;        // right wheel pulses (Luke's .cpp)
extern volatile uint32_t pulseCountL;        // left wheel pulses  (Luke's .cpp)

// ---- What Navigation remembers ----
NavMode       mode         = DRIVING;
int           routeIndex   = 0;      // which intersection is next
bool          sawLight     = false;  // saw a light color at this stop line
bool          pivoting     = false;  // CROSSING: false = driving in, true = pivoting
unsigned long timer        = 0;      // start time for the current wait
unsigned long lockoutUntil = 0;      // ignore stop lines until this time

// ---------------------------------------------------------------------
// Runs once: set up Luke's motors, wait for start, start the timer.
// ---------------------------------------------------------------------
void navigationSetup() {
  motors.stopMotorR();
  motors.stopMotorL();

  waitForStartSignal();           
  startTimer();                   
}

// ---------------------------------------------------------------------
// Runs over and over. Each call makes one quick decision and returns.
// ---------------------------------------------------------------------
void navigationLoop() {

  // ---- Safety, checked first every time ----
  if (mode == DONE) {
    motors.stopMotorR();
    motors.stopMotorL();
    return;
  }
  if (isBatteryLow()) {                       
    Serial.println("Battery low");
    motors.stopMotorR();
    motors.stopMotorL();
    stopTimer();                              
    mode = DONE;
    return;
  }
  if (detectObstacle()) {                     
    motors.stopMotorR();
    motors.stopMotorL();
    return;
  }

  VisionResult v = getVision();               

  switch (mode) {

    // -----------------------------------------------------------------
    // 1. DRIVING: stay in the lane until a new stop line.
    // -----------------------------------------------------------------
    case DRIVING: {
      LaneReading lane = readIRSensors();     
      bool newStopLine = (lane == LANE_STOP_LINE) && (millis() > lockoutUntil);

      if (!newStopLine) {
        // Steer: slow one wheel to curve back into the lane.
        if (lane == LANE_DRIFT_LEFT) {                  // too far left: curve right
          motors.startMotorL(PWM_SPEED);
          motors.startMotorR(PWM_SPEED - STEER_SLOW);
        } else if (lane == LANE_DRIFT_RIGHT) {          // too far right: curve left
          motors.startMotorL(PWM_SPEED - STEER_SLOW);
          motors.startMotorR(PWM_SPEED);
        } else if (lane == LANE_LOST) {                 // lost: slow, go straight
          motors.startBothMotors(LOST_SPEED);
        } else {                                        // centered: straight
          motors.startBothMotors(PWM_SPEED);
        }

      } else if (steadyGreen(v, GREEN_HOLD_MS)) {
        // Green at the line: start crossing without stopping.
        Serial.println("Green - go");
        motors.resetEncoders();
        pivoting = false;
        timer = millis();                      // start the crossing timeout
        mode = CROSSING;

      } else {
        // Anything else: stop and decide at the line.
        motors.stopMotorR();
        motors.stopMotorL();
        sawLight = false;
        timer = millis();
        mode = AT_STOP_LINE;
      }
      break;
    }

    // -----------------------------------------------------------------
    // 2. AT_STOP_LINE: go on steady green, or if there's no light.
    // -----------------------------------------------------------------
    case AT_STOP_LINE: {
      motors.stopMotorR();
      motors.stopMotorL();

      bool goNow = false;
      if (steadyGreen(v, GREEN_HOLD_MS)) {
        goNow = true;                          // green: go
      } else if (!visionFresh(v)) {
        timer = millis();                      // camera answer too old: start over
      } else if (v.light != NO_LIGHT) {
        sawLight = true;                       // red / yellow / new green: wait
      } else if (!sawLight && millis() - timer >= NO_LIGHT_MS) {
        goNow = true;                          // 1 s of nothing: no light here
      }

      if (goNow) {
        Serial.println("Go");
        motors.resetEncoders();
        pivoting = false;
        timer = millis();                      // start the crossing timeout
        mode = CROSSING;
      }
      break;
    }

    // -----------------------------------------------------------------
    // 3. CROSSING: drive through the intersection.
    //    Straight: drive PULSES_STRAIGHT.
    //    Turn:     drive PULSES_INTO_INTERSECTION, then pivot PULSES_PER_TURN.
    // -----------------------------------------------------------------
    case CROSSING: {
      RouteStep dir = ROUTE[routeIndex];
      uint32_t  r   = pulseCountR;
      uint32_t  l   = pulseCountL;
      bool turnDone = false;

      if (millis() - timer > CROSS_TIMEOUT_MS) {
        Serial.println("Crossing timeout - check encoders");
        turnDone = true;                                // safety: don't turn forever

      } else if (dir == GO_STRAIGHT) {
        if ((r + l) / 2 >= PULSES_STRAIGHT) turnDone = true;
        else motors.startBothMotors(PWM_SPEED);

      } else if (!pivoting) {                           // drive into the intersection
        if ((r + l) / 2 >= PULSES_INTO_INTERSECTION) {
          motors.resetEncoders();
          pivoting = true;
        } else {
          motors.startBothMotors(PWM_SPEED);
        }

      } else if (dir == TURN_RIGHT) {                   // pivot right: left wheel drives
        if (l >= PULSES_PER_TURN) turnDone = true;
        else { motors.startMotorL(PWM_SPEED); motors.stopMotorR(); }

      } else {                                          // pivot left: right wheel drives
        if (r >= PULSES_PER_TURN) turnDone = true;
        else { motors.startMotorR(PWM_SPEED); motors.stopMotorL(); }
      }

      if (turnDone) {
        motors.stopMotorR();
        motors.stopMotorL();
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
    }

    // -----------------------------------------------------------------
    // 4. EXITING: after the last turn, drive until the stop sign.
    // -----------------------------------------------------------------
    case EXITING:
      if ((visionFresh(v) && v.stopSign) || millis() - timer > SIGN_GIVEUP_MS) {
        Serial.println(v.stopSign ? "Stop sign - done" : "No stop sign found");
        motors.stopMotorR();
        motors.stopMotorL();
        stopTimer();                           // ARTURO
        mode = DONE;
      } else {
        motors.startBothMotors(PWM_SPEED);
      }
      break;

    case DONE:
      break;
  }
}