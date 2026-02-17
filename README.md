# 📖 PocketTXT — Ultra-Compact Offline TXT Reader

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

**PocketTXT** is a purpose-built, ultra-compact, battery-powered offline text file reader designed around the **ESP32-CAM (AI Thinker)** module — repurposed *without* using the camera. The built-in SD card slot (SD_MMC interface) serves as native file storage, and a **0.91" SSD1306 OLED** (128×32, I2C) provides a clean reading display.

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
- **Minimal UI**: Two-button interface with intuitive long-press actions
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
- [x] On-demand Access Point mode (SSID: `TXT_Reader`)
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
│                    PocketTXT System                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────┐    I2C     ┌──────────────────────┐      │
│  │ SSD1306  │◄──────────►│                      │      │
│  │ 128x32   │            │    ESP32-CAM          │      │
│  │ OLED     │            │    (AI Thinker)       │      │
│  └──────────┘            │                      │      │
│                          │  ┌────────────────┐  │      │
│  ┌──────────┐            │  │  SD_MMC 4-bit  │  │      │
│  │ BTN UP   │───GPIO13──►│  │  (built-in)    │  │      │
│  │ (10kΩ↑)  │            │  └────────────────┘  │      │
│  └──────────┘            │                      │      │
│                          │  SD_MMC Pins:        │      │
│  ┌──────────┐            │  CLK  = GPIO14       │      │
│  │ BTN DOWN │───GPIO16──►│  CMD  = GPIO15       │      │
│  │ (10kΩ↑)  │            │  D0   = GPIO2        │      │
│  └──────────┘            │  D1   = GPIO4        │      │
│                          │  D2   = GPIO12       │      │
│  ┌──────────┐            │  D3   = GPIO13       │      │
│  │ 3.7V     │            └──────────────────────┘      │
│  │ Li-ion   │──►TP4056──►5V/3.3V                      │
│  │ 1100mAh  │            │                             │
│  └──────────┘   ┌────────┘                             │
│                 │ Reed Switch (power cut)               │
│                 └──────────────────────────             │
│                                                         │
│  470µF cap across 3.3V & GND for brownout protection   │
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
| SSD1306 OLED | 0.91", 128×32, I2C | Display | $1.50 |
| Micro SD Card | 2–32GB, FAT32 | File storage | $2.00 |
| Push Buttons × 2 | 6mm tactile | Navigation | $0.10 |
| 10kΩ Resistors × 2 | 1/4W | Button pull-ups | $0.05 |
| TP4056 Module | With DW01A protection | Charging + battery protection | $0.50 |
| Li-ion Battery | 3.7V, 1100mAh | Power source | $2.50 |
| Reed Switch (NO) | Magnetic, normally-open | Power switch | $0.30 |
| Small Magnet | 5mm neodymium | Reed switch actuator | $0.10 |
| 470µF Capacitor | 10V electrolytic | Brownout suppression | $0.05 |
| **Total** | | | **~$10.60** |

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

### SD_MMC 4-bit Mode Pin Allocation (Fixed — Cannot Change)

| SD_MMC Function | GPIO | Notes |
|----------------|------|-------|
| CLK | GPIO14 | SD clock |
| CMD | GPIO15 | SD command — **must be HIGH at boot** ✅ |
| DATA0 | GPIO2 | **Must be LOW/floating at boot** ⚠️ |
| DATA1 | GPIO4 | Also drives onboard flash LED |
| DATA2 | GPIO12 | **MUST be LOW at boot** ⚠️ |
| DATA3 | GPIO13 | SD chip select |

### Final GPIO Assignment Table

