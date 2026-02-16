# DEV_Darshan — Pocket TXT eBook Reader

> Ultra-compact, battery-powered, offline `.txt` file reader built on the **ESP32-CAM (AI Thinker)** — camera module removed. Designed to fit inside a calculator shell, replacing the solar cell with a 0.91" OLED display.

---

## Features

| Feature | Details |
|---|---|
| **Display** | 0.91" SSD1306 OLED, 128×32 pixels, I2C |
| **Storage** | MicroSD via built-in SD_MMC slot (1-bit mode) |
| **Controls** | 3 tactile buttons — UP, DOWN, SELECT |
| **Power** | 3.7 V 800 mAh Li-ion + TP4056 charger + magnetic reed switch |
| **WiFi Upload** | Temporary AP mode with mobile-responsive web portal |
| **Battery Life** | ~14-17 hrs reading / ~4-6 hrs WiFi active |

---

## Architecture

```
┌─────────────────────────────────────────┐
│              main.cpp                   │
│         (State Machine Controller)      │
│                                         │
│   BOOT → FILE_MENU → READING           │
│              ↕            ↕             │
│         WIFI_PORTAL   (back to menu)    │
├─────────────────────────────────────────┤
│  display_manager  │  sd_manager         │
│  input_manager    │  wifi_portal_manager│
│  power_manager    │  config.h           │
└─────────────────────────────────────────┘
```

### Module Responsibilities

| Module | Purpose |
|---|---|
| `config.h` | All pin definitions, constants, timing parameters |
| `display_manager` | SSD1306 OLED driver — splash, menus, reader, messages |
| `sd_manager` | SD_MMC 1-bit mount, `.txt` listing, streaming line reader |
| `input_manager` | 3-button polling with debounce, long-press, auto-repeat |
| `wifi_portal_manager` | AP mode, embedded web server, file upload handler |
| `power_manager` | WiFi/BT disable, CPU scaling, battery ADC, sleep modes |
| `main.cpp` | State machine, word-wrap engine, UI flow controller |

---

## GPIO Mapping (ESP32-CAM AI Thinker — Boot-Safe)

### Boot Constraints

| GPIO | Boot Requirement | Consequence if Wrong |
|------|-----------------|---------------------|
| GPIO 0 | Must be **HIGH** | Enters flash/download mode |
| GPIO 2 | Must be **HIGH** | Boot fails |
| GPIO 12 | Must be **LOW** | Sets wrong flash voltage → crash |
| GPIO 15 | Must be **HIGH** | Suppresses boot log (usually OK) |

### Final Pin Assignment

| Function | GPIO | Pull Resistor | Notes |
|----------|------|--------------|-------|
| SD DATA0 | 2 | SD card built-in | Boot-safe (HIGH) |
| SD CLK | 14 | — | Shared with I2C SCL |
| SD CMD | 15 | SD card built-in | Boot-safe (HIGH) |
| OLED SDA | 13 | OLED module built-in | No external resistor needed |
| OLED SCL | 14 | OLED module built-in | Shared with SD CLK — no conflict |
| BTN UP | 16 | ESP32 internal pull-up | Active LOW, no external resistor |
| BTN DOWN | 3 (RX) | ESP32 internal pull-up | Active LOW, no external resistor |
| BTN SELECT | 1 (TX) | ESP32 internal pull-up | Active LOW, no external resistor |

### Why These Pins Are Safe (and No External Resistors Needed!)

- **GPIO 16** — No boot function, perfect for buttons with internal pull-up.
- **GPIO 1 & 3** (TX/RX) — Only used by UART0 during boot. After boot, they're free for GPIO with internal pull-ups enabled.
- **GPIO 13** — Not a strapping pin. Free when SD_MMC runs in 1-bit mode.
- **GPIO 14** — Shared between SD CLK and I2C SCL. Works perfectly because:
  - OLED initializes first (before SD mount)
  - Display updates happen between SD reads (not simultaneously)
  - The OLED module's built-in pull-ups don't interfere with SD communication
- **No GPIO 12** — We avoid GPIO 12 entirely (it needs LOW at boot), so no pull-down resistor needed!
- **All pull-ups built-in** — SD cards, OLED modules, and ESP32 GPIOs all have built-in pull-up/pull-down resistors

---

## Wiring Diagram

