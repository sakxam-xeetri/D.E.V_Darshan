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

### OLED Display (I2C - 4 Wires)
```
OLED Pin    →    ESP32-CAM Pin
────────────────────────────────
VCC         →    3.3V
GND         →    GND
SDA         →    GPIO 13
SCL         →    GPIO 14
```

### Buttons (2 Wires Each - Direct to GND)
```
Button      →    ESP32 Pin    →    Other Leg
──────────────────────────────────────────────
UP          →    GPIO 12      →    GND
DOWN        →    GPIO 15      →    GND
SELECT      →    GPIO 2       →    GND
```

### Visual Wiring Diagram
```
                    ┌─────────────────────────┐
                    │      ESP32-CAM          │
                    │      (AI Thinker)       │
                    │                         │
        OLED SDA ───┤ GPIO 13          3.3V ├─── OLED VCC
        OLED SCL ───┤ GPIO 14          GND  ├─── OLED GND
                    │                         │
      BTN_UP ───────┤ GPIO 12                │
                    │                         │
    BTN_DOWN ───────┤ GPIO 15                │
                    │                         │
  BTN_SELECT ───────┤ GPIO 2 (Built-in LED)  │
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

### GPIO 2 (Built-in LED)
- GPIO 2 is used for SELECT button
- It has a built-in LED that may blink when button is pressed
- This is normal and doesn't affect functionality

### SD Card
- Uses **SD_MMC 1-bit mode** (not SPI)
- No additional wiring needed - uses internal ESP32-CAM slot
- Format SD card as **FAT32**
- Place `.txt` files in **root directory**

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
1. Connect ESP32-CAM to FTDI/USB-Serial adapter
2. Connect GPIO 0 to GND (boot mode)
3. Press RST/EN button
4. Click Upload in Arduino IDE
5. After upload, disconnect GPIO 0 from GND
6. Press RST to boot normally

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
| OLED blank | Verify I2C address (0x3C or 0x3D) |
| Upload fails | Connect GPIO 0 to GND during upload |
| No files shown | Ensure .txt files are in root, not folders |
| WiFi won't start | Check 5-second hold timing |
| Buttons unresponsive | Check connections to GND |
| Boot loop/resets | Don't use GPIO 1/3 (serial pins) |

## 📝 License

This project is open source. Feel free to modify and improve!

## 👤 Author

**DEV_Darshan**

---

*A pocket-sized reading companion that fits anywhere, needs no internet, and puts your text files at your fingertips.*
