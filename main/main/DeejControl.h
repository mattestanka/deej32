#ifndef DEEJCONTROL_H
#define DEEJCONTROL_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <Encoder.h> // Include Encoder library

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

extern const int ENCODER1_CLK;
extern const int ENCODER1_DT;
extern const int ENCODER1_SW;

extern const int ENCODER2_CLK;
extern const int ENCODER2_DT;
extern const int ENCODER2_SW;

extern Encoder encoder1;
extern Encoder encoder2;

extern bool inWifiSetupMode;

extern void startWifiSetupMode();
extern void displayError(const char* line1, const char* line2);

void initDeejControl();
void runDeejControl();

#endif
