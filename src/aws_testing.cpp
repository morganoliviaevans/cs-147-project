// Modified Lab 4 code 

#include <SPI.h>
#include <WiFi.h>
#include <stdio.h>
#include <Arduino.h>
#include <inttypes.h>
#include <HttpClient.h>

#include "nvs.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

// Network SSID
char ssid[50];
// Network Password
char pass[50];

unsigned long stateStartTime = millis();
unsigned long playTime = 0; // For tracking PLAY state total time
unsigned long sleepTime = 0; // For tracking SLEEP state total time
unsigned long playStartTime = 0;
unsigned long sleepStartTime = 0;

// ------------------------------------- PART B --------------------------------------
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
// ------------------------------------- PART B --------------------------------------

// // --------------------------------- FIND MAC ADDRESS -----------------------------------
// void setup() {
//     Serial.begin(9600);
//     Serial.print("MAC Address: ");
//     Serial.println(WiFi.macAddress());
// }

// void loop() {
//   Serial.println("MAC: ");
//   Serial.println(WiFi.macAddress());
//   delay(1000);
// }
// // --------------------------------- FIND MAC ADDRESS -----------------------------------

// // ------------------------------------- PART A --------------------------------------
// void setup()
// {
//     Serial.begin(9600);
//     delay(1000);
//     // Initialize NVS
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // NVS partition was truncated and needs to be erased
//         // Retry nvs_flash_init
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(err);
//     Serial.printf("\n");
//     Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
//     nvs_handle_t my_handle;
//     err = nvs_open("storage", NVS_READWRITE, &my_handle);
//     if (err != ESP_OK) {
//         Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
//     } else {
//         Serial.printf("Done\n");
//         // Write
//         Serial.printf("Updating ssid/pass in NVS ... ");
//         // Enter Wifi network name here
//         char ssid[] = "That one phone";
//         // Enter WiFi network password here       
//         char pass[] = "vpyn2327";        
//         err = nvs_set_str(my_handle, "ssid", ssid);
//         err |= nvs_set_str(my_handle, "pass", pass);
//         Serial.printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
//         /*
//         Commit written value:
//         After setting any values, nvs_commit() must be called to ensure changes are written to flash storage.
//         Implementations may write to storage at other times, but this is not guaranteed.
//         */
//         Serial.printf("Committing updates in NVS ... ");
//         err = nvs_commit(my_handle);
//         Serial.printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
//         // Close
//         nvs_close(my_handle);
//     }
// }
// // ------------------------------------- PART A --------------------------------------

// ------------------------------------- PART B --------------------------------------

void setup() {
    Serial.begin(9600);
    Wire.begin();
    delay(1000);

    // Retrieve SSID/PASSWD from flash before anything else
    nvs_access();

    // Connect to Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC address: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    unsigned long currentPlayTime = millis() - playStartTime;
    playTime += currentPlayTime;
}


void dht() {

    //  READ DATA
    uint32_t start = micros();
    uint32_t stop = micros();

    // Read temperature and humidity from the sensor
    float sleepTime;
    // Print values to the serial monitor

    int err = 0;
    WiFiClient c;
    // Format the query string with temperature and humidity
    char query[100];

    err = http.get("3.85.208.114", 5000, query, NULL);
    if (err == 0) {
        Serial.println();
        Serial.println("We're looking good...");

        err = http.responseStatusCode();
        if (err >= 0) {
            Serial.print("Response status code: ");
            Serial.println(err);
        } 
        if (err == 200) {
            Serial.print("Success! Woohoo!");
            Serial.println();
        }
        else {
            Serial.print("Failed to get response status code: ");
            Serial.println(err);
        }
    } else {
        Serial.print("HTTP request failed: ");
        Serial.println(err);
    }
    http.stop();

    // Pause before repeating
    delay(2000);
}

// ------------------------------------- PART B --------------------------------------
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
void save_time_to_csv(unsigned long playTime, unsigned long sleepTime) {
    // APPENDS new times
    File dataFile = SD.open("/times.csv", FILE_APPEND);

    if (dataFile) {
        dataFile.print(playTime);
        dataFile.print(",");
        dataFile.println(sleepTime);  
        dataFile.close();
    } else {
        Serial.println("Error opening times.csv");
    }
}