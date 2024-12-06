/*
CS 147: IoT Software and Systems
Course Project - Motorized Cat Toy
Team Member 1: Morgan Newton
Team Member 2: Carm Hermosilla
Due Date: 12-13-2024
*/

/*
Project Breakdown:

Initialization and Setup
    1. Initialize the microcontroller and set up peripherals: LEDs, motor, accelerometer, buzzer
    2. Define all GPIO pins for the connected components
    3. Set default states for LEDs (off), motor (idle), and buzzer (off)

State 1: Play State
    1. Detect accelerometer activity
    2. Did our x-axis, y-axis, or z-axis move? Activate the toy!
    3. Adjust LED flashing rate to conserve battery (flash once every 30 seconds)
    4. Continue randomizing motor speed and direction
    5. Continue periodic bird noises
    6. If the toy is idle for 1 minute, switch to hunting state
    7. Track play time within play state -- are we playing? Add this play time to our stored play time variable
    8. Send play state analytics to the Cloud

State 2: Hunting State
    1. Check accelerometer data for inactivity for 1 minute
    2. Trigger attention-grabbing behavior -- LED flashing, bird noises
    3. Once cat moves the ball, we switch back to play state
    4. If the cat does not move the ball after 2 minutes, go into sleep state

State 3: Sleep State
    1. Turn off LEDs and turn off the motor to conserve battery
    2. Keep accelerometer active to detect the cat’s return
    3. Re-enter play state upon detecting motion
    4. Track sleep time within sleep state -- are we sleeping? Add this sleep time to our stored sleep time variable
    5. Send sleep state analytics to the Cloud
*/

// ----------------------- BASIC LIBRARIES -----------------------------
#include <SPI.h>
#include <Wire.h>
#include <stdio.h>
#include <Arduino.h>
// ----------------------- CLOUD ---------------------------------------
// #include "nvs.h"
// #include "esp_wifi.h"
// #include <inttypes.h>
#include "nvs_flash.h"
// #include "esp_system.h"
#include <HttpClient.h>
// #include "freertos/task.h"
// #include "freertos/FreeRTOS.h"
#include <WiFi.h>
// #include <AWS_IOT.h> // IoT SDK
// ----------------------- ACCELEROMETER -------------------------------
#include "SparkFunLSM6DSO.h"
// ----------------------- LED -----------------------------------------
#include <Adafruit_NeoPixel.h>

// ----------------------- VARIABLE DECLARATIONS -----------------------

#define BUZZER_PIN 12
#define LED_PIN 32
#define NUM_LEDS 7

// Motor pins
#define ENA 2           // PWM pin for Motor 1 (speed control)
#define ENB 15          // PWM pin for Motor 2 (speed control)
#define IN1 27          // Motor 1 Direction
#define IN2 26          // Motor 1 Direction
#define IN3 25          // Motor 2 Direction
#define IN4 33          // Motor 2 Direction

// Network SSID
char ssid[50];
// Network Password
char pass[50];

// Motor PWM Configurations (speed control)
int freq = 5000;        // PWM frequency
int resolution = 8;     // 8-bit resolution (0-255 for duty cycle)
int pwmChannelA = 0;    // PWM Channel for Motor 1
int pwmChannelB = 1;    // PWM Channel for Motor 2

unsigned long stateStartTime = millis();
unsigned long playTime = 0; // For tracking PLAY state total time
unsigned long sleepTime = 0; // For tracking SLEEP state total time
unsigned long playStartTime = 0;
unsigned long sleepStartTime = 0;

// NeoPixel strip object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Device States
enum DeviceState { PLAY, HUNTING, SLEEP };
DeviceState currentState = PLAY;

// Accelerometer object
LSM6DSO myIMU;
// Accelerometer variables
float x_axis, y_axis;
float magnitude;

// ----------------------- FUNCTION DECLARATIONS -----------------------

void chirp();
void off_led();
void slow_led();
void play_mode();
void flash_led();
void run_motors(int speed);
void stop_motors();
void sleep_mode();
void hunting_mode();
void random_colors();
void nvs_access();
void send_time_AWS();

// ----------------------- SETUP ---------------------------------------

