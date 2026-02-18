/*
 * ============================================================================
 *  D.E.V_Darshan — Main Firmware
 * ============================================================================
 *  Ultra-compact offline TXT reader built on ESP32-CAM (AI Thinker).
 *
 *  Hardware:
 *    - ESP32-CAM (camera NOT used — repurposed for SD + GPIO)
 *    - 0.91" SSD1306 OLED (128×32, I2C on GPIO1/GPIO3)
 *    - SD card via SD_MMC 1-bit mode (built-in slot, zero resistors)
 *    - Three tactile buttons (GPIO13=UP, GPIO0=DOWN, GPIO12=SELECT)
 *    - 3.7V 1100mAh Li-ion + TP4056 charger
 *    - Magnetic reed switch for hardware power control
 *
 *  Navigation (context-aware SELECT, no long press):
 *    - UP            → Move up / scroll up
 *    - DOWN          → Move down / scroll down
 *    - SELECT short  → Enter (if sub-options) / Back (if terminal screen)
 *    - UP/DOWN hold  → Continuous line-by-line scroll (reading mode)
 *
 *  Screen Flow:
 *    Boot → Home → { WiFi Portal, Files → Reading, Settings → Sub-screens }
 *
 *  Author:  Sakshyam Bastakoti
 *  Version: 1.0
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
#include <esp_sleep.h>
#include <driver/gpio.h>

// ─── Application State ───────────────────────────────────────────────────────

static AppState appState = STATE_BOOT;

// Home & Settings menu cursors
static int homeIndex     = 0;
static int settingsIndex = 0;

// File menu state
static int menuSelectedIndex = 0;
static int menuTopIndex      = 0;
static const char* fileNamePtrs[MAX_FILES];

// Reading state
static char currentFileName[MAX_FILENAME_LEN] = {0};
static int  scrollPosition = 0;
static int  totalLines     = 0;
static int  scrollCounter  = 0;

// Display line buffers for reading view
static char displayLines[READING_LINES][CHARS_PER_LINE + 1];
static const char* displayLinePtrs[READING_LINES];

// Activity tracking
static unsigned long lastActivityMs = 0;
static bool displayAsleep = false;

// ─── Power Management ────────────────────────────────────────────────────────

static void disableRadios() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
}

static void disableFlashLED() {
    pinMode(PIN_FLASH_LED, OUTPUT);
    digitalWrite(PIN_FLASH_LED, LOW);
}

// ─── Diagnostic LED (GPIO33, active LOW) ─────────────────────────────────────

static void diagLedInit() {
    pinMode(PIN_LED_DIAG, OUTPUT);
    digitalWrite(PIN_LED_DIAG, HIGH);  // OFF
}

static void diagBlink(int count, int onMs = 100, int offMs = 100) {
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_LED_DIAG, LOW);   // ON
        delay(onMs);
        digitalWrite(PIN_LED_DIAG, HIGH);  // OFF
        if (i < count - 1) delay(offMs);
    }
}

// ─── Display Power ───────────────────────────────────────────────────────────

static void resetActivityTimer() {
    lastActivityMs = millis();
    if (displayAsleep) {
        display_wake();
        displayAsleep = false;
    }
}

static void enterLightSleep() {
    display_sleep();
    displayAsleep = true;

    // Configure GPIO wake sources (active LOW buttons)
    gpio_wakeup_enable((gpio_num_t)PIN_BTN_UP,     GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)PIN_BTN_DOWN,    GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)PIN_BTN_SELECT,  GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // Enter light sleep — execution pauses here
    esp_light_sleep_start();

    // ── Woke up! ──
    delay(100);           // Let hardware settle
    buttons_init();       // Reset button states (consume wake press)
    resetActivityTimer(); // Display ON, timer reset
}

static void checkIdleTimeout() {
    if (appState == STATE_WIFI_PORTAL) return;
    if (appState == STATE_BOOT) return;
    if (appState == STATE_ERROR) return;
    if (!displayAsleep && (millis() - lastActivityMs) > DISPLAY_TIMEOUT_MS) {
        enterLightSleep();
    }
}

// ─── Screen: Home ────────────────────────────────────────────────────────────

static void enterHome() {
    appState = STATE_HOME;
    homeIndex = 0;
    display_home(homeIndex);
}

// ─── Screen: File Menu ───────────────────────────────────────────────────────

static void enterFileMenu() {
    appState = STATE_FILE_MENU;

    int count = sd_scanFiles();
    
    // Always show menu with "< Back" at top
    menuSelectedIndex = 0;  // Start at "< Back"
    menuTopIndex = 0;

    for (int i = 0; i < count && i < MAX_FILES; i++) {
        fileNamePtrs[i] = sd_getFileName(i);
    }
    display_fileMenu(menuSelectedIndex, menuTopIndex, fileNamePtrs, count);
}

static void updateFileMenu() {
    int count = sd_getFileCount();
    display_fileMenu(menuSelectedIndex, menuTopIndex, fileNamePtrs, count);
}

static void menuScrollUp() {
    if (menuSelectedIndex <= 0) return;

    menuSelectedIndex--;
    if (menuSelectedIndex < menuTopIndex) {
        menuTopIndex = menuSelectedIndex;
    }
    updateFileMenu();
}

static void menuScrollDown() {
    int count = sd_getFileCount();
    
    // If no files, can't scroll down from "< Back"
    if (count == 0) return;
    
    int totalItems = count + 1;  // +1 for "< Back"
    if (menuSelectedIndex >= totalItems - 1) return;

    menuSelectedIndex++;
    if (menuSelectedIndex >= menuTopIndex + MENU_LINES) {
        menuTopIndex = menuSelectedIndex - MENU_LINES + 1;
    }
    updateFileMenu();
}

// ─── Screen: Reading (Full Immersion) ────────────────────────────────────────

static void updateReadingView() {
    int read = sd_readWrappedLines(scrollPosition, displayLines, READING_LINES);
    for (int i = 0; i < READING_LINES; i++) {
        displayLinePtrs[i] = (i < read) ? displayLines[i] : "";
    }
    display_reading(displayLinePtrs, read);
}

static void menuSelectFile() {
    int count = sd_getFileCount();
    int fileIndex = menuSelectedIndex - 1;  // Offset for "< Back" at 0
    if (count == 0 || fileIndex < 0 || fileIndex >= count) return;

    const char* name = sd_getFileName(fileIndex);
    strncpy(currentFileName, name, MAX_FILENAME_LEN - 1);
    currentFileName[MAX_FILENAME_LEN - 1] = '\0';

    if (!sd_openFile(currentFileName)) {
        display_error("Open failed", currentFileName);
        delay(2000);
        enterFileMenu();
        return;
    }

    totalLines = sd_getTotalWrappedLines();
    if (totalLines <= 0) {
        display_error("Empty file", currentFileName);
        sd_closeFile();
        delay(2000);
        enterFileMenu();
        return;
    }

    // Load bookmark or start at beginning
    int savedPos = sd_loadBookmark(currentFileName);
    scrollPosition = (savedPos >= 0 && savedPos < totalLines) ? savedPos : 0;
    scrollCounter = 0;

    appState = STATE_READING;
    updateReadingView();
}

static void readingPageUp() {
    if (totalLines <= READING_LINES) return;
    int maxPos = totalLines - READING_LINES;
    if (scrollPosition <= 0) {
        scrollPosition = maxPos;  // Wrap to end
    } else {
        scrollPosition -= READING_LINES;
        if (scrollPosition < 0) scrollPosition = 0;
    }
    scrollCounter += READING_LINES;
    updateReadingView();
    if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
        sd_saveBookmark(currentFileName, scrollPosition);
        scrollCounter = 0;
    }
}

static void readingPageDown() {
    if (totalLines <= READING_LINES) return;
    int maxPos = totalLines - READING_LINES;
    if (scrollPosition >= maxPos) {
        scrollPosition = 0;  // Wrap to beginning
    } else {
        scrollPosition += READING_LINES;
        if (scrollPosition > maxPos) scrollPosition = maxPos;
    }
    scrollCounter += READING_LINES;
    updateReadingView();
    if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
        sd_saveBookmark(currentFileName, scrollPosition);
        scrollCounter = 0;
    }
}

static void readingLineUp() {
    if (totalLines <= READING_LINES) return;
    if (scrollPosition <= 0) {
        scrollPosition = totalLines - READING_LINES;  // Wrap to end
    } else {
        scrollPosition--;
    }
    scrollCounter++;
    updateReadingView();
    if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
        sd_saveBookmark(currentFileName, scrollPosition);
        scrollCounter = 0;
    }
}

static void readingLineDown() {
    if (totalLines <= READING_LINES) return;
    int maxPos = totalLines - READING_LINES;
    if (scrollPosition >= maxPos) {
        scrollPosition = 0;  // Wrap to beginning
    } else {
        scrollPosition++;
    }
    scrollCounter++;
    updateReadingView();
    if (scrollCounter >= BOOKMARK_SAVE_EVERY) {
        sd_saveBookmark(currentFileName, scrollPosition);
        scrollCounter = 0;
    }
}

static void exitReading() {
    sd_saveBookmark(currentFileName, scrollPosition);
    sd_closeFile();
    appState = STATE_FILE_MENU;
    updateFileMenu();  // Return to file menu at same position
}

// ─── Screen: WiFi Portal ─────────────────────────────────────────────────────

static void enterWifiPortal() {
    appState = STATE_WIFI_PORTAL;
    display_wifiPortal(WIFI_SSID, PORTAL_IP);

    if (!portal_start()) {
        display_error("WiFi Failed", "Try again");
        delay(2000);
        disableRadios();
        enterHome();
        return;
    }
}

static void exitWifiPortal() {
    portal_stop();
    disableRadios();
    enterHome();
}

// ─── Screen: Settings ────────────────────────────────────────────────────────

static void enterSettings() {
    appState = STATE_SETTINGS;
    settingsIndex = 1;  // Start at first real option (after "< Back")
    display_settingsMenu(settingsIndex);
}

static void showSystemInfo() {
    appState = STATE_SETTINGS_INFO;
    display_systemInfo(sd_isMounted());
}

static void showFileCount() {
    appState = STATE_SETTINGS_FILES;
    int count = sd_scanFiles();
    display_fileCount(count);
}

static void showStorage() {
    appState = STATE_SETTINGS_STORAGE;
    float usedMB = sd_getUsedBytes() / 1048576.0f;
    float freeMB = sd_getFreeBytes() / 1048576.0f;
    display_storageInfo(usedMB, freeMB);
}

// ─── Arduino Setup ───────────────────────────────────────────────────────────

void setup() {
    // 0. Diagnostic LED — confirms firmware is running
    diagLedInit();
    diagBlink(1);

    // 1. Disable radios IMMEDIATELY for power saving
    disableRadios();
    setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);
    disableFlashLED();
    diagBlink(2);

    // 2. Release UART0 so GPIO1/GPIO3 can be used for I2C
    Serial.end();
    delay(50);

    // 3. Initialize buttons BEFORE SD_MMC
    buttons_init();
    buttons_readEarlyState();
    delay(100);
    diagBlink(3);

    // 4. Initialize OLED display + boot screen
    display_init();
    display_boot();
    delay(1000);  // Boot screen visible for 1 second
    diagBlink(4);

    // 5. Initialize SD card
    if (!sd_init()) {
        display_error("SD Card Error", "Insert & restart");
        appState = STATE_ERROR;
        diagBlink(10, 50, 50);
        return;
    }
    diagBlink(5);

    // 6. Enter Home screen
    lastActivityMs = millis();
    enterHome();
}

// ─── Arduino Main Loop ──────────────────────────────────────────────────────

void loop() {
    // Handle WiFi portal client requests
    if (appState == STATE_WIFI_PORTAL) {
        portal_handleClient();
    }

    // Read button input
    ButtonEvent event = buttons_update();

    // Wake display on any button press
    if (event != BTN_NONE) {
        resetActivityTimer();
        if (displayAsleep) return;  // Consume event to just wake up
    }

    // ── State machine ──
    switch (appState) {

        // ── HOME SCREEN ──────────────────────────────────────────────────
        case STATE_HOME:
            switch (event) {
                case BTN_UP_SHORT:
                    if (homeIndex > 0) {
                        homeIndex--;
                        display_home(homeIndex);
                    }
                    break;
                case BTN_DOWN_SHORT:
                    if (homeIndex < HOME_ITEMS - 1) {
                        homeIndex++;
                        display_home(homeIndex);
                    }
                    break;
                case BTN_SELECT_SHORT:
                    switch (homeIndex) {
                        case 0: enterWifiPortal(); break;  // WiFi Portal
                        case 1: enterFileMenu();   break;  // Files
                        case 2: enterSettings();   break;  // Settings
                    }
                    break;
                default:
                    break;
            }
            break;

        // ── FILE MENU ────────────────────────────────────────────────────
        case STATE_FILE_MENU:
            switch (event) {
                case BTN_UP_SHORT:
                    menuScrollUp();
                    break;
                case BTN_DOWN_SHORT:
                    menuScrollDown();
                    break;
                case BTN_UP_HELD:
                    menuScrollUp();     // Fast scroll
                    break;
                case BTN_DOWN_HELD:
                    menuScrollDown();   // Fast scroll
                    break;
                case BTN_SELECT_SHORT:
                    if (menuSelectedIndex == 0) {
                        enterHome();        // "< Back" selected → Home
                    } else {
                        menuSelectFile();   // Open file
                    }
                    break;
                default:
                    break;
            }
            break;

        // ── READING MODE (Full Immersion) ────────────────────────────────
        case STATE_READING:
            switch (event) {
                case BTN_UP_SHORT:
                    readingPageUp();     // Short press = page scroll
                    break;
                case BTN_DOWN_SHORT:
                    readingPageDown();   // Short press = page scroll
                    break;
                case BTN_UP_LONG:
                case BTN_UP_HELD:
                    readingLineUp();     // Long hold = line-by-line
                    break;
                case BTN_DOWN_LONG:
                case BTN_DOWN_HELD:
                    readingLineDown();   // Long hold = line-by-line
                    break;
                case BTN_SELECT_SHORT:
                    exitReading();       // SELECT = Back (terminal screen)
                    break;
                default:
                    break;
            }
            break;

        // ── WIFI PORTAL ──────────────────────────────────────────────────
        case STATE_WIFI_PORTAL:
            // SELECT = exit portal (manual close only)
            if (event == BTN_SELECT_SHORT) {
                exitWifiPortal();
            }
            // Portal stays active until manually closed
            break;

        // ── SETTINGS MENU ────────────────────────────────────────────────
        case STATE_SETTINGS:
            switch (event) {
                case BTN_UP_SHORT:
                    if (settingsIndex > 0) {
                        settingsIndex--;
                        display_settingsMenu(settingsIndex);
                    }
                    break;
                case BTN_DOWN_SHORT:
                    if (settingsIndex < SETTINGS_ITEMS - 1) {
                        settingsIndex++;
                        display_settingsMenu(settingsIndex);
                    }
                    break;
                case BTN_SELECT_SHORT:
                    switch (settingsIndex) {
                        case 0: enterHome();      break;  // "< Back"
                        case 1: showSystemInfo(); break;
                        case 2: showFileCount();  break;
                        case 3: showStorage();    break;
                    }
                    break;
                default:
                    break;
            }
            break;

        // ── SETTINGS SUB-SCREENS (terminal — SELECT = Back) ─────────────
        case STATE_SETTINGS_INFO:
        case STATE_SETTINGS_FILES:
        case STATE_SETTINGS_STORAGE:
            if (event == BTN_SELECT_SHORT) {
                enterSettings();        // Back → Settings menu
            }
            break;

        // ── ERROR STATE ──────────────────────────────────────────────────
        case STATE_ERROR:
            break;

        case STATE_BOOT:
            break;
    }

    // Idle timeout check
    checkIdleTimeout();

    // Prevent CPU spinning
    delay(5);
}
