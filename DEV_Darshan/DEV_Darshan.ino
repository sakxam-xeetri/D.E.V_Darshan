/*
 * ============================================================
 *  D.E.V_Darshan — ESP32-CAM Mini TXT Reader
 *  Developer : Sakshyam Bastakoti — IOT & Robotics Developer
 *  Board     : AI Thinker ESP32-CAM
 *  Display   : 0.96" SSD1306 OLED 128x64 (I2C via U8g2)
 *  Storage   : Built-in SD_MMC (1-bit mode)
 * ============================================================
 *
 *  Features:
 *   - Offline TXT reading from SD card
 *   - Wi-Fi AP portal to upload .txt files
 *   - Two-button interface (UP / DOWN)
 *   - Word-wrapped text display
 *
 *  Libraries required:
 *   - U8g2        (install via Library Manager)
 *   - SD_MMC      (built-in with ESP32 core)
 *   - WiFi        (built-in with ESP32 core)
 *   - WebServer   (built-in with ESP32 core)
 * ============================================================
 */

#include <U8g2lib.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <WebServer.h>

// ─── Pin Definitions ────────────────────────────────────────
#define SDA_PIN   12
#define SCL_PIN   14
#define BTN_UP    13
#define BTN_DOWN  15

// ─── Display Setup (Software I2C) ──────────────────────────
// U8g2 constructor: SW I2C, SDA=GPIO12, SCL=GPIO14
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0,        // rotation
  /* clock=*/ SCL_PIN,
  /* data=*/  SDA_PIN,
  /* reset=*/ U8X8_PIN_NONE
);

// ─── Constants ──────────────────────────────────────────────
static const int  SCREEN_WIDTH      = 128;
static const int  SCREEN_HEIGHT     = 64;
static const int  CHAR_WIDTH        = 6;     // u8g2 font 6x10
static const int  LINE_HEIGHT       = 11;    // pixel height per text line
static const int  MAX_CHARS_PER_LINE = 21;   // 128 / 6 = 21
static const int  MAX_VISIBLE_LINES = 5;     // 64 / 11 ≈ 5 lines (full OLED)
static const unsigned long LONG_PRESS_MS = 2000;
static const int  MAX_FILES         = 50;

// ─── Application Modes ──────────────────────────────────────
enum Mode {
  HOME_MODE,
  READ_MODE,
  PORTAL_MODE
};

Mode currentMode = HOME_MODE;

// ─── File List ──────────────────────────────────────────────
String fileList[MAX_FILES];
int    fileCount      = 0;
int    selectedIndex  = 0;   // cursor in file list
int    scrollOffset   = 0;   // first visible item in home list

// ─── Text Reader State ─────────────────────────────────────
String wrappedLines[2000];   // wrapped lines buffer
int    totalWrappedLines = 0;
int    readScrollIndex   = 0; // first visible line in reader

// ─── Button State ───────────────────────────────────────────
bool   upPressed        = false;
bool   downPressed      = false;
unsigned long upPressTime   = 0;
unsigned long downPressTime = 0;
bool   upHandled        = false;
bool   downHandled      = false;

// ─── Wi-Fi Portal ───────────────────────────────────────────
const char* AP_SSID = "D.E.V AP";
const char* AP_PASS = "Darshan";
WebServer server(80);
bool portalActive = false;

// ─── Forward Declarations ───────────────────────────────────
void scanFiles();
void drawHome();
void openFile(const String &filename);
void drawReader();
void handleButtons();
void startPortal();
void stopPortal();
void handlePortalRoot();
void handleFileUpload();
void wrapText(const String &text);
void showMessage(const String &msg);

// =============================================================
//  SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[D.E.V_Darshan] Booting...");

  // --- Button pins ---
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  // --- OLED init ---
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);  // 6-wide, 10-tall ASCII font
  showMessage("D.E.V_Darshan\nStarting...");
  delay(1000);

  // --- SD Card init (1-bit mode) ---
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("[SD] Mount FAILED");
    showMessage("SD Card\nMount Failed!");
    while (true) delay(1000);  // halt
  }
  Serial.println("[SD] Mounted OK");

  // --- Scan .txt files ---
  scanFiles();
  drawHome();
}

// =============================================================
//  LOOP
// =============================================================
void loop() {
  handleButtons();

  if (currentMode == PORTAL_MODE && portalActive) {
    server.handleClient();
  }
}

