#include <Arduino.h>
#include "MotorControls.h"

#define PIN_BUTTON  0

#define LED_PIN 38
#define NUM_LEDS 1

MotorControls motors;
bool running = false;

/*
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
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---------------- Encoder Test ----------------

// encoder pins
#define ENC_B 42

// LED pin 
//#define LED_PIN 38

//volatile uint32_t pulseB = 0;
static uint32_t lastB = 0;

// ISR routines
//void IRAM_ATTR onEncB() { pulseB++; }

void setup() {

    Serial.begin(19200);
    //pinMode(PIN_BUTTON, INPUT_PULLUP);
    //Serial.println("Boot complete. Waiting for button press...");

    //Serial.begin(115200);

    //pinMode(LED_PIN, OUTPUT);
    //digitalWrite(LED_PIN, LOW);

    pinMode(ENC_B, INPUT);

    //attachInterrupt(digitalPinToInterrupt(ENC_B), onEncB, CHANGE);

    Serial.println("Encoder test ready. Spin the wheels.");
}

void loop() {
        
    int pulseB = digitalRead(ENC_B);
    Serial.println(pulseB);
    /*
    if(pulseB == HIGH){
        Serial.println("High");
    }
    else{
        Serial.println("Low");
    }
    */

    // when the button is pressed, print message, delay 1s and then continue.
    // if (!running) {
    //     if (buttonPressed()) {
    //         Serial.println("Button pressed! Starting in 1 second...");
    //         delay(1000);
    //         running = true;
    //         Serial.println("Running!");
    //         pulseB=0;
    //     }
    //     return;
    // }
/*
    digitalWrite(LED_PIN, HIGH); // not working bc its an RGB LED
    Serial.println("LED on...");
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED off...");
    delay(1000);*/

    
/*
    // If either encoder count changed, flash LED and print counts
    if (pulseB != lastB) {

        Serial.print("  B: ");
        Serial.println(pulseB);

        lastB = pulseB;
    }
    Serial.println("loop restarting");


    if (pulseB > 6000) {
        Serial.println("Stopped.");
        Serial.print("  B: ");
        Serial.println(pulseB);
        running = false;
    }
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

