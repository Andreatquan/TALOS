#include <Arduino.h>
#include <driver/ledc.h>
#include "MotorControls.h"



// ─────────────────────────────────────────────────────────────
// Encoder counters (volatile, 32‑bit for safety)
// ─────────────────────────────────────────────────────────────
volatile uint32_t pulseCountA = 0;
volatile uint32_t pulseCountB = 0;

// ISR: increment counters on rising edge
void IRAM_ATTR onEncoderA() { pulseCountA++; }
void IRAM_ATTR onEncoderB() { pulseCountB++; }

// ─────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────
MotorControls::MotorControls() {

    // Configure PWM channels (LEDC)
    ledcAttachPin(PIN_MOTOR_A_PWM, 0);
    ledcSetup(0, 20000, 8);   // 20 kHz, 8‑bit

    ledcAttachPin(PIN_MOTOR_B_PWM, 1);
    ledcSetup(1, 20000, 8);   // 20 kHz, 8‑bit

    // Direction pins
    pinMode(PIN_MOTOR_A_IN2, OUTPUT);
    pinMode(PIN_MOTOR_B_IN2, OUTPUT);

    // Encoder inputs
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), onEncoderA, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), onEncoderB, RISING);
}

// ─────────────────────────────────────────────────────────────
// Public movement functions
// ─────────────────────────────────────────────────────────────

void MotorControls::MoveForward(uint32_t targetPulses) {

    // Reset counters safely
    noInterrupts();
    pulseCountA = 0;
    pulseCountB = 0;
    interrupts();

    setMotorA(PWM_SPEED);
    setMotorB(PWM_SPEED);

    bool doneA = false;
    bool doneB = false;

    while (!doneA || !doneB) {

        if (!doneA && pulseCountA >= targetPulses) {
            setMotorA(0);
            doneA = true;
        }

        if (!doneB && pulseCountB >= targetPulses) {
            setMotorB(0);
            doneB = true;
        }

        delay(1);   // prevents watchdog reset, keeps USB alive
    }
}

void MotorControls::TurnRight(uint32_t targetPulses) {

    noInterrupts();
    pulseCountB = 0;
    interrupts();

    setMotorA(0);
    setMotorB(PWM_SPEED);

    waitForPulses(pulseCountB, targetPulses);

    setMotorB(0);
}

void MotorControls::TurnLeft(uint32_t targetPulses) {

    noInterrupts();
    pulseCountA = 0;
    interrupts();

    setMotorA(PWM_SPEED);
    setMotorB(0);

    waitForPulses(pulseCountA, targetPulses);

    setMotorA(0);
}

// ─────────────────────────────────────────────────────────────
// No‑encoder timed movement
// ─────────────────────────────────────────────────────────────

void MotorControls::MoveForwardNoEncoder(int ms) {
    setMotorA(PWM_SPEED + 20);
    setMotorB(PWM_SPEED);
    delay(ms);
    setMotorA(0);
    setMotorB(0);
}

void MotorControls::TurnRightNoEncoder(int ms) {
    setMotorA(0);
    setMotorB(PWM_SPEED);
    delay(ms);
    setMotorB(0);
}

void MotorControls::TurnLeftNoEncoder(int ms) {
    setMotorA(PWM_SPEED);
    setMotorB(0);
    delay(ms);
    setMotorA(0);
}

// ─────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────

void MotorControls::setMotorA(int pwm) {
    ledcWrite(0, pwm);
    digitalWrite(PIN_MOTOR_A_IN2, LOW);
}

void MotorControls::setMotorB(int pwm) {
    ledcWrite(1, pwm);
    digitalWrite(PIN_MOTOR_B_IN2, LOW);
}

void MotorControls::waitForPulses(volatile uint32_t &counter, uint32_t target) {
    while (counter < target) {
        delay(1);   // keeps watchdog happy
    }
}
