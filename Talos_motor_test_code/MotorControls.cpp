#include <Arduino.h>
#include <driver/ledc.h>
#include "MotorControls.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- local variables ----------------
// ─────────────────────────────────────────────────────────────


// Encoder counters (volatile, 32‑bit for safety)
volatile uint32_t pulseCountR= 0;// counts pulses through interrupts outside of method actions
volatile uint32_t pulseCountL= 0;

static uint32_t LastPulseCountL = 0;//intitialization of pulse counter value storage. Changes only when changed buy code(update pulses methods) 
static uint32_t LastPulseCountR = 0;

// ISR: increment counters on rising edge
void IRAM_ATTR onEncoderR() { pulseCountR++; }
void IRAM_ATTR onEncoderL() { pulseCountL++; }

// IR trigger flags
volatile bool IR_Left_Flag  = false;
volatile bool IR_Right_Flag = false;
volatile bool IR_Front_Flag = false;

// ISR for IR sensors
void IRAM_ATTR onIR_Left()  { IR_Left_Flag  = true; }
void IRAM_ATTR onIR_Right() { IR_Right_Flag = true; }
void IRAM_ATTR onIR_Front() { IR_Front_Flag = true; }




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- Constructors ----------------
// ─────────────────────────────────────────────────────────────

MotorControls::MotorControls() {

    // Motors -------------------------------------------------------
    // --------------------------------------------------------------

    // PWM for Motor Right
    ledcAttach(PIN_MOTOR_R_PWM, 20000, 8);   // freq, resolution
    pinMode(PIN_MOTOR_R_IN2, OUTPUT);

    // PWM for Motor Left
    ledcAttach(PIN_MOTOR_L_PWM, 20000, 8);
    pinMode(PIN_MOTOR_L_IN2, OUTPUT);

    // Encoders -----------------------------------------------------
    // --------------------------------------------------------------
    
    // Encoder inputs
    pinMode(PIN_ENC_R, INPUT_PULLUP);
    pinMode(PIN_ENC_L, INPUT_PULLUP);

    // Encoder interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), onEncoderR, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), onEncoderL, RISING);

    // Encoder Variables
    pulseCountR = 0;
    pulseCountL = 0;

    LastPulseCountL = 0;  
    LastPulseCountR = 0;

    // IR Sensors -----------------------------------------------------
    // --------------------------------------------------------------

    // IR inputs
    pinMode(PIN_IR_Left,  INPUT_PULLUP);
    pinMode(PIN_IR_Right, INPUT_PULLUP);
    pinMode(PIN_IR_Front, INPUT_PULLUP);


    // IR Sensor interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_IR_Left),  onIR_Left,  RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_IR_Right), onIR_Right, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_IR_Front), onIR_Front, RISING);

}



// ─────────────────────────────────────────────────────────────
// Public movement functions
// ─────────────────────────────────────────────────────────────

void MotorControls::MoveForward(uint32_t targetPulses) {

    resetEncoders();
    startBothMotors(PWM_SPEED);

    // Wait for each motor independently
    waitForPulses(pulseCountR, targetPulses);
    stopMotorR();

    waitForPulses(pulseCountR, targetPulses);
    stopMotorL();
}

void MotorControls::TurnRight(uint32_t targetPulses) {

    resetEncoderL();          // clear right encoder
    startMotorL(PWM_SPEED);
    stopMotorR();          // left wheel stays still

    waitForPulses(pulseCountL, targetPulses);

    stopMotorR();
}


void MotorControls::TurnLeft(uint32_t targetPulses) {

    resetEncoderR();          // clear left encoder
    startMotorR(PWM_SPEED);
    stopMotorL();         // left wheel stays still

    waitForPulses(pulseCountR, targetPulses);

    stopMotorL();
}


// ─────────────────────────────────────────────────────────────
// No‑encoder timed movement
// ─────────────────────────────────────────────────────────────

void MotorControls::MoveForwardNoEncoder(int ms) {
    setMotorR(PWM_SPEED + 20);
    setMotorL(PWM_SPEED);
    delay(ms);
    setMotorR(0);
    setMotorL(0);
}

