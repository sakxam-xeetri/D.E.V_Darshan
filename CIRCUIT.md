# D.E.V_Darshan — Circuit Connections

## ASCII Wiring Diagram

```
                     ┌──────────────────────────────┐
                     │        ESP32-CAM (top)        │
                     │         AI Thinker            │
                     │                               │
                     │  5V ●──────────┐              │
                     │ GND ●──────┐   │              │
                     │ GPIO12 ●   │   │              │  (SDA → OLED)
                     │ GPIO14 ●   │   │              │  (SCL → OLED)
                     │ GPIO13 ●   │   │              │  (UP Button)
                     │ GPIO15 ●   │   │              │  (DOWN Button)
                     │ 3V3  ●─┐   │   │              │
                     │        │   │   │              │
                     └────────┼───┼───┼──────────────┘
                              │   │   │
         ┌────────────────────┼───┼───┼──────────────────────┐
         │                    │   │   │                      │
         │   ┌────────────┐   │   │   │   ┌──────────────┐   │
         │   │ 0.96" OLED │   │   │   │   │   470µF Cap   │   │
         │   │  SSD1306   │   │   │   │   │  (16V)       │   │
         │   │            │   │   │   │   │              │   │
         │   │ VCC ●──────┘   │   │   │   │  (+)●────────┼───┘ (5V)
         │   │ GND ●──────────┘   │   │   │  (-)●────────┘ (GND)
         │   │ SDA ●──── GPIO12   │   │   └──────────────┘
         │   │ SCL ●──── GPIO14   │   │
         │   └────────────┘       │   │
         │                        │   │
         │   ┌────────────┐       │   │
         │   │  UP Button │       │   │
         │   │  (GPIO13)  │       │   │
         │   │            │       │   │
         │   │ Pin1 ●──── GPIO13  │   │
         │   │ Pin2 ●─────────────┘   │  (GND)
         │   └────────────┘           │
         │                            │
         │   ┌────────────┐           │
         │   │ DOWN Button│           │
         │   │  (GPIO15)  │           │
         │   │            │           │
         │   │ Pin1 ●──── GPIO15      │
         │   │ Pin2 ●────────────────┘  (GND)
         │   └────────────┘
         │
         └───────────────────────────────────────────────────┘
```

---

## Detailed Connections

### 1. OLED Display (0.96" SSD1306 — I2C)

| OLED Pin | ESP32-CAM Pin | Notes |
|----------|---------------|-------|
| **VCC** | **3.3V** | Power supply for OLED |
| **GND** | **GND** | Common ground |
| **SDA** | **GPIO 13** | I2C Data (software I2C via U8g2) |
| **SCL** | **GPIO 0** | I2C Clock (software I2C via U8g2) |

> **Why GPIO 13 & 0?** GPIO 14/15 conflict with SD_MMC (even in 1-bit mode). GPIO 12 as I2C with pull-up causes boot failure (strapping pin). GPIO 13 is clean, and GPIO 0's pull-up keeps it HIGH at boot → normal boot mode. **Note:** Disconnect OLED for serial flashing (GPIO 0 must be LOW for flash mode), or use OTA updates.

---

### 2. Push Buttons

| Button | ESP32-CAM Pin | Wiring | Internal Pull-up |
|--------|---------------|--------|------------------|
| **UP** | **GPIO 12** | One leg → GPIO 12, other leg → GND | Yes (`INPUT_PULLUP`) |
| **DOWN** | **GPIO 3** | One leg → GPIO 3, other leg → GND | Yes (`INPUT_PULLUP`) |

> Buttons connect the GPIO to GND when pressed. The internal pull-up resistor holds the pin HIGH when released. **LOW = pressed, HIGH = released.**
> **GPIO 12:** No external pull-up, so defaults LOW at boot → 3.3V flash → safe.
> **GPIO 3 (RX):** Serial RX is disabled in firmware to free this pin for button use.

**Button behavior:**

| Action | Duration | Result |
|--------|----------|--------|
| Short press UP | < 2 s | Scroll up |
| Short press DOWN | < 2 s | Scroll down |
| Long press UP | ≥ 2 s | Select file / Enter |
| Long press DOWN | ≥ 2 s | Back to home |
| Long press BOTH | ≥ 2 s | Toggle Wi-Fi portal |

---

### 3. MicroSD Card (Built-in SD_MMC)

| SD_MMC Signal | ESP32-CAM Pin | Notes |
|---------------|---------------|-------|
| CLK | GPIO 14 | Shared internally |
| CMD | GPIO 15 | Shared internally |
| DATA0 | GPIO 2 | 1-bit mode |

> **No external wiring needed.** The SD card slot is built into the ESP32-CAM board. We use 1-bit mode (`SD_MMC.begin("/sdcard", true)`) to minimize pin conflicts.

> **Important:** GPIO 2 must be left floating or LOW during boot for reliable startup. The firmware handles this after initialization.

---

### 4. Power Supply

```
                    ┌───────────┐
  18650 Battery ──► │  TP4056   │ ──► 5V Boost ──► ESP32-CAM 5V pin
  (3.7V)            │  Charger  │                       │
                    └───────────┘                       │
                                                   ┌────┴────┐
                                          470µF Cap │ (+) (-) │
                                                   └────┬────┘
                                                        │
                                                       GND
```

| Power Pin | Connection | Notes |
|-----------|------------|-------|
| **5V** | USB or TP4056 output via boost converter | Main power input |
| **GND** | Common ground for all components | |
| **3.3V** | Onboard regulator output → OLED VCC | Do not draw >200 mA |

> **470 µF / 16V electrolytic capacitor** placed across **5V** and **GND** as close to ESP32-CAM as possible. This absorbs voltage spikes during Wi-Fi transmissions and SD card operations.

---

## GPIO Pin Summary

| GPIO | Used For | Direction | Notes |
|------|----------|-----------|-------|
| 13 | OLED SDA | Output | Software I2C data |
| 0 | OLED SCL | Output | Software I2C clock (disconnect for serial flash) |
| 12 | UP Button | Input | `INPUT_PULLUP`, active LOW, strapping safe |
| 3 | DOWN Button | Input | `INPUT_PULLUP`, active LOW (Serial RX disabled) |
| 2 | SD DATA0 | Internal | SD_MMC 1-bit mode |
| 14 | SD CLK | Internal | SD_MMC 1-bit mode |
| 15 | SD CMD | Internal | SD_MMC 1-bit mode |
| 4 | Flash LED | — | Not used (built-in LED) |

---

## Pins to Avoid for Buttons

| GPIO | Reason |
|------|--------|
| **2** | Strapping pin — used by SD_MMC DATA0 |
| **14** | Used by SD_MMC CLK (even in 1-bit mode) |
| **15** | Used by SD_MMC CMD (even in 1-bit mode) |
| **16** | PSRAM (used internally on WROVER modules) |
| **4** | Connected to flash LED — INPUT_PULLUP turns LED on |

---

## Assembly Tips

1. **Solder headers** on the ESP32-CAM if not already present.
2. **Keep wires short** for I2C — long wires can cause display glitches.
3. **Place the capacitor** as close to the ESP32-CAM 5V/GND pins as possible.
4. **Test without battery first** — use USB power via FTDI for debugging.
5. **Format SD card as FAT32** before first use.
6. **Hot-glue** the OLED and buttons to a small enclosure for pocket carry.

---

**Built by Sakshyam Bastakoti — D.E.V_Darshan**
