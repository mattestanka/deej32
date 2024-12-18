#include "WiFiSetup.h"
#include "DeejControl.h"
#include <SPIFFS.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 17, 5);

void setup() {
    Serial.begin(115200);
    u8g2.begin();

     // Initialize SPIFFS
    if (!SPIFFS.begin(true)) { // `true` formats the file system if it's not initialized
        Serial.println("SPIFFS Mount Failed");
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 15, "SPIFFS Mount Failed");
        u8g2.drawStr(0, 35, "Restart Required");
        u8g2.sendBuffer();
        while (true) { // Halt execution if the file system fails to mount
            delay(1000);
        }
    } else {
        Serial.println("SPIFFS Mounted Successfully");
    }

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
