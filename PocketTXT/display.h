/*
 * ============================================================================
 *  PocketTXT — Display Module Header
 * ============================================================================
 *  SSD1306 128×32 OLED display functions using U8g2lib.
 * ============================================================================
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

// Initialize OLED display on custom I2C pins
void display_init();

// Show splash screen at boot
void display_splash();

// Show error message (centered)
void display_error(const char* line1, const char* line2 = nullptr);

// ── File Menu ──
// Draw file selection menu
// selectedIndex: currently highlighted file
// topIndex: first visible file in the scrolled view
// fileNames: array of filenames
// fileCount: total number of files
void display_fileMenu(int selectedIndex, int topIndex,
                      const char* fileNames[], int fileCount);

// ── Reading View ──
// Draw reading screen with filename header and wrapped text
// filename: current file name
// lines: array of wrapped text lines to display
// lineCount: number of lines in array
// scrollPos: current scroll position (for scroll indicator)
// totalLines: total wrapped lines in file (for scroll indicator)
void display_reading(const char* filename, const char* lines[],
                     int lineCount, int scrollPos, int totalLines);

// ── WiFi Portal Screen ──
void display_wifiInfo(const char* ssid, const char* password, const char* ip);

// ── Utility ──
void display_setInverted(bool inverted);
void display_sleep();
void display_wake();
void display_clear();

#endif // DISPLAY_H
