/*  =========================================================================
 *  DEV_Darshan — Master Configuration & GPIO Mapping
 *  =========================================================================
 *
 *  ┌──────────────────────────────────────────────────────────────────────┐
 *  │  ESP32-CAM (AI Thinker) — SAFE GPIO MAP FOR THIS PROJECT           │
 *  ├────────────┬─────────────┬──────────────────────────────────────────┤
 *  │  Function  │  GPIO       │  Notes                                   │
 *  ├────────────┼─────────────┼──────────────────────────────────────────┤
 *  │  OLED SDA  │  GPIO 14    │  Safe. No boot constraints.             │
 *  │  OLED SCL  │  GPIO 15    │  Must be HIGH at boot (OLED pulls HIGH) │
 *  │  SD_MMC D0 │  GPIO 2     │  Must be HIGH at boot (SD pullup OK)    │
 *  │  SD_MMC D1 │  GPIO 4     │  (Also onboard flash LED — unused)      │
 *  │  SD_MMC D2 │  GPIO 12    │  MUST be LOW at boot — see note below   │
 *  │  SD_MMC D3 │  GPIO 13    │  Safe, used as SD CS/D3                 │
 *  │  SD_MMC CLK│  GPIO 14    │  Shared with SDA — see I2C remap below  │
 *  │  BTN UP    │  GPIO 16    │  Safe, no boot constraint               │
 *  │  BTN DOWN  │  GPIO 3 (RX)│  Safe after boot (UART0 RX)            │
 *  │  BTN SEL   │  GPIO 1 (TX)│  Safe after boot (UART0 TX)            │
 *  ├────────────┴─────────────┴──────────────────────────────────────────┤
 *  │  KEY BOOT CONSTRAINTS:                                              │
 *  │  • GPIO 0  — Must be HIGH at boot (not exposed, left floating)     │
 *  │  • GPIO 2  — Must be HIGH at boot → 10 kΩ pull-up recommended      │
 *  │  • GPIO 12 — MUST be LOW at boot (controls flash voltage)           │
 *  │             → 10 kΩ pull-down on GPIO 12                            │
 *  │  • GPIO 15 — Must be HIGH at boot → 10 kΩ pull-up recommended      │
 *  │                                                                      │
 *  │  SD_MMC vs I2C CONFLICT RESOLUTION:                                 │
 *  │  The built-in SD slot uses GPIO 14 for CLK. We remap I2C to        │
 *  │  GPIO 14 (SDA) and GPIO 15 (SCL) ONLY when SD is NOT active.       │
 *  │  In practice we init SD first, read a chunk, unmount, then refresh  │
 *  │  display — or we use 1-bit SD_MMC mode freeing GPIO 4,12,13 and    │
 *  │  remap I2C to non-conflicting pins.                                 │
 *  │                                                                      │
 *  │  *** CHOSEN STRATEGY: SD_MMC 1-BIT MODE ***                        │
 *  │  Uses ONLY GPIO 2 (DATA0) and GPIO 14 (CLK).                       │
 *  │  GPIO 15 (CMD) is still needed by SD_MMC.                           │
 *  │  This FREES GPIO 4, 12, 13 for other use.                          │
 *  │  We use GPIO 13 (SDA) and GPIO 12 (SCL) for I2C.                   │
 *  │  GPIO 12 is safe AFTER boot (just must be LOW DURING boot).         │
 *  │  10 kΩ pull-down on GPIO 12 ensures safe boot, then I2C pull-ups   │
 *  │  (internal or 4.7 kΩ) drive it for SCL after init.                  │
 *  └──────────────────────────────────────────────────────────────────────┘
 *
 *  FINAL ACTIVE PIN ASSIGNMENT (1-bit SD_MMC) — NO EXTERNAL RESISTORS:
 *  ┌────────────┬──────────┬───────────────────────────────────────┐
 *  │  Function  │  GPIO    │  Pull resistor                        │
 *  ├────────────┼──────────┼───────────────────────────────────────┤
 *  │  SD DATA0  │  GPIO 2  │  SD card built-in pull-up (boot HIGH) │
 *  │  SD CLK    │  GPIO 14 │  —  (shared with I2C SCL)             │
 *  │  SD CMD    │  GPIO 15 │  SD card built-in pull-up (boot HIGH) │
 *  │  OLED SDA  │  GPIO 13 │  OLED module built-in pull-up         │
 *  │  OLED SCL  │  GPIO 14 │  OLED module built-in pull-up         │
 *  │            │          │  (shared with SD CLK — no conflict)   │
 *  │  BTN UP    │  GPIO 16 │  ESP32 internal pull-up               │
 *  │  BTN DOWN  │  GPIO 3  │  ESP32 internal pull-up               │
 *  │  BTN SEL   │  GPIO 1  │  ESP32 internal pull-up               │
 *  └────────────┴──────────┴───────────────────────────────────────┘
 *
 *  HARDWARE NOTES:
 *  • NO EXTERNAL RESISTORS NEEDED — all pull-ups/downs are built-in!
 *  • 470 µF electrolytic cap across 3.3 V rail — prevents brownout on
 *    WiFi TX bursts and SD card inrush current.
 *  • Magnetic reed switch cuts battery positive line → true hardware OFF.
 *  • TP4056 module with DW01A protection IC for safe Li-ion charging.
 *  • GPIO 14 is shared between SD CLK and I2C SCL — works fine since
 *    OLED init happens before SD mount, and display updates happen
 *    between SD reads (not simultaneously).
 *  =========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── Project identity ────────────────────────────────────────────────────
#define PROJECT_NAME     "DEV_Darshan"
#define FW_VERSION       "1.0.0"

// ── Display (SSD1306 128×32, I2C) ──────────────────────────────────────
#define OLED_WIDTH       128
#define OLED_HEIGHT      32
#define OLED_I2C_ADDR    0x3C
#define PIN_SDA          13
#define PIN_SCL          14      // Shared with SD CLK — no conflict

// ── SD Card (SD_MMC 1-bit mode) ────────────────────────────────────────
// In 1-bit mode only DATA0 (GPIO2), CLK (GPIO14), CMD (GPIO15) are used.
// No extra pin defines needed — the SD_MMC driver handles them internally.

// ── Buttons (active LOW — press connects to GND) ───────────────────────
#define PIN_BTN_UP       16
#define PIN_BTN_DOWN      3      // GPIO3 = UART0 RX — safe after boot
#define PIN_BTN_SELECT    1      // GPIO1 = UART0 TX — safe after boot

// ── Button timing (milliseconds) ───────────────────────────────────────
#define DEBOUNCE_MS            50
#define LONG_PRESS_MS         800
#define REPEAT_INITIAL_MS     400
#define REPEAT_FAST_MS        100

// ── Display layout ─────────────────────────────────────────────────────
#define FONT_HEIGHT            8     // Pixel height of the 6×8 font
#define LINES_PER_SCREEN       4     // 32 px / 8 px = 4 lines
#define CHARS_PER_LINE        21     // 128 px / 6 px ≈ 21 chars

// ── WiFi Access Point ──────────────────────────────────────────────────
#define AP_SSID          "DEV_Reader"
#define AP_PASS          "read1234"     // min 8 chars for WPA2
#define AP_CHANNEL       1
#define AP_MAX_CONN      1              // single client saves RAM

// ── Reader engine ──────────────────────────────────────────────────────
#define READ_BUF_SIZE    512            // SD read buffer (bytes)
#define MAX_FILES        64             // max file entries in menu
#define MAX_FILENAME_LEN 32             // truncated display name length

// ── Power ──────────────────────────────────────────────────────────────
#define BATTERY_MV_FULL  4200
#define BATTERY_MV_EMPTY 3300

#endif // CONFIG_H
