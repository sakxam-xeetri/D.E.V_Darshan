/*
 * OLED Display Test for ESP32-CAM
 * 
 * This simplified test helps diagnose OLED connection issues.
 * Upload this first to verify OLED is working before using main code.
 * 
 * Connections:
 * - OLED VCC to ESP32-CAM 3.3V
 * - OLED GND to ESP32-CAM GND
 * - OLED SDA to GPIO 13
 * - OLED SCL to GPIO 12
 */

#include <Wire.h>
#include <U8g2lib.h>

// Pin definitions
#define OLED_SDA 13
#define OLED_SCL 12

// Try this constructor first (address 0x3C)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// If above doesn't work, try this instead (address 0x3D):
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32-CAM OLED Test ===");
  
  // Initialize I2C
  Serial.print("Initializing I2C on SDA=");
  Serial.print(OLED_SDA);
  Serial.print(", SCL=");
  Serial.println(OLED_SCL);
  
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(100);
  
  // Scan I2C bus
  Serial.println("\nScanning I2C bus...");
  scanI2C();
  
  // Initialize OLED
  Serial.println("Initializing OLED...");
  
  if (!u8g2.begin()) {
    Serial.println("ERROR: u8g2.begin() failed!");
  } else {
    Serial.println("u8g2.begin() successful!");
  }
  
  // Test 1: Clear display
  Serial.println("\nTest 1: Clearing display");
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(500);
  
  // Test 2: Draw text
  Serial.println("Test 2: Drawing text");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Hello!");
  u8g2.drawStr(0, 25, "OLED Test");
  u8g2.drawStr(0, 40, "ESP32-CAM");
  u8g2.sendBuffer();
  Serial.println("Text sent to display");
  delay(2000);
  
  // Test 3: Draw shapes
  Serial.println("Test 3: Drawing shapes");
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);  // Border
  u8g2.drawCircle(32, 32, 15);     // Circle
  u8g2.drawBox(70, 20, 30, 20);    // Filled box
  u8g2.sendBuffer();
  Serial.println("Shapes sent to display");
  delay(2000);
  
  // Test 4: Large text
  Serial.println("Test 4: Large text");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(10, 30, "WORKS!");
  u8g2.sendBuffer();
  Serial.println("Large text sent to display");
  
  Serial.println("\n=== Test Complete ===");
  Serial.println("Can you see text on OLED?");
  Serial.println("\nIf NO display:");
  Serial.println("1. Check Serial Monitor for I2C address");
  Serial.println("2. Verify OLED is 3.3V (not 5V only)");
  Serial.println("3. Check SDA/SCL aren't swapped");
  Serial.println("4. Try different I2C address (0x3D)");
}

void loop() {
  // Blink counter on display
  static int counter = 0;
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(10, 30, "Count:");
  u8g2.setCursor(80, 30);
  u8g2.print(counter);
  u8g2.sendBuffer();
  
  Serial.print("Counter: ");
  Serial.println(counter);
  
  counter++;
  delay(1000);
}

void scanI2C() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning...");
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      
      if (address == 0x3C) {
        Serial.println("  -> This is likely your OLED (0x3C)");
      } else if (address == 0x3D) {
        Serial.println("  -> This is likely your OLED (0x3D)");
        Serial.println("  -> CHANGE constructor to use 0x3D!");
      }
      
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0) {
    Serial.println("ERROR: No I2C devices found!\n");
    Serial.println("Troubleshooting:");
    Serial.println("1. Check all 4 wires connected:");
    Serial.println("   VCC -> 3.3V");
    Serial.println("   GND -> GND");
    Serial.println("   SDA -> GPIO 13");
    Serial.println("   SCL -> GPIO 12");
    Serial.println("2. Verify OLED power LED is on");
    Serial.println("3. Try different OLED if available");
  } else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)\n");
  }
}
