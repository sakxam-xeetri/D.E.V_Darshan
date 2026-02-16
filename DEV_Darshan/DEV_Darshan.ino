/*
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                         DEV_Darshan TXT Reader                           ║
 * ║                    Pocket-Sized eBook Reader v1.0                        ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║  Hardware: ESP32-CAM (AI Thinker) + 0.91" SSD1306 OLED (128x32)          ║
 * ║  Storage:  SD_MMC (Built-in SD slot)                                     ║
 * ║  Controls: UP, DOWN, SELECT buttons                                      ║
 * ║  Features: Offline TXT reader with WiFi upload portal                    ║
 * ║  Library:  U8g2 for OLED display                                         ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 * 
 * Pin Configuration:
 * ------------------
 * OLED SDA    -> GPIO 13
 * OLED SCL    -> GPIO 14
 * BTN_UP      -> GPIO 12 (to GND)
 * BTN_DOWN    -> GPIO 15 (to GND)
 * BTN_SELECT  -> GPIO 2  (to GND)
 * 
 * SD_MMC uses internal ESP32-CAM pins (no additional wiring needed)
 */

#include <U8g2lib.h>
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

// Display Configuration
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT    32
#define OLED_SDA         13
#define OLED_SCL         14

// Button Configuration
#define BTN_UP           12
#define BTN_DOWN         15
#define BTN_SELECT       2

// Button Timing
#define DEBOUNCE_DELAY   50
#define LONG_PRESS_TIME  5000  // 5 seconds for WiFi mode

// WiFi AP Configuration
#define AP_SSID          "TXT_Reader"
#define AP_PASSWORD      "devdarshan123"
#define DNS_PORT         53

// Reading Configuration
#define MAX_FILES        50
#define MAX_FILENAME_LEN 32
#define CHARS_PER_LINE   21    // For 128px width with 6px font
#define LINES_PER_PAGE   4     // 4 lines on 32px height display
#define LINE_HEIGHT      8

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

// U8g2 display object - Software I2C for custom pins
// SSD1306 128x32 with SCL=GPIO14, SDA=GPIO13
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* SCL=*/ OLED_SCL, /* SDA=*/ OLED_SDA, /* reset=*/ U8X8_PIN_NONE);

WebServer server(80);
DNSServer dnsServer;

// ═══════════════════════════════════════════════════════════════════════════
// STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

enum AppMode {
  MODE_FILE_BROWSER,
  MODE_READING,
  MODE_WIFI_PORTAL
};

AppMode currentMode = MODE_FILE_BROWSER;

// File Browser State
String fileList[MAX_FILES];
int fileCount = 0;
int selectedFileIndex = 0;
int displayStartIndex = 0;

// Reading State
File currentFile;
String currentFilePath;
uint32_t filePosition = 0;
uint32_t fileSize = 0;
int currentPage = 0;
int totalPages = 0;
String displayLines[LINES_PER_PAGE];

// Button State
unsigned long lastDebounceTime = 0;
unsigned long selectPressStart = 0;
bool selectWasPressed = false;
bool buttonProcessed = false;

// WiFi State
bool wifiActive = false;
String uploadStatus = "";

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n╔══════════════════════════════════════╗");
  Serial.println("║     DEV_Darshan TXT Reader v1.0      ║");
  Serial.println("╚══════════════════════════════════════╝");
  
  // Initialize U8g2 Display
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);  // 6x10 pixel font for good readability
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();  // Set text position to top-left corner
  u8g2.setFontDirection(0);
  
  showBootScreen();
  
  // Initialize Buttons
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  // Initialize SD Card (SD_MMC 1-bit mode for ESP32-CAM)
  if (!initSDCard()) {
    showError("SD Card Error!");
    while (true) delay(1000);
  }
  
  // Load file list
  loadFileList();
  
  delay(500);
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
// SD CARD FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

bool initSDCard() {
  Serial.println("Initializing SD Card (SD_MMC 1-bit mode)...");
  
  // SD_MMC 1-bit mode for ESP32-CAM compatibility
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("SD_MMC Mount Failed!");
    return false;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached!");
    return false;
  }
  
  Serial.print("SD Card Type: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC");  break;
    case CARD_SD:   Serial.println("SD");   break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("UNKNOWN"); break;
  }
  
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  return true;
}

