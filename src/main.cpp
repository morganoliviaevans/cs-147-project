/*
CS 147: IoT Software and Systems
Course Project - Motorized Cat Toy
Team Member 1: Morgan Newton
Team Member 2: Carm Hermosilla
Due Date: 
*/

/*
Project Breakdown:

Initialization and Setup
    1. Initialize the microcontroller and set up peripherals: LEDs, motor, accelerometer, buzzer
    2. Define all GPIO pins for the connected components
    3. Set default states for LEDs (off), motor (idle), and buzzer (off)

State 1: Play Mode
    1. Detect accelerometer activity
    2. Did our x-axis, y-axis, or z-axis move? Activate the toy!
    3. Adjust LED flashing rate to conserve battery (flash once every 30 seconds)
    4. Continue randomizing motor speed and direction
    5. Continue periodic bird noises
    6. If the toy is idle for 1 minute, switch to hunting mode

State 2: Hunting Mode
    1. Check accelerometer data for inactivity for 1 minute
    2. Trigger attention-grabbing behavior -- LED flashing, bird noises
    3. Once cat moves the ball, we switch back to play mode
    4. If the cat does not move the ball after 2 minutes, go into sleep mode

State 3: Sleep Mode
    1. Turn off LEDs and turn off the motor to conserve battery
    2. Keep accelerometer active to detect the cat’s return
    3. Re-enter play mode upon detecting motion
*/

// ----------------------- BASIC LIBRARIES -----------------------------
#include <SPI.h>
#include <Wire.h>
#include <stdio.h>
#include <Arduino.h>
// ----------------------- CLOUD ---------------------------------------
#include "nvs.h"
#include "esp_wifi.h"
#include <inttypes.h>
#include "nvs_flash.h"
#include "esp_system.h"
#include <HttpClient.h>
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
// ----------------------- ACCELEROMETER -------------------------------
#include <SparkFunLSM6DSO.h>
// ----------------------- LED -----------------------------------------
#include <Adafruit_NeoPixel.h>

// ----------------------- VARIABLE DECLARATIONS -----------------------

#define BUZZER_PIN 13                                                   // TO-DO: Need to set pins
#define LED_PIN 15                                                      // TO-DO: Need to set pins
#define NUM_LEDS 7                                                      // TO-DO: Need to set pins

// Motor pins
#define IN1 5                                                           // TO-DO: GPIO pin connected to IN1 on L298N
#define IN2 6                                                           // TO-DO: GPIO pin connected to IN2 on L298N
#define EN 9                                                            // TO-DO: GPIO pin connected to EN on L298N (for speed control)

unsigned long stateStartTime = millis();

// NeoPixel strip object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Device States
enum DeviceState { PLAY, HUNTING, SLEEP };
DeviceState currentState = PLAY;

// Accelerometer object
LSM6DSO myIMU;
float x_axis = myIMU.readFloatAccelX();
float y_axis = myIMU.readFloatAccelY();
float z_axis = myIMU.readFloatAccelZ();

// ----------------------- FUNCTION DECLARATIONS -----------------------

void chirp();
void off_led();
void slow_led();
void play_mode();
void flash_led();
void run_motor();
void stop_motor();
void sleep_mode();
void hunting_mode();
void random_colors();

// ----------------------- SETUP ---------------------------------------

void setup() {
    // Serial communication
    Serial.begin(9600);
    // Initialize I2C  
    Wire.begin();
    delay(500); 
   
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Initialize LED
    strip.begin();
    // Initialize our color to off
    strip.show();  

    // Initialize accelerometer
    if (myIMU.begin()) {
        Serial.println("Ready.");
    } else { 
        Serial.println("Could not connect to IMU.");
    }
    // Apply a set of default configuration settings to the accelerometer sensor
    myIMU.initialize(BASIC_SETTINGS);

    // Initialize motor
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(EN, OUTPUT);

    // Start the motor driver in the disabled state
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    // Set motor speed to 0
    analogWrite(EN, 0); 

    // After device is turned on and initialized, we delay for 30 seconds to screw the ball back together and put it down for play
    delay(30000);
}

// ----------------------- LOOP ----------------------------------------

void loop() {

    // TO-DO: Incorporate data analytics for AWS

    // Update accelerometer data every loop
    x_axis = myIMU.readFloatAccelX();
    y_axis = myIMU.readFloatAccelY();
    z_axis = myIMU.readFloatAccelZ();

    switch (currentState) {

        case PLAY:
            // Go into play mode
            play_mode();
            // Debugging
            Serial.println("State changed to PLAY");
            // If motion is not detected
            if (x_axis == 0 && y_axis == 0 && z_axis == 0) {
                // And if it has been more than 1 minute
                if (millis() - stateStartTime > 60000) { 
                    // Switch to hunting state
                    currentState = HUNTING;
                    // Reset timer for hunting state
                    stateStartTime = millis();
                }
                // If it has been more than 2 minutes
                if (millis() - stateStartTime > 120000) {
                    // Switch to sleep state
                    currentState = SLEEP;
                }
            }
            break;

        case HUNTING:
            // Go into hunting mode
            hunting_mode();
            // Debugging
            Serial.println("State changed to HUNTING");
            // If motion is detected
            if (x_axis > 0 || y_axis > 0 || z_axis > 0) {
                // Switch to play state
                currentState = PLAY;
            } 
            // If it has been more than 2 minutes
            if (millis() - stateStartTime > 120000) {
                // Switch to sleep state
                currentState = SLEEP;
            }
            break;

        case SLEEP:
            // Go into sleep mode
            sleep_mode();
            // Debugging
            Serial.println("State changed to SLEEP");
            // If motion is detected
            if (x_axis || y_axis || z_axis > 0) { 
                // Switch to play state    
                currentState = PLAY;
            }
            break;
    }
}

