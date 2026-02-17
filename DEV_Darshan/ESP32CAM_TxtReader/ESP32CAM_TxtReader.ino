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
 *    - U8g2  (by oliver)
 *
 *  BOARD SETTINGS in Arduino IDE:
 *    Board → AI Thinker ESP32-CAM
 *    Partition Scheme → Huge APP (3MB No OTA / 1MB SPIFFS)
 * ============================================================
 */

#include <Wire.h>
#include <U8g2lib.h>
#include "SD_MMC.h"

// ─── Display ────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32

#define I2C_SDA        13
#define I2C_SCL        12

// SSD1306 128×32, hardware I2C, full framebuffer
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(
    U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* scl=*/ I2C_SCL, /* sda=*/ I2C_SDA);

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

  // OLED init (U8g2 handles I2C setup internally)
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);   // 6-wide fixed-width font

  u8g2.clearBuffer();
  u8g2.drawStr(6, 10, "TXT File Reader");
  u8g2.drawStr(6, 20, "---------------");
  u8g2.drawStr(6, 30, "Init SD card...");
  u8g2.sendBuffer();
  delay(500);

  // SD card init (1-bit mode — uses GPIO 2, 14, 15)
  if (!SD_MMC.begin("/sdcard", true)) {          // true = 1-bit mode
    Serial.println("[SD] Mount FAILED");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "SD Card Error!");
    u8g2.drawStr(0, 20, "Insert card and");
    u8g2.drawStr(0, 30, "press RST to retry");
    u8g2.sendBuffer();
    while (true) delay(1000);
  }
  Serial.printf("[SD] Card size: %llu MB\n", SD_MMC.cardSize() / (1024 * 1024));

  // Scan for .txt files
  scanTxtFiles();

  if (fileCount == 0) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "No .txt files");
    u8g2.drawStr(0, 20, "found on SD card.");
    u8g2.drawStr(0, 30, "Add files & reset.");
    u8g2.sendBuffer();
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
  u8g2.clearBuffer();

  for (int row = 0; row < LINE_COUNT; row++) {
    int idx = listOffset + row;
    if (idx >= fileCount) break;

    int y = (row + 1) * 8;  // U8g2 draws text at baseline

    // build display string: cursor + filename
    String line;
    if (idx == fileCursor) {
      line = "> ";
    } else {
      line = "  ";
    }

    // show only the filename (strip path)
    String name = fileList[idx];
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) name = name.substring(lastSlash + 1);

    // truncate to fit screen (21 chars - 2 for cursor)
    if (name.length() > LINE_CHARS - 2)
      name = name.substring(0, LINE_CHARS - 2);

    line += name;
    u8g2.drawStr(0, y, line.c_str());
  }

  // scroll indicators on top-right if there are more files
  if (fileCount > LINE_COUNT) {
    if (listOffset > 0)
      u8g2.drawStr(122, 8, "^");               // more above
    if (listOffset + LINE_COUNT < fileCount)
      u8g2.drawStr(122, 32, "v");              // more below
  }

  u8g2.sendBuffer();
}

// ═════════════════════════════════════════════════════════════
//  OPEN A FILE AND PREPARE WRAPPED LINES
// ═════════════════════════════════════════════════════════════
void openFile(int index) {
  currentFileName = fileList[index];
  currentFile = SD_MMC.open(currentFileName);
  if (!currentFile) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "Open failed!");
    u8g2.sendBuffer();
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
  u8g2.clearBuffer();

  for (int row = 0; row < LINE_COUNT; row++) {
    int idx = viewOffset + row;
    if (idx >= totalLines) break;

    int y = (row + 1) * 8;  // baseline
    u8g2.drawStr(0, y, textLines[idx].c_str());
  }

  // thin scroll-bar on the right edge
  if (totalLines > LINE_COUNT) {
    int barHeight = max(2, (int)((float)LINE_COUNT / totalLines * SCREEN_HEIGHT));
    int barY = (int)((float)viewOffset / totalLines * SCREEN_HEIGHT);
    u8g2.drawBox(SCREEN_WIDTH - 2, barY, 2, barHeight);
  }

  u8g2.sendBuffer();
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
