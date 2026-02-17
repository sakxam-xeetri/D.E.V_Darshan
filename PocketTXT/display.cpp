/*
 * ============================================================================
 *  D.E.V_Darshan — Display Module Implementation
 * ============================================================================
 *  All OLED rendering via U8g2lib in page buffer mode (minimal RAM).
 *  Custom I2C pins: GPIO1=SCL, GPIO3=SDA.
 *
 *  Screen Layout (128×32, 5×7 font @ 8px line height = 4 lines):
 *    Line 0 (y=7)   Line 1 (y=15)   Line 2 (y=23)   Line 3 (y=31)
 *
 *  Reading mode uses the FULL 128×32 area — no header, no icons,
 *  no scroll bar. Pure text immersion.
 * ============================================================================
 */

#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>

// ─── U8g2 Instance ───────────────────────────────────────────────────────────
// SSD1306 128×32 I2C, page buffer mode (saves RAM)
static U8G2_SSD1306_128X32_UNIVISION_1_SW_I2C u8g2(
    U8G2_R0,          // No rotation
    PIN_SCL,           // Clock = GPIO1
    PIN_SDA,           // Data  = GPIO3
    U8X8_PIN_NONE      // No reset pin
);

// ─── Helper: draw centered text at given y baseline ──────────────────────────
static void drawCentered(const char* text, int y) {
    int w = u8g2.getStrWidth(text);
    u8g2.drawStr((OLED_WIDTH - w) / 2, y, text);
}

// ─── Initialization ──────────────────────────────────────────────────────────

void display_init() {
    u8g2.begin();
    u8g2.setContrast(180);
    u8g2.enableUTF8Print();
    display_clear();
}

// ─── Boot Screen (2 seconds) ────────────────────────────────────────────────
//  D.E.V_Darshan
//  v.1.0
//  Dev: Sakshyam Bastakoti
//  Initializing...

void display_boot() {
    u8g2.firstPage();
    do {
        // Line 1: Product name (5×7 font)
        u8g2.setFont(u8g2_font_5x7_tr);
        drawCentered(DEVICE_NAME, 7);

        // Line 2: Version (4×6 font — smaller)
        u8g2.setFont(u8g2_font_4x6_tr);
        char vBuf[12];
        snprintf(vBuf, sizeof(vBuf), "v.%s", FW_VERSION);
        drawCentered(vBuf, 15);

        // Line 3: Developer (4×6 font)
        char devBuf[32];
        snprintf(devBuf, sizeof(devBuf), "Dev: %s", DEVELOPER_NAME);
        drawCentered(devBuf, 23);

        // Line 4: Status
        u8g2.setFont(u8g2_font_5x7_tr);
        drawCentered("Initializing...", 31);
    } while (u8g2.nextPage());
}

// ─── Home Screen ─────────────────────────────────────────────────────────────
//  > WiFi Portal
//    Files
//    Settings

void display_home(int selectedIndex) {
    static const char* items[] = {"WiFi Portal", "Files", "Settings"};
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < HOME_ITEMS; i++) {
            int y = 10 + i * 10;
            if (i == selectedIndex) {
                u8g2.drawStr(1, y, ">");
            }
            u8g2.drawStr(10, y, items[i]);
        }
    } while (u8g2.nextPage());
}

// ─── File Menu (cursor-based, scrollable) ────────────────────────────────────
//  > physics.txt
//    math.txt
//    science.txt
//    more...

void display_fileMenu(int selectedIndex, int topIndex,
                      const char* fileNames[], int fileCount) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);

        for (int i = 0; i < MENU_LINES && (topIndex + i) < fileCount; i++) {
            int fileIdx = topIndex + i;
            int y = 7 + (i * 8);

            // Cursor arrow for selected item
            if (fileIdx == selectedIndex) {
                u8g2.drawStr(1, y, ">");
            }

            // Truncate filename to fit (19 chars after "> " prefix)
            char displayName[21];
            strncpy(displayName, fileNames[fileIdx], 19);
            displayName[19] = '\0';
            u8g2.drawStr(10, y, displayName);
        }
    } while (u8g2.nextPage());
}

