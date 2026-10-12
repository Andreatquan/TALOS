#include <Arduino.h>
#pragma once
#include <cstdint>


// ─── Pin Definitions ───────────────────────────────────────────────────────

// Motor R (Right wheel) – AIN1 and PWM tied together
#define PIN_MOTOR_R_PWM   39
#define PIN_MOTOR_R_IN2   38
#define PIN_ENC_R         1  // Right wheel Encoder (single-phase only)

// Motor L (Left wheel) – BIN1 and PWM tied together
#define PIN_MOTOR_L_PWM   41
#define PIN_MOTOR_L_IN2   40
#define PIN_ENC_L         2  // Left wheel Encoder (single-phase only)

// STBY is wired directly to 3.3V, no GPIO needed

// IR Sensors
#define PIN_IR_Front      14  // Front IR sensor(downward facing)
#define PIN_IR_Right      47  // Right IR sensor
#define PIN_IR_Left       21  // Left IR sensor

// Ultrasonic Sensor
//#define PIN_Ultra_Trig       1   // GPIO 1 = Ultrasonic Trig
//#define PIN_Ultra_Echo       2   // GPIO 2 = Ultrasonic Echo

// ─── Tuning Constants (calibrate these for your bot) ──────────────────────
#define PWM_SPEED         150   // 0-255 duty cycle. this is the base speed, speed can be adjusted by some methods.
#define PULSES_PER_TURN   150   // encoder pulses for a 90-degree pivot turn



//────────────────────────────────────────────────────────────────────────────


class MotorControls {
public:
    MotorControls();

    // Move both wheels forward until each has counted x encoder pulses
    void MoveForward(uint32_t targetPulses);// drive forward until both motors have moved a given number of encoder pulses
    void MoveForwardNoEncoder(int x);       // turn both motors on for x milisecs
    void MoveForward_CM(uint32_t targetCM); // drive forward given number of centimeters - NOT YET IMPLEMENTED************************
    void Dive_Until_Stop_Line();            // drive straight until you detect a stop line with the front IR sensor - NOT YET IMPLEMENTED************************ 

    // Turn the robot left 90 degrees
    void TurnLeft(uint32_t targetPulses);   // manual turn 
    void IntersectionLeftTurn();            // standardized 90 degree left turn + forward intersection distance - NOT YET IMPLEMENTED************************
    void TurnLeftNoEncoder(int x);          // manual turn based on time. x is in milisec

    // Turn the robot right 90 degrees
    void TurnRight(uint32_t targetPulses);  // manual turn
    void IntersectionRightTurn();           // standardized 90 degree right turn + forward intersection distance - NOT YET IMPLEMENTED************************
    void TurnRightNoEncoder(int x);         // manual turn based on time. x is in milisec



//future Private methods::
//
    // Drive a single motor at the given PWM duty (0 = stop)
    void setMotorR(int pwm);
    void setMotorL(int pwm);

    void stopMotorR();
    void stopMotorL();
    void stopBothMotors();
    void brakeBothMotors();

    void resetEncoders();
    void resetEncoderR();
    void resetEncoderL();

    void startBothMotors(int pwm);
    void startMotorR(int pwm);
    void startMotorL(int pwm);

    // Block until the given counter reaches targetPulses
    void waitForPulses(volatile uint32_t &counter, uint32_t targetPulses);

private:

};