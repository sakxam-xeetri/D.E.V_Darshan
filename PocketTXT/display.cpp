/*
 * ============================================================================
 *  PocketTXT — Display Module Implementation
 * ============================================================================
 *  Handles all OLED rendering using U8g2lib in page buffer mode for
 *  minimal RAM usage. Custom I2C pins (GPIO1=SCL, GPIO3=SDA).
 *
 *  Display layout (128×32):
 *    Line 0 (y=7):   Header (filename or menu title) — 5×7 font
 *    Line 1 (y=17):  Text line 1 — 6×10 font
 *    Line 2 (y=25):  Text line 2 — 6×10 font
 *    Line 3 (y=32):  Text line 3 + scroll indicator — 6×10 font
 * ============================================================================
 */

#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>

// ─── U8g2 Instance ───────────────────────────────────────────────────────────
// SSD1306 128×32 I2C, page buffer mode (saves RAM vs full buffer)
// Using SW I2C on custom pins (GPIO3=SDA, GPIO1=SCL)
static U8G2_SSD1306_128X32_UNIVISION_1_SW_I2C u8g2(
    U8G2_R0,          // No rotation
    PIN_SCL,           // Clock = GPIO1
    PIN_SDA,           // Data  = GPIO3
    U8X8_PIN_NONE      // No reset pin
);

static bool isInverted = false;

// ─── Initialization ──────────────────────────────────────────────────────────

void display_init() {
    u8g2.begin();
    u8g2.setContrast(180);       // Medium brightness — saves power
    u8g2.enableUTF8Print();
    display_clear();
}

// ─── Splash Screen ──────────────────────────────────────────────────────────

void display_splash() {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        // Center "PocketTXT" (9 chars × 6px = 54px → offset = 37)
        u8g2.drawStr(37, 14, "PocketTXT");
        u8g2.setFont(u8g2_font_5x7_tr);
        // Center version string
        char vStr[16];
        snprintf(vStr, sizeof(vStr), "v%s", FW_VERSION);
        int vWidth = u8g2.getStrWidth(vStr);
        u8g2.drawStr((128 - vWidth) / 2, 28, vStr);
    } while (u8g2.nextPage());
}

// ─── Error Display ───────────────────────────────────────────────────────────

void display_error(const char* line1, const char* line2) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        if (line1) {
            int w1 = u8g2.getStrWidth(line1);
            u8g2.drawStr((128 - w1) / 2, line2 ? 14 : 20, line1);
        }
        if (line2) {
            int w2 = u8g2.getStrWidth(line2);
            u8g2.drawStr((128 - w2) / 2, 28, line2);
        }
    } while (u8g2.nextPage());
}

// ─── File Menu ───────────────────────────────────────────────────────────────

void display_fileMenu(int selectedIndex, int topIndex,
                      const char* fileNames[], int fileCount) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);

        // Draw up to 4 visible file entries
        for (int i = 0; i < DISPLAY_LINES && (topIndex + i) < fileCount; i++) {
            int fileIdx = topIndex + i;
            int yPos = 7 + (i * 8);  // 8px per line with 5×7 font

            if (fileIdx == selectedIndex) {
                // Draw inverted bar for selection
                u8g2.drawBox(0, yPos - 7, 120, 8);
                u8g2.setDrawColor(0);  // White text on black
            }

            // Truncate filename if needed
            char displayName[MAX_FILENAME_LEN];
            strncpy(displayName, fileNames[fileIdx], MAX_FILENAME_LEN - 1);
            displayName[MAX_FILENAME_LEN - 1] = '\0';

            // Add selection arrow
            if (fileIdx == selectedIndex) {
                u8g2.drawStr(1, yPos, ">");
            }
            u8g2.drawStr(8, yPos, displayName);

            if (fileIdx == selectedIndex) {
                u8g2.setDrawColor(1);  // Restore normal draw color
            }
        }

        // Draw file counter in bottom-right
        char counter[10];
        snprintf(counter, sizeof(counter), "%d/%d", selectedIndex + 1, fileCount);
        int cWidth = u8g2.getStrWidth(counter);
        u8g2.drawStr(128 - cWidth - 1, 32, counter);

    } while (u8g2.nextPage());
}

// ─── Reading View ────────────────────────────────────────────────────────────

void display_reading(const char* filename, const char* lines[],
                     int lineCount, int scrollPos, int totalLines) {
    u8g2.firstPage();
    do {
        // ── Header: filename in small font ──
        u8g2.setFont(u8g2_font_5x7_tr);
        char header[CHARS_PER_LINE + 1];
        strncpy(header, filename, CHARS_PER_LINE);
        header[CHARS_PER_LINE] = '\0';
        u8g2.drawStr(0, 7, header);

        // Thin separator line
        u8g2.drawHLine(0, 8, 126);

        // ── Body: up to TEXT_LINES of wrapped text ──
        u8g2.setFont(u8g2_font_6x10_tr);
        for (int i = 0; i < TEXT_LINES && i < lineCount; i++) {
            if (lines[i]) {
                u8g2.drawStr(0, 19 + (i * 9), lines[i]);
            }
        }

        // ── Scroll indicator (right edge) ──
        if (totalLines > TEXT_LINES) {
            // Calculate scroll bar position and size
            int barAreaHeight = 22;  // Pixels available for scroll bar
            int barHeight = max(3, barAreaHeight * TEXT_LINES / totalLines);
            int barPos = 10 + (barAreaHeight - barHeight) * scrollPos / 
                         max(1, totalLines - TEXT_LINES);

            u8g2.drawBox(128 - SCROLL_BAR_WIDTH, barPos,
                        SCROLL_BAR_WIDTH, barHeight);
        }

    } while (u8g2.nextPage());
}

// ─── WiFi Info Screen ────────────────────────────────────────────────────────

void display_wifiInfo(const char* ssid, const char* password, const char* ip) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_5x7_tr);
        
        char buf[24];
        snprintf(buf, sizeof(buf), "WiFi:%s", ssid);
        u8g2.drawStr(0, 7, buf);
        
        snprintf(buf, sizeof(buf), "Pass:%s", password);
        u8g2.drawStr(0, 15, buf);
        
        snprintf(buf, sizeof(buf), "IP:%s", ip);
        u8g2.drawStr(0, 23, buf);

        u8g2.drawStr(0, 31, "Press SEL to exit");

    } while (u8g2.nextPage());
}

// ─── Utility Functions ───────────────────────────────────────────────────────

void display_setInverted(bool inverted) {
    isInverted = inverted;
    if (inverted) {
        u8g2.sendF("c", 0xA7);  // SSD1306 invert display command
    } else {
        u8g2.sendF("c", 0xA6);  // SSD1306 normal display command
    }
}

void display_sleep() {
    u8g2.sendF("c", 0xAE);      // SSD1306 display OFF command
}

void display_wake() {
    u8g2.sendF("c", 0xAF);      // SSD1306 display ON command
}

void display_clear() {
    u8g2.firstPage();
    do {
        // Empty page — clears display
    } while (u8g2.nextPage());
}