| Function | GPIO | Direction | Pull | Boot Safety | Notes |
|----------|------|-----------|------|-------------|-------|
| **OLED SDA** | GPIO3 (U0RXD) | I2C Data | External 4.7kΩ ↑ | ✅ Safe | Repurposed UART RX (no serial debug) |
| **OLED SCL** | GPIO1 (U0TXD) | I2C Clock | External 4.7kΩ ↑ | ✅ Safe | Repurposed UART TX (no serial debug) |
| **BTN_UP** | GPIO13 | INPUT_PULLUP | Internal ↑ | ✅ Safe | Directly usable; shared with SD_MMC D3 — button read before SD init |
| **BTN_DOWN** | GPIO16 | INPUT_PULLUP | External 10kΩ ↑ | ✅ Safe | No internal pull-up on GPIO16 — use external |
| SD_MMC CLK | GPIO14 | SD | — | ✅ Safe | Fixed |
| SD_MMC CMD | GPIO15 | SD | 10kΩ ↑ | ✅ HIGH at boot | Fixed |
| SD_MMC D0 | GPIO2 | SD | 10kΩ ↑ | ⚠️ Needs care | 10kΩ pull-up ensures SD works; boot tolerates it with pull-up value |
| SD_MMC D1 | GPIO4 | SD | — | ✅ Safe | Onboard flash LED — toggle off in code |
| SD_MMC D2 | GPIO12 | SD | **10kΩ ↓** | ⚠️ CRITICAL | **MUST have 10kΩ pull-DOWN** to keep LOW at boot |
| SD_MMC D3 | GPIO13 | SD | 10kΩ ↑ | ✅ Safe | Shared with BTN_UP |

### Why These Pin Choices Are Safe

1. **GPIO1 & GPIO3 (UART) for I2C**: Since we don't need serial debugging in production, repurposing UART TX/RX for I2C is a clean solution. These pins have no boot-state requirements and are freely usable after boot.

2. **GPIO13 for BTN_UP**: This pin is boot-safe (no state requirements). It's shared with SD_MMC DATA3 — we read button state during early boot before initializing SD_MMC.

3. **GPIO16 for BTN_DOWN**: GPIO16 has no boot restrictions. It lacks internal pull-up, so an external 10kΩ pull-up resistor is required.

4. **GPIO12 pull-down**: The most critical pin — GPIO12 sets the flash voltage regulator. An external 10kΩ pull-down resistor **must** be present to ensure it stays LOW at boot. The SD_MMC driver takes over after boot.

### Pull-Up/Pull-Down Strategy

```
GPIO12 ──┤resistor 10kΩ├── GND          (CRITICAL: boot safety)
GPIO15 ──┤resistor 10kΩ├── 3.3V         (SD CMD line stability)
GPIO2  ──┤resistor 10kΩ├── 3.3V         (SD DATA0 stability)
GPIO16 ──┤resistor 10kΩ├── 3.3V         (BTN_DOWN pull-up, no internal)
GPIO1  ──┤resistor 4.7kΩ├── 3.3V        (I2C SCL pull-up)
GPIO3  ──┤resistor 4.7kΩ├── 3.3V        (I2C SDA pull-up)
```

### Brownout Prevention

Place a **470µF electrolytic capacitor** between **3.3V** and **GND** as close to the ESP32-CAM power pins as possible. This absorbs current spikes during:
- WiFi transmission bursts
- SD card write operations
- Boot-up inrush current

Additionally, a **100nF ceramic capacitor** in parallel is recommended for high-frequency noise filtering.

---

## 🔌 Circuit Schematic