```
                    ┌─────────────────────┐
                    │    ESP32-CAM         │
                    │   (AI Thinker)       │
                    │                      │
  ┌──── 3.3V ──────┤ 3V3            GND ├──────── GND ────┐
  │                 │                      │                │
  │  ┌── SDA ──────┤ GPIO 13              │                │
  │  │  ┌ SCL ─────┤ GPIO 14 (also SD CLK)│                │
  │  │  │           │                      │                │
  │  │  │  ┌───────┤ GPIO 16   GPIO 2 ├──── SD D0          │
  │  │  │  │        │           GPIO 14├──── SD CLK         │
  │  │  │  │        │           GPIO 15├──── SD CMD         │
  │  │  │  │        │                      │                │
  │  │  │  │  ┌────┤ GPIO 3 (RX)          │                │
  │  │  │  │  │ ┌──┤ GPIO 1 (TX)          │                │
  │  │  │  │  │ │   │                      │                │
  │  │  │  │  │ │   │    5V ──────────┤    │                │
  │  │  │  │  │ │   └─────────────────────┘                │
  │  │  │  │  │ │                                           │
  │  ▼  ▼  │  │ │                                           │
  │ ┌──────┐│  │ │    ┌─────────┐       ┌──────────────┐   │
  │ │SSD1306││  │ │    │ TP4056  │       │  3.7V 800mAh │   │
  │ │ OLED ││  │ │    │ Charger │       │   Li-ion     │   │
  │ │128×32││  │ │    │         │       │   Battery    │   │
  │ │      ││  │ │    │ USB-C──IN│       │              │   │
  │ │VCC←──┘│  │ │    │ OUT+ ───┼───┐   │  (+)──┐      │   │
  │ │GND←───┘  │ │    │ OUT- ───┼─┐ │   │  (-)──┼──┐   │   │
  │ │SDA←──┘   │ │    └─────────┘ │ │   └───────┘  │   │   │
  │ │SCL←──┘   │ │                │ │      │    ▲   │   │   │
  │ └──────┘   │ │                │ │      │    │   │   │   │
  │            │ │                │ └──────┼── Reed  │   │   │
  │  Buttons:  │ │                │        │  Switch │   │   │
  │ ┌──┐       │ │                │        │  (mag)  │   │   │
  │ │UP├───────┘ │                │        │         │   │   │
  │ └┬─┘         │                └────────┼─────────┘   │   │
  │  └─── GND   │                         │             │   │
  │ ┌────┐       │                         └─────────────┘   │
  │ │DOWN├───────┘                                           │
  │ └┬───┘                                                   │
  │  └─── GND                                               │
  │ ┌───┐                                                    │
  │ │SEL├────────┘                                           │
  │ └┬──┘                                                    │
  │  └─── GND                                               │
  └──────────────────────────────────────────────────────────┘

  ✅ NO EXTERNAL RESISTORS NEEDED!
  
  • SD card has built-in pull-ups on DATA0 and CMD lines
  • OLED module has built-in 4.7kΩ or 10kΩ pull-ups on SDA/SCL
  • ESP32 internal pull-ups handle all 3 buttons (enabled in code)
  • GPIO 14 shared between SD CLK and I2C SCL — works perfectly!

  Power Stability:
  • 470 µF electrolytic capacitor across 3.3V and GND (near ESP32)
```

---

## Physical Stacking Layout (Calculator Shell)

```
  TOP VIEW (inside calculator body)
  ┌─────────────────────────┐
  │  ┌───────────────────┐  │ ← Calculator front panel
  │  │  [OLED 128×32]    │  │    (solar cell cutout = display window)
  │  └───────────────────┘  │
  │                         │
  │  [UP]  [DOWN]  [SELECT] │ ← Calculator keys repurposed
  │                         │
  └─────────────────────────┘

  SIDE VIEW (stacking order, ~8mm total):
  ┌─────────────────────────┐
  │  Calculator front shell │  ~1.5 mm
  ├─────────────────────────┤
  │  SSD1306 OLED module    │  ~1.5 mm
  ├─────────────────────────┤
  │  ESP32-CAM board        │  ~3.0 mm (camera connector removed)
  ├─────────────────────────┤
  │  Li-ion battery (flat)  │  ~4.0 mm (800 mAh pouch cell)
  ├─────────────────────────┤
  │  TP4056 module          │  ~1.5 mm (trimmed to minimal size)
  ├─────────────────────────┤
  │  Calculator back shell  │  ~1.5 mm
  └─────────────────────────┘
  Total: ~13 mm (fits most scientific calc shells)

  MAGNETIC REED SWITCH placement:
  • Glued to inside edge of case
  • Small neodymium magnet on a slide tab or external position
  • Moving magnet away = switch opens = power cut (true OFF)
```

---

## Power Budget

### Reading Mode (WiFi OFF, BT OFF, CPU @ 80 MHz)

| Component | Current Draw |
|-----------|-------------|
| ESP32 core @ 80 MHz | ~30 mA |
| SSD1306 OLED | ~10-15 mA |
| SD card (idle, brief read spikes) | ~1-2 mA avg |
| LDO quiescent + misc | ~2 mA |
| **Total** | **~45-55 mA** |

**Battery life**: 800 mAh ÷ 50 mA ≈ **16 hours** continuous reading

### WiFi Upload Mode

| Component | Current Draw |
|-----------|-------------|
| ESP32 WiFi AP active | ~120-180 mA |
| OLED | ~15 mA |
| SD writes | ~40 mA peaks |
| **Total** | **~150-180 mA** |

**Battery life**: 800 mAh ÷ 165 mA ≈ **4.8 hours** WiFi active

### Sleep Mode (future enhancement)