// ----------------------- FUNCTION DEFINTIONS -------------------------

void random_colors() {
    /*
    Structure of uint32_t in NeoPixel:
	Bits 31–24: Reserved
	Bits 23–16: Red (0–255)
	Bits 15–8:  Green (0–255)
	Bits 7–0:   Blue (0–255)
    strip.Color(red, green, blue)
    */

    // Create a beautiful array of colors using RGB values
    uint32_t colors[] = {
        strip.Color(255, 0, 0),     // Red
        strip.Color(255, 165, 0),   // Orange
        strip.Color(255, 215, 0),   // Gold
        strip.Color(255, 255, 0),   // Yellow
        strip.Color(0, 255, 0),     // Green
        strip.Color(0, 128, 128),   // Teal
        strip.Color(0, 0, 255),     // Blue
        strip.Color(75, 0, 130),    // Indigo
        strip.Color(148, 0, 211),   // Violet
        strip.Color(255, 192, 203)  // Pink
    };

    for (int i = 0; i < 2; i++) {
        // Cycle through each color in the array
        for (int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
            // Set the LED to the current color
            strip.setPixelColor(0, colors[i]);  
            // Display our color
            strip.show();
            delay(250);  
        }
        off_led();
        delay(500);
    }
}

void off_led() {
    // Shut off the LED
    digitalWrite(LED_PIN, LOW);
}

void slow_led() {
    // Turn on LED
    digitalWrite(LED_PIN, HIGH);
    // Flash LED
    random_colors();
    // Delay for 30 seconds
    delay(30000);
}

void flash_led() {
    // Turn on LED
    digitalWrite(LED_PIN, HIGH);
    // Flash LED
    random_colors();
    // Delay for 10 seconds
    delay(10000);
}

void stop_motor() {
    // Stop our motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    // Set motor speed to 0
    analogWrite(EN, 0); 
}

void run_motor(int speed) {                                  // TO-DO: Will likely need to adjust motor speed and movements
    // Move forward
    digitalWrite(IN1, HIGH); 
    digitalWrite(IN2, LOW);  
    analogWrite(EN, speed);
    // Move forward for a random duration (0.5 second to 2 seconds)
    delay(random(500, 2000));

    // Pause motor for a random duration (0.5 second to 1 second)
    stop_motor();
    delay(random(500, 1000));

    // Move backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(EN, speed);
    // Move backward for a random duration (0.5 second to 2 seconds)
    delay(random(500, 2000));

    // Pause motor for a random duration (0.5 second to 1 second)
    stop_motor();
    delay(random(500, 1000));
}

void play_mode() {
    for (int i = 0; i < 10; i++) {  
        // Run the motor with random speed (between 100 and 200)
        int randomSpeed = random(100, 200);
        run_motor(randomSpeed);
        // Random chirping (e.g. 50% chance)
        if (random(0, 10) > 5) {
            chirp();
        }
        // Flash the slower LED
        slow_led();
        // Short random pause after LED (pause between 500ms and 1 second)
        delay(random(500, 1000));
    }
}

// Chirp and flash to get the cat's attention
void hunting_mode() {
    for (int i = 0; i < 10; i++) {  
        // Random chance to chirp first or flash LED first
        if (random(0, 2) == 0) {
            chirp();
            delay(random(300, 700));  
            flash_led();
        } else {
            flash_led();
            delay(random(300, 700));  
            chirp();
        }
        // Short pause after LED
        delay(1000);
    }
}

void sleep_mode() {
    // Turn off LED
    off_led();
    // Stop motor
    stop_motor();
}

void chirp() {
    // High-to-low chirps
    int high_to_low_chirps = 5;
    while (high_to_low_chirps > 0) {
        for (int pitch = 300; pitch > 0; pitch--) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(pitch);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(pitch);
        }
        delay(100);
        high_to_low_chirps--;
    }

    // Low-to-high chirps
    int low_to_high_chirps = 10;
    while (low_to_high_chirps > 0) {
        for (int pitch = 0; pitch < 300; pitch++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(pitch);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(pitch);
        }
        delay(100);
        low_to_high_chirps--;
    }

    // Final high-to-low chirp
    for (int pitch = 300; pitch > 0; pitch--) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(pitch);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(pitch);
    }
    delay(400);

    // High-to-low chirps with a longer delay
    high_to_low_chirps = 5;
    while (high_to_low_chirps > 0) {
        for (int pitch = 300; pitch > 0; pitch--) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(pitch);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(pitch);
        }
        delay(400);
        high_to_low_chirps--;
    }

    // Low-to-high chirps
    low_to_high_chirps = 10;
    while (low_to_high_chirps > 0) {
        for (int pitch = 0; pitch < 300; pitch++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(pitch);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(pitch);
        }
        delay(100);
        low_to_high_chirps--;
    }

    // Final high-to-low chirp
    for (int pitch = 300; pitch > 0; pitch--) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(pitch);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(pitch);
    }
}