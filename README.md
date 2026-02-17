# ESP32-CAM OLED Text File Reader

A simple Arduino sketch that reads text files from an SD card and displays them on a 0.96" OLED display using ESP32-CAM.

## Hardware Requirements

- ESP32-CAM board
- 0.96" OLED Display (SSD1306, I2C interface)
- SD Card (with .txt files)
- Jumper wires

## Wiring Connections

| OLED Pin | ESP32-CAM Pin |
|----------|---------------|
| VCC      | 3.3V          |
| GND      | GND           |
| SDA      | GPIO 12       |
| SCL      | GPIO 13       |

**IMPORTANT:** 
- SD card MUST be in **1-bit mode** (the code handles this automatically)
- 1-bit SD mode uses GPIO 2, 14, 15 - GPIO 12/13 are safe for I2C
- Do NOT use GPIO 14, 15, or 2 for I2C - these are used by SD card
- The code uses software I2C on GPIO 12/13 for compatibility

**Note:** 
- SD card slot is already built into the ESP32-CAM module

## Required Libraries

Install these libraries via Arduino IDE Library Manager:

1. **U8g2** - by oliver

Built-in libraries (already included):
- Wire
- SD_MMC
- FS

## Installation Steps

1. Open Arduino IDE
2. Install ESP32 board support:
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search for "ESP32" and install "esp32 by Espressif Systems"

3. Install required libraries (mentioned above)

4. Select board: Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM

5. Upload the sketch to ESP32-CAM

## Features

- Automatically scans SD card for .txt files
- Displays list of available text files
- Auto-cycles through files displaying content
- Shows up to 5 lines per file on OLED
- Truncates long filenames and lines to fit display

## Usage

**IMPORTANT - SD Card Preparation:**
1. Format SD card as **FAT32** (Windows: right-click drive → Format → FAT32)
2. Use SD card **32GB or smaller** (ESP32-CAM limitation)
3. Create some .txt files and copy them to the **root directory** (not in folders)
4. **Insert SD card BEFORE powering on** the ESP32-CAM
5. Push SD card in firmly until it clicks

**Operation:**
1. Connect OLED display as per wiring diagram
2. Power up the ESP32-CAM
3. The display will show:
   - List of text files found
   - Content of each file (auto-cycling)
4. Check Serial Monitor (115200 baud) for debug info

## Customization

### Add Button Navigation

To add buttons for manual file navigation, you can modify the loop() function:

```cpp
// Define button pins
#define BUTTON_NEXT 13
#define BUTTON_PREV 12

// In setup():
pinMode(BUTTON_NEXT, INPUT_PULLUP);
pinMode(BUTTON_PREV, INPUT_PULLUP);

// In loop():
if(digitalRead(BUTTON_NEXT) == LOW) {
  currentFileIndex++;
  if(currentFileIndex >= fileCount) currentFileIndex = 0;
  delay(200); // debounce
}
```

### Change Display Duration

Modify the delay values in loop():
```cpp
delay(3000);  // Time to show file list (3 seconds)
delay(10000); // Time to show file content (10 seconds)
```

## Troubleshooting

**SD card not detected:**
- **PIN CONFLICT**: Make sure you're using GPIO 12/13 for OLED I2C
- SD card MUST work in **1-bit mode** (code forces this automatically)
- **Most Common**: Ensure SD card is formatted as **FAT32** (not exFAT or NTFS)
- Use SD cards **32GB or smaller** (larger cards may not work)
- Try different SD cards - some brands work better than others
- **Reseat the SD card** - push it in firmly until it clicks
- Make sure contacts are clean
- Insert SD card **before** powering on ESP32-CAM
- Check serial monitor at 115200 baud for detailed error messages

**OLED not displaying:**
- Verify wiring: SDA to GPIO 12, SCL to GPIO 13
- Code uses software I2C for better compatibility
- Check I2C address (most SSD1306 displays use 0x3C)
- Ensure 3.3V power (not 5V)
- Check serial monitor - should say "OLED display initialized OK"

**No files shown:**
- Ensure .txt files are in **root directory** of SD card
- Check file extensions (.txt or .TXT)
- Maximum 20 files supported in this version
- Files must have proper text encoding (ASCII or UTF-8)

## Serial Monitor Output

Open Serial Monitor at 115200 baud to see:
- Initialization status
- List of detected text files
- Current file being displayed
- Error messages if any

## License

Free to use and modify for personal and commercial projects.
