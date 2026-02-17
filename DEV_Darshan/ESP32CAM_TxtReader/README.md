# ESP32-CAM TXT File Reader

A minimal **text file reader** using the **ESP32-CAM** board (camera disabled), its **built-in SD card slot**, and a **0.91" OLED display** (128×32 SSD1306).

---

## Hardware Required

| Component | Qty |
|---|---|
| AI-Thinker ESP32-CAM | 1 |
| 0.91" OLED (SSD1306, I2C, 128×32) | 1 |
| Micro-SD card (FAT32 formatted) | 1 |
| Push button (momentary) | 1 (optional — GPIO 16) |
| Jumper wires | 4–6 |

> **GPIO 0** already has a built-in button (BOOT) on most ESP32-CAM boards — that acts as the **NEXT / Scroll** button.

---

## Wiring Diagram

```
  ESP32-CAM              0.91" OLED
  ─────────              ──────────
   3.3V  ──────────────▶  VCC
   GND   ──────────────▶  GND
   GPIO 13 ────────────▶  SDA
   GPIO 12 ────────────▶  SCL


  ESP32-CAM              BUTTON (SELECT / BACK)
  ─────────              ─────────────────────
   GPIO 16 ────┤ ├──── GND
```

```
  ┌──────────────────────────────────┐
  │         ESP32-CAM  (top view)    │
  │                                  │
  │  [SD Card Slot on back]          │
  │                                  │
  │  3V3  ●──────────── OLED VCC     │
  │  GND  ●──────────── OLED GND     │
  │  IO13 ●──────────── OLED SDA     │
  │  IO12 ●──────────── OLED SCL     │
  │                                  │
  │  IO16 ●──┤BTN├───── GND          │
  │  IO0  ● (built-in BOOT button)   │
  │                                  │
  └──────────────────────────────────┘
```

---

## How It Works

1. **Boot** → SD card is mounted, all `.txt` files are discovered (recursively).
2. **File List** → File names are shown on the OLED.
   - Press **NEXT** (GPIO 0 / BOOT button) to scroll through files.
   - Press **SELECT** (GPIO 16 button) to open the highlighted file.
3. **Reader View** → The file content is word-wrapped and displayed.
   - Press **NEXT** to scroll down line by line.
   - Press **SELECT** to go back to the file list.
4. A **scroll bar** on the right edge shows your position in the file.

---

## Button Map

| Button | File List Screen | Reader Screen |
|---|---|---|
| **NEXT** (GPIO 0) | Move cursor down | Scroll text down |
| **SELECT** (GPIO 16) | Open selected file | Go back to file list |

---

## Arduino IDE Setup

### 1. Board Manager
- Install **ESP32** board support (by Espressif Systems) via *Boards Manager*.
- Select board: **AI Thinker ESP32-CAM**

### 2. Libraries (install via Library Manager)
- **U8g2** (by oliver)

### 3. Board Settings
| Setting | Value |
|---|---|
| Board | AI Thinker ESP32-CAM |
| Upload Speed | 115200 |
| Flash Frequency | 80 MHz |
| Partition Scheme | Huge APP (3MB No OTA / 1MB SPIFFS) |

### 4. Upload
- Connect via USB-to-Serial adapter (TX→U0R, RX→U0T, GND→GND).
- Hold **GPIO 0 (BOOT)** button while pressing **RST** to enter flash mode.
- Click **Upload** in Arduino IDE.
- After upload, press **RST** to run.

---

## SD Card Preparation

1. Format the micro-SD card as **FAT32**.
2. Copy `.txt` files to the root or any sub-folder.
3. Insert into the ESP32-CAM's SD card slot.
4. Reset the board.

### Example SD card layout:
```
SD Card (FAT32)
├── books/
│   ├── chapter1.txt
│   └── chapter2.txt
├── notes.txt
└── readme.txt
```

---

## Specifications

- **Display**: 128×32 pixels → 21 characters × 4 lines per screen
- **Max files**: 64 `.txt` files detected
- **Max lines**: 512 wrapped display lines per file
- **Word-wrap**: Intelligent break at spaces when possible
- **Scanning**: Recursive (finds `.txt` in sub-folders)

---

## Troubleshooting

| Problem | Fix |
|---|---|
| "SD Card Error!" on boot | Check SD card is FAT32, firmly inserted. Try a different card. |
| OLED stays blank | Verify I2C wiring (SDA→13, SCL→12). Check OLED address is `0x3C`. |
| No files found | Ensure files have `.txt` extension. Check Serial Monitor at 115200 baud for logs. |
| Won't upload | Hold BOOT (GPIO 0) + press RST before uploading. Release BOOT after upload starts. |
| GPIO 12 boot issue | If board won't boot, momentarily ground GPIO 12 during power-on. |

---

## License

MIT — free to use, modify, and share.