void setup() {
    // Serial communication
    Serial.begin(9600);
    delay(500);
    // Initial state
    Serial.println("Our initial state is PLAY!"); 
    // Initialize I2C  
    Wire.begin();
    delay(500); 
    // Get Wifi creds for non-volatile storage
    nvs_access(); 
    // Connect to Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    // While wifi not connected...
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println(".");
    }
    Serial.println("Connected to WiFi. Have fun!");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC address: ");
    Serial.println(WiFi.macAddress());
    // // Begins AWS instance 
    // AWS_IOT aws; 
    // aws.begin();
    send_time_AWS(playTime, sleepTime);
    playStartTime = millis();
   
    // Initialize accelerometer
    if (myIMU.begin()) {
        Serial.println("Ready.");
    } else { 
        Serial.println("Could not connect to IMU.");
    }
    // Apply a set of default configuration settings to the accelerometer sensor
    myIMU.initialize(BASIC_SETTINGS);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Initialize LED
    strip.begin();
    // Initialize our color to off
    strip.show();  

    // Initialize motor
    // Set direction pins as outputs
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    // Start the motor driver in a disabled state
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    // Configure PWM channels
    ledcSetup(pwmChannelA, freq, resolution);
    ledcSetup(pwmChannelB, freq, resolution);

    // Attach PWM channels to GPIO pins
    ledcAttachPin(ENA, pwmChannelA);
    ledcAttachPin(ENB, pwmChannelB);

    // Set motor speed to 0
    ledcWrite(pwmChannelA, 0);
    ledcWrite(pwmChannelB, 0);

    // After device is turned on and initialized, we delay for 30 seconds to screw the ball back together and put it down for play
    //delay(30000);
}

// ----------------------- LOOP ----------------------------------------