// ─── Reading Mode (Full Immersion) ──────────────────────────────────────────
//  No file name. No header. No UI elements. No icons.
//  Full 128×32 screen for text only. 4 lines, 5×7 font.

void display_reading(const char* lines[], int lineCount) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < READING_LINES && i < lineCount; i++) {
            if (lines[i]) {
                u8g2.drawStr(0, 7 + (i * 8), lines[i]);
            }
        }
    } while (u8g2.nextPage());
}

// ─── WiFi Portal Screen ─────────────────────────────────────────────────────
//  WiFi Portal
//  SSID: TXT_Reader
//  IP: 192.168.4.1
//  Hold SEL = Exit

void display_wifiPortal(const char* ssid, const char* ip) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        drawCentered("WiFi Portal", 7);

        char buf[24];
        snprintf(buf, sizeof(buf), "SSID: %s", ssid);
        u8g2.drawStr(0, 16, buf);

        snprintf(buf, sizeof(buf), "IP: %s", ip);
        u8g2.drawStr(0, 25, buf);

        u8g2.setFont(u8g2_font_4x6_tr);
        drawCentered("Hold SEL = Exit", 32);
    } while (u8g2.nextPage());
}

// ─── Settings Menu ───────────────────────────────────────────────────────────
//  > System Info
//    File Count
//    Storage

void display_settingsMenu(int selectedIndex) {
    static const char* items[] = {"System Info", "File Count", "Storage"};
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < SETTINGS_ITEMS; i++) {
            int y = 10 + i * 10;
            if (i == selectedIndex) {
                u8g2.drawStr(1, y, ">");
            }
            u8g2.drawStr(10, y, items[i]);
        }
    } while (u8g2.nextPage());
}

// ─── System Info Sub-screen ──────────────────────────────────────────────────
//  DEV_Darshan
//  ESP32-CAM
//  FW: v1.0
//  SD: Mounted

void display_systemInfo(bool sdMounted) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(0, 7,  "DEV_Darshan");
        u8g2.drawStr(0, 15, "ESP32-CAM");

        char buf[22];
        snprintf(buf, sizeof(buf), "FW: v%s", FW_VERSION);
        u8g2.drawStr(0, 23, buf);

        u8g2.drawStr(0, 31, sdMounted ? "SD: Mounted" : "SD: Error");
    } while (u8g2.nextPage());
}

// ─── File Count Sub-screen ───────────────────────────────────────────────────
//  Total Files:
//       3

void display_fileCount(int count) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        drawCentered("Total Files:", 12);

        char buf[8];
        snprintf(buf, sizeof(buf), "%d", count);
        u8g2.setFont(u8g2_font_6x10_tr);
        drawCentered(buf, 26);
    } while (u8g2.nextPage());
}

// ─── Storage Sub-screen ──────────────────────────────────────────────────────
//  Used: 2.3MB
//  Free: 5.7MB

void display_storageInfo(float usedMB, float freeMB) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);

        char buf[22];
        snprintf(buf, sizeof(buf), "Used: %.1fMB", usedMB);
        u8g2.drawStr(0, 14, buf);

        snprintf(buf, sizeof(buf), "Free: %.1fMB", freeMB);
        u8g2.drawStr(0, 26, buf);
    } while (u8g2.nextPage());
}

// ─── Error Display ───────────────────────────────────────────────────────────

void display_error(const char* line1, const char* line2) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        if (line1) {
            int w1 = u8g2.getStrWidth(line1);
            u8g2.drawStr((OLED_WIDTH - w1) / 2, line2 ? 14 : 20, line1);
        }
        if (line2) {
            int w2 = u8g2.getStrWidth(line2);
            u8g2.drawStr((OLED_WIDTH - w2) / 2, 28, line2);
        }
    } while (u8g2.nextPage());
}

// ─── Utility Functions ───────────────────────────────────────────────────────

void display_setInverted(bool inverted) {
    u8g2.sendF("c", inverted ? 0xA7 : 0xA6);
}

void display_sleep() {
    u8g2.sendF("c", 0xAE);  // SSD1306 display OFF
}

void display_wake() {
    u8g2.sendF("c", 0xAF);  // SSD1306 display ON
}

void display_clear() {
    u8g2.firstPage();
    do { } while (u8g2.nextPage());
}
