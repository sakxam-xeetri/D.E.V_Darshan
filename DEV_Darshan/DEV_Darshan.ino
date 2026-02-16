/*
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                         DEV_Darshan TXT Reader                           ║
 * ║                    Pocket-Sized eBook Reader v2.0                        ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║  Hardware: ESP32-CAM (AI Thinker) + 0.91" SSD1306 OLED (128x32)          ║
 * ║  Storage:  SD_MMC (Built-in SD slot, 1-bit mode)                         ║
 * ║  Controls: UP, DOWN, SELECT buttons (to GND, zero resistors)             ║
 * ║  Features: Offline TXT reader with WiFi upload portal                    ║
 * ║  Library:  U8g2 for OLED display                                         ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  PIN MAP — VERIFIED SAFE FOR ESP32-CAM AI THINKER                         ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║                                                                          ║
 * ║  RESERVED (internal, never touch):                                        ║
 * ║    GPIO 2  = SD DATA0                                                    ║
 * ║    GPIO 14 = SD CLK                                                      ║
 * ║    GPIO 15 = SD CMD                                                      ║
 * ║    GPIO 16 = PSRAM CS (AI Thinker specific)                              ║
 * ║                                                                          ║
 * ║  OLED Display (Software I2C):                                             ║
 * ║    GPIO 13 = SDA  (safest pin on the board, no strapping, no HW)         ║
 * ║    GPIO 0  = SCL  (onboard 10K pull-up + OLED pull-up → HIGH at boot    ║
 * ║                     → normal boot. No flash LED. Perfect for I2C.)       ║
 * ║                                                                          ║
 * ║  Buttons (all wired directly to GND, using internal pull-ups):            ║
 * ║    GPIO 12 = BTN_UP     (VDD_SDIO strapping pin: button-to-GND keeps    ║
 * ║                           it LOW at boot → 3.3V flash → SD works.       ║
 * ║                           INPUT_PULLUP enabled after strapping latches.) ║
 * ║    GPIO 3  = BTN_DOWN   (RX pin: freed by TX-only serial config.        ║
 * ║                           Input during boot → button press harmless.)    ║
 * ║    GPIO 4  = BTN_SELECT (Flash LED pin: held OUTPUT LOW to keep LED     ║
 * ║                           off. Momentarily pulsed INPUT_PULLUP for      ║
 * ║                           ~20µs to read button. LED invisible.)          ║
 * ║                                                                          ║
 * ║  Serial Debug (TX only, permanent):                                       ║
 * ║    GPIO 1  = UART TX    (kept as serial output forever. Full runtime     ║
 * ║                           debug logging. RX disabled to free GPIO 3.)    ║
 * ║                                                                          ║
 * ║  SAFETY SUMMARY:                                                          ║
 * ║    ✓ No flash LED power drain (GPIO 4 held LOW)                          ║
 * ║    ✓ No short-circuit risk (GPIO 1 stays TX, never a button)             ║
 * ║    ✓ VDD_SDIO = 3.3V guaranteed (GPIO 12 LOW at boot via button/pull)   ║
 * ║    ✓ Normal boot guaranteed (GPIO 0 pulled HIGH by board + OLED)         ║
 * ║    ✓ Serial debug works permanently (TX-only on GPIO 1)                  ║
 * ║    ✓ SD_MMC pins 2/14/15 completely untouched                            ║
 * ║                                                                          ║
 * ║  CAUTION:                                                                 ║
 * ║    • Do NOT hold BTN_UP (GPIO 0… wait, GPIO 0 is OLED now)              ║
 * ║    • OLED must be connected during boot (pull-ups keep GPIO 0 HIGH).     ║
 * ║      Without OLED, GPIO 0 has only the onboard 10K pull-up → still OK.  ║
 * ║    • Disconnect BTN_DOWN (GPIO 3) when uploading firmware via serial.    ║
 * ║    • GPIO 4 flash LED stays OFF. If you need the flash, don't use this. ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

#include <U8g2lib.h>
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ═══════════════════════════════════════════════════════════════════════════
// PIN CONFIGURATION — Optimal Safe Assignment
// ═══════════════════════════════════════════════════════════════════════════

// OLED Display - Software I2C
#define OLED_SDA         13   // Safest GPIO: no strapping, no onboard hardware
#define OLED_SCL         0    // Onboard 10K pull-up → HIGH at boot → normal boot

// Buttons - all wired to GND, read with internal pull-ups
#define BTN_UP           12   // VDD_SDIO strapping: button-to-GND = LOW = 3.3V = correct
#define BTN_DOWN         3    // RX pin: freed by TX-only serial, harmless during boot
#define BTN_SELECT       4    // Flash LED pin: special read (OUTPUT LOW + momentary INPUT)

// Serial TX stays on GPIO 1 (permanent debug output, never used as button)
// SD_MMC 1-bit: GPIO 2 (DATA0), 14 (CLK), 15 (CMD) — never touch
// PSRAM: GPIO 16 — never touch

// ═══════════════════════════════════════════════════════════════════════════
// SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT    32

#define DEBOUNCE_DELAY   50
#define LONG_PRESS_TIME  5000  // 5 seconds hold for WiFi mode

#define AP_SSID          "TXT_Reader"
#define AP_PASSWORD      "devdarshan123"
#define DNS_PORT         53

#define MAX_FILES        50
#define CHARS_PER_LINE   21
#define LINES_PER_PAGE   4
#define LINE_HEIGHT      8

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

// U8g2: Software I2C, SSD1306 128x32, SCL=GPIO0, SDA=GPIO13
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(
  U8G2_R0,
  /* SCL=*/ OLED_SCL,   // GPIO 0
  /* SDA=*/ OLED_SDA,   // GPIO 13
  /* reset=*/ U8X8_PIN_NONE
);