// =============================================================
//  FILE SCANNING
// =============================================================
void scanFiles() {
  fileCount = 0;
  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("[SD] Cannot open root");
    return;
  }

  File entry;
  while ((entry = root.openNextFile()) && fileCount < MAX_FILES) {
    String name = entry.name();
    // SD_MMC may prefix with "/"
    if (name.startsWith("/")) name = name.substring(1);
    // Only .txt files
    if (!entry.isDirectory() && name.endsWith(".txt")) {
      fileList[fileCount++] = name;
      Serial.printf("[SD] Found: %s\n", name.c_str());
    }
    entry.close();
  }
  root.close();

  selectedIndex = 0;
  scrollOffset  = 0;

  if (fileCount == 0) {
    Serial.println("[SD] No .txt files found");
  }
}

// =============================================================
//  HOME SCREEN — File List
// =============================================================
void drawHome() {
  u8g2.clearBuffer();

  if (fileCount == 0) {
    u8g2.drawStr(0, 12, "No .txt files");
    u8g2.drawStr(0, 26, "Hold BOTH btns");
    u8g2.drawStr(0, 40, "to open Wi-Fi");
    u8g2.drawStr(0, 54, "and upload files");
    u8g2.sendBuffer();
    return;
  }

  // Ensure selected item is visible
  if (selectedIndex < scrollOffset) {
    scrollOffset = selectedIndex;
  }
  if (selectedIndex >= scrollOffset + MAX_VISIBLE_LINES) {
    scrollOffset = selectedIndex - MAX_VISIBLE_LINES + 1;
  }

  for (int i = 0; i < MAX_VISIBLE_LINES; i++) {
    int idx = scrollOffset + i;
    if (idx >= fileCount) break;

    int y = (i + 1) * LINE_HEIGHT;  // baseline Y
    String display;
    if (idx == selectedIndex) {
      display = "> " + fileList[idx];
    } else {
      display = "  " + fileList[idx];
    }

    // Truncate if too long
    if (display.length() > MAX_CHARS_PER_LINE) {
      display = display.substring(0, MAX_CHARS_PER_LINE - 2) + "..";
    }

    u8g2.drawStr(0, y, display.c_str());
  }

  // Scroll indicators
  if (scrollOffset > 0) {
    u8g2.drawTriangle(120, 2, 124, 6, 116, 6);  // up arrow
  }
  if (scrollOffset + MAX_VISIBLE_LINES < fileCount) {
    u8g2.drawTriangle(120, 62, 124, 58, 116, 58); // down arrow
  }

  u8g2.sendBuffer();
}

// =============================================================
//  FILE READER — Text Display
// =============================================================
void openFile(const String &filename) {
  String path = "/" + filename;
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) {
    showMessage("Cannot open\n" + filename);
    delay(1500);
    drawHome();
    return;
  }

  // Read entire file content
  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();

  // Word-wrap the content
  wrapText(content);
  readScrollIndex = 0;
  currentMode = READ_MODE;
  drawReader();
}

void drawReader() {
  u8g2.clearBuffer();

  if (totalWrappedLines == 0) {
    u8g2.drawStr(0, 12, "(empty file)");
    u8g2.sendBuffer();
    return;
  }

  for (int i = 0; i < MAX_VISIBLE_LINES; i++) {
    int lineIdx = readScrollIndex + i;
    if (lineIdx >= totalWrappedLines) break;

    int y = (i + 1) * LINE_HEIGHT;
    u8g2.drawStr(0, y, wrappedLines[lineIdx].c_str());
  }

  // Page indicator in bottom-right
  int totalPages = (totalWrappedLines + MAX_VISIBLE_LINES - 1) / MAX_VISIBLE_LINES;
  int currentPage = (readScrollIndex / MAX_VISIBLE_LINES) + 1;
  String pageStr = String(currentPage) + "/" + String(totalPages);
  int pw = pageStr.length() * CHAR_WIDTH;
  u8g2.drawStr(SCREEN_WIDTH - pw, SCREEN_HEIGHT, pageStr.c_str());

  u8g2.sendBuffer();
}

