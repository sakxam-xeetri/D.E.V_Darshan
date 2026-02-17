# D.E.V_Darshan — ESP32-CAM Mini TXT Reader

> A compact, pocketable, offline TXT reader built on ESP32-CAM — no camera needed.

**Developer:** Sakshyam Bastakoti — IOT & Robotics Developer

---

## Overview

D.E.V_Darshan turns an ESP32-CAM into a standalone offline text file reader. Users upload `.txt` files over Wi-Fi, then read them on a 0.96" OLED display using just two tactile buttons — no internet, no phone, no PC required after uploading.

---

## Features

| Feature | Details |
|---|---|
| Offline reading | Read `.txt` files from the built-in SD card |
| Wi-Fi upload portal | Add files without removing the SD card |
| Two-button UI | Scroll, select, go back — all with two buttons |
| Word wrapping | Long lines wrap automatically (~21 chars/line) |
| Full-screen reading | Maximizes OLED space for text content |
| Scrollable file list | Home screen lists all `.txt` files on SD |
| Power-safe | 470 µF capacitor protects against voltage spikes |

---

## Hardware

| Component | Specification | Qty |
|---|---|---|
| ESP32-CAM | AI Thinker Module | 1 |
| OLED Display | 0.96" SSD1306 I2C 128×64 | 1 |
| MicroSD Card | 1–8 GB, FAT32 formatted | 1 |
| Push Buttons | Tactile 6×6 mm | 2 |
| Battery | 18650 Li-ion + TP4056 charger | 1 |
| Capacitor | 470 µF / 16 V electrolytic | 1 |
| Jumper Wires | Male-to-Female | as needed |

---

## Wiring

See **[CIRCUIT.md](CIRCUIT.md)** for full pin-by-pin connection details and an ASCII diagram.

### Quick Reference

| Connection | OLED / Button | ESP32-CAM Pin |
|---|---|---|
| OLED SDA | SDA | GPIO 12 |
| OLED SCL | SCL | GPIO 14 |
| OLED VCC | VCC | 3.3 V |
| OLED GND | GND | GND |
| UP Button | — | GPIO 13 (INPUT_PULLUP) |
| DOWN Button | — | GPIO 15 (INPUT_PULLUP) |

SD card uses the built-in SD_MMC slot (1-bit mode) — no extra wiring.

---

## Button Controls

| Action | Result |
|---|---|
| **Short press UP** | Scroll up |
| **Short press DOWN** | Scroll down |
| **Hold UP 2 s** | Select / open file |
| **Hold DOWN 2 s** | Go back to home screen |
| **Hold BOTH 2 s** | Open Wi-Fi upload portal |

---

## Firmware Modes

```
┌─────────────┐     Hold UP 2s     ┌─────────────┐
│  HOME_MODE  │ ──────────────────► │  READ_MODE  │
│ (file list) │ ◄────────────────── │ (text view) │
└──────┬──────┘     Hold DOWN 2s    └─────────────┘
       │
       │ Hold BOTH 2s
       ▼
┌─────────────┐
│ PORTAL_MODE │
│ (Wi-Fi AP)  │
└─────────────┘
```

---

## Getting Started

### 1. Prerequisites

- **Arduino IDE** (1.8+ or 2.x)
- **ESP32 Board Package** — add this URL in Preferences → Additional Board Manager URLs:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- **Libraries** (install via Library Manager):
  - `U8g2` — OLED display driver
  - `SD_MMC` — built-in with ESP32 core
  - `WebServer` — built-in with ESP32 core
  - `WiFi` — built-in with ESP32 core

### 2. Hardware Setup

1. Wire the OLED and buttons as described in [CIRCUIT.md](CIRCUIT.md).
2. Insert a FAT32-formatted MicroSD card with some `.txt` files.
3. Power via USB or 18650 battery through TP4056.

### 3. Upload Firmware

1. Open `DEV_Darshan.ino` in Arduino IDE.
2. Select **Board → AI Thinker ESP32-CAM**.
3. Connect ESP32-CAM via FTDI programmer (GPIO0 → GND for flash mode).
4. Upload.
5. Disconnect GPIO0 from GND and reset.

### 4. Usage

1. **Power on** → Home screen shows `.txt` file list.
2. **UP / DOWN** short press → scroll through file list.
3. **Hold UP 2 s** → open the selected file.
4. **UP / DOWN** short press → scroll through file content.
5. **Hold DOWN 2 s** → return to file list.
6. **Hold BOTH 2 s** → start Wi-Fi portal.
7. Connect to AP **"D.E.V AP"** (password: `Darshan`), open `192.168.4.1` in a browser, upload `.txt` files.

---

## Project Structure

```
DEV_Darshan/
├── DEV_Darshan.ino    # Main Arduino firmware
├── CIRCUIT.md         # Detailed circuit connections & diagram
└── README.md          # This file
```

---

## Notes

- **GPIO0, GPIO2, GPIO12** should be avoided for buttons to prevent ESP32 boot issues. GPIO12 is used for OLED SDA (configured after boot) which is safe.
- The camera module is **not used** — this project repurposes the ESP32-CAM purely for its SD slot, Wi-Fi, and GPIO.
- SD_MMC runs in **1-bit mode** to free up GPIO pins.
- OLED I2C runs on non-standard pins (GPIO12/14) via software I2C through U8g2.

---

## License

This project is open-source for educational and personal use.

**Built with passion by Sakshyam Bastakoti — D.E.V_Darshan**
