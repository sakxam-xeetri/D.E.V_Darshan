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

// ─── Home Screen (">" cursor indicator) ───────────────────────────────────────────

void display_home(int selectedIndex) {
    static const char* items[] = {"WiFi Portal", "Files", "Settings"};
    int yOffset = (OLED_HEIGHT - HOME_ITEMS * 8) / 2;  // Center vertically
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < HOME_ITEMS; i++) {
            int textY = yOffset + (i * 8) + 7;
            if (i == selectedIndex) {
                // Draw ">" cursor before selected item
                u8g2.drawStr(0, textY, ">");
                u8g2.drawStr(8, textY, items[i]);
            } else {
                // No cursor, just indent
                u8g2.drawStr(8, textY, items[i]);
            }
        }
    } while (u8g2.nextPage());
}

// ─── File Menu (">" cursor, "< Back" at index 0) ────────────────────
//  > < Back
//    physics.txt
//    math.txt

void display_fileMenu(int selectedIndex, int topIndex,
                      const char* fileNames[], int fileCount) {
    int totalItems = fileCount + 1;  // +1 for "< Back" at index 0
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);

        // Show menu items
        for (int i = 0; i < MENU_LINES && (topIndex + i) < totalItems; i++) {
            int itemIdx = topIndex + i;
            int textY = 7 + i * 8;

            // Build display text
            char displayBuf[22];
            if (itemIdx == 0) {
                strcpy(displayBuf, "< Back");
            } else {
                strncpy(displayBuf, fileNames[itemIdx - 1], 19);
                displayBuf[19] = '\0';
            }

            // Draw ">" cursor for selected item
            if (itemIdx == selectedIndex) {
                u8g2.drawStr(0, textY, ">");
                u8g2.drawStr(8, textY, displayBuf);
            } else {
                u8g2.drawStr(8, textY, displayBuf);
            }
        }

        // If no files, show info text on line 2 (non-selectable, greyed)
        if (fileCount == 0) {
            u8g2.setFont(u8g2_font_4x6_tr);
            u8g2.drawStr(10, 15, "No TXT Files Found");
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
//  SSID: TXT_Reader
//  Pass: readmore
//  IP: 192.168.4.1
//  SEL = Exit

void display_wifiPortal(const char* ssid, const char* ip) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_4x6_tr);

        char buf[24];
        snprintf(buf, sizeof(buf), "SSID: %s", ssid);
        u8g2.drawStr(0, 6, buf);

        snprintf(buf, sizeof(buf), "Pass: %s", WIFI_PASSWORD);
        u8g2.drawStr(0, 14, buf);

        snprintf(buf, sizeof(buf), "IP: %s", ip);
        u8g2.drawStr(0, 22, buf);

        drawCentered("SEL = Exit", 31);
    } while (u8g2.nextPage());
}

// ─── Settings Menu (">" cursor, "< Back" at index 0) ─────────────────

void display_settingsMenu(int selectedIndex) {
    static const char* items[] = {"< Back", "System Info", "File Count", "Storage"};
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < SETTINGS_ITEMS; i++) {
            int textY = 7 + i * 8;
            if (i == selectedIndex) {
                // Draw ">" cursor before selected item
                u8g2.drawStr(0, textY, ">");
                u8g2.drawStr(8, textY, items[i]);
            } else {
                // No cursor, just indent
                u8g2.drawStr(8, textY, items[i]);
            }
        }
    } while (u8g2.nextPage());
}

// ─── System Info Sub-screen ──────────────────────────────────────────────────

void display_systemInfo(bool sdMounted) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 6,  DEVICE_NAME);

        char buf[26];
        snprintf(buf, sizeof(buf), "FW: v%s  ESP32-CAM", FW_VERSION);
        u8g2.drawStr(0, 14, buf);

        u8g2.drawStr(0, 22, sdMounted ? "SD: Mounted" : "SD: Error");

        drawCentered("SEL = Back", 31);
    } while (u8g2.nextPage());
}

// ─── File Count Sub-screen ───────────────────────────────────────────────────

void display_fileCount(int count) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        drawCentered("TXT Files:", 10);

        char buf[8];
        snprintf(buf, sizeof(buf), "%d", count);
        u8g2.setFont(u8g2_font_6x10_tr);
        drawCentered(buf, 22);

        u8g2.setFont(u8g2_font_4x6_tr);
        drawCentered("SEL = Back", 31);
    } while (u8g2.nextPage());
}

// ─── Storage Sub-screen ──────────────────────────────────────────────────────

void display_storageInfo(float usedMB, float freeMB) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);

        char buf[22];
        snprintf(buf, sizeof(buf), "Used: %.1fMB", usedMB);
        u8g2.drawStr(0, 8, buf);

        snprintf(buf, sizeof(buf), "Free: %.1fMB", freeMB);
        u8g2.drawStr(0, 18, buf);

        u8g2.setFont(u8g2_font_4x6_tr);
        drawCentered("SEL = Back", 31);
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
