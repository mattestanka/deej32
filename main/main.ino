#include "WiFiSetup.h"
#include "DeejControl.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 17, 5);

void setup() {
    Serial.begin(115200);
    u8g2.begin();

    // Initialize WiFi setup
    initWiFiSetup();

    // Initialize Deej Slider Control
    initDeejControl();
}

void loop() {
    if (!wifiSetupDone) {
        handleWiFiTasks(); // Manage AP tasks
    } else {
        handleWiFiTasks(); // Still serve any incoming web requests
        runDeejControl();  // Run Deej functionality
    }
}
