/*
 * ============================================================================
 *  PocketTXT — Main Firmware
 * ============================================================================
 *  Ultra-compact offline TXT reader built on ESP32-CAM (AI Thinker).
 *
 *  Hardware:
 *    - ESP32-CAM (camera module NOT used — repurposed for SD + GPIO)
 *    - 0.91" SSD1306 OLED (128×32, I2C on GPIO1/GPIO3)
 *    - SD card via SD_MMC 1-bit mode (built-in slot, zero resistors)
 *    - Two tactile buttons (GPIO13=UP, GPIO0=DOWN)
 *    - Both buttons use internal pull-ups — zero resistors
 *    - 3.7V 1100mAh Li-ion + TP4056 charger
 *    - Magnetic reed switch for hardware power control
 *
 *  Controls:
 *    - Short UP/DOWN → scroll text or navigate menu
 *    - Hold UP 2s → select file (menu) / toggle invert (reading)
 *    - Hold DOWN 2s → back to menu (reading) / exit portal (WiFi)
 *    - Hold UP+DOWN 2s → open WiFi upload portal
 *
 *  Author:  PocketTXT Project
 *  Version: 1.0.0
 *  License: MIT
 * ============================================================================
 */

#include "config.h"
#include "display.h"
#include "sd_reader.h"
#include "buttons.h"
#include "wifi_portal.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>

// ─── Application State ───────────────────────────────────────────────────────

static AppState appState = STATE_BOOT;

// File menu state
static int menuSelectedIndex = 0;
static int menuTopIndex      = 0;
static const char* fileNamePtrs[MAX_FILES];  // Pointer array for display module

// Reading state
static char currentFileName[MAX_FILENAME_LEN] = {0};
static int  scrollPosition   = 0;      // Current top wrapped line
static int  totalLines       = 0;      // Total wrapped lines in file
static int  scrollCounter    = 0;      // For periodic bookmark save
static bool displayInverted  = false;

// Display line buffers (for reading view)
static char displayLines[TEXT_LINES][CHARS_PER_LINE + 1];
static const char* displayLinePtrs[TEXT_LINES];

// Activity tracking
static unsigned long lastActivityMs = 0;
static bool displayAsleep = false;

// ─── Power Management ────────────────────────────────────────────────────────

static void disableRadios() {
    // Disable WiFi — use safe high-level API only
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // NOTE: Do NOT call esp_wifi_stop() directly — it can crash if WiFi
    // subsystem wasn't fully initialized. WiFi.mode(WIFI_OFF) handles it.

    // Disable Bluetooth — btStop() is sufficient and safe
    btStop();
    // NOTE: Do NOT call esp_bt_controller_disable() directly.
}

// Note: GPIO4 flash LED is disabled at boot (OUTPUT LOW) to save power.

static void disableFlashLED() {
    pinMode(PIN_FLASH_LED, OUTPUT);
    digitalWrite(PIN_FLASH_LED, LOW);
}

static void setLowPowerMode() {
    setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);
}

// ─── Diagnostic LED Helpers (GPIO33 onboard red LED, ACTIVE LOW) ─────────
// These blinks let you confirm the firmware is running even without
// Serial output (since TX/RX pins are repurposed for OLED I2C).

static void diagLedInit() {
    pinMode(PIN_LED_DIAG, OUTPUT);
    digitalWrite(PIN_LED_DIAG, HIGH);  // OFF (active LOW)
}

static void diagBlink(int count, int onMs = 100, int offMs = 100) {
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_LED_DIAG, LOW);   // ON
        delay(onMs);
        digitalWrite(PIN_LED_DIAG, HIGH);  // OFF
        if (i < count - 1) delay(offMs);
    }
}

static void resetActivityTimer() {
    lastActivityMs = millis();
    if (displayAsleep) {
        display_wake();
        displayAsleep = false;
    }
}

static void checkIdleTimeout() {
    if (appState == STATE_WIFI_PORTAL) return;  // Don't sleep in portal mode

    unsigned long now = millis();
    if (!displayAsleep && (now - lastActivityMs) > DISPLAY_TIMEOUT_MS) {
        display_sleep();
        displayAsleep = true;
    }
}

// ─── File Menu Logic ─────────────────────────────────────────────────────────

