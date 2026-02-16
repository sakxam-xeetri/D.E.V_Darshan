/*  =========================================================================
 *  DEV_Darshan — Display Manager (Header)
 *  SSD1306 128×32 OLED via I2C
 *  =========================================================================
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// ── Lifecycle ──────────────────────────────────────────────────────────
void display_init();

// ── Primitives ─────────────────────────────────────────────────────────
void display_clear();
void display_show();                        // push frame buffer to OLED
void display_text(uint8_t line, const char* text);   // 0-based line index
void display_textSmall(uint8_t line, const char* text);

// ── High-level screens ─────────────────────────────────────────────────
void display_splash();                      // boot splash
void display_fileMenu(const char* files[], uint8_t count,
                      uint8_t selected, uint8_t topIndex);
void display_readerPage(const char* lines[], uint8_t count);
void display_message(const char* line1, const char* line2 = nullptr);
void display_progress(const char* label, uint8_t percent);
void display_wifiInfo(const char* ssid, const char* ip);

// ── Utilities ──────────────────────────────────────────────────────────
void display_setBrightness(uint8_t val);    // 0-255

#endif // DISPLAY_MANAGER_H
