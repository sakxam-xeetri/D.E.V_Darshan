# Circuit Connections - D.E.V_Darshan TXT Reader

This document provides detailed wiring instructions for the ESP32-CAM Mini TXT Reader.

---

## 🔌 Complete Wiring Diagram

### ESP32-CAM Pinout Reference

```
                    ESP32-CAM (Top View)
                 ┌─────────────────────┐
                 │                     │
            ANT  │  ╔═══════════╗     │
                 │  ║  ANTENNA  ║     │
                 │  ╚═══════════╝     │
                 │                     │
         5V  ●───┤                     ├───●  GND
        GND  ●───┤                     ├───●  IO12
       IO13 ●───┤                     ├───●  IO13  
       IO15 ●───┤   [AI-THINKER]      ├───●  IO15
       IO14 ●───┤    ESP32-CAM        ├───●  IO14
       IO2  ●───┤                     ├───●  IO2
       IO4  ●───┤                     ├───●  IO4
       RX   ●───┤                     ├───●  TX
       IO16 ●───┤   [SD CARD SLOT]    ├───●  GND
                 │                     │
       3.3V ●───┤                     ├───●  IO33
        GND ●───┤                     ├───●  IO0
        RST ●───┤                     ├───●  U0R
                 └─────────────────────┘
                      (Camera facing up)
```

---

## 📋 Component Connection Tables

### 1. OLED Display (SSD1306 0.96" I2C)

| OLED Pin | ESP32-CAM Pin | Wire Color (suggested) |
|----------|---------------|------------------------|
| VCC | 3.3V | Red |
| GND | GND | Black |
| SCL | GPIO 12 | Yellow |
| SDA | GPIO 13 | Blue |

**Notes:**
- Use **3.3V** not 5V for OLED to prevent damage
- Some OLED displays have pins in different order (GND/VCC/SCL/SDA)
- Keep wires short (< 15cm) for reliable I2C communication

---

### 2. SD Card (Built-in Slot - 1-bit Mode)

| SD Pin | ESP32-CAM GPIO | Function |
|--------|----------------|----------|
| CMD | GPIO 15 | Command line |
| CLK | GPIO 14 | Clock |
| D0 | GPIO 2 | Data 0 |
| VSS | GND | Ground |
| VDD | 3.3V | Power |

**Notes:**
- **Use built-in SD card slot** on ESP32-CAM
- Format SD card as **FAT32**
- Keep SD card size between 1-8 GB for best compatibility
- Insert SD card with contacts facing the board

---

### 3. Push Buttons

#### UP Button (Navigation/Select)

| Button Terminal | Connection | Wire Color (suggested) |
|-----------------|------------|------------------------|
| Terminal 1 | GPIO 4 | Green |
| Terminal 2 | GND | Black |

**Optional Pull-up Resistor (if needed):**
- 10kΩ resistor between GPIO 4 and 3.3V
- ESP32 has internal pull-ups, so external resistor is **optional**

---

#### DOWN Button (Navigation/Back)

| Button Terminal | Connection | Wire Color (suggested) |
|-----------------|------------|------------------------|
| Terminal 1 | GPIO 0 | Orange |
| Terminal 2 | GND | Black |

**Optional Pull-up Resistor (if needed):**
- 10kΩ resistor between GPIO 0 and 3.3V
- Internal pull-ups are used in code

**Important:** GPIO 0 is the boot mode pin. When using it as a button:
- Internal pull-up keeps it HIGH during normal operation
- To program ESP32-CAM: Press DOWN button (GPIO 0 to GND), then press RESET
- After programming, release DOWN button for normal operation

---

### 4. Power Supply

#### Battery Setup (18650 Li-ion with TP4056)

| TP4056 Module | Connection |
|---------------|------------|
| OUT+ | ESP32-CAM 5V |
| OUT- | ESP32-CAM GND |
| B+ | Battery Positive |
| B- | Battery Negative |
| IN+ | Micro USB 5V (charging) |
| IN- | Micro USB GND |

**Notes:**
- Use **1A or 2A TP4056 module** with protection
- 18650 battery: 3.7V nominal, 2000-3500mAh
- Connect 470µF capacitor between 5V and GND on ESP32-CAM side

---

#### USB Power (Alternative/Development)

| FTDI/USB-Serial | ESP32-CAM Pin |
|-----------------|---------------|
| 5V | 5V |
| GND | GND |
| TX | U0R (RX) |
| RX | U0T (TX) |

---

### 5. Voltage Stabilization Capacitor

| Capacitor (470µF/16V) | Connection |
|-----------------------|------------|
| Positive (+) leg | 5V pin |
| Negative (-) leg | GND pin |

**Notes:**
- **Important** for stable operation during Wi-Fi transmission
- Place capacitor physically close to ESP32-CAM power pins
- Polarity matters! Wrong connection can damage the capacitor