WebServer server(80);
DNSServer dnsServer;

// ═══════════════════════════════════════════════════════════════════════════
// STATE
// ═══════════════════════════════════════════════════════════════════════════

enum AppMode {
  MODE_FILE_BROWSER,
  MODE_READING,
  MODE_WIFI_PORTAL
};

AppMode currentMode = MODE_FILE_BROWSER;

// File Browser
String fileList[MAX_FILES];
int fileCount = 0;
int selectedFileIndex = 0;
int displayStartIndex = 0;

// Reading
File currentFile;
String currentFilePath;
uint32_t filePosition = 0;
uint32_t fileSize = 0;
int currentPage = 0;
int totalPages = 0;
String displayLines[LINES_PER_PAGE];

// Buttons
unsigned long lastDebounceTime = 0;
unsigned long selectPressStart = 0;
bool selectWasPressed = false;
bool buttonProcessed = false;

// WiFi
bool wifiActive = false;
String uploadStatus = "";

// ═══════════════════════════════════════════════════════════════════════════
// GPIO 4 SPECIAL BUTTON READ
// ═══════════════════════════════════════════════════════════════════════════
// GPIO 4 drives the onboard flash LED via an S8050 NPN transistor.
// If left HIGH (INPUT_PULLUP idle), the LED draws 300-700mA!
// Solution: Keep GPIO 4 as OUTPUT LOW (LED off). Momentarily switch to
// INPUT_PULLUP for ~20µs to sample the button, then back to OUTPUT LOW.
// The LED flash is < 20µs = completely invisible and zero power impact.
// ═══════════════════════════════════════════════════════════════════════════

bool readSelectButton() {
  pinMode(BTN_SELECT, INPUT_PULLUP);
  delayMicroseconds(20);  // Let internal pull-up charge the pin
  bool pressed = (digitalRead(BTN_SELECT) == LOW);
  pinMode(BTN_SELECT, OUTPUT);
  digitalWrite(BTN_SELECT, LOW);  // LED off
  return pressed;
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  // --- Serial: TX-only on GPIO 1 (RX disabled → frees GPIO 3 for button) ---
  // This keeps serial debug working PERMANENTLY, no Serial.end() needed!
  Serial.begin(115200, SERIAL_8N1, -1, 1);  // rxPin=-1, txPin=1
  delay(100);

  Serial.println("\n========================================");
  Serial.println("   DEV_Darshan TXT Reader v2.0 BOOT");
  Serial.println("========================================");
  Serial.println("[PIN MAP]");
  Serial.println("  OLED:   SDA=GPIO13  SCL=GPIO0");
  Serial.println("  BTN:    UP=GPIO12   DOWN=GPIO3   SEL=GPIO4");
  Serial.println("  SERIAL: TX=GPIO1 (permanent, RX disabled)");
  Serial.println("  SD_MMC: D0=GPIO2  CLK=GPIO14  CMD=GPIO15");
  Serial.println("  PSRAM:  GPIO16 (reserved)");

  // 1. Force GPIO 4 LOW immediately (kill flash LED before anything else)
  pinMode(BTN_SELECT, OUTPUT);
  digitalWrite(BTN_SELECT, LOW);
  Serial.println("[BOOT] GPIO4 forced LOW (flash LED off)");

  // 2. Initialize OLED (GPIO 13 SDA + GPIO 0 SCL)
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);

  showBootScreen();
  Serial.println("[BOOT] OLED: OK");

  // 3. Initialize SD Card (GPIO 2, 14, 15 are all free — no conflicts)
  if (!initSDCard()) {
    Serial.println("[BOOT] SD CARD: FAILED!");
    showError("SD Card Error!");
    // Still setup buttons so device isn't completely bricked
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    // BTN_SELECT already configured (OUTPUT LOW / momentary read)
    while (true) {
      delay(1000);
    }
  }
  Serial.println("[BOOT] SD CARD: OK");

  // 4. Load file list from SD
  loadFileList();
  Serial.printf("[BOOT] TXT files found: %d\n", fileCount);

  // 5. Configure buttons
  //    GPIO 12: INPUT_PULLUP (VDD_SDIO strapping already latched → safe)
  //    GPIO 3:  INPUT_PULLUP (freed from UART RX by TX-only serial config)
  //    GPIO 4:  Already configured (OUTPUT LOW + momentary read)
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  Serial.println("[BOOT] Buttons: OK");

  Serial.println("[BOOT] === READY ===");
  Serial.println("  Serial stays active (TX-only on GPIO1)");

  delay(200);
  updateDisplay();
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
  if (currentMode == MODE_WIFI_PORTAL) {
    dnsServer.processNextRequest();
    server.handleClient();
    handleWiFiButtons();
  } else {
    handleButtons();
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// SD CARD
// ═══════════════════════════════════════════════════════════════════════════

bool initSDCard() {
  // SD_MMC 1-bit mode: GPIO 2 (DATA0), 14 (CLK), 15 (CMD)
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    return false;
  }

  Serial.print("  Card Type: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC");  break;
    case CARD_SD:   Serial.println("SD");   break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("?");    break;
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("  Card Size: %llu MB\n", cardSize);

  // Quick write test
  File testFile = SD_MMC.open("/_test_.tmp", FILE_WRITE);
  if (testFile) {
    testFile.println("ok");
    testFile.close();
    SD_MMC.remove("/_test_.tmp");
    Serial.println("  Write Test: PASS");
  } else {
    Serial.println("  Write Test: FAIL (read-only or error)");
  }

  return true;
}

