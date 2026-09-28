#include "MotorControls.h"
#include <Arduino.h>

// ─── Encoder pulse counters (written by ISRs, read by main code) ───────────
volatile int pulseCountA = 0;
volatile int pulseCountB = 0;

// Interrupt Service Routines – called on every rising edge from each encoder
void IRAM_ATTR onEncoderA() { pulseCountA++; }
void IRAM_ATTR onEncoderB() { pulseCountB++; }

// ─── Constructor ──────────────────────────────────────────────────────────
MotorControls::MotorControls() {
    // Motor output pins
    pinMode(PIN_MOTOR_A_PWM, OUTPUT);
    pinMode(PIN_MOTOR_B_PWM, OUTPUT);

    // STBY is hardwired to 3.3V, so both motors are always enabled

    // Encoder input pins with pull-ups
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);

    // Attach interrupts – count every rising edge
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), onEncoderA, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), onEncoderB, RISING);
}

// ─── Public Methods ────────────────────────────────────────────────────────

void MotorControls::MoveForward(int x) {
    pulseCountA = 0;
    pulseCountB = 0;

    setMotorA(PWM_SPEED);
    setMotorB(PWM_SPEED);

    // Stop each motor independently the moment it hits the target
    bool doneA = false;
    bool doneB = false;

    while (!doneA || !doneB) {
        if (!doneA && pulseCountA >= x) {
            setMotorA(0);
            doneA = true;
        }
        if (!doneB && pulseCountB >= x) {
            setMotorB(0);
            doneB = true;
        }
    }
}

void MotorControls::TurnRight() {
    // Right wheel drives forward, left wheel stays still
    pulseCountB = 0;

    setMotorA(0);
    setMotorB(PWM_SPEED);

    waitForPulses(pulseCountB, PULSES_PER_TURN);

    setMotorB(0);
}

void MotorControls::TurnLeft() {
    // Left wheel drives forward, right wheel stays still
    pulseCountA = 0;

    setMotorA(PWM_SPEED);
    setMotorB(0);

    waitForPulses(pulseCountA, PULSES_PER_TURN);

    setMotorA(0);
}

//-------------------------------------------NoEncoder Methods

void MotorControls::MoveForwardNoEncoder(int x) {

    setMotorA(PWM_SPEED + 20);
    setMotorB(PWM_SPEED);

    delay(x);

    setMotorB(0);
    setMotorA(0);

}

void MotorControls::TurnRightNoEncoder(int x) {
    // Right wheel drives forward, left wheel stays still
    
    setMotorA(0);
    setMotorB(PWM_SPEED);

    delay(x);

    setMotorB(0);
}

void MotorControls::TurnLeftNoEncoder(int x) {
    // Left wheel drives forward, right wheel stays still

    setMotorA(PWM_SPEED);
    setMotorB(0);

    delay(x);

    setMotorA(0);
}

// ─── Private Helpers ───────────────────────────────────────────────────────

void MotorControls::setMotorA(int pwm) {
    analogWrite(PIN_MOTOR_A_PWM, pwm);
    digitalWrite(PIN_MOTOR_A_IN2, 0);
}

void MotorControls::setMotorB(int pwm) {
    analogWrite(PIN_MOTOR_B_PWM, pwm);
    digitalWrite(PIN_MOTOR_B_IN1, 0);
}

void MotorControls::waitForPulses(volatile int &counter, int targetPulses) {
    while (counter < targetPulses) {
        delay(1); // yield to watchdog while waiting
    }
}
