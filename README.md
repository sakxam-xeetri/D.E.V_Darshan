# 📖 DEV_Darshan TXT Reader

A compact, pocket-sized TXT eBook reader built using ESP32-CAM (AI Thinker) with a 0.91" OLED display. Perfect for fitting inside a calculator case, replacing the solar cell with a display!

## 🎯 Features

- **Offline TXT Reading** - Read text files without any internet connection
- **SD Card Storage** - Uses built-in SD_MMC slot (no SPI wiring needed)
- **WiFi Upload Portal** - Long-press SELECT for 5 seconds to enable mobile-friendly file upload
- **Battery Efficient** - WiFi disabled during normal reading
- **Clean UI** - Minimalist 4-line text display with word wrapping
- **File Browser** - Easy navigation through TXT files on SD card

## 🔧 Hardware Requirements

| Component | Specification |
|-----------|---------------|
| MCU | ESP32-CAM (AI Thinker) |
| Display | 0.91" SSD1306 OLED (128x32, I2C) |
| Storage | MicroSD Card (FAT32) |
| Buttons | 3x Tactile Push Buttons |
| Power | 3.3V / LiPo Battery |

## 📌 Wiring Diagram (Zero Resistors!)

### OLED Display (Software I2C - 4 Wires)
```
OLED Pin    →    ESP32-CAM Pin
────────────────────────────────
VCC         →    3.3V
GND         →    GND
SDA         →    GPIO 13
SCL         →    GPIO 0
```

### Buttons (2 Wires Each - Direct to GND)
```
Button      →    ESP32 Pin         →    Other Leg
──────────────────────────────────────────────────
UP          →    GPIO 12            →    GND
DOWN        →    GPIO 3  (RX pin)   →    GND
SELECT      →    GPIO 4  (flash)    →    GND
```

### Serial Debug (always active)
```
GPIO 1 (TX) stays as Serial TX output — permanent debug via serial monitor
```

### Visual Wiring Diagram
```
                    ┌─────────────────────────┐
                    │      ESP32-CAM          │
                    │      (AI Thinker)       │
                    │                         │
        OLED SDA ───┤ GPIO 13          3.3V ├─── OLED VCC
        OLED SCL ───┤ GPIO 0           GND  ├─── OLED GND
                    │                         │
      BTN_UP ───────┤ GPIO 12 (VDD_SDIO)     │
                    │                         │
    BTN_DOWN ───────┤ GPIO 3  (RX, freed)    │
                    │                         │
  BTN_SELECT ───────┤ GPIO 4  (flash LED)    │
                    │                         │
  Serial TX ────────┤ GPIO 1  (permanent)    │
                    │                         │
                    │  ┌─────────┐            │
                    │  │ SD CARD │            │
                    │  │ (Built  │            │
                    │  │   in)   │            │
                    │  └─────────┘            │
                    └─────────────────────────┘
                         │
                        GND ─── All button other legs
```

## ⚠️ Important Notes

### Pin Assignment Rationale (every pin verified safe)
- **GPIO 2, 14, 15** → Reserved for **SD_MMC** (DATA0, CLK, CMD). Never share!
- **GPIO 16** → Reserved for **PSRAM** on AI Thinker. Never use!
- **GPIO 13** (OLED SDA) → Safest pin on the board. No strapping, no onboard hardware.
- **GPIO 0** (OLED SCL) → Onboard 10K pull-up + OLED module pull-up keep it HIGH at boot → **normal boot guaranteed**. Perfect for I2C clock.
- **GPIO 12** (BTN_UP) → VDD_SDIO strapping pin. Button-to-GND keeps it LOW at boot → **3.3V flash guaranteed** → SD card works. `INPUT_PULLUP` enabled only after strapping latches.
- **GPIO 3** (BTN_DOWN) → RX pin. Freed by TX-only serial config (`Serial.begin(115200, SERIAL_8N1, -1, 1)`). Input during boot → button press harmless.
- **GPIO 4** (BTN_SELECT) → Flash LED pin. Held OUTPUT LOW (LED off). Momentarily pulsed INPUT_PULLUP for 20µs to read button — **LED completely invisible, zero power waste**.
- **GPIO 1** (Serial TX) → Stays as UART TX permanently. **Full runtime serial debug!** Never used as a button (avoids short-circuit risk during boot).

### SD Card
- Uses **SD_MMC 1-bit mode** (not SPI)
- No additional wiring needed — uses internal ESP32-CAM slot
- Format SD card as **FAT32**
- Place `.txt` files in **root directory**

### Serial Debugging
- Serial monitor works **permanently** at 115200 baud (TX-only on GPIO 1)
- Full runtime debug output — file opens, page turns, WiFi events, uploads all logged
- RX is disabled to free GPIO 3 for the DOWN button
- **Disconnect BTN_DOWN (GPIO 3) when uploading firmware** via serial programmer

## 📲 Installation