void loadFileList() {
  fileCount = 0;
  File root = SD_MMC.open("/");

  if (!root || !root.isDirectory()) {
    Serial.println("[SD] Cannot open root directory");
    return;
  }

  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      if (fileName.endsWith(".txt") || fileName.endsWith(".TXT")) {
        fileList[fileCount] = fileName;
        fileCount++;
      }
    }
    file = root.openNextFile();
  }
  root.close();

  // Sort alphabetically
  for (int i = 0; i < fileCount - 1; i++) {
    for (int j = i + 1; j < fileCount; j++) {
      if (fileList[i] > fileList[j]) {
        String temp = fileList[i];
        fileList[i] = fileList[j];
        fileList[j] = temp;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// BUTTONS
// ═══════════════════════════════════════════════════════════════════════════

void handleButtons() {
  unsigned long currentTime = millis();

  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool selectPressed = readSelectButton();  // Special GPIO 4 read

  // SELECT long press detection (5 sec hold → WiFi portal)
  if (selectPressed) {
    if (!selectWasPressed) {
      selectPressStart = currentTime;
      selectWasPressed = true;
      buttonProcessed = false;
    } else if (!buttonProcessed && (currentTime - selectPressStart >= LONG_PRESS_TIME)) {
      buttonProcessed = true;
      Serial.println("[BTN] SELECT long press → WiFi Portal");
      startWiFiPortal();
      return;
    }
  } else if (selectWasPressed) {
    if (!buttonProcessed && (currentTime - selectPressStart < LONG_PRESS_TIME)) {
      if (currentTime - lastDebounceTime > DEBOUNCE_DELAY) {
        Serial.println("[BTN] SELECT short press");
        handleSelectPress();
        lastDebounceTime = currentTime;
      }
    }
    selectWasPressed = false;
    buttonProcessed = false;
  }

  // UP
  if (upPressed && (currentTime - lastDebounceTime > DEBOUNCE_DELAY)) {
    handleUpPress();
    lastDebounceTime = currentTime;
    delay(150);
  }

  // DOWN
  if (downPressed && (currentTime - lastDebounceTime > DEBOUNCE_DELAY)) {
    handleDownPress();
    lastDebounceTime = currentTime;
    delay(150);
  }
}

void handleUpPress() {
  if (currentMode == MODE_FILE_BROWSER) {
    if (selectedFileIndex > 0) {
      selectedFileIndex--;
      if (selectedFileIndex < displayStartIndex) {
        displayStartIndex = selectedFileIndex;
      }
      Serial.printf("[NAV] UP → file %d: %s\n", selectedFileIndex, fileList[selectedFileIndex].c_str());
      updateDisplay();
    }
  } else if (currentMode == MODE_READING) {
    previousPage();
  }
}

void handleDownPress() {
  if (currentMode == MODE_FILE_BROWSER) {
    if (selectedFileIndex < fileCount - 1) {
      selectedFileIndex++;
      if (selectedFileIndex >= displayStartIndex + LINES_PER_PAGE) {
        displayStartIndex = selectedFileIndex - LINES_PER_PAGE + 1;
      }
      Serial.printf("[NAV] DOWN → file %d: %s\n", selectedFileIndex, fileList[selectedFileIndex].c_str());
      updateDisplay();
    }
  } else if (currentMode == MODE_READING) {
    nextPage();
  }
}

void handleSelectPress() {
  if (currentMode == MODE_FILE_BROWSER) {
    if (fileCount > 0) {
      openFile(fileList[selectedFileIndex]);
    }
  } else if (currentMode == MODE_READING) {
    closeFile();
  }
}

void handleWiFiButtons() {
  if (readSelectButton()) {  // Special GPIO 4 read
    delay(200);
    if (readSelectButton()) {
      Serial.println("[WIFI] SELECT pressed → exiting portal");
      stopWiFiPortal();
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// FILE READING
// ═══════════════════════════════════════════════════════════════════════════

void openFile(String fileName) {
  currentFilePath = "/" + fileName;
  currentFile = SD_MMC.open(currentFilePath);

  if (!currentFile) {
    Serial.printf("[FILE] FAILED to open: %s\n", currentFilePath.c_str());
    showError("Open Failed!");
    delay(1000);
    updateDisplay();
    return;
  }

  fileSize = currentFile.size();
  filePosition = 0;
  currentPage = 0;
  totalPages = max(1, (int)(fileSize / (CHARS_PER_LINE * LINES_PER_PAGE)));

  Serial.printf("[FILE] Opened: %s (%u bytes, ~%d pages)\n",
                fileName.c_str(), fileSize, totalPages);

  currentMode = MODE_READING;
  loadCurrentPage();
  updateDisplay();
}

void closeFile() {
  if (currentFile) {
    Serial.printf("[FILE] Closed: %s\n", currentFilePath.c_str());
    currentFile.close();
  }
  currentMode = MODE_FILE_BROWSER;
  updateDisplay();
}

void loadCurrentPage() {
  if (!currentFile) return;

  currentFile.seek(filePosition);

  for (int i = 0; i < LINES_PER_PAGE; i++) {
    displayLines[i] = "";
  }

  int lineIndex = 0;
  String currentLine = "";

  while (currentFile.available() && lineIndex < LINES_PER_PAGE) {
    char c = currentFile.read();

    if (c == '\n' || c == '\r') {
      if (currentLine.length() > 0 || c == '\n') {
        displayLines[lineIndex] = currentLine;
        lineIndex++;
        currentLine = "";
      }
      if (c == '\r' && currentFile.peek() == '\n') {
        currentFile.read();
      }
    } else if (currentLine.length() >= CHARS_PER_LINE) {
      // Word wrap
      int lastSpace = currentLine.lastIndexOf(' ');
      if (lastSpace > CHARS_PER_LINE / 2) {
        displayLines[lineIndex] = currentLine.substring(0, lastSpace);
        currentLine = currentLine.substring(lastSpace + 1) + c;
      } else {
        displayLines[lineIndex] = currentLine;
        currentLine = String(c);
      }
      lineIndex++;
    } else {
      currentLine += c;
    }
  }

  if (lineIndex < LINES_PER_PAGE && currentLine.length() > 0) {
    displayLines[lineIndex] = currentLine;
  }
}

void nextPage() {
  if (!currentFile || !currentFile.available()) return;

  uint32_t newPosition = currentFile.position();
  if (newPosition < fileSize) {
    filePosition = newPosition;
    currentPage++;
    loadCurrentPage();
    Serial.printf("[READ] Page %d (pos %u/%u)\n", currentPage, filePosition, fileSize);
    updateDisplay();
  }
}

void previousPage() {
  if (filePosition == 0) return;

  int charsPerPage = CHARS_PER_LINE * LINES_PER_PAGE;
  if (filePosition > charsPerPage) {
    filePosition -= charsPerPage;
  } else {
    filePosition = 0;
  }

  currentPage = max(0, currentPage - 1);
  loadCurrentPage();
  Serial.printf("[READ] Page %d (pos %u/%u)\n", currentPage, filePosition, fileSize);
  updateDisplay();
}

// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY (U8g2)
// ═══════════════════════════════════════════════════════════════════════════

void updateDisplay() {
  u8g2.clearBuffer();

  switch (currentMode) {
    case MODE_FILE_BROWSER:
      drawFileBrowser();
      break;
    case MODE_READING:
      drawReadingView();
      break;
    case MODE_WIFI_PORTAL:
      drawWiFiPortal();
      break;
  }

  u8g2.sendBuffer();
}

void drawFileBrowser() {
  if (fileCount == 0) {
    u8g2.drawStr(10, 12, "No TXT files");
    return;
  }

  for (int i = 0; i < LINES_PER_PAGE && (displayStartIndex + i) < fileCount; i++) {
    int fileIndex = displayStartIndex + i;
    int yPos = i * LINE_HEIGHT;

    String lineText;
    if (fileIndex == selectedFileIndex) {
      lineText = "> ";
    } else {
      lineText = "  ";
    }

    String displayName = fileList[fileIndex];
    if (displayName.length() > CHARS_PER_LINE - 2) {
      displayName = displayName.substring(0, CHARS_PER_LINE - 5) + "...";
    }
    lineText += displayName;

    u8g2.drawStr(0, yPos, lineText.c_str());
  }

  // Scroll indicator
  if (fileCount > LINES_PER_PAGE) {
    int scrollBarHeight = max(4, (SCREEN_HEIGHT * LINES_PER_PAGE) / fileCount);
    int scrollBarPos = (SCREEN_HEIGHT - scrollBarHeight) * selectedFileIndex / (fileCount - 1);
    u8g2.drawVLine(SCREEN_WIDTH - 1, 0, SCREEN_HEIGHT);
    u8g2.drawBox(SCREEN_WIDTH - 2, scrollBarPos, 2, scrollBarHeight);
  }
}

void drawReadingView() {
  for (int i = 0; i < LINES_PER_PAGE; i++) {
    u8g2.drawStr(0, i * LINE_HEIGHT, displayLines[i].c_str());
  }

  // Progress bar on right edge
  if (fileSize > 0) {
    int progress = map(filePosition, 0, fileSize, 0, SCREEN_HEIGHT - 4);
    u8g2.setDrawColor(0);
    u8g2.drawBox(SCREEN_WIDTH - 2, 0, 2, SCREEN_HEIGHT);
    u8g2.setDrawColor(1);
    u8g2.drawBox(SCREEN_WIDTH - 2, progress, 2, 4);
  }
}

void drawWiFiPortal() {
  u8g2.drawStr(0, 0, "WiFi: TXT_Reader");
  u8g2.drawStr(0, 11, "IP: 192.168.4.1");
  u8g2.drawStr(0, 22, "SELECT to exit");
}

void showBootScreen() {
  u8g2.clearBuffer();
  u8g2.drawStr(20, 2, "DEV_Darshan");
  u8g2.drawStr(30, 13, "TXT Reader");
  u8g2.drawStr(50, 24, "v2.0");
  u8g2.sendBuffer();
  delay(1500);

  u8g2.clearBuffer();
  u8g2.drawStr(10, 12, "Loading SD...");
  u8g2.sendBuffer();
}

void showError(const char* message) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 8, "ERR:");
  u8g2.drawStr(30, 8, message);
  u8g2.sendBuffer();
}

void showMessage(const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 6, line1);
  u8g2.drawStr(0, 18, line2);
  u8g2.sendBuffer();
}

// ═══════════════════════════════════════════════════════════════════════════
// WIFI PORTAL
// ═══════════════════════════════════════════════════════════════════════════

void startWiFiPortal() {
  Serial.println("[WIFI] Starting Access Point...");
  showMessage("Starting WiFi...", "Please wait...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);

  IPAddress IP = WiFi.softAPIP();
  Serial.printf("[WIFI] AP SSID: %s  IP: %s\n", AP_SSID, IP.toString().c_str());

  // DNS captive portal
  dnsServer.start(DNS_PORT, "*", IP);

  // Web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, handleUploadComplete, handleFileUpload);
  server.on("/list", HTTP_GET, handleListFiles);
  server.on("/delete", HTTP_GET, handleDeleteFile);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleRoot);

  server.begin();
  Serial.println("[WIFI] Web server started");

  currentMode = MODE_WIFI_PORTAL;
  wifiActive = true;

  updateDisplay();
}

void stopWiFiPortal() {
  Serial.println("[WIFI] Shutting down...");
  showMessage("Closing WiFi...", "Please wait...");

  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  wifiActive = false;
  currentMode = MODE_FILE_BROWSER;

  // Reload file list (new files may have been uploaded)
  loadFileList();
  selectedFileIndex = 0;
  displayStartIndex = 0;

  Serial.printf("[WIFI] Closed. %d files on SD.\n", fileCount);
  updateDisplay();
}

// ═══════════════════════════════════════════════════════════════════════════
// WEB HANDLERS
// ═══════════════════════════════════════════════════════════════════════════

void handleRoot() {
  server.send(200, "text/html", getUploadPage());
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;
  static String currentFileName;
  static size_t bytesWritten = 0;

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    Serial.printf("[UPLOAD] Start: %s\n", filename.c_str());

    // Validate .txt extension
    if (!filename.endsWith(".txt") && !filename.endsWith(".TXT")) {
      uploadStatus = "Error: Only .txt files allowed!";
      Serial.println("[UPLOAD] Rejected: not a .txt file");
      return;
    }

    // Sanitize filename
    filename.replace(" ", "_");
    filename.replace("/", "");
    filename.replace("\\", "");
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }

    currentFileName = filename;
    bytesWritten = 0;

    // Close any previously open file
    if (uploadFile) {
      uploadFile.close();
    }

    // Remove existing file
    if (SD_MMC.exists(filename)) {
      SD_MMC.remove(filename);
      delay(50);
    }

    // Open for writing
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
    if (!uploadFile) {
      uploadStatus = "Error: Cannot create file on SD!";
      Serial.printf("[UPLOAD] FAILED to create: %s\n", filename.c_str());
    } else {
      uploadStatus = "Uploading...";
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      size_t written = uploadFile.write(upload.buf, upload.currentSize);
      bytesWritten += written;
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.flush();
      uploadFile.close();

      // Verify
      if (SD_MMC.exists(currentFileName)) {
        File check = SD_MMC.open(currentFileName);
        size_t actualSize = check.size();
        check.close();
        uploadStatus = "Success! " + String(actualSize) + " bytes saved";
        Serial.printf("[UPLOAD] OK: %s (%u bytes)\n", currentFileName.c_str(), actualSize);
      } else {
        uploadStatus = "Error: File not found after write!";
        Serial.println("[UPLOAD] VERIFY FAILED: file not found after write");
      }
    } else {
      uploadStatus = "Error: File was not open!";
      Serial.println("[UPLOAD] ERROR: file handle was null at END");
    }

  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) {
      uploadFile.close();
    }
    uploadStatus = "Upload aborted!";
    Serial.println("[UPLOAD] Aborted by client");
  }
}