void loadFileList() {
  fileCount = 0;
  File root = SD_MMC.open("/");
  
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open root directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      // Filter only .txt files
      if (fileName.endsWith(".txt") || fileName.endsWith(".TXT")) {
        fileList[fileCount] = fileName;
        fileCount++;
        Serial.printf("Found: %s\n", fileName.c_str());
      }
    }
    file = root.openNextFile();
  }
  root.close();
  
  Serial.printf("Total TXT files: %d\n", fileCount);
  
  // Sort files alphabetically
  sortFileList();
}

void sortFileList() {
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
// BUTTON HANDLING
// ═══════════════════════════════════════════════════════════════════════════

void handleButtons() {
  unsigned long currentTime = millis();
  
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool selectPressed = (digitalRead(BTN_SELECT) == LOW);
  
  // Handle SELECT long press detection
  if (selectPressed) {
    if (!selectWasPressed) {
      selectPressStart = currentTime;
      selectWasPressed = true;
      buttonProcessed = false;
    } else if (!buttonProcessed && (currentTime - selectPressStart >= LONG_PRESS_TIME)) {
      // Long press detected - activate WiFi
      buttonProcessed = true;
      startWiFiPortal();
      return;
    }
  } else if (selectWasPressed) {
    // SELECT was released
    if (!buttonProcessed && (currentTime - selectPressStart < LONG_PRESS_TIME)) {
      // Short press
      if (currentTime - lastDebounceTime > DEBOUNCE_DELAY) {
        handleSelectPress();
        lastDebounceTime = currentTime;
      }
    }
    selectWasPressed = false;
    buttonProcessed = false;
  }
  
  // Handle UP button
  if (upPressed && (currentTime - lastDebounceTime > DEBOUNCE_DELAY)) {
    handleUpPress();
    lastDebounceTime = currentTime;
    delay(150);  // Repeat delay
  }
  
  // Handle DOWN button  
  if (downPressed && (currentTime - lastDebounceTime > DEBOUNCE_DELAY)) {
    handleDownPress();
    lastDebounceTime = currentTime;
    delay(150);  // Repeat delay
  }
}

void handleUpPress() {
  if (currentMode == MODE_FILE_BROWSER) {
    if (selectedFileIndex > 0) {
      selectedFileIndex--;
      if (selectedFileIndex < displayStartIndex) {
        displayStartIndex = selectedFileIndex;
      }
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
  // In WiFi mode, SELECT button exits
  if (digitalRead(BTN_SELECT) == LOW) {
    delay(200);  // Debounce
    if (digitalRead(BTN_SELECT) == LOW) {
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
    showError("Open Failed!");
    delay(1000);
    return;
  }
  
  fileSize = currentFile.size();
  filePosition = 0;
  currentPage = 0;
  
  // Calculate approximate total pages
  totalPages = max(1, (int)(fileSize / (CHARS_PER_LINE * LINES_PER_PAGE)));
  
  currentMode = MODE_READING;
  loadCurrentPage();
  updateDisplay();
  
  Serial.printf("Opened: %s (%d bytes)\n", fileName.c_str(), fileSize);
}

void closeFile() {
  if (currentFile) {
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
      // Skip \n after \r (Windows line endings)
      if (c == '\r' && currentFile.peek() == '\n') {
        currentFile.read();
      }
    } else if (currentLine.length() >= CHARS_PER_LINE) {
      // Word wrap - try to break at space
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
  
  // Add remaining text
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
    updateDisplay();
  }
}

void previousPage() {
  if (filePosition == 0) return;
  
  // Go back approximately one page worth of characters
  int charsPerPage = CHARS_PER_LINE * LINES_PER_PAGE;
  if (filePosition > charsPerPage) {
    filePosition -= charsPerPage;
  } else {
    filePosition = 0;
  }
  
  currentPage = max(0, currentPage - 1);
  loadCurrentPage();
  updateDisplay();
}

// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY FUNCTIONS (U8g2)
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
    
    // Selection indicator
    String lineText;
    if (fileIndex == selectedFileIndex) {
      lineText = "> ";
    } else {
      lineText = "  ";
    }
    
    // Truncate filename if too long
    String displayName = fileList[fileIndex];
    if (displayName.length() > CHARS_PER_LINE - 2) {
      displayName = displayName.substring(0, CHARS_PER_LINE - 5) + "...";
    }
    lineText += displayName;
    
    u8g2.drawStr(0, yPos, lineText.c_str());
  }
  
  // Scroll indicator
  drawScrollIndicator();
}

void drawReadingView() {
  for (int i = 0; i < LINES_PER_PAGE; i++) {
    int yPos = i * LINE_HEIGHT;
    u8g2.drawStr(0, yPos, displayLines[i].c_str());
  }
  
  // Progress indicator (small bar on right edge)
  if (fileSize > 0) {
    int progress = map(filePosition, 0, fileSize, 0, SCREEN_HEIGHT - 4);
    // Clear the progress bar area
    u8g2.setDrawColor(0);
    u8g2.drawBox(SCREEN_WIDTH - 2, 0, 2, SCREEN_HEIGHT);
    u8g2.setDrawColor(1);
    // Draw progress indicator
    u8g2.drawBox(SCREEN_WIDTH - 2, progress, 2, 4);
  }
}

void drawWiFiPortal() {
  u8g2.drawStr(0, 0, "WiFi: TXT_Reader");
  u8g2.drawStr(0, 11, "IP: 192.168.4.1");
  u8g2.drawStr(0, 22, "SELECT to exit");
}

void drawScrollIndicator() {
  if (fileCount <= LINES_PER_PAGE) return;
  
  int scrollBarHeight = max(4, (SCREEN_HEIGHT * LINES_PER_PAGE) / fileCount);
  int scrollBarPos = (SCREEN_HEIGHT - scrollBarHeight) * selectedFileIndex / (fileCount - 1);
  
  // Draw scroll track
  u8g2.drawVLine(SCREEN_WIDTH - 1, 0, SCREEN_HEIGHT);
  // Draw scroll thumb
  u8g2.drawBox(SCREEN_WIDTH - 2, scrollBarPos, 2, scrollBarHeight);
}

void showBootScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  u8g2.drawStr(20, 2, "DEV_Darshan");
  u8g2.drawStr(30, 13, "TXT Reader");
  u8g2.drawStr(50, 24, "v1.0");
  
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
  Serial.println("\n=== Starting WiFi Portal ===");
  
  showMessage("Starting WiFi...", "Please wait...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  delay(500);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Setup DNS for captive portal
  dnsServer.start(DNS_PORT, "*", IP);
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, handleUploadComplete, handleFileUpload);
  server.on("/list", HTTP_GET, handleListFiles);
  server.on("/delete", HTTP_GET, handleDeleteFile);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleRoot);
  
  server.begin();
  
  currentMode = MODE_WIFI_PORTAL;
  wifiActive = true;
  
  updateDisplay();
  
  Serial.println("WiFi Portal Active");
  Serial.printf("SSID: %s\n", AP_SSID);
  Serial.printf("Password: %s\n", AP_PASSWORD);
}