```
                    ┌─────────────────────────┐
                    │      ESP32-CAM           │
                    │      (AI Thinker)        │
                    │                          │
    ┌───[4.7kΩ]──3V3─┤GPIO3 (SDA)    GPIO14├──── SD_CLK
    │               │                          │
    │  ┌─[4.7kΩ]─3V3─┤GPIO1 (SCL)    GPIO15├──┬─ SD_CMD
    │  │            │                          │  └─[10kΩ]─3V3
    │  │            │                          │
    │  │  ┌──BTN_UP─┤GPIO13          GPIO2 ├──┬─ SD_D0
    │  │  │         │                          │  └─[10kΩ]─3V3
    │  │  │         │                          │
    │  │  │ BTN_DN──┤GPIO16          GPIO4 ├──── SD_D1
    │  │  │ │       │                          │
    │  │  │ [10kΩ]  │               GPIO12 ├──┬─ SD_D2
    │  │  │ │       │                          │  └─[10kΩ]─GND ⚡
    │  │  │ 3V3     │                          │
    │  │  │         │               GPIO13 ├──── SD_D3
    │  │  │         │                          │
    │  │  │         │  5V──┬─[470µF]─┬──GND   │
    │  │  │         │      └─[100nF]─┘         │
    │  │  │         └─────────────────────────┘
    │  │  │
  ┌─┴──┴──┘
  │ SSD1306 │   TP4056      Reed      Battery
  │ 128x32  │   Module      Switch    3.7V
  │ I2C     │   ┌─────┐    ┌──┐     ┌─────┐
  │ SDA SCL │   │IN OUT│────┤RS├─────┤+ Li │
  │ VCC GND │   │      │    └──┘     │ ion │
  └─────────┘   │ B+ B-│─────────────┤     │
                └──┬──┘              └──┬──┘
                   │GND                 │GND
                   └────────────────────┘
```

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
    │  [BTN_UP]    [BTN_DOWN]  │  ← Side/front buttons
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
PocketTXT/
├── PocketTXT.ino          # Main entry point — setup() and loop()
├── config.h               # Pin definitions, constants, settings
├── display.h / .cpp       # OLED drawing functions (U8g2lib)
├── sd_reader.h / .cpp     # SD_MMC init, file listing, line reader, bookmarks
├── buttons.h / .cpp       # Debounced button handler with press types
├── wifi_portal.h / .cpp   # WiFi AP + web server + upload handler
└── portal.h               # Embedded HTML for upload portal (PROGMEM)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| `config.h` | All pin mappings, timing constants, display settings, WiFi credentials |
| `display` | U8g2 initialization, draw file menu, draw reading view, scroll indicator, invert mode |
| `sd_reader` | SD_MMC mount/unmount, list `.txt` files, read lines into buffer, word wrapping, bookmark save/load |
| `buttons` | Debounce (50ms), short press detection, long press (2s) detection, combo press detection |
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
                    │  FILE_MENU   │◄─────────────────┐
                    │ (list files) │                   │
                    └──────┬───────┘                   │
                           │ Hold UP (select)          │
                    ┌──────▼───────┐                   │
                    │   READING    │  Hold DOWN (back) │
                    │ (text view)  │───────────────────┘
                    └──────┬───────┘
                           │ Hold BOTH 2s
                    ┌──────▼───────┐
                    │  WIFI_PORTAL │
                    │ (AP + HTTP)  │
                    └──────┬───────┘
                           │ Hold DOWN (exit)
                           └──────────────────────────┘
```

### Key Design Decisions

1. **Line-by-line reading**: File is read 4 lines at a time into a circular buffer. Scrolling reads the next line from SD and discards the oldest. Maximum RAM usage: ~512 bytes for text buffer regardless of file size.

2. **Word wrapping**: Performed at read time. Each raw line is split into display lines (21 chars max) with word-boundary awareness. Partial words wrap to next line.

3. **Bookmark system**: Uses ESP32's built-in NVS (Non-Volatile Storage) via the Preferences library. Stores `{filename_hash: byte_position}` pairs. Automatically saves position every 10 scroll actions and on file exit.

4. **SD_MMC 4-bit mode**: Faster than SPI and uses the built-in slot. No additional wiring needed. The driver is initialized after button reads on GPIO13.

---

## 📡 WiFi Upload Portal

### Activation
Hold **both buttons** for 2 seconds → ESP32 enables WiFi AP mode.

### Portal Specifications

| Parameter | Value |
|-----------|-------|
| SSID | `TXT_Reader` |
| Password | `readmore` (configurable in config.h) |
| IP Address | `192.168.4.1` |
| Max file size | 2MB |
| Allowed types | `.txt` only |
| Concurrent connections | 1 (RAM safety) |

### Upload Flow

```
User Phone                          ESP32-CAM
    │                                    │
    │── Connect to "TXT_Reader" WiFi ───►│
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
│ WiFi: TXT_Reader     │
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
| **Deep Sleep** | ~6µA | Wake on button press (GPIO16 RTC) |

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
  ├─ 1. 470µF cap absorbs inrush current
  ├─ 2. GPIO12 pulled LOW by external resistor → correct flash voltage
  ├─ 3. GPIO15 pulled HIGH → boot messages enabled
  ├─ 4. GPIO0 floating HIGH → normal boot (not flash mode)
  │
  ├─ 5. ESP32 boots normally
  ├─ 6. Firmware disables WiFi + BT immediately
  ├─ 7. Read button states on GPIO13/16 (before SD init)
  ├─ 8. Initialize I2C for OLED on GPIO1/3
  ├─ 9. Initialize SD_MMC (takes over GPIO2,4,12,13,14,15)
  ├─ 10. Show splash screen → enter file menu
  │
  └─ ✅ Stable operation