void handleUploadComplete() {
  String response = "{\"status\":\"" + uploadStatus + "\"}";
  server.send(200, "application/json", response);
}

void handleListFiles() {
  String json = "[";
  File root = SD_MMC.open("/");

  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      if (!file.isDirectory()) {
        String name = file.name();
        if (name.endsWith(".txt") || name.endsWith(".TXT")) {
          if (!first) json += ",";
          json += "{\"name\":\"" + name + "\",\"size\":" + String(file.size()) + "}";
          first = false;
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  json += "]";
  server.send(200, "application/json", json);
}

void handleDeleteFile() {
  if (server.hasArg("file")) {
    String filename = "/" + server.arg("file");
    if (SD_MMC.exists(filename)) {
      SD_MMC.remove(filename);
      Serial.printf("[DELETE] %s\n", filename.c_str());
      server.send(200, "text/plain", "Deleted");
    } else {
      server.send(404, "text/plain", "Not found");
    }
  } else {
    server.send(400, "text/plain", "No file specified");
  }
}

void handleStatus() {
  uint64_t totalBytes = SD_MMC.totalBytes();
  uint64_t usedBytes = SD_MMC.usedBytes();

  String json = "{";
  json += "\"total\":" + String((uint32_t)(totalBytes / 1024)) + ",";
  json += "\"used\":" + String((uint32_t)(usedBytes / 1024)) + ",";
  json += "\"free\":" + String((uint32_t)((totalBytes - usedBytes) / 1024));
  json += "}";

  server.send(200, "application/json", json);
}

// ═══════════════════════════════════════════════════════════════════════════
// HTML UPLOAD PAGE
// ═══════════════════════════════════════════════════════════════════════════

String getUploadPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>DEV_Darshan TXT Reader</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      min-height: 100vh;
      color: #fff;
      padding: 20px;
    }
    .container { max-width: 500px; margin: 0 auto; }
    h1 { text-align: center; font-size: 1.5rem; margin-bottom: 5px; color: #00d4ff; }
    .subtitle { text-align: center; color: #888; font-size: 0.9rem; margin-bottom: 20px; }
    .card {
      background: rgba(255,255,255,0.05);
      border-radius: 15px;
      padding: 20px;
      margin-bottom: 15px;
      border: 1px solid rgba(255,255,255,0.1);
    }
    .card h2 { font-size: 1rem; margin-bottom: 15px; color: #00d4ff; }
    .upload-area {
      border: 2px dashed rgba(0,212,255,0.5);
      border-radius: 10px;
      padding: 30px;
      text-align: center;
      transition: all 0.3s;
      cursor: pointer;
    }
    .upload-area:hover, .upload-area.dragover {
      border-color: #00d4ff;
      background: rgba(0,212,255,0.1);
    }
    .upload-area input { display: none; }
    .upload-area .icon { font-size: 2.5rem; margin-bottom: 10px; }
    .btn {
      display: block; width: 100%; padding: 12px; border: none;
      border-radius: 8px; font-size: 1rem; font-weight: 600;
      cursor: pointer; margin-top: 15px; transition: transform 0.2s;
    }
    .btn:active { transform: scale(0.98); }
    .btn-primary { background: linear-gradient(135deg, #00d4ff, #0099ff); color: #fff; }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    .status { padding: 10px; border-radius: 8px; text-align: center; margin-top: 10px; }
    .status.success { background: rgba(0,255,136,0.2); color: #00ff88; }
    .status.error { background: rgba(255,71,87,0.2); color: #ff4757; }
    .status.info { background: rgba(0,212,255,0.2); color: #00d4ff; }
    .storage-bar {
      height: 8px; background: rgba(255,255,255,0.1);
      border-radius: 4px; overflow: hidden; margin: 10px 0;
    }
    .storage-bar .fill {
      height: 100%;
      background: linear-gradient(90deg, #00d4ff, #00ff88);
      transition: width 0.5s;
    }
    .storage-text { display: flex; justify-content: space-between; font-size: 0.8rem; color: #888; }
    .file-list { max-height: 250px; overflow-y: auto; }
    .file-item {
      display: flex; justify-content: space-between; align-items: center;
      padding: 10px; background: rgba(255,255,255,0.05);
      border-radius: 8px; margin-bottom: 8px;
    }
    .file-name { flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; font-size: 0.9rem; }
    .file-size { color: #888; font-size: 0.8rem; margin-left: 10px; }
    .file-delete { background: none; border: none; color: #ff4757; font-size: 1.2rem; cursor: pointer; padding: 5px; }
    .progress { display: none; margin-top: 10px; }
    .progress-bar { height: 6px; background: rgba(255,255,255,0.1); border-radius: 3px; overflow: hidden; }
    .progress-bar .fill { height: 100%; background: #00d4ff; width: 0%; transition: width 0.3s; }
    .progress-text { text-align: center; margin-top: 5px; font-size: 0.8rem; color: #888; }
  </style>
</head>
<body>
  <div class="container">
    <h1>DEV_Darshan</h1>
    <p class="subtitle">TXT Reader File Manager</p>

    <div class="card">
      <h2>Upload TXT Files</h2>
      <form id="uploadForm">
        <div class="upload-area" id="dropZone">
          <div class="icon">+</div>
          <p>Tap to select or drop .txt files</p>
          <p style="font-size:0.8rem;color:#888;margin-top:5px">Multiple files supported</p>
          <input type="file" id="fileInput" accept=".txt" multiple>
        </div>
        <div class="progress" id="progress">
          <div class="progress-bar"><div class="fill" id="progressFill"></div></div>
          <p class="progress-text" id="progressText"></p>
        </div>
        <button type="submit" class="btn btn-primary" id="uploadBtn">Upload Files</button>
      </form>
      <div id="uploadStatus"></div>
    </div>

    <div class="card">
      <h2>Storage</h2>
      <div class="storage-bar">
        <div class="fill" id="storageFill" style="width: 0%"></div>
      </div>
      <div class="storage-text">
        <span id="usedSpace">Loading...</span>
        <span id="freeSpace"></span>
      </div>
    </div>

    <div class="card">
      <h2>Files on Device</h2>
      <div class="file-list" id="fileList">
        <p style="text-align:center;color:#888">Loading...</p>
      </div>
    </div>
  </div>

<script>
var dropZone = document.getElementById('dropZone');
var fileInput = document.getElementById('fileInput');
var uploadForm = document.getElementById('uploadForm');
var uploadBtn = document.getElementById('uploadBtn');
var statusDiv = document.getElementById('uploadStatus');
var progress = document.getElementById('progress');
var progressFill = document.getElementById('progressFill');
var progressText = document.getElementById('progressText');
var uploading = false;

dropZone.onclick = function() { fileInput.click(); };
dropZone.ondragover = function(e) { e.preventDefault(); dropZone.classList.add('dragover'); };
dropZone.ondragleave = function() { dropZone.classList.remove('dragover'); };
dropZone.ondrop = function(e) {
  e.preventDefault();
  dropZone.classList.remove('dragover');
  if (e.dataTransfer.files.length) {
    fileInput.files = e.dataTransfer.files;
    showSelected();
  }
};
fileInput.onchange = showSelected;

function showSelected() {
  if (!fileInput.files.length) return;
  if (fileInput.files.length === 1) {
    var f = fileInput.files[0];
    dropZone.innerHTML = '<div class="icon">OK</div><p>' + f.name + '</p><p style="color:#888;font-size:0.8rem">' + fmtSize(f.size) + '</p>';
  } else {
    var total = 0;
    for (var i = 0; i < fileInput.files.length; i++) total += fileInput.files[i].size;
    dropZone.innerHTML = '<div class="icon">OK</div><p>' + fileInput.files.length + ' files selected</p><p style="color:#888;font-size:0.8rem">' + fmtSize(total) + ' total</p>';
  }
}

uploadForm.onsubmit = function(e) {
  e.preventDefault();
  if (uploading) return;
  if (!fileInput.files.length) { showMsg('Select a file first', 'error'); return; }

  var files = [];
  for (var i = 0; i < fileInput.files.length; i++) {
    if (fileInput.files[i].name.toLowerCase().endsWith('.txt')) {
      files.push(fileInput.files[i]);
    }
  }
  if (!files.length) { showMsg('No .txt files selected', 'error'); return; }

  uploading = true;
  uploadBtn.disabled = true;
  uploadSequential(files, 0, 0, files.length);
};

function uploadSequential(files, idx, ok, total) {
  if (idx >= files.length) {
    uploading = false;
    uploadBtn.disabled = false;
    progress.style.display = 'none';
    if (ok === total) {
      showMsg('All ' + ok + ' file(s) uploaded!', 'success');
    } else {
      showMsg(ok + ' of ' + total + ' uploaded', ok > 0 ? 'info' : 'error');
    }
    fileInput.value = '';
    dropZone.innerHTML = '<div class="icon">+</div><p>Tap to select or drop .txt files</p><p style="font-size:0.8rem;color:#888;margin-top:5px">Multiple files supported</p>';
    setTimeout(function() { loadFiles(); loadStatus(); }, 300);
    return;
  }

  var file = files[idx];
  var fd = new FormData();
  fd.append('file', file);

  progress.style.display = 'block';
  progressFill.style.width = '0%';
  progressText.textContent = 'Uploading ' + (idx+1) + '/' + total + ': ' + file.name;

  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/upload', true);
  xhr.timeout = 60000;

  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) progressFill.style.width = (e.loaded / e.total * 100) + '%';
  };

  xhr.onload = function() {
    if (xhr.status === 200) {
      var success = false;
      try {
        var r = JSON.parse(xhr.responseText);
        success = r.status && r.status.indexOf('Success') >= 0;
      } catch(e) {
        success = true;
      }
      uploadSequential(files, idx + 1, ok + (success ? 1 : 0), total);
    } else {
      uploadSequential(files, idx + 1, ok, total);
    }
  };

  xhr.onerror = function() { uploadSequential(files, idx + 1, ok, total); };
  xhr.ontimeout = function() { uploadSequential(files, idx + 1, ok, total); };

  xhr.send(fd);
}

function showMsg(msg, type) {
  statusDiv.innerHTML = '<div class="status ' + type + '">' + msg + '</div>';
  setTimeout(function() { statusDiv.innerHTML = ''; }, 5000);
}

function loadFiles() {
  fetch('/list').then(function(r) { return r.json(); }).then(function(files) {
    var list = document.getElementById('fileList');
    if (!files.length) { list.innerHTML = '<p style="text-align:center;color:#888">No files on SD</p>'; return; }
    var html = '';
    for (var i = 0; i < files.length; i++) {
      html += '<div class="file-item"><span class="file-name">' + files[i].name +
        '</span><span class="file-size">' + fmtSize(files[i].size) +
        '</span><button class="file-delete" onclick="delFile(\'' + files[i].name + '\')">X</button></div>';
    }
    list.innerHTML = html;
  }).catch(function() {
    document.getElementById('fileList').innerHTML = '<p style="text-align:center;color:#f55">SD Card error</p>';
  });
}

function delFile(name) {
  if (confirm('Delete ' + name + '?')) {
    fetch('/delete?file=' + encodeURIComponent(name)).then(function() {
      loadFiles(); loadStatus();
    });
  }
}

function loadStatus() {
  fetch('/status').then(function(r) { return r.json(); }).then(function(s) {
    if (s.total > 0) {
      var pct = (s.used / s.total * 100).toFixed(1);
      document.getElementById('storageFill').style.width = pct + '%';
      document.getElementById('usedSpace').textContent = fmtSize(s.used * 1024) + ' used';
      document.getElementById('freeSpace').textContent = fmtSize(s.free * 1024) + ' free';
    } else {
      document.getElementById('usedSpace').textContent = 'SD not available';
      document.getElementById('freeSpace').textContent = '';
    }
  }).catch(function() {
    document.getElementById('usedSpace').textContent = 'SD error';
    document.getElementById('freeSpace').textContent = '';
  });
}

function fmtSize(b) {
  if (b < 1024) return b + ' B';
  if (b < 1048576) return (b / 1024).toFixed(1) + ' KB';
  return (b / 1048576).toFixed(1) + ' MB';
}

loadFiles();
loadStatus();
</script>
</body>
</html>
)rawliteral";

  return page;
}