// =============================================================
//  WORD WRAPPING
// =============================================================
void wrapText(const String &text) {
  totalWrappedLines = 0;
  int len = text.length();
  int pos = 0;

  while (pos < len && totalWrappedLines < 2000) {
    // Find end of current line (newline or EOF)
    int nlPos = text.indexOf('\n', pos);
    if (nlPos == -1) nlPos = len;

    String line = text.substring(pos, nlPos);
    pos = nlPos + 1;  // skip past newline

    // Handle empty lines
    if (line.length() == 0) {
      wrappedLines[totalWrappedLines++] = "";
      continue;
    }

    // Wrap this line into chunks of MAX_CHARS_PER_LINE
    int linePos = 0;
    while (linePos < (int)line.length() && totalWrappedLines < 2000) {
      if ((int)line.length() - linePos <= MAX_CHARS_PER_LINE) {
        // Remaining fits in one line
        wrappedLines[totalWrappedLines++] = line.substring(linePos);
        break;
      }

      // Find a good break point (last space within limit)
      int breakAt = MAX_CHARS_PER_LINE;
      int lastSpace = -1;
      for (int j = linePos; j < linePos + MAX_CHARS_PER_LINE && j < (int)line.length(); j++) {
        if (line.charAt(j) == ' ') {
          lastSpace = j - linePos;
        }
      }

      if (lastSpace > 0) {
        breakAt = lastSpace;
      }

      wrappedLines[totalWrappedLines++] = line.substring(linePos, linePos + breakAt);
      linePos += breakAt;

      // Skip the space at break point
      if (lastSpace > 0 && linePos < (int)line.length() && line.charAt(linePos) == ' ') {
        linePos++;
      }
    }
  }
}

// =============================================================
//  BUTTON HANDLING
// =============================================================
void handleButtons() {
  bool upState   = (digitalRead(BTN_UP) == LOW);
  bool downState = (digitalRead(BTN_DOWN) == LOW);
  unsigned long now = millis();

  // ── Detect press start ──
  if (upState && !upPressed) {
    upPressed    = true;
    upPressTime  = now;
    upHandled    = false;
  }
  if (downState && !downPressed) {
    downPressed    = true;
    downPressTime  = now;
    downHandled    = false;
  }

  // ── Both held ≥ 2s → Wi-Fi Portal ──
  if (upPressed && downPressed && !upHandled && !downHandled) {
    if ((now - upPressTime >= LONG_PRESS_MS) && (now - downPressTime >= LONG_PRESS_MS)) {
      upHandled   = true;
      downHandled = true;
      if (currentMode != PORTAL_MODE) {
        startPortal();
      } else {
        stopPortal();
        currentMode = HOME_MODE;
        scanFiles();
        drawHome();
      }
      return;
    }
  }

  // ── UP long press (only UP held) ──
  if (upPressed && !downPressed && !upHandled) {
    if (now - upPressTime >= LONG_PRESS_MS) {
      upHandled = true;
      if (currentMode == HOME_MODE && fileCount > 0) {
        openFile(fileList[selectedIndex]);
      }
      return;
    }
  }

  // ── DOWN long press (only DOWN held) ──
  if (downPressed && !upPressed && !downHandled) {
    if (now - downPressTime >= LONG_PRESS_MS) {
      downHandled = true;
      if (currentMode == READ_MODE) {
        currentMode = HOME_MODE;
        drawHome();
      }
      return;
    }
  }

  // ── UP released → short press ──
  if (!upState && upPressed) {
    upPressed = false;
    if (!upHandled) {
      // Short press UP
      if (currentMode == HOME_MODE) {
        if (selectedIndex > 0) {
          selectedIndex--;
          drawHome();
        }
      } else if (currentMode == READ_MODE) {
        if (readScrollIndex > 0) {
          readScrollIndex--;
          drawReader();
        }
      }
    }
  }

  // ── DOWN released → short press ──
  if (!downState && downPressed) {
    downPressed = false;
    if (!downHandled) {
      // Short press DOWN
      if (currentMode == HOME_MODE) {
        if (selectedIndex < fileCount - 1) {
          selectedIndex++;
          drawHome();
        }
      } else if (currentMode == READ_MODE) {
        if (readScrollIndex < totalWrappedLines - MAX_VISIBLE_LINES) {
          readScrollIndex++;
          drawReader();
        }
      }
    }
  }
}

