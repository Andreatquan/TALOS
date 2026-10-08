#include <Arduino.h>
#include "MotorControls.h"

// button pin
#define PIN_BUTTON  0
// encoder pins
#define ENC_L 42

//intitialization of pulse counter for testing within main
volatile uint32_t pulseL = 0;
static uint32_t lastL = 0;

// ISR routines
void IRAM_ATTR onEncL() { pulseL++; }

//MotorControl object 
MotorControls motors;

// Variable to start and stop looping in test
bool running = false;



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- local methods ----------------

// buttonPressed()
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
            running = true;
            Serial.println("Running!");
        }
        return;
    }
    else {
        if (buttonPressed()) {
            Serial.println("Button pressed! Shutting down in 1 second...");
            delay(1000);
            running = false;
            Serial.println("Stopping!");
        }
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- Encoder Test ----------------
void setup() {

    Serial.begin(115200); // 115200 is default, but i've also been using 19200
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    pinMode(ENC_L, INPUT);

    attachInterrupt(digitalPinToInterrupt(ENC_L), onEncL, CHANGE);

    Serial.println("Encoder test ready. Spin the wheels!");
    Serial.println("Boot complete. Waiting for button press...");
}

void loop() {
    
    On_Off_Switch(); // if off, wait till button press. if on, stop if button pressed.
    if(running){
        
        pulseL = digitalRead(ENC_L);
        if(pulseL == HIGH){
            Serial.println("High");
        }
        else{
            Serial.println("Low");
        }
        

        
    /*
        // If either encoder count changed, flash LED and print counts
        if (pulseL != lastL) {

            Serial.print("  L: ");
            Serial.println(pulseL);

            lastL = pulseL;
        }
        Serial.println("loop restarting");


        if (pulseL > 6000) {
            Serial.println("Stopped.");
            Serial.print("  L: ");
            Serial.println(pulseL);
            running = false;
        }
    */
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- D1 Test adapted (unsuccessfully tested) ----------------
/*

// setup()
// standard Arduino initialization for a serial console and a pull‑up‑configured button input.
// initializes USB serial communication at 115200 baud, configures the button pin as an input with an internal pull‑up resistor, and prints a startup message.
void setup() {
    /*Serial.begin(115200);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("Boot complete. Waiting for button press...");
    

}


//loop()
// Main loop for motor testing demonstration. this is a hardcoded demo using the outdated time-based methods.
void loop() {
    
 
    // when the button is pressed, print message, delay 2s and then continue.
    if (!running) {
        if (buttonPressed()) {
            Serial.println("Button pressed! Starting in 2 seconds...");
            delay(2000);
            running = true;
            Serial.println("Running!");
        }
        return;
    }

    //pull up to intersection
    Serial.println("Moving forward...");
    motors.MoveForward(100);
    */
/*
    //wait at stop bar
    delay(2000);

    //turn right
    motors.MoveForward(500);
    Serial.println("Turning right...");
    motors.TurnRight(800);

    //drive down road
    Serial.println("Moving forward...");
    motors.MoveForward(2000);
    
    //u-turn left
    Serial.println("Turning left...");
    motors.TurnLeft(1700);

    //drive up to stop bar
    Serial.println("Moving forward...");
    motors.MoveForward(1500);

    Serial.println("Turning left...");
    //motors.TurnLeftNoEncoder(2000);

    running = false;


    if (buttonPressed()) {
        Serial.println("Stopped.");
        running = false;
    }
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

