#include <Arduino.h>
#include "MotorControls.h"


#define PIN_BUTTON  0   // button pin
#define ENC_L 2        // left encoder pin
#define ENC_R 1        // Right encoder pin

MotorControls motors;   //MotorControl object


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- varibles for testing: ----------------

bool running = false;   // Variable to start and stop looping in test
uint TargetPulses;      // number to stop when the count reaches it.
volatile uint32_t PulseCounterL;//intitialization of pulse counter for testing within main
static uint32_t LastPulseCountL;//intitialization of pulse counter value storage for testing within main

volatile uint32_t PulseCounterR;//intitialization of pulse counter for testing within main
static uint32_t LastPulseCountR;//intitialization of pulse counter value storage for testing within main


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- local methods ----------------

// ISR routines - increments the Pulse counter L variable every time a pulse is detected on the left motor
void IRAM_ATTR onEncL() { PulseCounterL++; }   
void IRAM_ATTR onEncR() { PulseCounterR++; }   

// The function returns true only once per physical press, after confirming the button is held for ~50 ms (debounce) and then waiting until the user releases it before returning.
bool buttonPressed() {
    if (digitalRead(PIN_BUTTON) == LOW) {
        delay(50);
        if (digitalRead(PIN_BUTTON) == LOW) {
            while (digitalRead(PIN_BUTTON) == LOW) {}
            return true;
        }
    }
    return false;
}

//makes the running varible swap between true and false upon pressing boot button. 
void On_Off_Switch(){
    if (!running) {
        if (buttonPressed()) {
            Serial.println("Button pressed! Starting in 1 second...");
            delay(1000);
            turnOnRunning();
        }
        return;
    }
    else {
        if (buttonPressed()) {
            Serial.println("Button pressed! Shutting down in 1 second...");
            delay(1000);
            turnOffRunning();
        }
        return;
    }
}
void turnOnRunning(){
    running = true;
    Serial.println("Running!");
}
void turnOffRunning(){
    running = false;
    Serial.println("Stopping!");
}

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
bool IsPulseTargetMet(int PulseCount){
    if (PulseCount > TargetPulses) { 
        return true;
    }
    return false;
}

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
    if (PulseCounterL != LastPulseCountL) {
        //set count to new value
        SetPulseCount(LastPulseCountL, PulseCounterL);
        change = true;
    }
    if (PulseCounterR != LastPulseCountR) {
        //set count to new value
        SetPulseCount(LastPulseCountR, PulseCounterR);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- Encoder Test ----------------
void setup() {
    Serial.begin(19200); // 115200 is default, but i've also been using 19200
    pinMode(PIN_BUTTON, INPUT_PULLUP); // button state setting

    pinMode(ENC_L, INPUT);// Left encoder state setting
    pinMode(ENC_R, INPUT);// right encoder state setting

    attachInterrupt(digitalPinToInterrupt(ENC_L), onEncL, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_R), onEncR, CHANGE);

    Serial.println("Encoder test ready. Spin the wheels!");
    Serial.println("Boot complete. Waiting for button press...");

    PulseCounterL=0;
    LastPulseCountL=0;
    PulseCounterR=0;
    LastPulseCountR=0;
    TargetPulses=5000;

    motors = MotorControls();
}

void loop() {
    
    On_Off_Switch(); // if off, wait till button press. if on, stop if button pressed.
    if(running){
        if (!IsPulseTargetMet(PulseCounterR)){
            motors.setMotorR(200);
        }
        if (!IsPulseTargetMet(PulseCounterL)){
            motors.setMotorL(200);
        }
        
        
        /*
        if(IsPinHigh(ENC_R, PulseCounterR)){ // print whether the right encoder pin is high or low
            Serial.println("High");
        }
        else{
            Serial.println("Low");
        }
        if(IsPinHigh(ENC_L, PulseCounterL)){ // print whether the left encoder pin is high or low
            Serial.println("High");
        }
        else{
            Serial.println("Low");
        }
        */
        if(UpdatePulseCount()){//if the pulse count has changed
            if (IsPulseTargetMet(PulseCounterR)) { // check if target pulses have been reached/passed
                //If so,
                PrintPulseCountWithName(PulseCounterR, "R");  //print the count
                motors.setMotorR(0);
                //turnOffRunning();                             // and stop the loop.
            }
            if (IsPulseTargetMet(PulseCounterL)) { // check if target pulses have been reached/passed
                //If so,
                PrintPulseCountWithName(PulseCounterL, "L");  //print the count
                motors.setMotorL(0);
                //turnOffRunning();                             // and stop the loop.
            }
        }

        if(IsPulseTargetMet(PulseCounterR) && IsPulseTargetMet(PulseCounterL)){
            turnOffRunning();                             // and stop the loop.
        }
        PrintPulseCountWithName(PulseCounterR, "R");
        PrintPulseCountWithName(PulseCounterL, "L");
    }
    else{

        
    }
    //Serial.println("Main loop restarting");
}

///////////////////////////////////////////////////////////////////////////////turn right
/*
        motors.setMotorL(PWM_SPEED);
        while(!IsPulseTargetMet(PulseCounterL)){//if the left pulse count target has not yet been met
            UpdatePulseCount();
            PrintPulseCountWithName(PulseCounterL, "left enc: "); 
        }
        motors.setMotorL(0);
        PrintPulseCountWithName(PulseCounterL, "left enc: ");
        turnOffRunning();
        SetPulseCount(PulseCounterL, 0);//reset counters
        SetPulseCount(PulseCounterR, 0);
*/ 
///////////////////////////////////////////////////////////////////////////////turn left 


        motors.setMotorR(PWM_SPEED);
        while(!IsPulseTargetMet(PulseCounterR)){//if the left pulse count target has not yet been met
            UpdatePulseCount();
            PrintPulseCountWithName(PulseCounterR, "right enc: "); 
        }
        motors.setMotorR(0);
        PrintPulseCountWithName(PulseCounterR, "right enc: ");
        turnOffRunning();
        SetPulseCount(PulseCounterL, 0);//reset counters
        SetPulseCount(PulseCounterR, 0);


    }



    //Serial.println("Main loop restarting");
}