void loop() {

    // TO-DO: Incorporate data analytics for AWS
    // Create variable to store play time
    // Create variable to store sleep time
    // Track play time within PLAY state -- are we playing? Add this play time to our stored play time variable
    // Track sleep time within SLEEP state -- are we sleeping? Add this sleep time to our stored sleep time variable
    // This will let us know on average how often the cats are playing with the device, and how long the device is sleeping
    // Send both PLAY state and SLEEP state analytics to the Cloud

    // Update accelerometer data every loop
    x_axis = myIMU.readFloatAccelX();
    y_axis = myIMU.readFloatAccelY();

    // Print values for debugging
    Serial.print("x_axis: "); 
    Serial.println(x_axis);
    Serial.print("y_axis: "); 
    Serial.println(y_axis);
    Serial.print("Magnitude: "); 
    Serial.println(magnitude);
    Serial.println();

    switch (currentState) {

        case PLAY:
            // Debugging
            Serial.println("State: PLAY");
            Serial.println();
            // Go into play mode
            play_mode();
            // For tracking the current play time session
            unsigned long currentPlayTime = millis() - playStartTime;
            playTime += currentPlayTime;
            // stateStartTime = millis(); // Reset timer
            send_time_AWS(playTime, sleepTime);
            // Calculate motion magnitude
            magnitude = sqrt(x_axis * x_axis + y_axis * y_axis);

            // If motion is not detected
            if (magnitude < 0.5) {
                unsigned long elapsed = millis() - stateStartTime;
                // And if it has been more than 1 minute
                if (millis() - stateStartTime > 10000) { 
                    // Switch to hunting state
                    currentState = HUNTING;
                    // Reset timer for hunting state
                    stateStartTime = millis();
                }
            }
            break;

        case HUNTING:
            // Debugging
            Serial.println("State: HUNTING");
            Serial.println();
            // Reset playTime 
            send_time_AWS(playTime, sleepTime);
            playTime = 0; // Reset playTime after sending
            // Go into hunting mode
            hunting_mode();

            // Calculate motion magnitude
            magnitude = sqrt(x_axis * x_axis + y_axis * y_axis);

            // If motion is detected
            if (magnitude > 0.5) {
                // Switch to play state
                currentState = PLAY;
                stateStartTime = millis();
            } 
            // If it has been more than 2 minutes
            if (millis() - stateStartTime > 20000) {
                // Switch to sleep state
                currentState = SLEEP;
                stateStartTime = millis();
            }
            break;

        case SLEEP:
            // Debugging
            Serial.println("State: SLEEP");
            Serial.println();
            sleepTime = millis() - sleepStartTime;

            // Send play and sleep time to AWS
            send_time_AWS(playTime, sleepTime);
            // Go into sleep mode
            sleep_mode();

            // Calculate motion magnitude
            magnitude = sqrt(x_axis * x_axis + y_axis * y_axis);

            // If motion is detected
            if (magnitude > 0.5) { 
                // Switch to play state    
                currentState = PLAY;
                stateStartTime = millis();
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

    for (int i = 0; i < 4; i++) {
        // Cycle through each color in our array
        for (int colorIndex = 0; colorIndex < sizeof(colors) / sizeof(colors[0]); colorIndex++) {
            // Set all LEDs to the same color
            for (int i = 0; i < NUM_LEDS; i++) {
                strip.setPixelColor(i, colors[colorIndex]);
            }
            // Display the color on the LED
            strip.show();
            // Hold each color for 0.200 second
            delay(200); 
        }
        // Turn off the LED
        off_led();
    }
}

void off_led() {
    // Shut off the LED by setting it to black
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    // Display "black" e.g. show that it is shut off
    strip.show();
}

void slow_led() {
    // Turn on LED
    digitalWrite(LED_PIN, HIGH);
    // Flash LED
    random_colors();
    // Delay for 2 seconds
    delay(2000);
}

void flash_led() {
    // Turn on LED
    digitalWrite(LED_PIN, HIGH);
    // Flash LED
    random_colors();
}

void stop_motors() {
    // Stop Motor 1
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(pwmChannelA, 0);

    // Stop Motor 2
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(pwmChannelB, 0);
}

void run_motors(int speed) {                                            
    // Move forward (both motors complement each other)
    // Motor 1 forward
    Serial.println("Motor 1 Forward...");
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    // Motor 2 forward (opposite wiring)
    Serial.println("Motor 2 Forward...");
    digitalWrite(IN3, LOW);   
    digitalWrite(IN4, HIGH);
    ledcWrite(pwmChannelA, speed);
    ledcWrite(pwmChannelB, speed);
    // Move forward for a random duration (0.200 second to 0.5 second)
    delay(random(200, 500));

    // Pause motors for a random duration (0.200 second to 0.5 second)
    stop_motors();
    Serial.println("Stopping Motors...");
    delay(random(200, 500));

    // Move backward (both motors complement each other)
    // Motor 1 backward
    Serial.println("Motor 1 Backward...");
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    // Motor 2 backward (opposite wiring)
    Serial.println("Motor 2 Backward...");
    digitalWrite(IN3, HIGH); 
    digitalWrite(IN4, LOW);
    ledcWrite(pwmChannelA, speed);
    ledcWrite(pwmChannelB, speed);
    // Move backward for a random duration (0.200 second to 0.5 second)
    delay(random(200, 500));

    // Pause motors
    stop_motors();
}

void play_mode() {
        Serial.println("Play Mode...");
        delay(1000);
        // Run the motor with random speed (between 100 and 200)
        //int randomSpeed = random(100, 200);
        run_motors(200);
        // Chirp!
        chirp();
        // Flash the slower LED
        slow_led();
}

// Chirp and flash to get the cat's attention
void hunting_mode() {
        Serial.println("Hunting Mode...");
        delay(1000);
        // Random chance to chirp first or flash LED first
        if (random(0, 2) == 0) {
            chirp();
            flash_led();
        } else {
            flash_led();  
            chirp();
        }
}

void sleep_mode() {
    Serial.println("Sleep Mode...");
    // Turn off LED
    off_led();
    // Stop motors
    stop_motors();
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

// ------------------------------------- Data Analytics --------------------------------------

// Non-volatile storage: Keeps data even if toy runs out of battery
void nvs_access() {

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    Serial.printf("\n");
    Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        Serial.printf("Done\n");
        Serial.printf("Retrieving SSID/PASSWD\n");

        size_t ssid_len;
        size_t pass_len;

        err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        err |= nvs_get_str(my_handle, "pass", pass, &pass_len);

        switch (err) {
            case ESP_OK:
                Serial.printf("Done\n");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                Serial.printf("The value is not initialized yet!\n");
                break;
            default:
                Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }
    // Close
    nvs_close(my_handle);
}

void send_time_AWS(unsigned long playTime, unsigned long sleepTime) {
    Serial.print("Sending play time to AWS: ");
    Serial.println(playTime);
    Serial.print("Sending sleep time to AWS: ");
    Serial.println(sleepTime);

    WiFiClient client; 
    // Send POST request
    if (client.connect("3.85.208.114", 5000)) {
        String payload = "{\"playTime\": " + String(playTime) + ", \"sleepTime\": " + String(sleepTime) + "}";

        client.println("POST /send-time HTTP/1.1");
        client.println("Host: 3.85.208.114");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(payload.length());
        client.println();
        client.print(payload);

        // Read response from client (the debugger)
        String response = client.readString();
        Serial.println("Response: " + response);

        client.stop();
    } else {
        Serial.println("Failed to connect to the server.");
    }
}

// Saves sleep & play times as a .csv for matplotlib graphing (OPT)
// TODO: Fix for non-SD card purposes
// void save_time_to_csv(unsigned long playTime, unsigned long sleepTime) {
//     // APPENDS new times
//     File dataFile = SD.open("/times.csv", FILE_APPEND);

//     if (dataFile) {
//         dataFile.print(playTime);
//         dataFile.print(",");
//         dataFile.println(sleepTime);  
//         dataFile.close();
//     } else {
//         Serial.println("Error opening times.csv");
//     }
// }