#include <Arduino.h>
#include "MotorControls.h"

#define PIN_BUTTON  0

MotorControls motors;
bool running = false;


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

// setup()
// standard Arduino initialization for a serial console and a pull‑up‑configured button input.
// initializes USB serial communication at 115200 baud, configures the button pin as an input with an internal pull‑up resistor, and prints a startup message.
void setup() {
    Serial.begin(115200);
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
*/

    if (buttonPressed()) {
        Serial.println("Stopped.");
        running = false;
    }
}
