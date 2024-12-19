#include "DeejControl.h"
#include "WiFiSetup.h"

const int SCALE_FACTOR = 2; 
const int MAX_VALUE = 100;
const int MIN_VALUE = 0;

int numSliders = 0;
int* sliderValues = nullptr;   
int* previousValues = nullptr;
bool* mutedStates = nullptr;
String* sliderNames = nullptr;

volatile int encoderDelta1 = 0;   
volatile int encoderDelta2 = 0;   
bool buttonPressed = false;
int currentSlider = 0;

unsigned long lastChangeTime = 0;   
unsigned long writeInterval = 10000; // 10 seconds wait after last change
bool dataDirty = false;
int* lastSavedValues = nullptr;  
bool* lastSavedMuted = nullptr;  
int* lastSavedPreviousValues = nullptr;

// For long-press on second encoder
unsigned long encoder2PressStart = 0;
bool encoder2LongPressActive = false;

// External variables
extern bool inWifiSetupMode;

void IRAM_ATTR encoder1ISR() {
    if (digitalRead(ENCODER1_CLK) == digitalRead(ENCODER1_DT))
        encoderDelta1 = -SCALE_FACTOR; 
    else
        encoderDelta1 = SCALE_FACTOR;  
}

void IRAM_ATTR encoder2ISR() {
    if (digitalRead(ENCODER2_CLK) == digitalRead(ENCODER2_DT))
        encoderDelta2 = -1; 
    else
        encoderDelta2 = 1;  
}

bool valuesAreDifferent() {
    for (int i = 0; i < numSliders; i++) {
        if (sliderValues[i] != lastSavedValues[i]) return true;
        if (mutedStates[i] != lastSavedMuted[i]) return true;
        if (previousValues[i] != lastSavedPreviousValues[i]) return true;
    }
    return false;
}

void markDataSaved() {
    for (int i = 0; i < numSliders; i++) {
        lastSavedValues[i] = sliderValues[i];
        lastSavedMuted[i] = mutedStates[i];
        lastSavedPreviousValues[i] = previousValues[i];
    }
    dataDirty = false;
}

bool loadSliderConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }

    if (!SPIFFS.exists("/sliders_config.json")) {
        Serial.println("No config found, creating default with 3 sliders.");
        StaticJsonDocument<512> doc;
        doc["num_sliders"] = 3;
        JsonArray sliders = doc.createNestedArray("sliders");

        const char* defaultNames[3] = {"Master", "System", "Mic"};
        int defaultValues[3] = {100, 100, 100};

        for (int i = 0; i < 3; i++) {
            JsonObject s = sliders.createNestedObject();
            s["name"] = defaultNames[i];
            s["value"] = defaultValues[i];
            s["muted"] = false;
            s["previous_value"] = defaultValues[i];
        }

        File file = SPIFFS.open("/sliders_config.json", "w");
        if (!file) {
            Serial.println("Failed to create default sliders_config.json");
            return false;
        }
        serializeJson(doc, file);
        file.close();
    }

    File file = SPIFFS.open("/sliders_config.json", "r");
    if (!file) {
        Serial.println("Failed to open sliders_config.json");
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.print("Failed to parse sliders_config.json: ");
        Serial.println(error.f_str());
        return false;
    }

    numSliders = doc["num_sliders"];
    if (numSliders <= 0) {
        Serial.println("Invalid number of sliders in config.");
        return false;
    }

    sliderValues = new int[numSliders];
    previousValues = new int[numSliders];
    mutedStates = new bool[numSliders];
    sliderNames = new String[numSliders];
    lastSavedValues = new int[numSliders];
    lastSavedMuted = new bool[numSliders];
    lastSavedPreviousValues = new int[numSliders];

    JsonArray sliders = doc["sliders"].as<JsonArray>();
    for (int i = 0; i < numSliders; i++) {
        JsonObject s = sliders[i];
        sliderNames[i] = s["name"].as<String>();

        int val = s["value"] | 50; 
        bool muted = s["muted"] | false;
        int prevVal = s["previous_value"] | val;

        sliderValues[i] = val;
        previousValues[i] = prevVal;
        mutedStates[i] = muted;

        lastSavedValues[i] = val;
        lastSavedMuted[i] = muted;
        lastSavedPreviousValues[i] = prevVal;
    }

    Serial.println("Slider config loaded successfully.");
    return true;
}

bool saveSliderConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }

    File file = SPIFFS.open("/sliders_config.json", "r");
    if (!file) {
        Serial.println("No config file to update, won't save.");
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.print("Failed to parse existing sliders_config.json: ");
        Serial.println(error.f_str());
        return false;
    }

    doc["num_sliders"] = numSliders;
    JsonArray sliders = doc["sliders"].as<JsonArray>();

    for (int i = 0; i < numSliders; i++) {
        JsonObject s = sliders[i];
        s["name"] = sliderNames[i];
        s["value"] = sliderValues[i];
        s["muted"] = mutedStates[i];
        s["previous_value"] = previousValues[i];
    }

    file = SPIFFS.open("/sliders_config.json", "w");
    if (!file) {
        Serial.println("Failed to open sliders_config.json for writing");
        return false;
    }
    serializeJson(doc, file);
    file.close();

    Serial.println("Slider config saved successfully.");
    return true;
}

