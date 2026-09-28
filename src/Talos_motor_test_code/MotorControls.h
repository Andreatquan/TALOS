#pragma once

// ─── Pin Definitions ───────────────────────────────────────────────────────

// Motor A (Right wheel) – AIN1 and PWM tied together
#define PIN_MOTOR_A_PWM   39
#define PIN_MOTOR_A_IN2   38
#define PIN_ENC_A         48  // Left wheel Encoder (single-phase only)

// Motor B (Left wheel) – BIN1 and PWM tied together
#define PIN_MOTOR_B_PWM   41
#define PIN_MOTOR_B_IN2   40
#define PIN_ENC_B         42  // Right wheel Encoder (single-phase only)

// STBY is wired directly to 3.3V, no GPIO needed

// IR Sensors
#define PIN_IR_Front      14  // Front IR sensor(downward facing)
#define PIN_IR_Right      47  // Right IR sensor
#define PIN_IR_left       21  // Left IR sensor

// Ultrasonic Sensor
#define PIN_Ultra_Trig       1   // GPIO 1 = Ultrasonic Trig
#define PIN_Ultra_Echo       2   // GPIO 2 = Ultrasonic Echo

// ─── Tuning Constants (calibrate these for your bot) ──────────────────────
#define PWM_SPEED         200   // 0-255 duty cycle
#define PULSES_PER_TURN   150   // encoder pulses for a 90-degree pivot turn



//────────────────────────────────────────────────────────────────────────────


class MotorControls {
public:
    MotorControls();

    // Move both wheels forward until each has counted x encoder pulses
    void MoveForward(int x);
    void MoveForwardNoEncoder(int x);

    // Turn the robot left 90 degrees in place
    void TurnLeft();
    void TurnLeftNoEncoder(int x);

    // Turn the robot right 90 degrees in place
    void TurnRight();
    void TurnRightNoEncoder(int x);

private:
    // Drive a single motor at the given PWM duty (0 = stop)
    void setMotorA(int pwm);
    void setMotorB(int pwm);

    // Block until the given counter reaches targetPulses
    void waitForPulses(volatile int &counter, int targetPulses);
};