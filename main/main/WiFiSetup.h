#ifndef WIFISETUP_H
#define WIFISETUP_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

extern bool wifiSetupDone;
extern bool useWifi;
extern bool inWifiSetupMode; // Declare that we're using AP mode and stopping Deej control
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern const bool useTxPowerControl;

void initWiFiSetup();
void handleWiFiTasks();
void displayMessage(String line1, String line2, String line3);
void startAccessPoint();
void startWifiSetupMode();
void saveWiFiCredentials(const char* ssid, const char* password);
bool loadWiFiCredentials(String &ssid, String &password);
void saveWiFiSetting(bool enabled);

#endif
