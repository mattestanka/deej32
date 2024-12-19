# ESP32 Deej Controller with Rotary Encoders and Display

This project allows you to control audio sliders in **deej** using an ESP32 with two rotary encoders and a display. One rotary encoder loops through the available sliders, and the other adjusts the volume levels in real-time.

> **Note:** This is a very early version, but it should work!

![ESP32 Deej Controller Image](https://github.com/user-attachments/assets/56917704-07d3-4f35-8b90-339ccc2c80d7)

---

## Features
- Two rotary encoders: one to select sliders and another to control volume.
- Display to show the currently selected slider and its volume level.
- JSON-based configuration for customization of slider names and values.
- Web UI for easy customization of slider settings.

---

## Required Hardware
1. **ESP32** development board (I used an ESP32 C3 Supermini).
2. **2 Rotary Encoders** (I used HW-040, but similar encoders can be used).
3. **Display** (e.g., OLED or LCD compatible with ESP32). 
   - I used the **SH1106 1.3-inch I2C OLED 128x64** display. If you use a different display, you may need to modify the code accordingly.

---

## Schematic & Wiring Guide
You can customize the pins in the `main.ino` file. The default wiring is as follows:

### Rotary Encoder 1:
- **GND** → GND
- **+** → 3.3V
- **SW** → 8
- **DT** → 10
- **CLK** → 3

### Rotary Encoder 2:
- **GND** → GND
- **+** → 3.3V
- **SW** → 5
- **DT** → 6
- **CLK** → 4

### Display (I2C):
- **SDA** → 2
- **SCL** → 1
- **VCC** → 3.3V
- **GND** → GND

---

## Setup Instructions
1. Install **deej** on your computer: [Get deej here](https://github.com/omriharel/deej).
2. Flash the provided Arduino code to your ESP32.
3. Connect the ESP32 to your computer via USB.
4. Connect to the `DEEJ` WiFi network and navigate to `192.168.4.1`.
5. Upload your custom slider configuration via the web UI.
6. Run **deej** on your computer, and configure it to recognize the serial input from the ESP32.
7. Test the rotary encoders to navigate sliders and control volume.

---

## JSON Customization
The project supports a simple JSON structure for defining sliders. Update this configuration to match your preferred setup:

```json
{
    "num_sliders": 3,
    "sliders": [
        {
            "name": "Main",
            "value": 100,
            "muted": false,
            "previous_value": 100
        },
        {
            "name": "System",
            "value": 100,
            "muted": false,
            "previous_value": 100
        },
        {
            "name": "Mic",
            "value": 100,
            "muted": false,
            "previous_value": 100
        }
    ]
}
```
### How to Use JSON Configuration:
- `num_sliders`: Total number of sliders.
- `name`: The display name of the slider (e.g., Main, System, Mic).
- `value`: Initial volume level (0-100).
- `muted`: Set to `true` or `false` to mute/unmute the slider.
- `previous_value`: Stores the last unmuted value for easy recovery.

Update these values and upload the JSON to the **ESP32 web UI** to customize the sliders.

---

## Enclosure
The case files and required hardware can be found here: [Deej32 Enclosure](https://www.printables.com/model/1113764-deej32-enclosure).

### Required Hardware for Enclosure:
- **4 M3 nuts and bolts** for the display.
- **2 M3 self-tapping screws** for the faceplate.
- **4 M2 self-tapping screws** for the rotary encoders.

---

## Video Build Log
Check out the prototyping log videos:

- **Part 1**: [Watch on YouTube](https://youtu.be/bxeFH4jwXGc?si=-9eGGEO2R_Kipmz3)
- **Part 2**: [Watch on YouTube](https://youtu.be/nTDPgiJ3POc)!

---

## Notes
- This is an early version of the project; improvements and bug fixes are expected.
- Feedback and contributions are welcome!

---

Happy building! 🚀
