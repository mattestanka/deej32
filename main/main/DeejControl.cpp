#include "DeejControl.h"

// External encoders defined in main.cpp
extern Encoder encoder1;
extern Encoder encoder2;

const int SCALE_FACTOR = 2;  // Adjust based on encoder resolution
const int MAX_VALUE = 100;
const int MIN_VALUE = 0;

int numSliders = 0;
int* sliderValues = nullptr;
int* previousValues = nullptr;
bool* mutedStates = nullptr;
String* sliderNames = nullptr;

int currentSlider = 0;
bool buttonPressed = false;

// For long-press on second encoder
unsigned long encoder2PressStart = 0;
bool encoder2LongPressActive = false;

unsigned long lastChangeTime = 0;
unsigned long writeInterval = 10000;  // 10 seconds wait after last change
bool dataDirty = false;
int* lastSavedValues = nullptr;
bool* lastSavedMuted = nullptr;
int* lastSavedPreviousValues = nullptr;

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

void adjustSliderValues(bool& valueChanged) {
    static long lastEncoder1Position = 0;  // Keep track of last hardware reading
    long currentPosition = encoder1.read();

    // Calculate the raw delta
    long rawDelta = currentPosition - lastEncoder1Position;
    
    // Invert direction: either multiply by -1 or flip subtraction
    rawDelta = -rawDelta; 

    // Convert raw delta to "steps" if your encoder goes 2 ticks per step
    int steps = rawDelta / 2;

    // Only act if we got at least one step
    if (steps != 0) {
        // --- CRITICAL CHANGE ---
        // Update lastEncoder1Position to the actual hardware reading
        // so that next loop we measure from the correct baseline
        lastEncoder1Position = currentPosition;

        // Unmute if needed
        if (mutedStates[currentSlider]) {
            sliderValues[currentSlider] = previousValues[currentSlider];
            mutedStates[currentSlider] = false;
        }

        // Adjust slider value, respecting min/max
        sliderValues[currentSlider] = constrain(
            sliderValues[currentSlider] + steps * SCALE_FACTOR,
            MIN_VALUE,
            MAX_VALUE
        );

        valueChanged = true;
    }
}


void changeSliderSelection() {
    static long lastEncoder2Position = 0;  // Track last encoder position
    long currentPosition = encoder2.read();

    int delta = currentPosition - lastEncoder2Position;
    if (delta != 0) {
        lastEncoder2Position = currentPosition;
        currentSlider = (currentSlider + delta + numSliders) % numSliders;
    }
}

void handleMuteUnmute(bool& valueChanged) {
    if (digitalRead(ENCODER1_SW) == LOW && !buttonPressed) {
        delay(200);  // Simple debounce
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
}

void updateDisplay() {
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

    String builtString;
    for (int i = 0; i < numSliders; i++) {
        int mappedVal = map(sliderValues[i], 0, 100, 0, 1023);
        builtString += String(mappedVal);
        if (i < numSliders - 1)
            builtString += "|";
    }
    Serial.println(builtString);
}

void handleSaving(bool valueChanged) {
    if (valueChanged) {
        dataDirty = true;
        lastChangeTime = millis();
    }

    if (dataDirty && (millis() - lastChangeTime > writeInterval)) {
        if (valuesAreDifferent()) {
            File file = SPIFFS.open("/sliders_config.json", "w");
            StaticJsonDocument<1024> doc;
            doc["num_sliders"] = numSliders;
            JsonArray sliders = doc.createNestedArray("sliders");

            for (int i = 0; i < numSliders; i++) {
                JsonObject s = sliders.createNestedObject();
                s["name"] = sliderNames[i];
                s["value"] = sliderValues[i];
                s["muted"] = mutedStates[i];
                s["previous_value"] = previousValues[i];
            }

            serializeJson(doc, file);
            file.close();
            markDataSaved();
            Serial.println("Saved after 10s of inactivity.");
        } else {
            dataDirty = false;
        }
    }
}

void initDeejControl() {
    if (!loadSliderConfig()) {
        Serial.println("Failed to load slider config, and no default could be created.");
        displayError("Config Error!", "Please upload config.");
        delay(1000);
        startWifiSetupMode();
    }

    pinMode(ENCODER1_SW, INPUT_PULLUP);
    pinMode(ENCODER2_SW, INPUT_PULLUP);

    Serial.println("Deej Control Initialized");
}

void checkLongPress() {
    if (digitalRead(ENCODER2_SW) == LOW && !encoder2LongPressActive) {
        encoder2PressStart = millis();
        encoder2LongPressActive = true;
    } else if (digitalRead(ENCODER2_SW) == HIGH && encoder2LongPressActive) {
        encoder2LongPressActive = false;
    }

    if (encoder2LongPressActive && (millis() - encoder2PressStart > 10000)) {
        encoder2LongPressActive = false;
        Serial.println("Long press detected. Entering WiFi setup mode.");
        startWifiSetupMode();
    }
}

void runDeejControl() {
    if (inWifiSetupMode) {
        return;
    }

    bool valueChanged = false;

    adjustSliderValues(valueChanged);
    changeSliderSelection();
    handleMuteUnmute(valueChanged);
    updateDisplay();
    handleSaving(valueChanged);
    checkLongPress();

    delay(1);  // Smooth loop
}
