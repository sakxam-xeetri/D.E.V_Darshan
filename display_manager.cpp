/*  =========================================================================
 *  DEV_Darshan — Display Manager Implementation
 *  SSD1306 128×32 OLED via custom-pin I2C (GPIO 13=SDA, GPIO 12=SCL)
 *  =========================================================================
 */

#include <Arduino.h>
#include "display_manager.h"
#include "config.h"
#include <Wire.h>
#include <SSD1306Wire.h>   // ThingPulse library

// ── OLED instance on custom I2C pins ───────────────────────────────────
static SSD1306Wire oled(OLED_I2C_ADDR, PIN_SDA, PIN_SCL);

// ═══════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════

void display_init() {
    oled.init();
    oled.flipScreenVertically();          // mount-dependent — change if needed
    oled.setFont(ArialMT_Plain_10);       // default font
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.clear();
    oled.display();
}

// ═══════════════════════════════════════════════════════════════════════
//  Primitives
// ═══════════════════════════════════════════════════════════════════════

void display_clear() {
    oled.clear();
}

void display_show() {
    oled.display();
}

void display_text(uint8_t line, const char* text) {
    // Each line = 8 px high (we use ArialMT_Plain_10 but pack 4 lines)
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(0, line * FONT_HEIGHT, text);
}

void display_textSmall(uint8_t line, const char* text) {
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(0, line * FONT_HEIGHT, text);
}

// ═══════════════════════════════════════════════════════════════════════
//  High-level screens
// ═══════════════════════════════════════════════════════════════════════

/* Boot splash --------------------------------------------------------- */
void display_splash() {
    oled.clear();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 0, PROJECT_NAME);
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(64, 20, "v" FW_VERSION);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.display();
}

/* File selection menu ------------------------------------------------- */
void display_fileMenu(const char* files[], uint8_t count,
                      uint8_t selected, uint8_t topIndex) {
    oled.clear();
    oled.setFont(ArialMT_Plain_10);

    for (uint8_t i = 0; i < LINES_PER_SCREEN && (topIndex + i) < count; i++) {
        uint8_t idx = topIndex + i;
        char line[CHARS_PER_LINE + 3];

        if (idx == selected) {
            snprintf(line, sizeof(line), "> %s", files[idx]);
        } else {
            snprintf(line, sizeof(line), "  %s", files[idx]);
        }
        // Truncate to fit screen
        line[CHARS_PER_LINE + 2] = '\0';
        oled.drawString(0, i * FONT_HEIGHT, line);
    }
    oled.display();
}

/* Reader page — show N lines of text --------------------------------- */
void display_readerPage(const char* lines[], uint8_t count) {
    oled.clear();
    oled.setFont(ArialMT_Plain_10);

    for (uint8_t i = 0; i < count && i < LINES_PER_SCREEN; i++) {
        oled.drawString(0, i * FONT_HEIGHT, lines[i]);
    }
    oled.display();
}

/* Generic 1-2 line message -------------------------------------------- */
void display_message(const char* line1, const char* line2) {
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);

    if (line2 == nullptr) {
        oled.drawString(64, 12, line1);
    } else {
        oled.drawString(64, 4, line1);
        oled.drawString(64, 20, line2);
    }
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.display();
}

/* Progress bar with label --------------------------------------------- */
void display_progress(const char* label, uint8_t percent) {
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 0, label);

    // Draw progress bar frame
    oled.drawRect(4, 18, 120, 10);
    // Fill
    uint16_t fillW = (uint16_t)(116.0f * percent / 100.0f);
    oled.fillRect(6, 20, fillW, 6);

    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.display();
}

/* WiFi info screen ---------------------------------------------------- */
void display_wifiInfo(const char* ssid, const char* ip) {
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(0, 0, "WiFi Upload Mode");
    
    char buf[32];
    snprintf(buf, sizeof(buf), "SSID: %s", ssid);
    oled.drawString(0, 10, buf);
    
    snprintf(buf, sizeof(buf), "IP: %s", ip);
    oled.drawString(0, 20, buf);
    oled.display();
}

// ═══════════════════════════════════════════════════════════════════════
//  Utilities
// ═══════════════════════════════════════════════════════════════════════

void display_setBrightness(uint8_t val) {
    oled.setBrightness(val);
}
