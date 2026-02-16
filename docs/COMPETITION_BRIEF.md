# DEV_Darshan — Competition Presentation Brief

---

## Project Title
**DEV_Darshan: Ultra-Compact Pocket TXT eBook Reader**

## One-Line Pitch
> A pocket-sized, battery-powered text file reader built inside a calculator shell using an ESP32 microcontroller, with wireless file upload capability.

---

## Problem Statement

Students and professionals often need to reference notes, formulas, or study material discreetly and portably. Existing solutions — phones, tablets, e-readers — are bulky, distracting, and expensive. There is no ultra-compact, single-purpose text reader that fits in a pocket or pencil case.

## Solution

**DEV_Darshan** repurposes the ESP32-CAM module (without its camera) as the brain of a minimalist `.txt` file reader:

- **Fits inside a calculator case** — the solar cell cutout becomes the display window
- **0.91" OLED display** shows 4 lines of text with word-wrapped scrolling
- **SD card storage** holds thousands of text files
- **WiFi upload portal** — transfer files from any phone without cables or apps
- **16+ hours battery life** on a single charge

---

## Technical Highlights

### Hardware Innovation
- **ESP32-CAM repurposed** — exploits the built-in SD card slot while disabling the unused camera, saving cost and board space
- **Boot-safe GPIO mapping** — carefully avoids ESP32 strapping pin conflicts that cause most hobbyist projects to fail
- **Magnetic reed switch** as power toggle — zero quiescent current when OFF, no moving parts to wear out
- **Single 470 µF capacitor** prevents brownout resets during WiFi bursts

### Software Engineering
- **Modular firmware architecture** — 6 independent modules with clean interfaces
- **Memory-efficient reader** — streams text line-by-line from SD; never loads full file into RAM
- **Word-wrap engine** — dynamically wraps text to 21-character display width with proper word boundaries
- **Smart power management** — WiFi and Bluetooth disabled by default; CPU downclocked to 80 MHz; radios enabled only on demand

### User Experience
- **3-button navigation** — intuitive UP/DOWN/SELECT with long-press for fast scroll
- **Mobile-responsive upload page** — clean, modern web UI with drag-and-drop file upload
- **Instant boot** — from power-on to file list in under 2 seconds
- **Error resilience** — SD retry logic, graceful error screens, recovery options

---

## Specifications

| Parameter | Value |
|-----------|-------|
| Processor | ESP32 dual-core @ 80 MHz (downclocked for power saving) |
| Display | 128×32 px OLED, I2C, 0.91" diagonal |
| Storage | MicroSD (FAT32), limited only by card size |
| Battery | 3.7V 800mAh Li-ion polymer |
| Charging | USB-C via TP4056 module with overcharge/overdischarge protection |
| Dimensions | ~85 × 55 × 13 mm (calculator form factor) |
| Weight | ~45g with battery |
| Read time | ~16 hours continuous |
| File format | Plain text (.txt), UTF-8 |

---

## Architecture Diagram

```
┌──────────────────────────────────────────────┐
│                 USER                         │
│          UP  DOWN  SELECT                    │
└──────────┬────┬────┬─────────────────────────┘
           │    │    │
    ┌──────▼────▼────▼──────┐
    │    INPUT MANAGER      │  debounce + long-press + repeat
    └───────────┬───────────┘
                │ events
    ┌───────────▼───────────┐
    │    MAIN CONTROLLER    │  state machine
    │  (MENU│READ│WIFI│ERR) │
    └──┬────┬────┬────┬─────┘
       │    │    │    │
  ┌────▼┐ ┌─▼──┐│ ┌──▼────┐
  │DISPL│ │ SD ││ │ WIFI  │
  │OLED │ │CARD││ │PORTAL │
  └─────┘ └────┘│ └───────┘
          ┌─────▼───┐
          │  POWER  │
          │MANAGER  │
          └─────────┘
```

---

## Cost Analysis

| Component | Approx. Cost (INR) |
|-----------|-------------------|
| ESP32-CAM (AI Thinker) | ₹350 |
| 0.91" SSD1306 OLED | ₹120 |
| TP4056 USB-C module | ₹30 |
| 800mAh Li-ion battery | ₹150 |
| 470µF capacitor | ₹5 |
| Reed switch + magnet | ₹20 |
| 3× tactile buttons | ₹10 |
| MicroSD card (4GB) | ₹100 |
| Wires + misc | ₹20 |
| **Total** | **~₹805** |

**✅ No external resistors needed!** All pull-ups/downs are built into the modules or ESP32 internal.

---

## What Makes This Project Stand Out

1. **Resourceful design** — repurposes a camera module board for a non-camera application, exploiting its built-in SD slot
2. **Production-quality firmware** — not a prototype sketch; modular, documented, error-handled code
3. **Real-world constraints solved** — boot safety, brownout prevention, memory management, power optimization
4. **Complete system** — hardware + firmware + documentation + upgrade path
5. **Ultra-low cost** — under ₹1000 for a complete device

---

## Live Demo Flow

1. Power ON → splash screen → file list appears (~2 sec)
2. Navigate to a file → SELECT to open → scroll through text
3. Long-press SELECT → WiFi AP activates → show on phone
4. Upload a new `.txt` file from phone → appears in file list
5. Exit WiFi → back to reading → show battery-efficient operation

---

*DEV_Darshan — Read anywhere. Built with engineering precision.*