### Required Libraries
Install via Arduino IDE Library Manager:
1. **U8g2** by oliver (search "U8g2" in Library Manager)

### Board Settings
```
Board:        "AI Thinker ESP32-CAM"
Flash Mode:   "QIO"
Flash Size:   "4MB (32Mb)"
Partition:    "Huge APP (3MB No OTA/1MB SPIFFS)"
Upload Speed: "115200"
```

### Upload Process
1. **Disconnect BTN_DOWN from GPIO 3 (RX)** before uploading
2. Connect ESP32-CAM to FTDI/USB-Serial adapter
3. Disconnect OLED SCL from GPIO 0 (or use a jumper to pull GPIO 0 to GND)
4. Connect GPIO 0 to GND (enter download/boot mode)
5. Press RST/EN button
6. Click Upload in Arduino IDE
7. After upload, disconnect GPIO 0 from GND
8. Reconnect OLED and button wires
9. Press RST to boot normally

> **Note:** GPIO 1 (TX) stays connected during upload — it's the serial TX line.
> Only GPIO 3 (RX) and GPIO 0 need temporary disconnection.

## 🎮 Controls

### File Browser Mode
| Button | Action |
|--------|--------|
| UP | Navigate up in file list |
| DOWN | Navigate down in file list |
| SELECT (short) | Open selected file |
| SELECT (5 sec hold) | Enable WiFi Upload Portal |

### Reading Mode
| Button | Action |
|--------|--------|
| UP | Previous page |
| DOWN | Next page |
| SELECT (short) | Return to file browser |

### WiFi Portal Mode
| Button | Action |
|--------|--------|
| SELECT | Exit WiFi mode |

## 📡 WiFi Upload Portal

### Activation
1. Hold SELECT button for 5 seconds
2. Display shows "WiFi: TXT_Reader"

### Connection
```
SSID:     TXT_Reader
Password: devdarshan123
IP:       192.168.4.1
```

### Features
- Mobile-responsive design
- Drag & drop file upload
- SD card storage display
- File deletion
- Only accepts .txt files

### Usage
1. Connect phone/laptop to "TXT_Reader" WiFi
2. Open browser to `192.168.4.1`
3. Upload your .txt files
4. Press SELECT on device to exit WiFi mode
5. New files appear in file browser

## 📁 File Requirements

- **Format:** Plain text (.txt)
- **Encoding:** UTF-8 or ASCII
- **Location:** Root of SD card
- **Names:** Avoid special characters

## 🔋 Power Considerations

- Use 3.3V regulated supply or LiPo with regulator
- WiFi significantly increases power consumption
- WiFi is completely disabled during normal reading
- Consider deep sleep modification for extended battery life

## 📊 Display Layout

### File Browser (128x32)
```
┌──────────────────────────┐
│> file1.txt               │
│  file2.txt               │
│  file3.txt               │
│  file4.txt              █│
└──────────────────────────┘
      ↑ Current selection
```

### Reading View (128x32)
```
┌──────────────────────────┐
│This is line one of      │
│the text file content    │
│that wraps automatically █│
│to fit the display.      │
└──────────────────────────┘
                ↑ Progress bar
```

## 🛠️ Customization

### Change WiFi Credentials
Edit these lines in the code:
```cpp
#define AP_SSID          "TXT_Reader"
#define AP_PASSWORD      "devdarshan123"
```

### Adjust Text Display
```cpp
#define CHARS_PER_LINE   21    // Characters per line
#define LINES_PER_PAGE   4     // Lines visible at once
```

### Modify Button Timing
```cpp
#define DEBOUNCE_DELAY   50      // Button debounce (ms)
#define LONG_PRESS_TIME  5000    // WiFi activation hold time (ms)
```

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| SD Card Error | Check card is FAT32, properly inserted |
| OLED blank | Verify SDA→GPIO13, SCL→GPIO0, I2C addr 0x3C |
| Upload fails | Disconnect GPIO 3 button, connect GPIO 0 to GND |
| No files shown | Ensure .txt files are in root, not folders |
| WiFi won't start | Check 5-second hold on SELECT (GPIO 4) |
| SD fails in WiFi | Ensure GPIO 2, 14, 15 have no other wiring |
| Boot enters download | OLED must be connected (pull-ups keep GPIO 0 HIGH) |
| VDD_SDIO 1.8V / SD fail | GPIO 12 must be LOW at boot — button-to-GND ensures this |
| Flash LED always on | GPIO 4 must be OUTPUT LOW — check code is v2.0 |
| Buttons unresponsive | Check all buttons wire to GND |
| Serial not showing | Connect to GPIO 1 (TX) at 115200 baud |

## 📝 License

This project is open source. Feel free to modify and improve!

## 👤 Author

**DEV_Darshan**

---

*A pocket-sized reading companion that fits anywhere, needs no internet, and puts your text files at your fingertips.*