---

## 🔧 Assembly Steps

### Step 1: Prepare ESP32-CAM
1. Insert microSD card into the slot (formatted as FAT32)
2. Keep GPIO 0 disconnected (or HIGH) for normal operation

### Step 2: Connect OLED Display
1. Connect VCC to 3.3V (Red wire)
2. Connect GND to GND (Black wire)
3. Connect SCL to GPIO 12 (Yellow wire)
4. Connect SDA to GPIO 13 (Blue wire)
5. Verify I2C address is 0x3C (default for most SSD1306 displays)

### Step 3: Wire Buttons
1. Connect UP button between GPIO 4 and GND
2. Connect DOWN button between GPIO 33 and GND
3. Buttons are active-LOW (pressed = LOW, released = HIGH with pull-up)

### Step 4: Add Power Capacitor
1. Identify polarity (long leg = positive, short leg = negative)
2. Connect positive to 5V rail
3. Connect negative to GND rail
4. Ensure tight connections

### Step 5: Power Connection
1. For battery: Connect TP4056 OUT+ to 5V, OUT- to GND
2. For USB: Connect USB 5V to ESP32-CAM 5V, GND to GND
3. Double-check voltage before powering on

---

## 🖼️ Visual Wiring Guide

### Breadboard Layout (Development/Testing)

```
                    ┌──────────────┐
                    │  ESP32-CAM   │
                    │              │
   [OLED]           │              │         [Buttons]
    ┌────┐          │              │
    │VCC├──────3.3V─┤              │
    │GND├──────GND──┤              ├──GND──┬──[UP]
    │SCL├──────GP12─┤              │       │
    │SDA├──────GP13─┤              ├──GP4──┘
    └────┘          │              │
                    │              ├──GND──┬──[DOWN]
     [Capacitor]    │              │       │
        470µF       │              ├──GP0──┘
         (+)────5V──┤              │
         (-)───GND──┤              │
                    │              │
   [TP4056/USB]     │              │
    5V─────────5V───┤              │
    GND────────GND──┤              │
                    └──────────────┘
                    
   * Both buttons connect to common GND
   * Internal pull-ups enabled in ESP32
```

---

## ⚡ Power Consumption

| Mode | Current Draw | Notes |
|------|------------|-------|
| Idle (reading) | ~80-120 mA | OLED on, SD idle |
| Wi-Fi Active | ~150-250 mA | Portal mode, file upload |
| Peak (transmit) | ~300-400 mA | Brief spikes during Wi-Fi |

**Battery Life Estimate:**
- 2500mAh 18650 battery
- Average 100mA (reading mostly)
- Runtime: ~20-25 hours continuous reading

---

## 🔍 Testing Checklist

- [ ] OLED displays correctly (I2C scanner test)
- [ ] SD card detected (serial monitor check)
- [ ] Both buttons respond (press and hold tests)
- [ ] No power fluctuations (multimeter check)
- [ ] Wi-Fi portal accessible
- [ ] File upload works
- [ ] Text display readable

---

## ⚠️ Important Safety Notes

1. **Polarity Matters:**
   - Always check battery polarity before connecting
   - Capacitor must be connected correctly (+ to 5V, - to GND)

2. **Voltage Levels:**
   - OLED uses 3.3V (some 5V tolerant, but use 3.3V to be safe)
   - ESP32-CAM can be powered by 5V through built-in regulator
   - Never connect LiPo battery directly to 5V pin without regulator

3. **Programming Mode:**
   - To upload code: Press DOWN button (pulls GPIO 0 to GND), then press RESET
   - After upload completes: Release DOWN button for normal operation
   - During normal use: Internal pull-up keeps GPIO 0 HIGH

4. **Heat Management:**
   - ESP32-CAM can get warm during Wi-Fi operation
   - Ensure adequate ventilation
   - Don't cover module completely in enclosed spaces

5. **SD Card:**
   - Always power off before inserting/removing SD card
   - Use quality SD cards (avoid cheap/fake cards)

---

## 📸 Connection Photos Reference

For clear visual reference, take photos of your completed wiring from multiple angles:
- Top view showing all connections
- Close-up of OLED wiring
- Button connections
- Power section with capacitor

---

## 🔧 Troubleshooting Connections

| Problem | Check |
|---------|-------|
| OLED blank | Verify 3.3V power, check SDA/SCL swap, I2C address |
| SD error | Ensure proper insertion, FAT32 format, check 3.3V |
| Button not working | Test with multimeter, check GND connection |
| Won't boot | Check 5V power, ensure GPIO 0 is HIGH |
| Random resets | Add/check capacitor, verify power supply current |
| Wi-Fi won't start | Insufficient power, check battery/capacitor |

---

**Circuit Designed by:** Sakshyam Bastakoti  
**Last Updated:** February 2026

---

