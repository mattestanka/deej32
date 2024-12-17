#ifndef DEEJ_CONTROL_H
#define DEEJ_CONTROL_H

#include <Wire.h>
#include <U8g2lib.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

extern int numSliders;
extern int* sliderValues;       // Current values (0–100)
extern int* previousValues;     // Values before muting
extern bool* mutedStates;       // True if slider is muted
extern String* sliderNames;

void initDeejControl();
void runDeejControl();

#endif
