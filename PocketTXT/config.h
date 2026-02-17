/*
 * ============================================================================
 *  PocketTXT — Configuration Header
 * ============================================================================
 *  All pin assignments, timing constants, display settings, and WiFi
 *  credentials in one place. Modify this file to customize your build.
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  VERSION
// ─────────────────────────────────────────────────────────────────────────────
#define FW_VERSION        "1.0.0"
#define DEVICE_NAME       "PocketTXT"

// ─────────────────────────────────────────────────────────────────────────────
//  GPIO PIN MAPPING (ESP32-CAM AI Thinker — ZERO External Resistors)
// ─────────────────────────────────────────────────────────────────────────────
//
//  SD_MMC runs in 1-BIT MODE → only GPIO2/14/15 used → GPIO4/12/13 freed.
//  This eliminates the critical GPIO12 pull-down resistor entirely.
//  All pull-ups are internal or provided by the OLED module's breakout board.
//
//    GPIO0  — BTN_DOWN (internal pull-up) ⚠️ Don't hold during power-on
//    GPIO1  — Repurposed from UART TX → I2C SCL (OLED module has pull-up)
//    GPIO2  — SD_MMC DATA0 (SD driver enables internal pull-up)
//    GPIO3  — Repurposed from UART RX → I2C SDA (OLED module has pull-up)
//    GPIO4  — Flash LED (disabled in firmware — OUTPUT LOW)
//    GPIO12 — FREE (not used in 1-bit SD mode — no pull-down needed)
//    GPIO13 — BTN_UP (internal pull-up)
//    GPIO14 — SD_MMC CLK
//    GPIO15 — SD_MMC CMD (SD driver enables internal pull-up)
//    GPIO16 — FREE
//

// I2C — OLED Display (SSD1306 128×32)
#define PIN_SDA           3     // GPIO3 (U0RXD repurposed)
#define PIN_SCL           1     // GPIO1 (U0TXD repurposed)
#define OLED_ADDRESS      0x3C  // Default SSD1306 I2C address
#define OLED_WIDTH        128
#define OLED_HEIGHT       32

// Buttons (active LOW — both use internal pull-ups, ZERO external resistors)
#define PIN_BTN_UP        13    // GPIO13 — internal pull-up (free in 1-bit SD mode)
#define PIN_BTN_DOWN      0     // GPIO0  — internal pull-up ⚠️ Don't hold during power-on

// Flash LED
#define PIN_FLASH_LED     4     // GPIO4 — onboard flash LED (disabled at boot)

// SD_MMC Pins — 1-BIT MODE (only 3 pins used)
// GPIO14 = CLK, GPIO15 = CMD, GPIO2 = D0
// GPIO4 = Flash LED (OFF), GPIO12/16 = FREE

// ─────────────────────────────────────────────────────────────────────────────
//  BUTTON TIMING (milliseconds)
// ─────────────────────────────────────────────────────────────────────────────
#define DEBOUNCE_MS           50      // Debounce window
#define LONG_PRESS_MS         2000    // Hold threshold for long press
#define FAST_SCROLL_INTERVAL  150     // Repeat interval during long hold scroll
#define COMBO_PRESS_MS        2000    // Hold BOTH buttons threshold

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define CHARS_PER_LINE        21      // 128 pixels ÷ 6px per char
#define DISPLAY_LINES         4       // Total lines on 128×32 display
#define TEXT_LINES            3       // Lines available for text (1 for header)
#define FONT_WIDTH            6       // Pixel width of body font
#define FONT_HEIGHT           10      // Pixel height of body font
#define HEADER_FONT_HEIGHT    7       // Pixel height of header font
#define SCROLL_BAR_WIDTH      2       // Scroll indicator width in pixels

// Display timeout
#define DISPLAY_TIMEOUT_MS    60000   // 60 seconds before dimming
#define IDLE_SLEEP_MS         300000  // 5 minutes before deep sleep (optional)

// ─────────────────────────────────────────────────────────────────────────────
//  SD CARD SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define SD_MOUNT_RETRIES      3       // Number of SD mount attempts
#define SD_RETRY_DELAY_MS     500     // Delay between mount retries
#define MAX_FILES             50      // Maximum number of files in menu
#define MAX_FILENAME_LEN      32      // Maximum displayed filename length

// ─────────────────────────────────────────────────────────────────────────────
//  TEXT READER SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define LINE_BUFFER_SIZE      128     // Max characters per raw line from file
#define DISPLAY_BUFFER_LINES  20      // Circular buffer size for wrapped lines
#define BOOKMARK_SAVE_EVERY   10      // Save bookmark every N scroll actions

// ─────────────────────────────────────────────────────────────────────────────
//  WIFI PORTAL SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define WIFI_SSID             "TXT_Reader"
#define WIFI_PASSWORD         "readmore"
#define WIFI_CHANNEL          6       // WiFi channel (1-13)
#define MAX_UPLOAD_SIZE       2097152 // 2MB max file upload size
#define PORTAL_IP             "192.168.4.1"

// ─────────────────────────────────────────────────────────────────────────────
//  POWER SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
#define LOW_POWER_CPU_MHZ     80      // CPU frequency for reading mode
#define NORMAL_CPU_MHZ        240     // CPU frequency for WiFi mode

// ─────────────────────────────────────────────────────────────────────────────
//  DIAGNOSTIC LED (ESP32-CAM onboard red LED on GPIO33, ACTIVE LOW)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_LED_DIAG          33      // Onboard red LED (active LOW)

// ─────────────────────────────────────────────────────────────────────────────
//  APPLICATION STATES
// ─────────────────────────────────────────────────────────────────────────────
enum AppState {
    STATE_BOOT,           // Initial boot and hardware init
    STATE_FILE_MENU,      // File selection menu
    STATE_READING,        // Text reading mode
    STATE_WIFI_PORTAL,    // WiFi upload portal active
    STATE_ERROR           // Error display
};

// ─────────────────────────────────────────────────────────────────────────────
//  BUTTON EVENT TYPES
// ─────────────────────────────────────────────────────────────────────────────
enum ButtonEvent {
    BTN_NONE,             // No event
    BTN_UP_SHORT,         // UP button short press
    BTN_DOWN_SHORT,       // DOWN button short press
    BTN_UP_LONG,          // UP button held 2s
    BTN_DOWN_LONG,        // DOWN button held 2s
    BTN_BOTH_LONG,        // UP+DOWN held 2s → WiFi portal
    BTN_UP_HELD,          // UP button still held (fast scroll)
    BTN_DOWN_HELD         // DOWN button still held (fast scroll)
};

#endif // CONFIG_H