// =============================================================
//  WI-FI PORTAL
// =============================================================
void startPortal() {
  currentMode = PORTAL_MODE;
  portalActive = true;

  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[WiFi] AP started: %s\n", ip.toString().c_str());

  server.on("/", HTTP_GET,  handlePortalRoot);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/html",
      "<html><body style='font-family:sans-serif;text-align:center;padding:40px;'>"
      "<h2>Upload Successful!</h2>"
      "<p>File saved to SD card.</p>"
      "<a href='/'>Upload Another</a>"
      "</body></html>");
  }, handleFileUpload);

  server.begin();
  Serial.println("[WiFi] Server started");

  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "Wi-Fi Portal ON");
  u8g2.drawStr(0, 28, "SSID: D.E.V AP");
  u8g2.drawStr(0, 42, "Pass: Darshan");
  String ipStr = "IP: " + ip.toString();
  u8g2.drawStr(0, 56, ipStr.c_str());
  u8g2.sendBuffer();
}

void stopPortal() {
  portalActive = false;
  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("[WiFi] Portal stopped");
}

void handlePortalRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>D.E.V_Darshan - Upload</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #0f0f23, #1a1a3e);
      color: #e0e0e0;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .container {
      background: rgba(255,255,255,0.06);
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: 16px;
      padding: 40px 30px;
      width: 90%;
      max-width: 420px;
      text-align: center;
      backdrop-filter: blur(10px);
    }
    h1 { font-size: 22px; margin-bottom: 6px; color: #7ecfff; }
    .subtitle { font-size: 13px; color: #999; margin-bottom: 28px; }
    .upload-area {
      border: 2px dashed #555;
      border-radius: 12px;
      padding: 30px 15px;
      margin-bottom: 20px;
      transition: border-color 0.3s;
    }
    .upload-area:hover { border-color: #7ecfff; }
    .upload-area p { font-size: 14px; color: #aaa; margin-bottom: 12px; }
    input[type=file] { font-size: 14px; color: #ccc; }
    button {
      background: linear-gradient(135deg, #2979ff, #00bcd4);
      color: #fff;
      border: none;
      padding: 14px 40px;
      font-size: 16px;
      border-radius: 8px;
      cursor: pointer;
      width: 100%;
      transition: opacity 0.3s;
    }
    button:hover { opacity: 0.85; }
    .files { margin-top: 24px; text-align: left; }
    .files h3 { font-size: 14px; color: #7ecfff; margin-bottom: 8px; }
    .files ul { list-style: none; padding: 0; }
    .files li {
      padding: 6px 10px;
      background: rgba(255,255,255,0.04);
      border-radius: 6px;
      margin-bottom: 4px;
      font-size: 13px;
      color: #ccc;
    }
  </style>
</head>
<body>
  <div class='container'>
    <h1>D.E.V_Darshan</h1>
    <p class='subtitle'>ESP32-CAM Mini TXT Reader</p>
    <form method='POST' action='/upload' enctype='multipart/form-data'>
      <div class='upload-area'>
        <p>Select a .txt file to upload</p>
        <input type='file' name='file' accept='.txt' required>
      </div>
      <button type='submit'>Upload to SD Card</button>
    </form>
    <div class='files'>
      <h3>Files on SD Card:</h3>
      <ul>
)rawliteral";

  // List current files
  File root = SD_MMC.open("/");
  if (root && root.isDirectory()) {
    File entry;
    while ((entry = root.openNextFile())) {
      String name = entry.name();
      if (name.startsWith("/")) name = name.substring(1);
      if (!entry.isDirectory() && name.endsWith(".txt")) {
        html += "<li>" + name + " (" + String(entry.size()) + " bytes)</li>";
      }
      entry.close();
    }
    root.close();
  }

  html += R"rawliteral(
      </ul>
    </div>
  </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  static File uploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;

    // Ensure it's a .txt file
    if (!filename.endsWith(".txt")) {
      filename += ".txt";
    }

    Serial.printf("[Upload] Start: %s\n", filename.c_str());
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("[Upload] Failed to open file for writing");
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("[Upload] Complete: %u bytes\n", upload.totalSize);
    }
  }
}

// =============================================================
//  UTILITY — Show a message on OLED
// =============================================================
void showMessage(const String &msg) {
  u8g2.clearBuffer();

  int y = 12;
  int start = 0;
  while (start < (int)msg.length()) {
    int nl = msg.indexOf('\n', start);
    if (nl == -1) nl = msg.length();
    String line = msg.substring(start, nl);
    u8g2.drawStr(0, y, line.c_str());
    y += LINE_HEIGHT;
    start = nl + 1;
  }

  u8g2.sendBuffer();
}