void initDeejControl() {
    if (!loadSliderConfig()) {
        numSliders = 5;
        sliderValues = new int[numSliders];
        previousValues = new int[numSliders];
        mutedStates = new bool[numSliders];
        sliderNames = new String[numSliders];
        lastSavedValues = new int[numSliders];
        lastSavedMuted = new bool[numSliders];
        lastSavedPreviousValues = new int[numSliders];

        const char* defaultNames[5] = {"Volume", "Brightness", "Bass", "Treble", "Balance"};
        int defaultValues[5] = {50, 30, 60, 40, 20};

        for (int i = 0; i < numSliders; i++) {
            sliderNames[i] = defaultNames[i];
            sliderValues[i] = defaultValues[i];
            previousValues[i] = defaultValues[i];
            mutedStates[i] = false;
            lastSavedValues[i] = defaultValues[i];
            lastSavedMuted[i] = false;
            lastSavedPreviousValues[i] = defaultValues[i];
        }
        Serial.println("Loaded default slider config.");
    }

    pinMode(ENCODER1_CLK, INPUT_PULLUP);
    pinMode(ENCODER1_DT, INPUT_PULLUP);
    pinMode(ENCODER1_SW, INPUT_PULLUP);
    pinMode(ENCODER2_CLK, INPUT_PULLUP);
    pinMode(ENCODER2_DT, INPUT_PULLUP);
    pinMode(ENCODER2_SW, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENCODER1_CLK), encoder1ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER2_CLK), encoder2ISR, CHANGE);

    Serial.println("Deej Control Initialized");
}

extern void startWifiSetupMode();

void runDeejControl() {
    // If we are in WiFi setup mode, do nothing
    if (inWifiSetupMode) {
        return;
    }

    bool valueChanged = false;

    // Check long-press on ENCODER2_SW
    if (digitalRead(ENCODER2_SW) == LOW && !encoder2LongPressActive) {
        encoder2PressStart = millis();
        encoder2LongPressActive = true;
    } else if (digitalRead(ENCODER2_SW) == HIGH && encoder2LongPressActive) {
        encoder2LongPressActive = false;
    }

    // If pressed > 10s
    if (encoder2LongPressActive && (millis() - encoder2PressStart > 10000)) {
        encoder2LongPressActive = false;
        Serial.println("Long press detected. Entering WiFi setup mode.");
        startWifiSetupMode();
        return;
    }

    // Adjust Slider Values
    if (encoderDelta1 != 0) {
        if (mutedStates[currentSlider]) {
            // Unmute before adjusting
            sliderValues[currentSlider] = previousValues[currentSlider];
            mutedStates[currentSlider] = false;
        }

        sliderValues[currentSlider] = constrain(sliderValues[currentSlider] + encoderDelta1, MIN_VALUE, MAX_VALUE);
        encoderDelta1 = 0;
        valueChanged = true;
    }

    // Change Slider Selection
    if (encoderDelta2 != 0) {
        currentSlider = (currentSlider + encoderDelta2 + numSliders) % numSliders;
        encoderDelta2 = 0;
    }

    // Check Mute/Unmute Button
    if (digitalRead(ENCODER1_SW) == LOW && !buttonPressed) {
        delay(200);  // Debounce
        if (mutedStates[currentSlider]) {
            sliderValues[currentSlider] = previousValues[currentSlider];
            mutedStates[currentSlider] = false;
        } else {
            previousValues[currentSlider] = sliderValues[currentSlider];
            sliderValues[currentSlider] = 0;
            mutedStates[currentSlider] = true;
        }
        buttonPressed = true;
        valueChanged = true;
    } else if (digitalRead(ENCODER1_SW) == HIGH) {
        buttonPressed = false;
    }

    // Update Display
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 15);
    u8g2.print(sliderNames[currentSlider] + " (" + String(currentSlider + 1) + "/" + String(numSliders) + ")");

    int barWidth = map(sliderValues[currentSlider], 0, 100, 0, 105); 
    u8g2.drawFrame(0, 64 - 15 - 2, 105, 15);
    u8g2.drawBox(0, 64 - 15 - 2, barWidth, 15);

    if (mutedStates[currentSlider]) {
        u8g2.setCursor(115, 59);
        u8g2.print("M");
    } else {
        u8g2.setCursor(110, 59);
        u8g2.print(String(sliderValues[currentSlider]));
    }

    u8g2.sendBuffer();

    // Print to Serial in 0–1023 scale
    String builtString;
    for (int i = 0; i < numSliders; i++) {
        int mappedVal = map(sliderValues[i], 0, 100, 0, 1023);
        builtString += String(mappedVal);
        if (i < numSliders - 1)
            builtString += "|";
    }
    Serial.println(builtString);

    // Handle saving logic
    if (valueChanged) {
        dataDirty = true;
        lastChangeTime = millis();
    }

    if (dataDirty && (millis() - lastChangeTime > writeInterval)) {
        // 10 seconds of inactivity
        if (valuesAreDifferent()) {
            if (saveSliderConfig()) {
                markDataSaved();
                Serial.println("Saved after 10s of inactivity.");
            }
        } else {
            dataDirty = false;
        }
    }

    delay(10);
}
