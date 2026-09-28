#include <Arduino.h>
#include "MotorControls.h"

#define PIN_BUTTON  0

MotorControls motors;
bool running = false;

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

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("Boot complete. Waiting for button press...");
}

void loop() {
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
    motors.MoveForwardNoEncoder(2500);

    //wait at stop bar
    delay(2000);

    //turn right
    motors.MoveForwardNoEncoder(500);
    Serial.println("Turning right...");
    motors.TurnRightNoEncoder(800);

    //drive down road
    Serial.println("Moving forward...");
    motors.MoveForwardNoEncoder(2000);
    
    //u-turn left
    Serial.println("Turning left...");
    motors.TurnLeftNoEncoder(1700);

    //drive up to stop bar
    Serial.println("Moving forward...");
    motors.MoveForwardNoEncoder(1500);

    Serial.println("Turning left...");
    //motors.TurnLeftNoEncoder(2000);

    running = false;

    if (buttonPressed()) {
        Serial.println("Stopped.");
        running = false;
    }
}