static void enterFileMenu() {
    appState = STATE_FILE_MENU;
    menuSelectedIndex = 0;
    menuTopIndex = 0;

    int count = sd_scanFiles();
    if (count == 0) {
        display_error("No TXT files", "Hold both: WiFi");
        return;
    }

    // Build pointer array for display
    for (int i = 0; i < count && i < MAX_FILES; i++) {
        fileNamePtrs[i] = sd_getFileName(i);
    }

    display_fileMenu(menuSelectedIndex, menuTopIndex, fileNamePtrs, count);
}

static void updateFileMenu() {
    int count = sd_getFileCount();
    if (count == 0) return;

    display_fileMenu(menuSelectedIndex, menuTopIndex, fileNamePtrs, count);
}

static void menuScrollUp() {
    int count = sd_getFileCount();
    if (count == 0) return;

    if (menuSelectedIndex > 0) {
        menuSelectedIndex--;
        if (menuSelectedIndex < menuTopIndex) {
            menuTopIndex = menuSelectedIndex;
        }
        updateFileMenu();
    }
}

static void menuScrollDown() {
    int count = sd_getFileCount();
    if (count == 0) return;

    if (menuSelectedIndex < count - 1) {
        menuSelectedIndex++;
        if (menuSelectedIndex >= menuTopIndex + DISPLAY_LINES) {
            menuTopIndex = menuSelectedIndex - DISPLAY_LINES + 1;
        }
        updateFileMenu();
    }
}

static void menuSelectFile() {
    int count = sd_getFileCount();
    if (count == 0) return;

    const char* name = sd_getFileName(menuSelectedIndex);
    strncpy(currentFileName, name, MAX_FILENAME_LEN - 1);
    currentFileName[MAX_FILENAME_LEN - 1] = '\0';

    if (!sd_openFile(currentFileName)) {
        display_error("Open failed", currentFileName);
        delay(2000);
        enterFileMenu();
        return;
    }

    // Get total wrapped lines
    totalLines = sd_getTotalWrappedLines();
    if (totalLines <= 0) {
        display_error("Empty file", currentFileName);
        sd_closeFile();
        delay(2000);
        enterFileMenu();
        return;
    }

    // Load bookmark (or start at beginning)
    int savedPos = sd_loadBookmark(currentFileName);
    scrollPosition = (savedPos >= 0 && savedPos < totalLines) ? savedPos : 0;
    scrollCounter = 0;

    appState = STATE_READING;
    updateReadingView();
}

// ─── Reading Logic ───────────────────────────────────────────────────────────

static void updateReadingView() {
    // Read current lines from file
    int read = sd_readWrappedLines(scrollPosition, displayLines, TEXT_LINES);

    // Build pointer array
    for (int i = 0; i < TEXT_LINES; i++) {
        displayLinePtrs[i] = (i < read) ? displayLines[i] : "";
    }

    display_reading(currentFileName, displayLinePtrs,
                    read, scrollPosition, totalLines);
}

static void readingScrollUp() {
    if (scrollPosition > 0) {
        scrollPosition--;
        scrollCounter++;
        updateReadingView();

        if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
            sd_saveBookmark(currentFileName, scrollPosition);
            scrollCounter = 0;
        }
    }
}

static void readingScrollDown() {
    if (scrollPosition < totalLines - TEXT_LINES) {
        scrollPosition++;
        scrollCounter++;
        updateReadingView();

        if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
            sd_saveBookmark(currentFileName, scrollPosition);
            scrollCounter = 0;
        }
    }
}

static void exitReading() {
    // Save final bookmark position
    sd_saveBookmark(currentFileName, scrollPosition);
    sd_closeFile();
    enterFileMenu();
}

// ─── WiFi Portal Logic ──────────────────────────────────────────────────────

static void enterWifiPortal() {
    appState = STATE_WIFI_PORTAL;

    display_wifiInfo(WIFI_SSID, WIFI_PASSWORD, PORTAL_IP);

    if (!portal_start()) {
        display_error("WiFi Failed", "Try again");
        delay(2000);
        disableRadios();
        enterFileMenu();
        return;
    }
}

static void exitWifiPortal() {
    portal_stop();
    disableRadios();

    // Re-scan files (new files may have been uploaded)
    enterFileMenu();
}

// ─── Arduino Setup ───────────────────────────────────────────────────────────