void stopWiFiPortal() {
  Serial.println("\n=== Stopping WiFi Portal ===");
  
  showMessage("Closing WiFi...", "Please wait...");
  
  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  delay(500);
  
  wifiActive = false;
  currentMode = MODE_FILE_BROWSER;
  
  // Reload file list in case new files were uploaded
  loadFileList();
  selectedFileIndex = 0;
  displayStartIndex = 0;
  
  updateDisplay();
  
  Serial.println("WiFi Portal Closed");
}

// ═══════════════════════════════════════════════════════════════════════════
// WEB SERVER HANDLERS
// ═══════════════════════════════════════════════════════════════════════════

void handleRoot() {
  String html = getUploadPage();
  server.send(200, "text/html", html);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    
    // Validate .txt extension
    if (!filename.endsWith(".txt") && !filename.endsWith(".TXT")) {
      uploadStatus = "Error: Only .txt files allowed!";
      Serial.println(uploadStatus);
      return;
    }
    
    // Sanitize filename
    filename.replace(" ", "_");
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    
    Serial.printf("Upload Start: %s\n", filename.c_str());
    
    // Delete existing file with same name
    if (SD_MMC.exists(filename)) {
      SD_MMC.remove(filename);
    }
    
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
    if (!uploadFile) {
      uploadStatus = "Error: Failed to create file!";
      Serial.println(uploadStatus);
    } else {
      uploadStatus = "Uploading...";
    }
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      uploadStatus = "Success! " + String(upload.totalSize) + " bytes";
      Serial.printf("Upload Complete: %d bytes\n", upload.totalSize);
    }
  }
}

