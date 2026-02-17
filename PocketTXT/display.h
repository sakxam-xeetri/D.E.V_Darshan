/*
 * ============================================================================
 *  D.E.V_Darshan — Display Module Header
 * ============================================================================
 *  SSD1306 128×32 OLED display functions using U8g2lib.
 *  Clean, distraction-free UI for an immersive reading device.
 * ============================================================================
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

// Initialize OLED display on custom I2C pins
void display_init();

// ── Boot & Home ──
void display_boot();
void display_home(int selectedIndex);

// ── File Menu ──
void display_fileMenu(int selectedIndex, int topIndex,
                      const char* fileNames[], int fileCount);

// ── Reading Mode (Full Immersion — no header, no UI chrome) ──
void display_reading(const char* lines[], int lineCount);

// ── WiFi Portal Screen ──
void display_wifiPortal(const char* ssid, const char* ip);

// ── Settings Screens ──
void display_settingsMenu(int selectedIndex);
void display_systemInfo(bool sdMounted);
void display_fileCount(int count);
void display_storageInfo(float usedMB, float freeMB);

// ── Utility ──
void display_error(const char* line1, const char* line2 = nullptr);
void display_setInverted(bool inverted);
void display_sleep();
void display_wake();
void display_clear();

#endif // DISPLAY_H
