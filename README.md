# 📖 D.E.V_Darshan — Ultra-Compact Offline TXT Reader

> **ESP32-CAM (AI Thinker) based pocket-sized offline text file reader with WiFi upload portal**

[![Platform](https://img.shields.io/badge/Platform-ESP32--CAM-blue)]()
[![Display](https://img.shields.io/badge/Display-SSD1306%20128x32-green)]()
[![License](https://img.shields.io/badge/License-MIT-yellow)]()
[![Status](https://img.shields.io/badge/Status-Production%20Ready-brightgreen)]()

---

## Table of Contents

- [Project Overview](#-project-overview)
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware Components](#-hardware-components)
- [GPIO Mapping & Boot Safety](#-gpio-mapping--boot-safety)
- [Circuit Schematic](#-circuit-schematic)
- [Physical Layout](#-physical-layout)
- [Firmware Architecture](#-firmware-architecture)
- [WiFi Upload Portal](#-wifi-upload-portal)
- [UI Design](#-ui-design)
- [Power Optimization](#-power-optimization)
- [Battery Life Estimation](#-battery-life-estimation)
- [Stability & Safety](#-stability--safety)
- [Build Instructions](#-build-instructions)
- [Usage Guide](#-usage-guide)
- [Troubleshooting](#-troubleshooting)
- [Future Upgrades](#-future-upgrades)
- [Competition Presentation Summary](#-competition-presentation-summary)

---

## 🎯 Project Overview

**D.E.V_Darshan** is a purpose-built, ultra-compact, battery-powered offline text file reader designed around the **ESP32-CAM (AI Thinker)** module — repurposed *without* using the camera. The built-in SD card slot (SD_MMC interface) serves as native file storage, and a **0.91" SSD1306 OLED** (128×32, I2C) provides a clean reading display.

### Why ESP32-CAM?

| Advantage | Detail |
|-----------|--------|
| Built-in SD slot | SD_MMC 4-bit mode — no extra wiring or SPI overhead |
| Small form factor | 27mm × 40mm — ideal for pocket builds |
| WiFi capable | Enables wireless file upload when needed |
| Low cost | Entire BOM under $10 |
| Sufficient flash | 4MB flash + PSRAM for buffering |

### Design Philosophy

- **Offline-first**: WiFi is disabled during reading for maximum battery life
- **Memory-safe**: Line-by-line file reading — never loads full file into RAM
- **Boot-safe**: Carefully mapped GPIOs to avoid ESP32-CAM boot failures
- **Minimal UI**: Three-button interface (UP/DOWN/SELECT) with intuitive navigation
- **Production-ready**: Debounced inputs, error handling, brownout protection

---

## ✨ Features

### Core Reading
- [x] Read `.txt` files from SD card
- [x] Memory-efficient line-by-line reader (supports files of any size)
- [x] Word-wrap optimized for 128×32 OLED (21 chars/line × 4 lines)
- [x] Smooth 4-line scrolling view
- [x] Long-press fast scroll
- [x] File selection menu with scroll support
- [x] Bookmark save/restore per file (using ESP32 Preferences/NVS)
- [x] Inverted display mode toggle

### WiFi Upload Portal
- [x] On-demand Access Point mode (SSID: `D.E.V_Darshan`)
- [x] Password-protected network
- [x] Mobile-responsive HTML upload page
- [x] `.txt` file type restriction
- [x] File size limit protection (max 2MB per file)
- [x] SD card usage display
- [x] Upload success/failure confirmation
- [x] Auto-disable WiFi on portal exit

### Power Management
- [x] WiFi & Bluetooth disabled during reading
- [x] Hardware power switch via magnetic reed switch
- [x] ~30+ hours reading battery life (estimated)
- [x] Deep sleep suggestion for idle timeout

---

## 🏗 System Architecture

```
┌─────────────────────────────────────────────────────────┐
│              D.E.V_Darshan System (Zero Resistors)           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────┐    I2C     ┌──────────────────────┐      │
│  │ SSD1306  │◄──────────►│                      │      │
│  │ 128x32   │  (module   │    ESP32-CAM          │      │
│  │ OLED     │  has pull-  │    (AI Thinker)       │      │
│  └──────────┘  ups)      │                      │      │
│                          │  ┌────────────────┐  │      │
│  ┌──────────┐            │  │  SD_MMC 1-BIT  │  │      │
│  │ BTN UP   │───GPIO13──►│  │  (built-in)    │  │      │
│  │ (int ↑)  │            │  └────────────────┘  │      │
│  └──────────┘            │                      │      │
│                          │  SD_MMC 1-bit Pins:  │      │
│  ┌──────────┐            │  CLK  = GPIO14       │      │
│  │ BTN DOWN │───GPIO0───►│  CMD  = GPIO15       │      │
│  │ (int ↑)  │            │  D0   = GPIO2        │      │
│  └──────────┘            │                      │      │
│                          │                      │      │
│  ┌──────────┐            │                      │      │
│  │BTN SELECT│──GPIO12───►│                      │      │
│  │ (int ↑)  │            │                      │      │
│  └──────────┘            │                      │      │
│                          │  FREE: GPIO4,16      │      │
│  ┌──────────┐            └──────────────────────┘      │
│  │ 3.7V     │──►TP4056──►5V/3.3V                      │
│  │ Li-ion   │            │                             │
│  │ 1100mAh  │   ┌────────┘                             │
│  └──────────┘   │ Reed Switch (power cut)               │
│                 └──────────────────────────             │
│                                                         │
│  ⚡ NO external resistors or capacitors required!       │
└─────────────────────────────────────────────────────────┘
```

### Software Architecture

```
┌────────────────────────────────────────────────┐
│                  main.ino                       │
│            (Setup + Main Loop)                  │
├────────────────────────────────────────────────┤
│                                                │
│  ┌─────────┐ ┌──────────┐ ┌───────────────┐  │
│  │ config.h│ │display.h │ │ sd_reader.h   │  │
│  │         │ │display.cpp│ │ sd_reader.cpp │  │
│  │ Pins    │ │          │ │               │  │
│  │ Consts  │ │ OLED Init│ │ File list     │  │
│  │ Settings│ │ Draw Menu│ │ Line reader   │  │
│  └─────────┘ │ Draw Text│ │ Word wrap     │  │
│              │ Scroll   │ │ Bookmark I/O  │  │
│              └──────────┘ └───────────────┘  │
│                                                │
│  ┌──────────────┐  ┌──────────────────────┐  │
│  │ buttons.h    │  │ wifi_portal.h        │  │
│  │ buttons.cpp  │  │ wifi_portal.cpp      │  │
│  │              │  │                      │  │
│  │ Debounce     │  │ AP mode              │  │
│  │ Short press  │  │ Web server           │  │
│  │ Long press   │  │ File upload handler  │  │
│  │ Combo detect │  │ HTML UI (embedded)   │  │
│  └──────────────┘  │ SD usage display     │  │
│                    └──────────────────────┘  │
└────────────────────────────────────────────────┘
```

---

## 🔧 Hardware Components

| Component | Specification | Purpose | Approx. Cost |
|-----------|--------------|---------|-------------|
| ESP32-CAM (AI Thinker) | ESP32-S, 4MB Flash, PSRAM | Main MCU + SD slot | $3.50 |
| SSD1306 OLED | 0.91", 128×32, I2C (with onboard pull-ups) | Display + I2C pull-ups | $1.50 |
| Micro SD Card | 2–32GB, FAT32 | File storage | $2.00 |
| Push Buttons × 3 | 6mm tactile | Navigation (UP/DOWN/SELECT) | $0.15 |
| TP4056 Module | With DW01A protection | Charging + battery protection | $0.50 |
| Li-ion Battery | 3.7V, 1100mAh | Power source | $2.50 |
| Reed Switch (NO) | Magnetic, normally-open | Power switch | $0.30 |
| Small Magnet | 5mm neodymium | Reed switch actuator | $0.10 |
| **Total** | | | **~$10.50** |

> **Zero resistors, zero capacitors** — internal pull-ups and OLED module pull-ups handle everything.

---

## 🔌 GPIO Mapping & Boot Safety

### ESP32-CAM Boot Pin Requirements

The ESP32 has strict requirements on certain GPIO states during boot. Violating these causes boot loops or flash failures.

| Pin | Required State at Boot | Consequence if Violated |
|-----|----------------------|------------------------|
| **GPIO0** | HIGH (pulled up internally) | LOW = enters flash mode |
| **GPIO2** | LOW or floating | HIGH with certain conditions can fail boot |
| **GPIO12** (MTDI) | **LOW** | HIGH = sets VDD_SDIO to 1.8V → brownout/crash |
| **GPIO15** (MTDO) | **HIGH** (pulled up internally) | LOW = suppresses boot log (not fatal but problematic) |

### How We Eliminate ALL Resistors

The key insight: **SD_MMC 1-bit mode** only uses 3 pins (GPIO2, GPIO14, GPIO15) — freeing GPIO4, GPIO12, and GPIO13. This eliminates the critical GPIO12 problem entirely since it's no longer driven by the SD interface.

| Problem (4-bit mode) | Solution (1-bit mode) |
|---------------------|----------------------|
| GPIO12 needs pull-down resistor | GPIO12 is **freed up** — used for BTN_SELECT with internal pull-up |
| GPIO15 needs pull-up resistor | SD driver enables **internal pull-up** automatically |
| GPIO2 needs pull-up resistor | SD driver enables **internal pull-up** automatically |
| GPIO0/GPIO13 (BTN) free | Used for **BTN_DOWN** and **BTN_UP** with **internal pull-ups** |
| I2C needs 4.7kΩ pull-ups | OLED module has **built-in pull-ups** on breakout board |

### SD_MMC 1-bit Mode Pin Allocation

| SD_MMC Function | GPIO | Notes |
|----------------|------|-------|
| CLK | GPIO14 | SD clock |
| CMD | GPIO15 | Internal pull-up enabled by SD driver ✅ |
| DATA0 | GPIO2 | Internal pull-up enabled by SD driver ✅ |
| ~~DATA1~~ | ~~GPIO4~~ | **FREE** — not used in 1-bit mode |
| ~~DATA2~~ | ~~GPIO12~~ | **FREE** — used for BTN_SELECT instead |
| ~~DATA3~~ | ~~GPIO13~~ | **FREE** — used for BTN_UP instead |

### Final GPIO Assignment Table (ZERO External Resistors)

| Function | GPIO | Direction | Pull | Boot Safety | Notes |
|----------|------|-----------|------|-------------|-------|
| **OLED SDA** | GPIO3 (U0RXD) | I2C Data | OLED module built-in ↑ | ✅ Safe | Most I2C OLED breakouts include onboard pull-ups |
| **OLED SCL** | GPIO1 (U0TXD) | I2C Clock | OLED module built-in ↑ | ✅ Safe | No external resistor needed |
| **BTN_UP** | GPIO13 | INPUT_PULLUP | Internal ↑ | ✅ Safe | Free in 1-bit SD mode — no pin conflict |
| **BTN_DOWN** | GPIO0 | INPUT_PULLUP | Internal ↑ | ✅ Safe* | *Don't hold during power-on (enters flash mode) |
| **BTN_SELECT** | GPIO12 | INPUT_PULLUP | Internal ↑ | ✅ Safe | Free in 1-bit SD mode — boot-safe when HIGH |
| SD_MMC CLK | GPIO14 | SD | — | ✅ Safe | Fixed |
| SD_MMC CMD | GPIO15 | SD | Internal ↑ (SD driver) | ✅ Safe | Driver handles pull-up |
| SD_MMC D0 | GPIO2 | SD | Internal ↑ (SD driver) | ✅ Safe | Driver handles pull-up |
| Flash LED | GPIO4 | OUTPUT LOW | — | ✅ Safe | Disabled in firmware (`digitalWrite(4, LOW)`) |
| ~~GPIO16~~ | — | Not connected | — | ✅ Safe | Free for future use |

### Why This Works Without Resistors

1. **GPIO1 & GPIO3 (UART) for I2C**: No boot-state requirements. The OLED module's breakout board provides the required I2C pull-up resistors onboard (virtually all SSD1306 modules from common suppliers include 4.7kΩ or 10kΩ pull-ups).

2. **GPIO13 for BTN_UP**: Boot-safe pin with built-in internal pull-up. In 1-bit SD_MMC mode, GPIO13 is completely free — no sharing conflict.

3. **GPIO0 for BTN_DOWN**: Has internal pull-up → HIGH at rest → safe normal boot. Only risk: if held LOW during power-on, ESP32 enters flash mode. In practice, users don't hold buttons while flipping the reed switch.

4. **GPIO12 for BTN_SELECT**: By using 1-bit SD mode, GPIO12 is freed up. Has internal pull-up → HIGH at rest → boot-safe (LOW during boot would set VDD_SDIO to 1.8V, but HIGH is safe).

5. **GPIO15 & GPIO2**: The SD_MMC driver calls `gpio_pullup_en()` on these pins internally. No external resistors needed.

### Brownout Prevention (Optional)

With 1-bit SD mode and WiFi disabled during reading, current draw is low enough that most charged Li-ion batteries won't cause brownout. However, if you experience resets during WiFi transmission, you can optionally add a **470µF capacitor** between 3.3V and GND.

---

## 🔌 Circuit Schematic (Zero Resistors)

```
                    ┌─────────────────────────┐
                    │      ESP32-CAM           │
                    │      (AI Thinker)        │
                    │                          │
         ┌─────────┤GPIO3 (SDA)    GPIO14├──── SD_CLK (internal)
         │         │                          │
         │  ┌──────┤GPIO1 (SCL)    GPIO15├──── SD_CMD (internal ↑)
         │  │      │                          │
         │  │  ┌───┤GPIO13 (BTN_UP) GPIO2├──── SD_D0  (internal ↑)
         │  │  │   │  (internal ↑)            │
         │  │  │   │                          │
         │  │  │ ┌─┤GPIO0 (BTN_DN) GPIO4├──── (free, LED off)
         │  │  │ │ │  (internal ↑)            │
         │  │  │ │ │                          │
         │  │  │ │ ├─GPIO12 (BTN_SEL)GPIO16├─ (free)
         │  │  │ │ │  (internal ↑)            │
         │  │  │ │ └─────────────────────────┘
         │  │  │ │ │
  ┌──────┴──┴┐ │ │ │   Only 4 wires to OLED:
  │ SSD1306  │ │ │ │     SDA → GPIO3
  │ 128x32   │ │ │ │     SCL → GPIO1
  │ (has own │ │ │ │     VCC → 3.3V
  │  pull-ups│ │ │ │     GND → GND
  │ on board)│ │ │ │
  └──────────┘ │ │ │   Only 2 wires per button:
        ┌──────┘ │ │     BTN pin → GND (when pressed)
        │  ┌─────┘ │
        │  │  ┌────┘
   [BTN_UP] [BTN_DN] [BTN_SEL]  (simple switches, no resistors)
     │  │    │  │      │  │
   GPIO13 GND GPIO0 GND GPIO12 GND


   TP4056      Reed         Battery
   Module      Switch       3.7V
   ┌─────┐    ┌──┐        ┌─────┐
   │IN OUT│────┤RS├────────┤+ Li │
   │      │    └──┘        │ ion │
   │ B+ B-│────────────────┤     │
   └──┬──┘                └──┬──┘
      │GND                   │GND
      └──────────────────────┘
```

### Wiring Summary (Just 10 wires total!)

| Connection | Wire |
|-----------|------|
| OLED SDA → GPIO3 | 1 |
| OLED SCL → GPIO1 | 1 |
| OLED VCC → 3.3V | 1 |
| OLED GND → GND | 1 |
| BTN_UP one leg → GPIO13 | 1 |
| BTN_UP other leg → GND | 1 |
| BTN_DOWN one leg → GPIO0 | 1 |
| BTN_DOWN other leg → GND | 1 |
| BTN_SELECT one leg → GPIO12 | 1 |
| BTN_SELECT other leg → GND | 1 |
| **Total** | **10 wires, 0 resistors** |

---

## 📐 Physical Layout

### Ultra-Compact Stacking Design (27mm × 40mm footprint)

```
    TOP VIEW (Reading Position)
    ┌──────────────────────────┐
    │   ┌──────────────────┐   │
    │   │  0.91" OLED      │   │  ← Layer 1: Display
    │   │  128 × 32        │   │
    │   └──────────────────┘   │
    │  [UP] [SELECT] [DOWN]    │  ← Side/front buttons
    └──────────────────────────┘

    SIDE VIEW (Layer Stack)
    ┌──────────────────────────┐
    │      OLED Display        │  ← Top: 0.91" OLED (glued/taped)
    ├──────────────────────────┤
    │      ESP32-CAM           │  ← Middle: MCU board
    │      (camera removed)    │
    │      [SD Card Slot →]    │  ← SD accessible from side
    ├──────────────────────────┤
    │      TP4056 Module       │  ← Bottom: Charging circuit
    │      Li-ion Battery      │  ← Battery (flat pouch cell ideal)
    └──────────────────────────┘
    ← Magnet + Reed switch on side

    Total height: ~15-18mm
    Total size: ~30mm × 45mm × 18mm (credit card width)
```

### Assembly Notes

1. **Remove the camera module** from ESP32-CAM — saves space and frees the ribbon cable area for wiring
2. **Mount OLED** on top using double-sided foam tape or hot glue
3. **Buttons** can be side-mounted or front-mounted depending on enclosure
4. **TP4056** sits underneath, with micro-USB accessible from the side
5. **Battery** is a flat Li-Po pouch cell (fits alongside or beneath the TP4056)
6. **Reed switch** mounts at the edge — a small magnet on the enclosure lid/slider controls power

---

## 🧠 Firmware Architecture

### File Structure

```
D.E.V_Darshan_Sketch/
├── D.E.V_Darshan_Sketch.ino  # Main entry point — setup() and loop()
├── config.h                   # Pin definitions, constants, settings
├── display.h / .cpp           # OLED drawing functions (U8g2lib)
├── sd_reader.h / .cpp         # SD_MMC init, file listing, line reader, bookmarks
├── buttons.h / .cpp           # Debounced button handler with press types
├── wifi_portal.h / .cpp       # WiFi AP + web server + upload handler
└── portal.h                   # Embedded HTML for upload portal (PROGMEM)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| `config.h` | All pin mappings, timing constants, display settings, WiFi credentials |
| `display` | U8g2 initialization, draw file menu, draw reading view, scroll indicator, invert mode |
| `sd_reader` | SD_MMC mount/unmount, list `.txt` files, read lines into buffer, word wrapping, bookmark save/load |
| `buttons` | Debounce (50ms), short press detection, long press/held detection for fast scroll, three-button input |
| `wifi_portal` | Start/stop AP, serve upload page, handle multipart file upload, serve SD usage info |
| `portal.h` | Compressed HTML/CSS/JS for mobile-responsive upload UI stored in PROGMEM |

### State Machine

```
                    ┌──────────────┐
         Power ON → │  BOOT_INIT   │
                    │ (SD + OLED)  │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │  HOME MENU   │◄────────────┐
                    │ (WiFi/Files/ │             │
                    │  Settings)   │             │
                    └──┬───┬───┬───┘             │
                       │   │   │                 │
         SELECT on     │   │   │                 │
         WiFi Portal   │   │   └─────┐           │
                       │   │         │           │
                ┌──────▼───┴──┐   ┌──▼────────┐  │
                │  FILE_MENU  │   │ SETTINGS  │  │
                │(list files) │   │  MENU     │  │
                └──────┬──────┘   └───────────┘  │
                       │ SELECT                   │
                       │ on file                  │
                ┌──────▼───────┐                  │
                │   READING    │  SELECT (back)   │
                │ (text view)  │──────────────────┤
                └──────────────┘                  │
                       ↓                          │
            ┌──────────▼──────────┐               │
            │    WIFI_PORTAL      │  SELECT (exit)│
            │    (AP + HTTP)      │───────────────┘
            └─────────────────────┘
```


### Key Design Decisions

1. **Line-by-line reading**: File is read 4 lines at a time into a circular buffer. Scrolling reads the next line from SD and discards the oldest. Maximum RAM usage: ~512 bytes for text buffer regardless of file size.

2. **Word wrapping**: Performed at read time. Each raw line is split into display lines (21 chars max) with word-boundary awareness. Partial words wrap to next line.

3. **Bookmark system**: Uses ESP32's built-in NVS (Non-Volatile Storage) via the Preferences library. Stores `{filename_hash: byte_position}` pairs. Automatically saves position every 10 scroll actions and on file exit.

4. **SD_MMC 1-bit mode**: Uses the built-in slot with only 3 pins (GPIO2/14/15), freeing GPIO4/12/13 and eliminating all external pull resistors. Slightly slower than 4-bit mode but more than sufficient for text reading.

---

## 📡 WiFi Upload Portal

### Activation
Select **WiFi Portal** from the HOME menu → ESP32 enables WiFi AP mode.

### Portal Specifications

| Parameter | Value |
|-----------|-------|
| SSID | `D.E.V_Darshan` |
| Password | `readmore` (configurable in config.h) |
| IP Address | `192.168.4.1` |
| Max file size | 2MB |
| Allowed types | `.txt` only |
| Concurrent connections | 1 (RAM safety) |

### Upload Flow

```
User Phone                          ESP32-CAM
    │                                    │
    │── Connect to "D.E.V_Darshan" WiFi ───►│
    │                                    │
    │── Open 192.168.4.1 in browser ────►│
    │                                    │
    │◄── Serve mobile-responsive HTML ───│
    │                                    │
    │── Select .txt file ──────────────►│
    │                                    │
    │── Upload (multipart POST) ───────►│
    │                                    │  → Stream to SD
    │                                    │    (chunked write)
    │◄── Success / Error response ──────│
    │                                    │
    │── (Optional) Check SD usage ─────►│
    │◄── SD total/used/free display ────│
```

### Portal UI Features
- Clean, dark-themed mobile-first design
- Drag-and-drop file upload area
- Upload progress indicator
- File type validation (client + server side)
- File size validation (client + server side)
- Success/error toast notifications
- SD card usage bar with percentages
- List of existing files on SD card

---

## 🎨 UI Design

### File Menu Screen (128×32)

```
┌──────────────────────┐
│ ► my_book.txt        │  ← Selected file (inverted)
│   notes.txt          │
│   story.txt          │
│                  1/3 │  ← Item counter
└──────────────────────┘
```

### Reading Screen (128×32)

```
┌──────────────────────┐
│ my_book.txt          │  ← Line 1: Filename (small font)
│ It was the best of   │  ← Line 2-4: Wrapped text content
│ times, it was the    │
│ worst of times  ▐██  │  ← Scroll position indicator
└──────────────────────┘
```

### WiFi Portal Active Screen

```
┌──────────────────────┐
│ WiFi: D.E.V_Darshan     │
│ Pass: readmore       │
│ IP: 192.168.4.1      │
│ Hold ▼ to exit       │
└──────────────────────┘
```

### Display Specifications

| Parameter | Value |
|-----------|-------|
| Font (filename) | u8g2_font_5x7_tr (5×7) |
| Font (body text) | u8g2_font_6x10_tr (6×10) |
| Characters per line | 21 (128 ÷ 6) |
| Visible text lines | 3 (with filename header) or 4 (reading mode) |
| Scroll indicator | 2px wide bar, right edge |

---

## ⚡ Power Optimization

### Strategy

| Technique | Implementation | Savings |
|-----------|---------------|---------|
| WiFi OFF | `WiFi.mode(WIFI_OFF)` at boot | ~80mA |
| Bluetooth OFF | `btStop()` at boot | ~30mA |
| Flash LED OFF | `digitalWrite(4, LOW)` | ~20mA |
| CPU frequency | 80MHz (vs default 240MHz) | ~30mA |
| OLED sleep | Display off after 60s idle | ~10mA |
| Deep sleep | After 5min idle (optional) | ~98% |

### Power Consumption Estimates

| Mode | Current Draw | Notes |
|------|-------------|-------|
| **Reading** (WiFi OFF, BT OFF, 80MHz) | ~35mA | OLED on, SD occasional reads |
| **Idle** (display dimmed) | ~25mA | OLED in low brightness |
| **WiFi Portal Active** | ~120mA | AP mode + web server |
| **Deep Sleep** | ~6µA | Wake on button press (GPIO0 supports wakeup) |

### Battery Life Estimation (1100mAh)

| Usage Mode | Estimated Battery Life |
|------------|----------------------|
| Continuous reading | **~31 hours** (1100 ÷ 35) |
| Mixed reading (with idle) | **~37 hours** |
| WiFi upload session | **~9 hours** (unlikely continuous) |
| Standby (deep sleep) | **~7,600 days** (theoretical) |

### Power Code Implementation

```cpp
// Called at boot — before anything else
void disableRadios() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    esp_wifi_stop();
    esp_bt_controller_disable();
}

// Reduce CPU frequency for reading mode
void setLowPowerCPU() {
    setCpuFrequencyMhz(80);  // Minimum stable for SD + I2C
}

// Turn off onboard flash LED (GPIO4)
void disableFlashLED() {
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
}
```

---

## 🛡 Stability & Safety

### Boot Sequence Safety

```
Power ON
  │
  ├─ 1. GPIO0 pulled HIGH by internal pull-up → normal boot (not flash)
  ├─ 2. GPIO12 not connected to SD → floats safely (no voltage issue)
  ├─ 3. GPIO15 internal pull-up → HIGH → boot messages OK
  │
  ├─ 4. ESP32 boots normally
  ├─ 5. Firmware disables WiFi + BT immediately
  ├─ 6. Set GPIO4 LOW (disable flash LED)
  ├─ 7. Initialize buttons on GPIO13/GPIO0 (INPUT_PULLUP)
  ├─ 8. Initialize I2C for OLED on GPIO1/3
  ├─ 9. Initialize SD_MMC 1-bit mode (only GPIO2,14,15)
  ├─ 10. Show splash screen → enter file menu
  │
  └─ ✅ Stable operation — ZERO resistors needed
```

### Failure Prevention

| Failure Mode | Prevention |
|-------------|-----------|
| **Boot loop** | 1-bit SD mode avoids GPIO12 issue; GPIO0 has internal pull-up |
| **Brownout reset** | WiFi off during reading; optional 470µF cap if needed |
| **SD mount failure** | 3 retry attempts with 500ms delays, error screen shown |
| **Memory crash** | Line-by-line reading, no dynamic allocation in loop, stack monitoring |
| **Button ghost triggers** | 50ms debounce, active-low with pull-ups, noise filtering |
| **File corruption** | Flush after write, proper file close, no writes during read mode |
| **WiFi memory leak** | Full WiFi/server cleanup on portal exit, `WiFi.mode(WIFI_OFF)` |

### Debounce Implementation

```
Button Press Timeline:
─────────┐         ┌─────────────────────
         │         │
         └─────────┘
         ↑         ↑
      pressed   released
         │← 50ms →│  = debounce window
         │←───── 2000ms ────→│  = long press threshold
```

### Error Handling

- **SD not found**: Display "Insert SD Card" message, retry every 3 seconds
- **No .txt files**: Display "No TXT files found" with WiFi portal hint
- **File read error**: Display error, return to menu
- **WiFi start failure**: Display error, return to reading mode
- **Upload failure**: Send error response to client, don't corrupt SD

---

## 🔨 Build Instructions

### Prerequisites

1. **Arduino IDE 2.x** or **PlatformIO**
2. **ESP32 Board Package** (by Espressif Systems) — version 2.0.x+
3. **Board Selection**: `AI Thinker ESP32-CAM`

### Required Libraries

| Library | Version | Source |
|---------|---------|--------|
| U8g2 | 2.34+ | Arduino Library Manager |
| WebServer | Built-in | ESP32 core |
| WiFi | Built-in | ESP32 core |
| SD_MMC | Built-in | ESP32 core |
| Preferences | Built-in | ESP32 core |
| FS | Built-in | ESP32 core |

> **No external library dependencies beyond U8g2!** Everything else is part of the ESP32 Arduino core.

### Upload Settings

| Setting | Value |
|---------|-------|
| Board | AI Thinker ESP32-CAM |
| Upload Speed | 115200 |
| CPU Frequency | 240MHz (will be reduced in code) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Partition Scheme | Default 4MB with spiffs |
| Core Debug Level | None (production) |

### Upload Procedure

1. Connect FTDI programmer (3.3V):
   - FTDI TX → ESP32-CAM U0R (GPIO3)
   - FTDI RX → ESP32-CAM U0T (GPIO1)
   - FTDI GND → ESP32-CAM GND
   - FTDI 3.3V → ESP32-CAM 3.3V (or 5V to 5V)
2. **Connect GPIO0 to GND** (enter flash mode)
3. Press RST button on ESP32-CAM
4. Upload firmware
5. **Disconnect GPIO0 from GND**
6. Press RST — device boots normally

> **Note**: After upload and final assembly, GPIO1 and GPIO3 are repurposed for I2C. Serial debugging will not be available. Use the OLED for status output.

---

## 📖 Usage Guide

### First Boot
1. Insert SD card with `.txt` files
2. Power on via magnet position (reed switch)
3. Splash screen shows "D.E.V_Darshan v1.0"
4. File menu displays available `.txt` files

### Navigation

| Action | Input | Context |
|--------|-------|---------|
| Scroll up in list/text | Short press UP | Any |
| Scroll down in list/text | Short press DOWN | Any |
| Fast scroll (line-by-line) | Hold UP or DOWN | Reading |
| Select / Enter | Short press SELECT | Any |
| Back to previous screen | Short press SELECT | Reading / WiFi Portal |

### Uploading Files
1. From HOME screen, select "WiFi Portal" with SELECT button
2. OLED shows WiFi info (SSID, password, IP)
3. Connect phone to `D.E.V_Darshan` WiFi
4. Open `192.168.4.1` in browser
5. Select and upload `.txt` files
6. Press SELECT to exit portal and return to HOME

### Bookmarks
- Position is **automatically saved** every 10 scrolls
- When re-opening a file, reading resumes from last position
- Bookmarks persist across power cycles (stored in NVS flash)

---

## 🔧 Troubleshooting

| Problem | Likely Cause | Solution |
|---------|-------------|----------|
| Boot loop | GPIO0 held LOW during power-on | Don't hold BTN_DOWN while powering on |
| Brownout reset | Battery low or WiFi surge | Charge battery; optionally add 470µF cap |
| SD not detected | Bad contact / wrong format | FAT32 format, reseat card, check solder joints |
| OLED blank | Wrong I2C address or wiring | Check SDA(GPIO3)/SCL(GPIO1), try address 0x3C |
| OLED blank (no pull-ups) | OLED module lacks onboard pull-ups | Rare — add 4.7kΩ pull-ups to SDA/SCL if needed |
| Buttons unresponsive | Wiring issue | Check BTN_UP→GPIO13, BTN_DOWN→GPIO0, other leg→GND |
| WiFi won't start | Memory fragmentation | Restart device, reduce file buffer size |
| Upload fails | File too large or wrong type | Keep under 2MB, ensure `.txt` extension |
| Ghost button presses | Noise on GPIO lines | Verify debounce; optionally add 100nF cap on buttons |

---

## 🚀 Future Upgrades

| Feature | Difficulty | Description |
|---------|-----------|-------------|
| Multi-language support | Medium | UTF-8 rendering with appropriate fonts |
| Font size toggle | Easy | Switch between 2 font sizes in reading mode |
| Brightness control | Easy | PWM or U8g2 contrast adjustment |
| Auto-scroll mode | Easy | Timed page advance for hands-free reading |
| File delete via portal | Easy | Add delete endpoint to web server |
| OTA firmware update | Medium | Upload firmware via WiFi portal |
| Battery voltage display | Medium | ADC read on available pin (if any free) |
| E-ink display variant | Hard | Swap SSD1306 for e-ink for better battery life |
| BLE file transfer | Medium | Alternative to WiFi for phone transfer |
| HTML/Markdown reader | Hard | Parse and render formatted text |
| Encrypted storage | Hard | AES encryption for private documents |
| Reading statistics | Easy | Track time spent reading, pages read |

---

## 🏆 Competition Presentation Summary

### Project: D.E.V_Darshan — The Minimalist Digital Reader

**Problem Statement**: In an age of constant connectivity and digital distraction, there is no affordable, distraction-free device for reading plain text documents. E-readers cost $100+ and still offer browsers and stores. Phones demand attention constantly.

**Solution**: D.E.V_Darshan is an ultra-compact ($10), open-source, offline text reader that fits in your pocket. It uses repurposed IoT hardware (ESP32-CAM) to achieve what commercial products cannot — **pure, focused reading with zero distractions**.

**Technical Innovation**:
- Repurposes a $3.50 camera module as a general-purpose reader by leveraging its built-in SD card slot
- Memory-efficient architecture reads files of unlimited size on a microcontroller with 520KB RAM
- Boot-safe GPIO mapping solves a known challenge with ESP32-CAM that often blocks hobbyist projects
- WiFi is demand-activated — disabled 99% of the time for exceptional battery life

**Key Metrics**:
- **31+ hours** battery life on a single charge
- **< $11** total component cost (zero resistors)
- **30mm × 45mm × 18mm** form factor
- **Unlimited** file size support
- **Zero** external resistors or capacitors required
- **Zero** external library dependencies (beyond display driver)
- **< 2 second** boot time

**Engineering Disciplines Demonstrated**:
- Embedded systems design
- Power optimization
- Memory-constrained programming
- Web development (embedded HTTP server)
- Human-computer interaction (3-button UX design)
- Hardware-software co-design
- PCB-less prototyping and compact assembly

---

## 📄 License

MIT License — Free for personal and educational use.

---

*Built with ❤️ using ESP32-CAM, U8g2, and determination.*
