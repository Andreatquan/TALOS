#pragma once

// ─── Pin Definitions ───────────────────────────────────────────────────────
// Motor A (Left wheel) – AIN1 and PWM tied together
#define PIN_MOTOR_A_PWM   38
#define PIN_MOTOR_A_IN2   37
// Motor B (Right wheel) – BIN1 and PWM tied together
#define PIN_MOTOR_B_PWM   40
#define PIN_MOTOR_B_IN1   41

// STBY is wired directly to 3.3V, no GPIO needed

// Encoders (single-phase only)
#define PIN_ENC_A         42  // Left wheel
#define PIN_ENC_B         48  // Right wheel

// ─── Tuning Constants (calibrate these for your bot) ──────────────────────
#define PWM_SPEED         200   // 0-255 duty cycle
#define PULSES_PER_TURN   150   // encoder pulses for a 90-degree pivot turn

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