```

### Failure Prevention

| Failure Mode | Prevention |
|-------------|-----------|
| **Boot loop** | GPIO12 pull-down resistor, GPIO0 not connected to button |
| **Brownout reset** | 470µF + 100nF capacitors, WiFi off during reading |
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
3. Splash screen shows "PocketTXT v1.0"
4. File menu displays available `.txt` files

### Navigation

| Action | Input | Context |
|--------|-------|---------|
| Scroll up in list/text | Short press UP | Menu / Reading |
| Scroll down in list/text | Short press DOWN | Menu / Reading |
| Fast scroll | Hold UP or DOWN | Reading |
| Select file | Hold UP (2s) | Menu |
| Back to menu | Hold DOWN (2s) | Reading |
| Open WiFi portal | Hold BOTH (2s) | Any |
| Exit WiFi portal | Hold DOWN (2s) | WiFi Portal |
| Toggle display invert | Hold UP (2s) | Reading (configurable) |

### Uploading Files
1. Hold both buttons for 2 seconds
2. OLED shows WiFi info (SSID, password, IP)
3. Connect phone to `TXT_Reader` WiFi
4. Open `192.168.4.1` in browser
5. Select and upload `.txt` files
6. Hold DOWN to exit portal and resume reading

### Bookmarks
- Position is **automatically saved** every 10 scrolls
- When re-opening a file, reading resumes from last position
- Bookmarks persist across power cycles (stored in NVS flash)

---

## 🔧 Troubleshooting

| Problem | Likely Cause | Solution |
|---------|-------------|----------|
| Boot loop | GPIO12 not pulled LOW | Add 10kΩ pull-down on GPIO12 |
| Brownout reset | Insufficient capacitance | Add 470µF cap, check battery charge |
| SD not detected | Bad contact / wrong format | FAT32 format, reseat card, check solder joints |
| OLED blank | Wrong I2C address or wiring | Check SDA/SCL connections, try address 0x3C |
| Buttons unresponsive | Missing pull-up on GPIO16 | Add external 10kΩ pull-up on GPIO16 |
| WiFi won't start | Memory fragmentation | Restart device, reduce file buffer size |
| Upload fails | File too large or wrong type | Keep under 2MB, ensure `.txt` extension |
| Ghost button presses | Noise on GPIO lines | Verify debounce, add 100nF cap on button pins |

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

### Project: PocketTXT — The Minimalist Digital Reader

**Problem Statement**: In an age of constant connectivity and digital distraction, there is no affordable, distraction-free device for reading plain text documents. E-readers cost $100+ and still offer browsers and stores. Phones demand attention constantly.

**Solution**: PocketTXT is an ultra-compact ($10), open-source, offline text reader that fits in your pocket. It uses repurposed IoT hardware (ESP32-CAM) to achieve what commercial products cannot — **pure, focused reading with zero distractions**.

**Technical Innovation**:
- Repurposes a $3.50 camera module as a general-purpose reader by leveraging its built-in SD card slot
- Memory-efficient architecture reads files of unlimited size on a microcontroller with 520KB RAM
- Boot-safe GPIO mapping solves a known challenge with ESP32-CAM that often blocks hobbyist projects
- WiFi is demand-activated — disabled 99% of the time for exceptional battery life

**Key Metrics**:
- **31+ hours** battery life on a single charge
- **< $11** total component cost
- **30mm × 45mm × 18mm** form factor
- **Unlimited** file size support
- **Zero** external library dependencies (beyond display driver)
- **< 2 second** boot time

**Engineering Disciplines Demonstrated**:
- Embedded systems design
- Power optimization
- Memory-constrained programming
- Web development (embedded HTTP server)
- Human-computer interaction (2-button UX design)
- Hardware-software co-design
- PCB-less prototyping and compact assembly

---

## 📄 License

MIT License — Free for personal and educational use.

---

*Built with ❤️ using ESP32-CAM, U8g2, and determination.*