void setup() {
    // ── 0. Diagnostic LED — confirms firmware is running ──
    diagLedInit();
    diagBlink(1);  // 1 blink = firmware started

    // ── 1. Disable radios IMMEDIATELY for power saving ──
    disableRadios();
    setLowPowerMode();
    disableFlashLED();
    diagBlink(2);  // 2 blinks = radios off OK

    // ── 2. CRITICAL: Release UART0 so GPIO1/GPIO3 can be used for I2C ──
    // The ESP32 bootloader uses GPIO1 (TX) / GPIO3 (RX) as UART0.
    // We must explicitly end Serial to free these pins for OLED I2C.
    Serial.end();
    delay(50);  // Let UART peripheral fully release

    // ── 3. Initialize buttons BEFORE SD_MMC (GPIO13 shared) ──
    buttons_init();
    buttons_readEarlyState();

    // Small delay for hardware stabilization
    delay(100);
    diagBlink(3);  // 3 blinks = buttons init OK

    // ── 4. Initialize OLED display ──
    display_init();
    display_splash();
    delay(1500);  // Show splash for 1.5 seconds
    diagBlink(4);  // 4 blinks = display init OK

    // ── 5. Initialize SD card ──
    if (!sd_init()) {
        display_error("SD Card Error", "Insert & restart");
        appState = STATE_ERROR;
        diagBlink(10, 50, 50);  // Rapid blinks = SD error
        return;
    }
    diagBlink(5);  // 5 blinks = SD card OK

    // ── 6. Enter file menu ──
    lastActivityMs = millis();
    enterFileMenu();
}

// ─── Arduino Main Loop ──────────────────────────────────────────────────────

void loop() {
    // ── Handle WiFi portal client requests ──
    if (appState == STATE_WIFI_PORTAL) {
        portal_handleClient();
    }

    // ── Read button input ──
    ButtonEvent event = buttons_update();

    // ── Wake display on any button activity ──
    if (event != BTN_NONE) {
        resetActivityTimer();

        // If display was asleep, consume the event (just wakes up)
        if (displayAsleep) {
            return;
        }
    }

    // ── Handle combo press (WiFi portal) from any state ──
    if (event == BTN_BOTH_LONG) {
        if (appState == STATE_WIFI_PORTAL) {
            // Already in portal — ignore
        } else {
            // Close file if open
            if (sd_isFileOpen()) {
                sd_saveBookmark(currentFileName, scrollPosition);
                sd_closeFile();
            }
            enterWifiPortal();
        }
        return;
    }

    // ── State-specific event handling ──
    switch (appState) {

        case STATE_FILE_MENU:
            switch (event) {
                case BTN_UP_SHORT:
                    menuScrollUp();
                    break;
                case BTN_DOWN_SHORT:
                    menuScrollDown();
                    break;
                case BTN_UP_LONG:
                    menuSelectFile();   // Hold UP = open file
                    break;
                case BTN_UP_HELD:
                    menuScrollUp();     // Fast scroll in menu
                    break;
                case BTN_DOWN_HELD:
                    menuScrollDown();
                    break;
                default:
                    break;
            }
            break;

        case STATE_READING:
            switch (event) {
                case BTN_UP_SHORT:
                    readingScrollUp();
                    break;
                case BTN_DOWN_SHORT:
                    readingScrollDown();
                    break;
                case BTN_UP_LONG:
                    // Toggle inverted display
                    displayInverted = !displayInverted;
                    display_setInverted(displayInverted);
                    updateReadingView();
                    break;
                case BTN_DOWN_LONG:
                    exitReading();      // Hold DOWN = back to menu
                    break;
                case BTN_UP_HELD:
                    readingScrollUp();   // Fast scroll
                    break;
                case BTN_DOWN_HELD:
                    readingScrollDown(); // Fast scroll
                    break;
                default:
                    break;
            }
            break;

        case STATE_WIFI_PORTAL:
            if (event == BTN_DOWN_LONG) {
                exitWifiPortal();   // Hold DOWN = exit portal
            }
            break;

        case STATE_ERROR:
            // In error state, allow WiFi portal access via combo
            // (already handled above). No other actions.
            break;

        case STATE_BOOT:
            // Should not reach here — setup transitions to FILE_MENU
            break;
    }

    // ── Idle timeout check ──
    checkIdleTimeout();

    // ── Small delay to prevent CPU spinning ──
    delay(5);
}
