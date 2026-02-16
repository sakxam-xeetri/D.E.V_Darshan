# DEV_Darshan — Wiring Reference

## Complete Wiring Table

| # | From | To | Wire/Component | Notes |
|---|------|-----|---------------|-------|
| 1 | ESP32-CAM `3V3` | Breadboard 3.3V rail | Power | Main 3.3V supply |
| 2 | ESP32-CAM `GND` | Breadboard GND rail | Power | Common ground |
| 3 | ESP32-CAM `GPIO 13` | OLED `SDA` | Signal wire | I2C Data |
| 4 | ESP32-CAM `GPIO 12` | OLED `SCL` | Signal wire | I2C Clock |
| 5 | OLED `VCC` | 3.3V rail | Power | OLED power |
| 6 | OLED `GND` | GND rail | Power | OLED ground |
| 7 | ESP32-CAM `GPIO 16` | BTN_UP one leg | Signal wire | Active LOW |
| 8 | BTN_UP other leg | GND rail | Wire | Button ground |
| 9 | ESP32-CAM `GPIO 3` | BTN_DOWN one leg | Signal wire | Active LOW |
| 10 | BTN_DOWN other leg | GND rail | Wire | Button ground |
| 11 | ESP32-CAM `GPIO 1` | BTN_SELECT one leg | Signal wire | Active LOW |
| 12 | BTN_SELECT other leg | GND rail | Wire | Button ground |

## Pull Resistor Wiring

| # | From | To | Component | Purpose |
|---|------|-----|----------|---------|
| R1 | GPIO 2 | 3.3V | 10 kΩ resistor | SD D0 boot pull-up |
| R2 | GPIO 15 | 3.3V | 10 kΩ resistor | SD CMD boot pull-up |
| R3 | GPIO 13 | 3.3V | 4.7 kΩ resistor | I2C SDA pull-up |
| R4 | GPIO 12 | 3.3V | 4.7 kΩ resistor | I2C SCL pull-up |
| R5 | GPIO 12 | GND | 10 kΩ resistor | **Critical**: keeps GPIO12 LOW at boot |
| R6 | GPIO 3 | 3.3V | 10 kΩ resistor | Button pull-up (RX pin) |
| R7 | GPIO 1 | 3.3V | 10 kΩ resistor | Button pull-up (TX pin) |

> **R4 + R5 together** on GPIO 12: The 10 kΩ to GND dominates at boot (pulls LOW). After boot, the I2C SCL driver actively toggles the pin — the pull-down doesn't interfere with I2C at 100 kHz.

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

## GPIO 12 Boot Safety — Detailed Explanation

GPIO 12 is a VDD_SDIO strapping pin on ESP32:
- If HIGH at boot → sets VDD_SDIO to 1.8V → **will crash with 3.3V flash**
- If LOW at boot → sets VDD_SDIO to 3.3V → **correct for ESP32-CAM**

Our solution:
```
GPIO 12 ──┬──── 4.7 kΩ ──── 3.3V   (I2C SCL pull-up)
           │
           └──── 10 kΩ ──── GND     (boot safety pull-down)
```

At boot (high-impedance state):
- Pull-down: 3.3V / 10kΩ = 0.33 mA pulling LOW
- Pull-up: 3.3V / 4.7kΩ = 0.70 mA pulling HIGH
- BUT the voltage divider gives: `3.3 × (10k / (10k + 4.7k)) = 2.24V`
- This is above the HIGH threshold...

**Better approach**: Use a 10 kΩ pull-down and **no** external pull-up on GPIO 12. The SSD1306 I2C works fine with just the internal weak pull-up enabled after boot, or rely on the OLED module's onboard pull-ups (most modules have 10 kΩ pull-ups).

**Revised R4**: Remove the external 4.7 kΩ on GPIO 12. Keep only R5 (10 kΩ to GND). The OLED module's built-in pull-up (or software `Wire.begin()` with internal pull-up) handles SCL after boot.

| Revised | From | To | Component | Purpose |
|---------|------|-----|----------|---------|
| R4 | ~~GPIO 12~~ | ~~3.3V~~ | ~~4.7 kΩ~~ | **REMOVED** — use OLED module built-in pull-up |
| R5 | GPIO 12 | GND | 10 kΩ | Boot safety — keeps LOW during boot |

## SD Card Notes

The ESP32-CAM has a **built-in** microSD slot wired to:
- GPIO 2 → Data 0
- GPIO 14 → CLK  
- GPIO 15 → CMD
- GPIO 4 → Data 1 (unused in 1-bit mode)
- GPIO 12 → Data 2 (unused in 1-bit mode)
- GPIO 13 → Data 3 / CS (unused in 1-bit mode)

By using **1-bit mode**, we only need GPIO 2, 14, 15 — freeing GPIO 4, 12, 13 for other purposes.

## Capacitor Placement

The **470 µF electrolytic capacitor** prevents brownout resets caused by:
1. SD card inrush current during mount (~100 mA spike)
2. WiFi TX power bursts (~150-300 mA spikes)
3. Voltage droop on weak battery connections

**Place it directly across the 3.3V and GND pins** on the ESP32-CAM, as close to the module as physically possible. Use short leads.

Optional: Add a 100 nF ceramic capacitor in parallel for high-frequency noise filtering.