| Mode | Current | Duration |
|------|---------|----------|
| Light sleep | ~2-5 mA | 160-400 hrs |
| Deep sleep | ~10 µA | Years |

---

## Build & Flash (Arduino IDE)

### Prerequisites
1. **Arduino IDE** 1.8.x or 2.x
2. **ESP32 Board Package** — add this URL in *File → Preferences → Additional Board Manager URLs*:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   Then install **"esp32" by Espressif** from *Tools → Board Manager*.
3. **OLED Library** — install via *Sketch → Include Library → Manage Libraries*:
   - Search for **"ESP8266 and ESP32 OLED driver for SSD1306 displays"** by ThingPulse → Install
4. **FTDI / USB-to-Serial adapter** (for uploading to ESP32-CAM)

### Board Settings
| Setting | Value |
|---------|-------|
| Board | **AI Thinker ESP32-CAM** |
| Upload Speed | 921600 (or 115200 if unstable) |
| CPU Frequency | 240 MHz (firmware downclocks to 80 MHz at runtime) |
| Flash Frequency | 80 MHz |
| Partition Scheme | Default 4MB with spiffs |
| Port | Your FTDI COM port |

### Upload Procedure
1. Connect FTDI adapter: `FTDI TX → ESP32 U0R`, `FTDI RX → ESP32 U0T`, `GND → GND`
2. **Connect GPIO 0 to GND** (put ESP32-CAM in flash/download mode)
3. Press the RST button on ESP32-CAM
4. Click **Upload** in Arduino IDE
5. When upload completes, **disconnect GPIO 0 from GND**
6. Press RST again — device boots normally

### Serial Monitor
Open *Tools → Serial Monitor*, set baud to **115200**

---

## Usage Guide

### Normal Operation
1. **Power ON** — slide magnet away from reed switch
2. **Boot splash** → SD card mounts → file list appears
3. **UP/DOWN** — navigate file list
4. **SELECT** — open selected `.txt` file
5. **UP/DOWN** — scroll through text (hold for fast scroll)
6. **SELECT** — return to file list

### WiFi Upload
1. **Long-press SELECT** (from any screen)
2. OLED shows SSID and IP address
3. Connect phone to **DEV_Reader** WiFi (password: `read1234`)
4. Open **192.168.4.1** in browser
5. Upload `.txt` files via the web portal
6. **Press SELECT** to exit WiFi mode and return to reader

---

## Stability Features

| Risk | Mitigation |
|------|-----------|
| **Boot failure** | Boot-safe GPIO mapping, proper pull-up/pull-down resistors, safe init order |
| **Brownout resets** | 470 µF cap, WiFi disabled at boot, CPU clocked down to 80 MHz |
| **SD mount failure** | 3-attempt retry with delays, error screen with recovery option |
| **Memory crashes** | Streaming line reader (no full file in RAM), fixed-size buffers, no dynamic allocation |
| **Button ghost triggers** | Hardware pull-ups on GPIO 1/3, proper debounce (50 ms), clean state machine |
| **WiFi RAM pressure** | Single AP client limit, embedded HTML in PROGMEM, stream upload to SD |

---

## Project Structure

```
DEV_Darshan/
├── DEV_Darshan.ino             # Main sketch (setup + loop + state machine)
├── config.h                    # Pin map, constants, all settings
├── display_manager.h           # OLED display API
├── display_manager.cpp         # SSD1306 I2C driver
├── sd_manager.h                # SD card API
├── sd_manager.cpp              # SD_MMC 1-bit driver
├── input_manager.h             # Button input API
├── input_manager.cpp           # Debounced button handler
├── wifi_portal_manager.h       # WiFi upload portal API
├── wifi_portal_manager.cpp     # AP + web server + upload page
├── power_manager.h             # Power management API
├── power_manager.cpp           # Radio/sleep/battery control
├── README.md                   # This file
└── docs/
    ├── WIRING.md               # Detailed wiring reference
    └── COMPETITION_BRIEF.md    # Presentation-ready summary
```

---

## Future Upgrade Ideas

| Enhancement | Difficulty | Impact |
|-------------|-----------|--------|
| **Bookmark system** — save last read position per file to SD | Easy | High |
| **Font size toggle** — switch between 4-line and 3-line modes | Easy | Medium |
| **Auto-sleep** — light sleep after 60s inactivity, wake on button | Medium | High |
| **e-Ink display** — replace OLED with 1.54" e-Ink for sunlight readability | Medium | High |
| **PDF/EPUB support** — parse formatted documents | Hard | High |
| **OTA firmware update** — update via WiFi portal | Medium | Medium |
| **Battery gauge IC** — MAX17048 for accurate % reading | Easy | Medium |
| **Multi-language fonts** — Unicode/Devanagari support | Hard | Medium |
| **Encryption** — password-protected files | Medium | Low |
| **Reading stats** — track pages/time per book | Easy | Low |

---

## License

This project is created as part of an educational/competition build. Free to use and modify.

---

**Built with precision for the DEV_Darshan competition entry.**
