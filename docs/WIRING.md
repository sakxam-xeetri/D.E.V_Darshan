# DEV_Darshan — Wiring Reference (NO RESISTORS NEEDED!)

## Complete Wiring Table

| # | From | To | Wire/Component | Notes |
|---|------|-----|---------------|-------|
| 1 | ESP32-CAM `3V3` | Breadboard 3.3V rail | Power | Main 3.3V supply |
| 2 | ESP32-CAM `GND` | Breadboard GND rail | Power | Common ground |
| 3 | ESP32-CAM `GPIO 13` | OLED `SDA` | Signal wire | I2C Data |
| 4 | ESP32-CAM `GPIO 14` | OLED `SCL` | Signal wire | I2C Clock (shared with SD CLK) |
| 5 | OLED `VCC` | 3.3V rail | Power | OLED power |
| 6 | OLED `GND` | GND rail | Power | OLED ground |
| 7 | ESP32-CAM `GPIO 16` | BTN_UP one leg | Signal wire | Active LOW |
| 8 | BTN_UP other leg | GND rail | Wire | Button ground |
| 9 | ESP32-CAM `GPIO 3` | BTN_DOWN one leg | Signal wire | Active LOW |
| 10 | BTN_DOWN other leg | GND rail | Wire | Button ground |
| 11 | ESP32-CAM `GPIO 1` | BTN_SELECT one leg | Signal wire | Active LOW |
| 12 | BTN_SELECT other leg | GND rail | Wire | Button ground |

## ✅ NO Pull Resistors Needed!

**All pull-ups and pull-downs are built-in!**

1. **SD Card** — has built-in pull-up resistors on DATA0 (GPIO 2) and CMD (GPIO 15) lines
2. **OLED Module** — most SSD1306 modules have built-in 4.7 kΩ or 10 kΩ pull-ups on SDA and SCL
3. **ESP32 Buttons** — use internal pull-ups (enabled via `pinMode(pin, INPUT_PULLUP)` in firmware)

**Result: Zero external resistors required for this project!**

## Power Section

```
Battery (+) ──── Reed Switch ──── TP4056 B+ ──── Battery (+)
Battery (-) ──────────────────── TP4056 B- ──── Battery (-)

TP4056 OUT+ ──── ESP32-CAM 5V pin (through the AMS1117 regulator)
TP4056 OUT- ──── ESP32-CAM GND

470 µF Capacitor: (+) to 3.3V rail, (-) to GND rail
   └── Place as close to ESP32-CAM 3V3 pin as possible
```

### Power Flow

```
Li-ion 3.7V → Reed Switch → TP4056 (protection) → ESP32-CAM 5V → AMS1117 → 3.3V
                  ↑                ↑
            Magnetic ON/OFF    USB-C charging
```

## SD Card Notes

The ESP32-CAM has a **built-in** microSD slot wired to:
- GPIO 2 → Data 0
- GPIO 14 → CLK  
- GPIO 15 → CMD
- GPIO 4 → Data 1 (unused in 1-bit mode)
- GPIO 12 → Data 2 (unused in 1-bit mode)
- GPIO 13 → Data 3 / CS (unused in 1-bit mode)

By using **1-bit mode**, we only need GPIO 2, 14, 15 — freeing GPIO 4, 12, 13 for other purposes.

## GPIO 14 Pin Sharing — I2C SCL and SD CLK

GPIO 14 is cleverly shared between:
1. **I2C SCL** (for OLED display)
2. **SD_MMC CLK** (for SD card)

**Why this works perfectly:**
- The OLED is initialized first (in `setup()`) before SD card mounting
- Display updates happen between SD read operations, never simultaneously
- When SD reads occur, I2C communication is idle
- The OLED module's built-in pull-ups don't interfere with SD clock signals
- Both protocols use open-drain/push-pull compatible signaling

This is a standard technique in embedded systems to maximize GPIO usage on pin-limited MCUs.

## Capacitor Placement

The **470 µF electrolytic capacitor** prevents brownout resets caused by:
1. SD card inrush current during mount (~100 mA spike)
2. WiFi TX power bursts (~150-300 mA spikes)
3. Voltage droop on weak battery connections

**Place it directly across the 3.3V and GND pins** on the ESP32-CAM, as close to the module as physically possible. Use short leads.

Optional: Add a 100 nF ceramic capacitor in parallel for high-frequency noise filtering.