void handleUploadComplete() {
  String html = getUploadPage();
  server.send(200, "text/html", html);
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
      server.send(200, "text/plain", "Deleted: " + filename);
    } else {
      server.send(404, "text/plain", "File not found");
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
    .container {
      max-width: 500px;
      margin: 0 auto;
    }
    h1 {
      text-align: center;
      font-size: 1.5rem;
      margin-bottom: 5px;
      color: #00d4ff;
    }
    .subtitle {
      text-align: center;
      color: #888;
      font-size: 0.9rem;
      margin-bottom: 20px;
    }
    .card {
      background: rgba(255,255,255,0.05);
      border-radius: 15px;
      padding: 20px;
      margin-bottom: 15px;
      border: 1px solid rgba(255,255,255,0.1);
    }
    .card h2 {
      font-size: 1rem;
      margin-bottom: 15px;
      color: #00d4ff;
    }
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
    .upload-area input {
      display: none;
    }
    .upload-area .icon {
      font-size: 2.5rem;
      margin-bottom: 10px;
    }
    .btn {
      display: block;
      width: 100%;
      padding: 12px;
      border: none;
      border-radius: 8px;
      font-size: 1rem;
      font-weight: 600;
      cursor: pointer;
      margin-top: 15px;
      transition: transform 0.2s;
    }
    .btn:active { transform: scale(0.98); }
    .btn-primary {
      background: linear-gradient(135deg, #00d4ff, #0099ff);
      color: #fff;
    }
    .btn-danger {
      background: #ff4757;
      color: #fff;
    }
    .status {
      padding: 10px;
      border-radius: 8px;
      text-align: center;
      margin-top: 10px;
    }
    .status.success { background: rgba(0,255,136,0.2); color: #00ff88; }
    .status.error { background: rgba(255,71,87,0.2); color: #ff4757; }
    .storage-bar {
      height: 8px;
      background: rgba(255,255,255,0.1);
      border-radius: 4px;
      overflow: hidden;
      margin: 10px 0;
    }
    .storage-bar .fill {
      height: 100%;
      background: linear-gradient(90deg, #00d4ff, #00ff88);
      transition: width 0.5s;
    }
    .storage-text {
      display: flex;
      justify-content: space-between;
      font-size: 0.8rem;
      color: #888;
    }
    .file-list {
      max-height: 200px;
      overflow-y: auto;
    }
    .file-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px;
      background: rgba(255,255,255,0.05);
      border-radius: 8px;
      margin-bottom: 8px;
    }
    .file-name {
      flex: 1;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font-size: 0.9rem;
    }
    .file-size {
      color: #888;
      font-size: 0.8rem;
      margin-left: 10px;
    }
    .file-delete {
      background: none;
      border: none;
      color: #ff4757;
      font-size: 1.2rem;
      cursor: pointer;
      padding: 5px;
    }
    .progress {
      display: none;
      margin-top: 10px;
    }
    .progress-bar {
      height: 6px;
      background: rgba(255,255,255,0.1);
      border-radius: 3px;
      overflow: hidden;
    }
    .progress-bar .fill {
      height: 100%;
      background: #00d4ff;
      width: 0%;
      transition: width 0.3s;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>DEV_Darshan</h1>
    <p class="subtitle">TXT Reader File Manager</p>
    
    <div class="card">
      <h2>Upload TXT File</h2>
      <form id="uploadForm" action="/upload" method="POST" enctype="multipart/form-data">
        <div class="upload-area" id="dropZone">
          <div class="icon">+</div>
          <p>Tap to select or drop .txt file</p>
          <input type="file" name="file" id="fileInput" accept=".txt">
        </div>
        <div class="progress" id="progress">
          <div class="progress-bar"><div class="fill" id="progressFill"></div></div>
        </div>
        <button type="submit" class="btn btn-primary" id="uploadBtn">Upload File</button>
      </form>
      <div id="uploadStatus"></div>
    </div>
    
    <div class="card">
      <h2>Storage</h2>
      <div class="storage-bar">
        <div class="fill" id="storageFill" style="width: 0%"></div>
      </div>
      <div class="storage-text">
        <span id="usedSpace">0 KB used</span>
        <span id="freeSpace">0 KB free</span>
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
    const dropZone = document.getElementById('dropZone');
    const fileInput = document.getElementById('fileInput');
    const uploadForm = document.getElementById('uploadForm');
    const uploadStatus = document.getElementById('uploadStatus');
    const progress = document.getElementById('progress');
    const progressFill = document.getElementById('progressFill');
    
    dropZone.addEventListener('click', () => fileInput.click());
    dropZone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropZone.classList.add('dragover');
    });
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
    dropZone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropZone.classList.remove('dragover');
      if (e.dataTransfer.files.length) {
        fileInput.files = e.dataTransfer.files;
        updateDropZone();
      }
    });
    
    fileInput.addEventListener('change', updateDropZone);
    
    function updateDropZone() {
      if (fileInput.files.length) {
        const file = fileInput.files[0];
        dropZone.innerHTML = '<div class="icon">OK</div><p>' + file.name + '</p><p style="color:#888;font-size:0.8rem">' + formatSize(file.size) + '</p>';
      }
    }
    
    uploadForm.addEventListener('submit', function(e) {
      e.preventDefault();
      
      if (!fileInput.files.length) {
        showStatus('Please select a file', 'error');
        return;
      }
      
      const file = fileInput.files[0];
      if (!file.name.toLowerCase().endsWith('.txt')) {
        showStatus('Only .txt files are allowed', 'error');
        return;
      }
      
      const formData = new FormData();
      formData.append('file', file);
      
      progress.style.display = 'block';
      progressFill.style.width = '0%';
      
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/upload', true);
      
      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          const pct = (e.loaded / e.total * 100);
          progressFill.style.width = pct + '%';
        }
      };
      
      xhr.onload = function() {
        progress.style.display = 'none';
        if (xhr.status === 200) {
          showStatus('File uploaded successfully!', 'success');
          fileInput.value = '';
          dropZone.innerHTML = '<div class="icon">+</div><p>Tap to select or drop .txt file</p>';
          loadFiles();
          loadStatus();
        } else {
          showStatus('Upload failed', 'error');
        }
      };
      
      xhr.onerror = function() {
        progress.style.display = 'none';
        showStatus('Upload error', 'error');
      };
      
      xhr.send(formData);
    });
    
    function showStatus(msg, type) {
      uploadStatus.innerHTML = '<div class="status ' + type + '">' + msg + '</div>';
      setTimeout(() => uploadStatus.innerHTML = '', 3000);
    }
    
    function loadFiles() {
      fetch('/list')
        .then(r => r.json())
        .then(files => {
          const list = document.getElementById('fileList');
          if (files.length === 0) {
            list.innerHTML = '<p style="text-align:center;color:#888">No files</p>';
            return;
          }
          list.innerHTML = files.map(f => 
            '<div class="file-item">' +
              '<span class="file-name">' + f.name + '</span>' +
              '<span class="file-size">' + formatSize(f.size) + '</span>' +
              '<button class="file-delete" onclick="deleteFile(\'' + f.name + '\')">X</button>' +
            '</div>'
          ).join('');
        });
    }
    
    function deleteFile(name) {
      if (confirm('Delete ' + name + '?')) {
        fetch('/delete?file=' + encodeURIComponent(name))
          .then(() => { loadFiles(); loadStatus(); });
      }
    }
    
    function loadStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(s => {
          const pct = (s.used / s.total * 100).toFixed(1);
          document.getElementById('storageFill').style.width = pct + '%';
          document.getElementById('usedSpace').textContent = formatSize(s.used * 1024) + ' used';
          document.getElementById('freeSpace').textContent = formatSize(s.free * 1024) + ' free';
        });
    }
    
    function formatSize(bytes) {
      if (bytes < 1024) return bytes + ' B';
      if (bytes < 1024*1024) return (bytes/1024).toFixed(1) + ' KB';
      return (bytes/1024/1024).toFixed(1) + ' MB';
    }
    
    loadFiles();
    loadStatus();
  </script>
</body>
</html>
)rawliteral";

  return page;
}
