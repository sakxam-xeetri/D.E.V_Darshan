/*
 * ESP32-CAM Text File Reader with 0.96" OLED Display
 * 
 * Hardware Requirements:
 * - ESP32-CAM board
 * - 0.96" OLED Display (SSD1306, I2C)
 * - SD Card with text files
 * 
 * Connections:
 * OLED SDA -> GPIO 12
 * OLED SCL -> GPIO 13
 * OLED VCC -> 3.3V
 * OLED GND -> GND
 * 
 * SD Card is already connected in ESP32-CAM module
 * Using 1-bit SD mode - GPIO 12/13 are safe for I2C
 * SD uses: GPIO 2 (DATA0), GPIO 14 (CLK), GPIO 15 (CMD)
 */

#include <Wire.h>
#include <U8g2lib.h>
#include "SD_MMC.h"
#include <FS.h>

// I2C pins for ESP32-CAM 
// In 1-bit SD mode, only GPIO 2/14/15 are used by SD card
// GPIO 12/13 are safe for I2C
#define I2C_SDA 12
#define I2C_SCL 13

// U8g2 Constructor for 0.96" OLED (SSD1306, 128x64, Software I2C)
// Using software I2C for better compatibility with custom pins
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA, /* reset=*/ U8X8_PIN_NONE);

// File variables
File root;
File currentFile;
String fileList[20];  // Store up to 20 files
int fileCount = 0;
int currentFileIndex = 0;
int scrollPosition = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32-CAM OLED Text Reader");
  Serial.print("I2C SDA: GPIO ");
  Serial.println(I2C_SDA);
  Serial.print("I2C SCL: GPIO ");
  Serial.println(I2C_SCL);
  
  // Initialize U8g2 display
  Serial.println("Initializing OLED display...");
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);  // Small font for more text
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Initializing...");
  u8g2.sendBuffer();
  Serial.println("OLED display initialized OK");
  
  // Add delay for SD card to stabilize
  delay(500);
  
  // Initialize SD card - MUST use 1-bit mode only (GPIO 12/13 used by I2C)
  Serial.println("Initializing SD card in 1-bit mode...");
  bool sdCardMounted = false;
  
  // Try 1-bit mode with path
  if(SD_MMC.begin("/sdcard", true)) {
    sdCardMounted = true;
    Serial.println("SD Card mounted in 1-bit mode");
  } else {
    Serial.println("1-bit mode with path failed, trying default 1-bit...");
    SD_MMC.end();
    delay(200);
    
    // Try 1-bit mode without path
    if(SD_MMC.begin()) {
      sdCardMounted = true;
      Serial.println("SD Card mounted (1-bit default mode)");
    }
  }
  
  // DO NOT try 4-bit mode - it conflicts with I2C on GPIO 12/13
  
  if(!sdCardMounted) {
    Serial.println("SD Card Mount Failed - Check:");
    Serial.println("1. SD card is inserted properly");
    Serial.println("2. SD card is FAT32 formatted");
    Serial.println("3. SD card size <= 32GB");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "SD Card Failed!");
    u8g2.drawStr(0, 22, "Check card:");
    u8g2.drawStr(0, 34, "-Insert properly");
    u8g2.drawStr(0, 46, "-Format FAT32");
    u8g2.sendBuffer();
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "No SD Card!");
    u8g2.sendBuffer();
    return;
  }
  
  Serial.println("SD Card initialized successfully");
  
  // List all text files
  listTextFiles("/");
  
  if(fileCount > 0) {
    displayFileList();
  } else {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "No .txt files");
    u8g2.drawStr(0, 22, "found on SD!");
    u8g2.sendBuffer();
  }
}

void loop() {
  // This example auto-displays the first text file
  // You can add button controls here to navigate between files
  
  if(fileCount > 0 && currentFileIndex < fileCount) {
    delay(3000);  // Show file list for 3 seconds
    displayTextFile(fileList[currentFileIndex]);
    
    // Auto-advance to next file after 10 seconds (optional)
    delay(10000);
    currentFileIndex++;
    if(currentFileIndex >= fileCount) {
      currentFileIndex = 0;  // Loop back to first file
    }
    displayFileList();
  }
  
  delay(100);
}

void listTextFiles(const char * dirname) {
  Serial.printf("Listing directory: %s\n", dirname);
  fileCount = 0;
  
  File root = SD_MMC.open(dirname);
  if(!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  
  File file = root.openNextFile();
  while(file && fileCount < 20) {
    if(!file.isDirectory()) {
      String fileName = String(file.name());
      // Check if it's a .txt file
      if(fileName.endsWith(".txt") || fileName.endsWith(".TXT")) {
        fileList[fileCount] = fileName;
        Serial.println(fileList[fileCount]);
        fileCount++;
      }
    }
    file = root.openNextFile();
  }
  
  Serial.printf("Found %d text files\n", fileCount);
}

void displayFileList() {
  u8g2.clearBuffer();
  
  u8g2.drawStr(0, 10, "Text Files:");
  u8g2.drawStr(0, 20, "-------------");
  
  int startIndex = currentFileIndex;
  int displayCount = 0;
  int yPos = 32;
  
  for(int i = startIndex; i < fileCount && displayCount < 4; i++) {
    String line = "";
    if(i == currentFileIndex) {
      line = "> ";  // Arrow for current file
    } else {
      line = "  ";
    }
    
    // Truncate long filenames
    String displayName = fileList[i];
    if(displayName.length() > 18) {
      displayName = displayName.substring(0, 15) + "...";
    }
    line += displayName;
    
    u8g2.drawStr(0, yPos, line.c_str());
    yPos += 10;
    displayCount++;
  }
  
  u8g2.sendBuffer();
}

void displayTextFile(String filename) {
  File file = SD_MMC.open("/" + filename);
  
  if(!file) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "Error opening:");
    u8g2.drawStr(0, 22, filename.c_str());
    u8g2.sendBuffer();
    Serial.println("Failed to open file: " + filename);
    return;
  }
  
  u8g2.clearBuffer();
  
  // Display filename at top (truncate if needed)
  String displayFilename = filename;
  if(displayFilename.length() > 21) {
    displayFilename = displayFilename.substring(0, 18) + "...";
  }
  u8g2.drawStr(0, 10, displayFilename.c_str());
  u8g2.drawStr(0, 20, "-------------");
  
  // Read and display file content
  int lineCount = 0;
  String line = "";
  int yPos = 30;
  
  while(file.available() && lineCount < 5) {  // Display up to 5 lines
    char c = file.read();
    
    if(c == '\n' || c == '\r') {
      if(line.length() > 0) {
        // Truncate long lines
        if(line.length() > 21) {
          u8g2.drawStr(0, yPos, line.substring(0, 21).c_str());
        } else {
          u8g2.drawStr(0, yPos, line.c_str());
        }
        yPos += 10;
        line = "";
        lineCount++;
      }
    } else {
      line += c;
    }
  }
  
  // Display last line if exists
  if(line.length() > 0 && lineCount < 5) {
    if(line.length() > 21) {
      u8g2.drawStr(0, yPos, line.substring(0, 21).c_str());
    } else {
      u8g2.drawStr(0, yPos, line.c_str());
    }
  }
  
  file.close();
  u8g2.sendBuffer();
  
  Serial.println("Displayed: " + filename);
}
