/*
 * ============================================================================
 *  D.E.V_Darshan — Configuration Header
 * ============================================================================
 *  All pin assignments, timing constants, display settings, and WiFi
 *  credentials. Modify this file to customize your build.
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  VERSION & IDENTITY
// ─────────────────────────────────────────────────────────────────────────────
#define FW_VERSION        "1.0"
#define DEVICE_NAME       "D.E.V_Darshan"
#define DEVELOPER_NAME    "Sakshyam Bastakoti"

// ─────────────────────────────────────────────────────────────────────────────
//  GPIO PIN MAPPING (ESP32-CAM AI Thinker — ZERO External Resistors)
// ─────────────────────────────────────────────────────────────────────────────
//
//  SD_MMC 1-BIT MODE → only GPIO2/14/15 used → GPIO4/12/13 freed.
//
//    GPIO0  — BTN_DOWN   (internal pull-up) ⚠️ Don't hold during power-on
//    GPIO1  — I2C SCL    (OLED module has pull-up)
//    GPIO2  — SD_MMC D0  (SD driver enables internal pull-up)
//    GPIO3  — I2C SDA    (OLED module has pull-up)
//    GPIO4  — Flash LED  (disabled in firmware — OUTPUT LOW)
//    GPIO12 — BTN_SELECT (internal pull-up, boot-safe: active LOW = 3.3V)
//    GPIO13 — BTN_UP     (internal pull-up)
//    GPIO14 — SD_MMC CLK
//    GPIO15 — SD_MMC CMD (SD driver enables internal pull-up)
//

// I2C — OLED Display (SSD1306 128×32)
#define PIN_SDA           3     // GPIO3 (U0RXD repurposed)
#define PIN_SCL           1     // GPIO1 (U0TXD repurposed)
#define OLED_ADDRESS      0x3C
#define OLED_WIDTH        128
#define OLED_HEIGHT       32

// Buttons (active LOW — all use internal pull-ups)
#define PIN_BTN_UP        13    // GPIO13
#define PIN_BTN_DOWN      0     // GPIO0 ⚠️ Don't hold during power-on
#define PIN_BTN_SELECT    12    // GPIO12 — boot-safe (active LOW = 3.3V flash)

// Flash LED
#define PIN_FLASH_LED     4     // GPIO4 — disabled at boot

// ─────────────────────────────────────────────────────────────────────────────
//  BUTTON TIMING (milliseconds)
// ─────────────────────────────────────────────────────────────────────────────
#define DEBOUNCE_MS           30      // Debounce window
#define SCROLL_HOLD_MS        400     // UP/DOWN continuous scroll threshold
#define FAST_SCROLL_INTERVAL  80      // Repeat interval during held scroll

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define CHARS_PER_LINE        32      // 128px ÷ 4px per char (4×6 monospace font)
#define MENU_LINES            4       // Visible items in file list menu (5x7 font)
#define READING_LINES         4       // Lines in full-screen reading mode
#define HOME_ITEMS            3       // Home menu entries
#define SETTINGS_ITEMS        4       // Settings menu entries (incl. Back)

// Sleep timeout (auto sleep after inactivity)
#define DISPLAY_TIMEOUT_MS    300000  // 5 min before auto light-sleep

// ─────────────────────────────────────────────────────────────────────────────
//  SD CARD SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define SD_MOUNT_RETRIES      3
#define SD_RETRY_DELAY_MS     500
#define MAX_FILES             50
#define MAX_FILENAME_LEN      32

// ─────────────────────────────────────────────────────────────────────────────
//  TEXT READER SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define LINE_BUFFER_SIZE      128     // Max chars per raw line from file
#define DISPLAY_BUFFER_LINES  20
#define BOOKMARK_SAVE_EVERY   10

// ─────────────────────────────────────────────────────────────────────────────
//  WIFI PORTAL SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define WIFI_SSID             "TXT_Reader"
#define WIFI_PASSWORD         "readmore"
#define WIFI_CHANNEL          6
#define MAX_UPLOAD_SIZE       2097152 // 2MB
#define PORTAL_IP             "192.168.4.1"
#define WIFI_TIMEOUT_MS       300000  // 5 min WiFi inactivity auto-shutdown

// ─────────────────────────────────────────────────────────────────────────────
//  POWER SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define LOW_POWER_CPU_MHZ     80
#define NORMAL_CPU_MHZ        240

// ─────────────────────────────────────────────────────────────────────────────
//  DIAGNOSTIC LED (ESP32-CAM onboard red LED on GPIO33, ACTIVE LOW)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_LED_DIAG          33

// ─────────────────────────────────────────────────────────────────────────────
//  APPLICATION STATES
// ─────────────────────────────────────────────────────────────────────────────
enum AppState {
    STATE_BOOT,
    STATE_HOME,               // Home menu: WiFi Portal, Files, Settings
    STATE_FILE_MENU,          // File selection list
    STATE_READING,            // Full-screen immersive reading
    STATE_WIFI_PORTAL,        // WiFi upload portal active
    STATE_SETTINGS,           // Settings menu
    STATE_SETTINGS_INFO,      // System Info sub-screen
    STATE_SETTINGS_FILES,     // File Count sub-screen
    STATE_SETTINGS_STORAGE,   // Storage sub-screen
    STATE_ERROR
};

// ─────────────────────────────────────────────────────────────────────────────
//  BUTTON EVENT TYPES
// ─────────────────────────────────────────────────────────────────────────────
enum ButtonEvent {
    BTN_NONE,
    BTN_UP_SHORT,             // UP short press
    BTN_DOWN_SHORT,           // DOWN short press
    BTN_SELECT_SHORT,         // SELECT short press → Enter / Back (context-aware)
    BTN_UP_LONG,              // UP held past SCROLL_HOLD_MS (continuous scroll start)
    BTN_DOWN_LONG,            // DOWN held past SCROLL_HOLD_MS
    BTN_UP_HELD,              // UP still held (continuous scroll repeat)
    BTN_DOWN_HELD             // DOWN still held (fast scroll repeat)
};

#endif // CONFIG_H
