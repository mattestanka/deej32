#ifndef DEEJCONTROL_H
#define DEEJCONTROL_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

extern const int ENCODER1_CLK;
extern const int ENCODER1_DT;
extern const int ENCODER1_SW;

extern const int ENCODER2_CLK;
extern const int ENCODER2_DT;
extern const int ENCODER2_SW;

void initDeejControl();
void runDeejControl();

#endif
