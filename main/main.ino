#include <Arduino.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include "WiFiSetup.h" // Handles WiFi-related functionality
#include "DeejControl.h" // Handles Deej slider control

// Pin definitions
const int ENCODER1_CLK = 3; // Define CLK pin for encoder 1
const int ENCODER1_DT = 10; // Define DT pin for encoder 1
const int ENCODER1_SW = 8; // Define SW pin for encoder 1

const int ENCODER2_CLK = 4; // Define CLK pin for encoder 2
const int ENCODER2_DT = 6; // Define DT pin for encoder 2
const int ENCODER2_SW = 5; // Define SW pin for encoder 2

const int OLED_SCL = 1; // Define SCL pin for OLED
const int OLED_SDA = 2; // Define SDA pin for OLED

const bool useTxPowerControl = true; // If you're using an ESP32 C3 Supermini and experiencing WiFi connection issues, set this to true.


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

extern bool wifiSetupDone;
extern bool useWifi;
extern bool inWifiSetupMode; 

void displayError(const char* line1, const char* line2) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, line1);
    u8g2.drawStr(0, 35, line2);
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    u8g2.begin();

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        displayError("SPIFFS Error", "Restart Required");
        while (true) { delay(1000); }
    } else {
        Serial.println("SPIFFS Mounted Successfully");
    }

    // Initialize WiFi setup
    initWiFiSetup();

    // Initialize Deej Slider Control
    initDeejControl();
}

void loop() {
    // Handle WiFi setup mode
    if (inWifiSetupMode) {
        handleWiFiTasks();
        return; // Skip DeejControl logic while in setup mode
    }

    // Normal operation
    if (!wifiSetupDone && useWifi) {
        handleWiFiTasks(); // Continue handling WiFi tasks until setup is done
    } else if (useWifi) {
        handleWiFiTasks(); // Run WiFi-related tasks
        runDeejControl();  // Run Deej logic alongside
    } else {
        runDeejControl();  // If WiFi is disabled, only run Deej logic
    }
}
