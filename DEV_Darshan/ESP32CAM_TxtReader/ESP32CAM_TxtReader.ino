/*
 * ============================================================
 *  ESP32-CAM  ·  SD Card  ·  0.91" OLED  ·  TXT File Reader
 * ============================================================
 *  Board  : AI-Thinker ESP32-CAM  (camera NOT used)
 *  Display: 0.91" 128×32 SSD1306 I2C OLED
 *  Storage: On-board micro-SD slot (SD_MMC 1-bit mode)
 *
 *  WIRING (only 4 wires for the OLED):
 *    OLED VCC  →  3.3 V
 *    OLED GND  →  GND
 *    OLED SDA  →  GPIO 13
 *    OLED SCL  →  GPIO 12
 *
 *  BUTTONS (directly to GND, internal pull-ups used):
 *    BTN_NEXT   →  GPIO 0   (built-in BOOT button on most boards)
 *    BTN_SELECT →  GPIO 16
 *
 *  LIBRARIES (install via Arduino Library Manager):
 *    - Adafruit SSD1306
 *    - Adafruit GFX Library
 *
 *  BOARD SETTINGS in Arduino IDE:
 *    Board → AI Thinker ESP32-CAM
 *    Partition Scheme → Huge APP (3MB No OTA / 1MB SPIFFS)
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SD_MMC.h"

// ─── Display ────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32
#define OLED_RESET     -1          // no reset pin
#define OLED_ADDR      0x3C        // typical address for 0.91"

#define I2C_SDA        13
#define I2C_SCL        12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &Wire, OLED_RESET);

// ─── Buttons ────────────────────────────────────────────────
#define BTN_NEXT    0   // scroll / navigate down
#define BTN_SELECT 16   // select file / go back

// debounce
#define DEBOUNCE_MS 200
unsigned long lastBtnNext   = 0;
unsigned long lastBtnSelect = 0;

// ─── Text layout constants ──────────────────────────────────
//  With the default 6×8 font the 128×32 screen fits:
//    COLS = 128 / 6 = 21 characters   ROWS = 32 / 8 = 4 lines
#define LINE_CHARS  21
#define LINE_COUNT   4

// ─── Application state ──────────────────────────────────────
enum AppState { STATE_FILE_LIST, STATE_READING };
AppState appState = STATE_FILE_LIST;

// File browser
#define MAX_FILES 64
String fileList[MAX_FILES];
int    fileCount   = 0;
int    fileCursor  = 0;      // highlighted file index
int    listOffset  = 0;      // scroll offset for file list

// Text reader
String currentFileName;
File   currentFile;
#define MAX_LINES 512
String textLines[MAX_LINES]; // wrapped lines of current file
int    totalLines  = 0;
int    viewOffset  = 0;      // first visible line

// ─── Forward declarations ───────────────────────────────────
void scanTxtFiles();
void drawFileList();
void openFile(int index);
void drawReader();
void wrapFileIntoLines(File &f);
bool btnPressed(int pin, unsigned long &lastTime);

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println("\n[TXT Reader] Booting...");

  // Buttons
  pinMode(BTN_NEXT,   INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  // I2C on custom pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] Init FAILED");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  TXT File Reader");
  display.println("  ---------------");
  display.println("  Init SD card...");
  display.display();
  delay(500);

  // SD card init (1-bit mode — uses GPIO 2, 14, 15)
  if (!SD_MMC.begin("/sdcard", true)) {          // true = 1-bit mode
    Serial.println("[SD] Mount FAILED");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" SD Card Error!");
    display.println(" Insert card and");
    display.println(" press RST to retry");
    display.display();
    while (true) delay(1000);
  }
  Serial.printf("[SD] Card size: %llu MB\n", SD_MMC.cardSize() / (1024 * 1024));

  // Scan for .txt files
  scanTxtFiles();

  if (fileCount == 0) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" No .txt files");
    display.println(" found on SD card.");
    display.println(" Add files & reset.");
    display.display();
    while (true) delay(1000);
  }

  drawFileList();
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════
void loop() {
  switch (appState) {
    // ── File browser ──────────────────────────────────────────
    case STATE_FILE_LIST:
      if (btnPressed(BTN_NEXT, lastBtnNext)) {
        fileCursor++;
        if (fileCursor >= fileCount) fileCursor = 0;   // wrap
        // adjust scroll window
        if (fileCursor >= listOffset + LINE_COUNT)
          listOffset = fileCursor - LINE_COUNT + 1;
        if (fileCursor < listOffset)
          listOffset = fileCursor;
        drawFileList();
      }
      if (btnPressed(BTN_SELECT, lastBtnSelect)) {
        openFile(fileCursor);
      }
      break;

    // ── Text reader ───────────────────────────────────────────
    case STATE_READING:
      if (btnPressed(BTN_NEXT, lastBtnNext)) {
        if (viewOffset + LINE_COUNT < totalLines) {
          viewOffset++;
          drawReader();
        }
      }
      if (btnPressed(BTN_SELECT, lastBtnSelect)) {
        // go back to file list
        appState = STATE_FILE_LIST;
        drawFileList();
      }
      break;
  }

  delay(10);   // keep loop responsive without hammering CPU
}

// ═════════════════════════════════════════════════════════════
//  SCAN SD CARD FOR .TXT FILES (recursive)
// ═════════════════════════════════════════════════════════════
void scanDir(File dir, String path) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (fileCount >= MAX_FILES) break;

    String name = path + "/" + String(entry.name());

    if (entry.isDirectory()) {
      scanDir(entry, name);
    } else {
      // check .txt extension (case-insensitive)
      String lower = name;
      lower.toLowerCase();
      if (lower.endsWith(".txt")) {
        fileList[fileCount++] = name;
        Serial.printf("[SD] Found: %s\n", name.c_str());
      }
    }
    entry.close();
  }
}

void scanTxtFiles() {
  fileCount = 0;
  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("[SD] Failed to open root");
    return;
  }
  scanDir(root, "");
  root.close();
  Serial.printf("[SD] Total .txt files: %d\n", fileCount);
}

// ═════════════════════════════════════════════════════════════
//  DRAW FILE LIST SCREEN
// ═════════════════════════════════════════════════════════════
void drawFileList() {
  display.clearDisplay();
  for (int row = 0; row < LINE_COUNT; row++) {
    int idx = listOffset + row;
    if (idx >= fileCount) break;

    display.setCursor(0, row * 8);

    // show cursor arrow
    if (idx == fileCursor) {
      display.print("> ");
    } else {
      display.print("  ");
    }

    // show only the filename (strip path)
    String name = fileList[idx];
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) name = name.substring(lastSlash + 1);

    // truncate to fit screen (21 chars - 2 for cursor)
    if (name.length() > LINE_CHARS - 2)
      name = name.substring(0, LINE_CHARS - 2);

    display.print(name);
  }

  // scroll indicator on top-right if there are more files
  if (fileCount > LINE_COUNT) {
    display.setCursor(120, 0);
    if (listOffset > 0) display.print("^");              // more above
    display.setCursor(120, 24);
    if (listOffset + LINE_COUNT < fileCount) display.print("v");  // more below
  }

  display.display();
}

// ═════════════════════════════════════════════════════════════
//  OPEN A FILE AND PREPARE WRAPPED LINES
// ═════════════════════════════════════════════════════════════
void openFile(int index) {
  currentFileName = fileList[index];
  currentFile = SD_MMC.open(currentFileName);
  if (!currentFile) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Open failed!");
    display.display();
    delay(1500);
    return;
  }

  Serial.printf("[OPEN] %s  (%d bytes)\n",
                currentFileName.c_str(), currentFile.size());

  wrapFileIntoLines(currentFile);
  currentFile.close();

  viewOffset = 0;
  appState = STATE_READING;
  drawReader();
}

// ═════════════════════════════════════════════════════════════
//  WORD-WRAP FILE CONTENT INTO DISPLAY LINES
// ═════════════════════════════════════════════════════════════
void wrapFileIntoLines(File &f) {
  totalLines = 0;
  String line = "";

  while (f.available() && totalLines < MAX_LINES) {
    char c = (char)f.read();

    if (c == '\r') continue;                 // ignore CR

    if (c == '\n') {
      // newline → push whatever we have (even if empty = blank line)
      textLines[totalLines++] = line;
      line = "";
      continue;
    }

    line += c;

    // hard-wrap at screen width
    if ((int)line.length() >= LINE_CHARS) {
      // try to break at last space for nicer wrapping
      int brk = line.lastIndexOf(' ');
      if (brk > LINE_CHARS / 2) {
        textLines[totalLines++] = line.substring(0, brk);
        line = line.substring(brk + 1);
      } else {
        textLines[totalLines++] = line;
        line = "";
      }
    }
  }
  // remaining text
  if (line.length() > 0 && totalLines < MAX_LINES) {
    textLines[totalLines++] = line;
  }

  Serial.printf("[WRAP] %d display lines\n", totalLines);
}

// ═════════════════════════════════════════════════════════════
//  DRAW TEXT READER SCREEN
// ═════════════════════════════════════════════════════════════
void drawReader() {
  display.clearDisplay();

  for (int row = 0; row < LINE_COUNT; row++) {
    int idx = viewOffset + row;
    if (idx >= totalLines) break;

    display.setCursor(0, row * 8);
    display.print(textLines[idx]);
  }

  // thin scroll-bar on the right edge
  if (totalLines > LINE_COUNT) {
    int barHeight = max(2, (int)((float)LINE_COUNT / totalLines * SCREEN_HEIGHT));
    int barY = (int)((float)viewOffset / totalLines * SCREEN_HEIGHT);
    display.fillRect(SCREEN_WIDTH - 2, barY, 2, barHeight, SSD1306_WHITE);
  }

  display.display();
}

// ═════════════════════════════════════════════════════════════
//  BUTTON HELPER  (active-low, with debounce)
// ═════════════════════════════════════════════════════════════
bool btnPressed(int pin, unsigned long &lastTime) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastTime > DEBOUNCE_MS) {
      lastTime = millis();
      return true;
    }
  }
  return false;
}
