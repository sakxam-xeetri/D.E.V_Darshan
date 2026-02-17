# D.E.V_Darshan - ESP32-CAM Mini TXT Reader

![Version](https://img.shields.io/badge/version-1.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)

A compact, pocketable offline TXT reader using ESP32-CAM with OLED display and two-button navigation.

**Developer:** Sakshyam Bastakoti - IOT and Robotics Developer

---

## 📝 Project Overview

This project transforms an ESP32-CAM board into a portable text file reader with Wi-Fi upload capability. The camera module is **not used**, making this purely a file reading device with the following features:

- **Offline TXT Reading** - Read .txt files stored on SD card
- **Wi-Fi Upload Portal** - Add files wirelessly without removing SD card
- **Two-Button Interface** - Simple and intuitive navigation
- **OLED Display** - 0.96" display for comfortable reading
- **Portable** - Battery-powered, pocket-sized design

---

## 🔧 Hardware Components

| Component | Specification | Quantity |
|-----------|--------------|----------|
| ESP32-CAM | AI Thinker Module | 1 |
| OLED Display | 0.96" SSD1306 I2C 128x64 | 1 |
| MicroSD Card | 1-8 GB, FAT32 format | 1 |
| Push Buttons | Tactile 6x6 mm | 2 |
| Battery | 18650 Li-ion + TP4056 charger | 1 |
| Capacitor | 470 µF / 16V electrolytic | 1 |
| Resistors | 10kΩ (pull-up for buttons) | 2 |
| Jumper Wires | Male-Female | As needed |

---

## 📌 Pin Mapping (ESP32-CAM)

### Safe Pin Configuration (Camera NOT Used, 1-bit SD Mode)

| Component | Pin Function | GPIO Pin | Notes |
|-----------|-------------|----------|-------|
| **OLED Display** | SDA | GPIO 13 | I2C Data |
| | SCL | GPIO 12 | I2C Clock |
| | VCC | 3.3V | Power |
| | GND | GND | Ground |
| **SD Card (1-bit)** | CMD | GPIO 15 | Command |
| | CLK | GPIO 14 | Clock |
| | D0 | GPIO 2 | Data 0 |
| | VCC | 3.3V | Power |
| | GND | GND | Ground |
| **Buttons** | UP Button | GPIO 4 | Active LOW (pull-up) |
| | DOWN Button | GPIO 0 | Active LOW (pull-up) |
| **Power** | VCC | 5V | From battery/USB |
| | GND | GND | Common ground |
| **Capacitor** | 470µF | Between 5V & GND | Voltage stabilization |

> **Note:** GPIO 0 is used for DOWN button. Internal pull-up keeps it HIGH during normal operation. For programming, press DOWN button while pressing RESET.

---

## 🔌 Circuit Connections

See [CIRCUIT.md](CIRCUIT.md) for detailed wiring diagrams and connection instructions.

---

## 💾 Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (version 1.8.19 or later)
2. **Add ESP32 Board Support:**
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search "ESP32" and install "esp32 by Espressif Systems"

3. **Install Required Libraries:**
   - U8g2 (for OLED display)
   - WiFi (built-in)
   - WebServer (built-in)
   - SD_MMC (built-in)
   - FS (built-in)

### Board Settings

- **Board:** "AI Thinker ESP32-CAM"
- **Upload Speed:** 115200
- **Flash Frequency:** 80MHz
- **Partition Scheme:** "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"

---

## 🎮 User Interface

### Button Controls

| Action | Function |
|--------|----------|
| **Short UP** | Scroll up / Previous item |
| **Short DOWN** | Scroll down / Next item |
| **Hold UP (2s)** | Select/Open file |
| **Hold DOWN (2s)** | Back to file list |
| **Hold BOTH (2s)** | Open Wi-Fi Portal |

### Display Screens

**Home Screen:**
```
> physics.txt
  math.txt
  computer.txt
```
- `>` indicates selected file
- Scrollable list of all .txt files

**File Viewer:**
```
The quick brown fox
jumps over the lazy
dog. This text wraps
automatically to fit
the display width.
```
- Multiple lines visible
- Automatic word wrapping
- Smooth scrolling

---

## 📡 Wi-Fi Upload Portal

1. **Activate Portal:** Hold BOTH buttons for 2 seconds
2. **Connect to Wi-Fi:**
   - SSID: `D.E.V AP`
   - Password: `Darshan`
3. **Upload Files:**
   - Open browser and go to: `http://192.168.4.1`
   - Select .txt files to upload
   - Files are saved to SD card
4. **Exit Portal:** Restart device or use portal exit button

---

## 🚀 Getting Started

### 1. Hardware Assembly
- Follow the pin mapping table and connect all components
- Insert formatted SD card (FAT32)
- Connect battery or USB power

### 2. Upload Firmware
- Open `D.E.V_Darshan.ino` in Arduino IDE
- Select correct board and port
- Click Upload

### 3. First Use
- Device boots to file list screen
- Add .txt files via SD card or Wi-Fi portal
- Navigate and read!

---

## 📂 File Structure

```
D.E.V_Darshan/
├── README.md              # This file
├── CIRCUIT.md             # Circuit diagrams and connections
├── D.E.V_Darshan.ino      # Main Arduino sketch
├── vision.txt             # Original project vision document
└── examples/              # Example .txt files (optional)
```

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| OLED not displaying | Check I2C connections, verify address (0x3C) |
| SD card not detected | Ensure FAT32 format, check connections |
| Can't upload code | Press RESET, hold GPIO 0 to GND during upload |
| Wi-Fi not starting | Verify both buttons held for full 2 seconds |
| Text not wrapping | Check u8g2 font settings in code |

---

## 📄 License

MIT License - Feel free to modify and distribute

---

## 👨‍💻 Developer

**Sakshyam Bastakoti**  
IOT and Robotics Developer

---

## 🔮 Future Enhancements

- [ ] PDF support
- [ ] Adjustable font size
- [ ] Bookmarks
- [ ] Last read position memory
- [ ] Dark/Light themes
- [ ] Battery percentage indicator

---

**Happy Reading! 📖**