void MotorControls::TurnRightNoEncoder(int ms) {
    setMotorR(0);
    setMotorL(PWM_SPEED);
    delay(ms);
    setMotorL(0);
}

void MotorControls::TurnLeftNoEncoder(int ms) {
    setMotorR(PWM_SPEED);
    setMotorL(0);
    delay(ms);
    setMotorR(0);
}

// ─────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────

void MotorControls::setMotorR(int pwm) {
    ledcWrite(PIN_MOTOR_R_PWM, pwm);
    digitalWrite(PIN_MOTOR_R_IN2, LOW);
}

void MotorControls::setMotorL(int pwm) {
    ledcWrite(PIN_MOTOR_L_PWM, pwm);
    digitalWrite(PIN_MOTOR_L_IN2, LOW);
}

void MotorControls::resetEncoders() {
    noInterrupts();
    pulseCountR = 0;
    pulseCountL = 0;
    interrupts();
}

void MotorControls::resetEncoderR() {
    noInterrupts();
    pulseCountR = 0;
    interrupts();
}

void MotorControls::resetEncoderL() {
    noInterrupts();
    pulseCountL = 0;
    interrupts();
}

void MotorControls::startBothMotors(int pwm) {
    setMotorR(pwm);
    setMotorL(pwm);
}

void MotorControls::startMotorR(int pwm) {
    setMotorR(pwm);
}

void MotorControls::startMotorL(int pwm) {
    setMotorL(pwm);
}

void MotorControls::stopMotorR() {
    setMotorR(0);
}

void MotorControls::stopMotorL() {
    setMotorL(0);
}

void MotorControls::stopBothMotors() {
    ledcWrite(PIN_MOTOR_R_PWM, 0);
    ledcWrite(PIN_MOTOR_L_PWM, 0);
}

void MotorControls::brakeBothMotors() {
    ledcWrite(PIN_MOTOR_R_PWM, 0);
    ledcWrite(PIN_MOTOR_L_PWM, 0);
}


void MotorControls::waitForPulses(volatile uint32_t &counter, uint32_t target) {
    while (counter < target) {
        delay(1);
    }

    // brief reverse pulse to counter overshoot
    ledcWrite(PIN_MOTOR_R_PWM, 0);
    ledcWrite(PIN_MOTOR_L_PWM, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Below are methods imported from test code.

// show on serial monitor whether the Left encoder is HIGH or LOW
bool IsPinHigh(int PinNum, int PulseCounter){
    PulseCounter = digitalRead(PinNum);
    if(PulseCounter){
        return true;
    }
    else{
        return false;
    }
}

// print count
void PrintPulseCount(int PulseCounter){
    //print the pulse counter
    Serial.println(PulseCounter);
    return;
}
// print count with name
void PrintPulseCountWithName(int PulseCounter, String PulseCounterName){
    //print the pulse counter with its label/name
    Serial.print(PulseCounterName);
    Serial.print(": ");
    Serial.println(PulseCounter);
    return;
}
// check if target pulses have been passed. NOTE: this method does not update the count
bool IsPulseTargetMet(int PulseTarget, int PulseCount){
    if (PulseCount > PulseTarget) { 
        return true;
    }
    return false;
}
/*bool IsPulseTargetMet(int PulseCount){
    if (PulseCount > TargetPulses) { 
        return true;
    }
    return false;
}*/

// check if pulse count has changed, If encoder count changed, update count and return true. else return false.
bool UpdatePulseCount(int OldPulseCount, int NewPulseCount){
    if (NewPulseCount != OldPulseCount) {
        //set count to new value
        SetPulseCount(OldPulseCount, NewPulseCount);
        return true;
    }
    return false;
}
bool UpdatePulseCount(){
    bool change=false;
    if (pulseCountL!= LastPulseCountL) {
        //set count to new value
        SetPulseCount(LastPulseCountL, pulseCountL);
        change = true;
    }
    if (pulseCountR!= LastPulseCountR) {
        //set count to new value
        SetPulseCount(LastPulseCountR, pulseCountR);
        change = true;
    }
    return change;
}
// change pulse count to new value
void SetPulseCount(int OldPulseCount, int NewPulseCount){
    // update last pulse count to the new count
    OldPulseCount = NewPulseCount;
    return;
}


