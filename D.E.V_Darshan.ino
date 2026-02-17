/*
 * D.E.V_Darshan - ESP32-CAM Mini TXT Reader
 * 
 * Developer: Sakshyam Bastakoti - IOT and Robotics Developer
 * Board: ESP32-CAM (AI Thinker)
 * 
 * Description:
 * Compact offline TXT reader with OLED display and Wi-Fi upload portal.
 * Navigate files using two buttons, read text with word wrapping.
 * 
 * Hardware:
 * - ESP32-CAM module
 * - 0.96" OLED (SSD1306, I2C)
 * - MicroSD card (1-8GB, FAT32)
 * - 2x Tactile buttons (UP, DOWN)
 * 
 * Pin Configuration:
 * - OLED SDA: GPIO 13
 * - OLED SCL: GPIO 12
 * - UP Button: GPIO 4 (Active LOW)
 * - DOWN Button: GPIO 0 (Active LOW, boot pin)
 * - SD Card: GPIO 15 (CMD), GPIO 14 (CLK), GPIO 2 (D0)
 * 
 * Libraries Required:
 * - U8g2 (OLED display)
 * - SD_MMC (SD card)
 * - WiFi, WebServer (Wi-Fi portal)
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SD_MMC.h"
#include "FS.h"

// ========== PIN DEFINITIONS ==========
#define OLED_SDA 13
#define OLED_SCL 12
#define BTN_UP 4
#define BTN_DOWN 0  // Boot pin - must be HIGH at boot for normal operation

// ========== CONSTANTS ==========
#define LONG_PRESS_TIME 2000  // 2 seconds for long press
#define DEBOUNCE_DELAY 50     // Button debounce time
#define CHARS_PER_LINE 21     // Characters per line with u8g2_font_6x10_tr
#define VISIBLE_LINES 6       // Lines visible on screen (full display)
#define AP_SSID "D.E.V AP"
#define AP_PASS "Darshan"

// ========== GLOBAL OBJECTS ==========
// OLED Display (128x64, I2C) - Try 0x3C address first, if not working change to 0x3D
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// Web Server
WebServer server(80);

// ========== ENUMS ==========
enum Mode {
  HOME_MODE,    // File list view
  READ_MODE,    // File reading view
  PORTAL_MODE   // Wi-Fi upload portal
};

// ========== GLOBAL VARIABLES ==========
Mode currentMode = HOME_MODE;

// File management
String fileList[50];       // Array to store filenames
int fileCount = 0;         // Total number of .txt files
int selectedFileIndex = 0; // Currently selected file in menu
int homeScrollOffset = 0;  // Scroll offset for file list

// Reading variables
String currentFileName = "";
int readScrollLine = 0;    // Current scroll position in file (line number)
int totalFileLines = 0;    // Total lines in current file

// Button state management
bool btnUpState = HIGH;
bool btnDownState = HIGH;
bool lastBtnUpState = HIGH;
bool lastBtnDownState = HIGH;
unsigned long btnUpPressTime = 0;
unsigned long btnDownPressTime = 0;
unsigned long lastDebounceTimeUp = 0;
unsigned long lastDebounceTimeDown = 0;

// Portal mode flag
bool portalActive = false;

// ========== I2C SCANNER FUNCTION ==========
void scanI2C() {
  Serial.println("\n=== I2C Scanner ===");
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found!");
    Serial.println("Check wiring:");
    Serial.println("- OLED VCC to 3.3V");
    Serial.println("- OLED GND to GND");
    Serial.println("- OLED SDA to GPIO 13");
    Serial.println("- OLED SCL to GPIO 12");
  } else {
    Serial.println("I2C scan complete");
  }
  Serial.println("==================\n");
}

// ========== SETUP FUNCTION ==========
void setup() {
  Serial.begin(115200);
  delay(1000);  // Give time for serial monitor to connect
  Serial.println("\n\n=== D.E.V_Darshan TXT Reader ===");
  Serial.println("Developer: Sakshyam Bastakoti");
  Serial.println("================================\n");
  
  // Initialize buttons with internal pull-up resistors
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  Serial.println("Buttons initialized");
  
  // Initialize I2C
  Serial.println("Initializing I2C...");
  Serial.printf("SDA: GPIO %d, SCL: GPIO %d\n", OLED_SDA, OLED_SCL);
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(100);
  
  // Scan I2C bus
  scanI2C();
  
  // Initialize OLED
  Serial.println("Initializing OLED display...");
  bool oledOK = u8g2.begin();
  
  if (!oledOK) {
    Serial.println("ERROR: OLED initialization failed!");
    Serial.println("Common fixes:");
    Serial.println("1. Check if display is 0x3C or 0x3D address");
    Serial.println("2. Verify 3.3V power connection");
    Serial.println("3. Check SDA/SCL wiring");
  } else {
    Serial.println("OLED initialized successfully!");
  }
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);  // 6x10 pixel font
  u8g2.drawStr(0, 10, "D.E.V_Darshan");
  u8g2.drawStr(0, 25, "TXT Reader");
  u8g2.drawStr(0, 40, "Initializing...");
  u8g2.sendBuffer();
  Serial.println("OLED test display sent");
  
  // Initialize SD card (1-bit mode)
  Serial.println("Initializing SD Card...");
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("SD Card Mount Failed!");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "SD Card");
    u8g2.drawStr(0, 25, "ERROR!");
    u8g2.drawStr(0, 40, "Check card");
    u8g2.sendBuffer();
    while (1) { delay(1000); }  // Halt if SD fails
  }
  
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  // Scan for .txt files
  scanTxtFiles();
  
  // Display ready message
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Ready!");
  u8g2.drawStr(0, 25, String("Files: " + String(fileCount)).c_str());
  u8g2.drawStr(0, 40, "Hold both btns");
  u8g2.drawStr(0, 55, "for Wi-Fi");
  u8g2.sendBuffer();
  delay(2000);
  
  // Switch to home mode
  currentMode = HOME_MODE;
  displayHomeScreen();
}

// ========== MAIN LOOP ==========
void loop() {
  handleButtons();
  
  if (currentMode == PORTAL_MODE) {
    server.handleClient();  // Handle web server requests
  }
  
  delay(10);  // Small delay for stability
}

// ========== BUTTON HANDLING ==========
void handleButtons() {
  unsigned long currentTime = millis();
  
  // Read button states
  bool currentUpState = digitalRead(BTN_UP);
  bool currentDownState = digitalRead(BTN_DOWN);
  
  // Debounce UP button
  if (currentUpState != lastBtnUpState) {
    lastDebounceTimeUp = currentTime;
  }
  if ((currentTime - lastDebounceTimeUp) > DEBOUNCE_DELAY) {
    if (currentUpState != btnUpState) {
      btnUpState = currentUpState;
      if (btnUpState == LOW) {  // Button pressed
        btnUpPressTime = currentTime;
      } else {  // Button released
        unsigned long pressDuration = currentTime - btnUpPressTime;
        if (pressDuration < LONG_PRESS_TIME) {
          handleUpShortPress();
        }
      }
    }
  }
  lastBtnUpState = currentUpState;
  
  // Debounce DOWN button
  if (currentDownState != lastBtnDownState) {
    lastDebounceTimeDown = currentTime;
  }
  if ((currentTime - lastDebounceTimeDown) > DEBOUNCE_DELAY) {
    if (currentDownState != btnDownState) {
      btnDownState = currentDownState;
      if (btnDownState == LOW) {  // Button pressed
        btnDownPressTime = currentTime;
      } else {  // Button released
        unsigned long pressDuration = currentTime - btnDownPressTime;
        if (pressDuration < LONG_PRESS_TIME) {
          handleDownShortPress();
        }
      }
    }
  }
  lastBtnDownState = currentDownState;
  
  // Check for long press actions
  if (btnUpState == LOW && (currentTime - btnUpPressTime) >= LONG_PRESS_TIME) {
    // Check if both buttons held
    if (btnDownState == LOW && (currentTime - btnDownPressTime) >= LONG_PRESS_TIME) {
      if (!portalActive) {
        handleBothLongPress();
        portalActive = true;
        // Wait for release
        while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW) {
          delay(50);
        }
      }
    } else {
      handleUpLongPress();
      // Wait for release
      while (digitalRead(BTN_UP) == LOW) {
        delay(50);
      }
    }
  }
  
  if (btnDownState == LOW && (currentTime - btnDownPressTime) >= LONG_PRESS_TIME) {
    if (btnUpState == HIGH) {  // Only DOWN held (not both)
      handleDownLongPress();
      // Wait for release
      while (digitalRead(BTN_DOWN) == LOW) {
        delay(50);
      }
    }
  }
  
  // Reset portal flag when buttons released
  if (btnUpState == HIGH && btnDownState == HIGH) {
    portalActive = false;
  }
}

// ========== BUTTON ACTION HANDLERS ==========
void handleUpShortPress() {
  Serial.println("UP short press");
  if (currentMode == HOME_MODE) {
    // Move selection up
    if (selectedFileIndex > 0) {
      selectedFileIndex--;
      if (selectedFileIndex < homeScrollOffset) {
        homeScrollOffset = selectedFileIndex;
      }
      displayHomeScreen();
    }
  } else if (currentMode == READ_MODE) {
    // Scroll up in file
    if (readScrollLine > 0) {
      readScrollLine--;
      displayFileContent();
    }
  }
}

void handleDownShortPress() {
  Serial.println("DOWN short press");
  if (currentMode == HOME_MODE) {
    // Move selection down
    if (selectedFileIndex < fileCount - 1) {
      selectedFileIndex++;
      if (selectedFileIndex >= homeScrollOffset + VISIBLE_LINES) {
        homeScrollOffset = selectedFileIndex - VISIBLE_LINES + 1;
      }
      displayHomeScreen();
    }
  } else if (currentMode == READ_MODE) {
    // Scroll down in file
    if (readScrollLine < totalFileLines - VISIBLE_LINES) {
      readScrollLine++;
      displayFileContent();
    }
  }
}

void handleUpLongPress() {
  Serial.println("UP long press - Select file");
  if (currentMode == HOME_MODE && fileCount > 0) {
    // Open selected file
    currentFileName = fileList[selectedFileIndex];
    readScrollLine = 0;
    currentMode = READ_MODE;
    countFileLines();
    displayFileContent();
  }
}

void handleDownLongPress() {
  Serial.println("DOWN long press - Back to menu");
  if (currentMode == READ_MODE) {
    // Return to file list
    currentMode = HOME_MODE;
    displayHomeScreen();
  }
}

void handleBothLongPress() {
  Serial.println("BOTH long press - Wi-Fi Portal");
  currentMode = PORTAL_MODE;
  startWiFiPortal();
}

// ========== FILE OPERATIONS ==========
void scanTxtFiles() {
  Serial.println("Scanning for .txt files...");
  fileCount = 0;
  
  File root = SD_MMC.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file && fileCount < 50) {
    if (!file.isDirectory()) {
      String fileName = String(file.name());
      if (fileName.endsWith(".txt") || fileName.endsWith(".TXT")) {
        fileList[fileCount] = fileName;
        fileCount++;
        Serial.println("Found: " + fileName);
      }
    }
    file = root.openNextFile();
  }
  
  Serial.printf("Total .txt files found: %d\n", fileCount);
}

void countFileLines() {
  totalFileLines = 0;
  String filePath = "/" + currentFileName;
  
  File file = SD_MMC.open(filePath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for counting");
    return;
  }
  
  String line = "";
  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      totalFileLines += wrapLine(line);
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  
  // Count last line if exists
  if (line.length() > 0) {
    totalFileLines += wrapLine(line);
  }
  
  file.close();
  Serial.printf("Total wrapped lines: %d\n", totalFileLines);
}

int wrapLine(String line) {
  if (line.length() == 0) return 1;
  return (line.length() + CHARS_PER_LINE - 1) / CHARS_PER_LINE;
}

// ========== DISPLAY FUNCTIONS ==========
void displayHomeScreen() {
  u8g2.clearBuffer();
  
  if (fileCount == 0) {
    u8g2.drawStr(0, 10, "No .txt files");
    u8g2.drawStr(0, 25, "found!");
    u8g2.drawStr(0, 45, "Upload via");
    u8g2.drawStr(0, 60, "Wi-Fi portal");
  } else {
    for (int i = 0; i < VISIBLE_LINES && (homeScrollOffset + i) < fileCount; i++) {
      int index = homeScrollOffset + i;
      int y = 10 + (i * 10);
      
      String displayName = fileList[index];
      if (displayName.length() > 20) {
        displayName = displayName.substring(0, 17) + "...";
      }
      
      if (index == selectedFileIndex) {
        u8g2.drawStr(0, y, ">");
        u8g2.drawStr(10, y, displayName.c_str());
      } else {
        u8g2.drawStr(10, y, displayName.c_str());
      }
    }
  }
  
  u8g2.sendBuffer();
}

void displayFileContent() {
  u8g2.clearBuffer();
  
  String filePath = "/" + currentFileName;
  File file = SD_MMC.open(filePath, FILE_READ);
  
  if (!file) {
    u8g2.drawStr(0, 10, "Error opening");
    u8g2.drawStr(0, 25, "file!");
    u8g2.sendBuffer();
    return;
  }
  
  // Read file and find starting position
  int currentLine = 0;
  int displayLine = 0;
  String line = "";
  
  while (file.available() && displayLine < VISIBLE_LINES) {
    char c = file.read();
    
    if (c == '\n' || c == '\r') {
      if (c == '\r') continue;  // Skip carriage return
      
      // Process the line
      if (line.length() == 0) {
        // Empty line
        if (currentLine >= readScrollLine) {
          int y = 10 + (displayLine * 10);
          u8g2.drawStr(0, y, " ");
          displayLine++;
        }
        currentLine++;
      } else {
        // Wrap and display line
        for (int i = 0; i < line.length(); i += CHARS_PER_LINE) {
          if (currentLine >= readScrollLine) {
            String segment = line.substring(i, min(i + CHARS_PER_LINE, (int)line.length()));
            int y = 10 + (displayLine * 10);
            u8g2.drawStr(0, y, segment.c_str());
            displayLine++;
            if (displayLine >= VISIBLE_LINES) break;
          }
          currentLine++;
        }
      }
      line = "";
    } else {
      line += c;
    }
  }
  
  // Handle last line if file doesn't end with newline
  if (line.length() > 0 && displayLine < VISIBLE_LINES) {
    for (int i = 0; i < line.length(); i += CHARS_PER_LINE) {
      if (currentLine >= readScrollLine) {
        String segment = line.substring(i, min(i + CHARS_PER_LINE, (int)line.length()));
        int y = 10 + (displayLine * 10);
        u8g2.drawStr(0, y, segment.c_str());
        displayLine++;
        if (displayLine >= VISIBLE_LINES) break;
      }
      currentLine++;
    }
  }
  
  file.close();
  u8g2.sendBuffer();
}

// ========== WI-FI PORTAL FUNCTIONS ==========
void startWiFiPortal() {
  Serial.println("Starting Wi-Fi Portal...");
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Wi-Fi Portal");
  u8g2.drawStr(0, 25, "Starting...");
  u8g2.sendBuffer();
  
  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, handleUploadResponse, handleFileUpload);
  server.on("/exit", HTTP_GET, handleExit);
  
  server.begin();
  Serial.println("Web server started");
  
  // Display portal info
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Wi-Fi Active");
  u8g2.drawStr(0, 25, "SSID: D.E.V AP");
  u8g2.drawStr(0, 40, "Pass: Darshan");
  u8g2.drawStr(0, 55, "IP: 192.168.4.1");
  u8g2.sendBuffer();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin:20px;background:#f0f0f0;}";
  html += "h1{color:#333;}";
  html += ".container{background:white;padding:20px;border-radius:10px;max-width:500px;margin:auto;}";
  html += "input[type=file]{margin:20px 0;padding:10px;}";
  html += "button{background:#4CAF50;color:white;padding:15px 30px;border:none;border-radius:5px;cursor:pointer;font-size:16px;margin:10px;}";
  html += "button:hover{background:#45a049;}";
  html += ".exit{background:#f44336;}";
  html += ".exit:hover{background:#da190b;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>📖 D.E.V_Darshan</h1>";
  html += "<h2>TXT File Upload</h2>";
  html += "<p>Total files: " + String(fileCount) + "</p>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='file' accept='.txt' required><br>";
  html += "<button type='submit'>Upload File</button>";
  html += "</form>";
  html += "<br><button class='exit' onclick='location.href=\"/exit\"'>Exit Portal</button>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    Serial.println("Upload start: " + filename);
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
    
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "Uploading:");
    String shortName = upload.filename;
    if (shortName.length() > 20) shortName = shortName.substring(0, 17) + "...";
    u8g2.drawStr(0, 30, shortName.c_str());
    u8g2.sendBuffer();
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.println("Upload complete: " + String(upload.totalSize) + " bytes");
    }
  }
}

void handleUploadResponse() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='3;url=/'>";
  html += "<style>body{font-family:Arial;text-align:center;margin:50px;}</style>";
  html += "</head><body>";
  html += "<h1>✅ Upload Successful!</h1>";
  html += "<p>Redirecting...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Rescan files
  delay(100);
  scanTxtFiles();
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Upload");
  u8g2.drawStr(0, 25, "Complete!");
  u8g2.drawStr(0, 45, String("Files: " + String(fileCount)).c_str());
  u8g2.sendBuffer();
}

void handleExit() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<style>body{font-family:Arial;text-align:center;margin:50px;}</style>";
  html += "</head><body>";
  html += "<h1>Portal Closing</h1>";
  html += "<p>Restart device to use reader</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  delay(1000);
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "Portal Closed");
  u8g2.drawStr(0, 35, "Restarting...");
  u8g2.sendBuffer();
  
  delay(2000);
  ESP.restart();
}
