#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <SPIFFS.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2; // Use extern for u8g2 instance
extern bool wifiSetupDone;

void initWiFiSetup();
void handleWiFiTasks();

#endif
