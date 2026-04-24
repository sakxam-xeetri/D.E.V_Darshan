# 1 "C:\\Users\\ACER\\AppData\\Local\\Temp\\tmps32gb7gl"
#include <Arduino.h>
# 1 "D:/D.E.V_Darshan/D.E.V_Darshan_Sketch.ino"
# 31 "D:/D.E.V_Darshan/D.E.V_Darshan_Sketch.ino"
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



static AppState appState = STATE_BOOT;


static int homeIndex = 0;
static int settingsIndex = 0;


static int menuSelectedIndex = 0;
static int menuTopIndex = 0;
static const char* fileNamePtrs[MAX_FILES];


static char currentFileName[MAX_FILENAME_LEN] = {0};
static int scrollPosition = 0;
static int totalLines = 0;
static int scrollCounter = 0;


static char displayLines[READING_LINES][CHARS_PER_LINE + 1];
static const char* displayLinePtrs[READING_LINES];


static unsigned long lastActivityMs = 0;
static bool displayAsleep = false;
static bool manualSleepLocked = false;
static bool sleepComboLatch = false;
static void disableRadios();
static void disableFlashLED();
static void diagLedInit();
static void resetActivityTimer();
static void enterLightSleep();
static void checkIdleTimeout();
static bool handleManualSleepToggle();
static void enterHome();
static void enterFileMenu();
static void updateFileMenu();
static void menuScrollUp();
static void menuScrollDown();
static void updateReadingView();
static void menuSelectFile();
static void readingPageUp();
static void readingPageDown();
static void readingLineUp();
static void readingLineDown();
static void exitReading();
static void enterWifiPortal();
static void exitWifiPortal();
static void enterSettings();
static void showSystemInfo();
static void showFileCount();
static void showStorage();
void setup();
void loop();
#line 74 "D:/D.E.V_Darshan/D.E.V_Darshan_Sketch.ino"
static void disableRadios() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
}

static void disableFlashLED() {
    pinMode(PIN_FLASH_LED, OUTPUT);
    digitalWrite(PIN_FLASH_LED, LOW);
}



static void diagLedInit() {
    pinMode(PIN_LED_DIAG, OUTPUT);
    digitalWrite(PIN_LED_DIAG, HIGH);
}

static void diagBlink(int count, int onMs = 100, int offMs = 100) {
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_LED_DIAG, LOW);
        delay(onMs);
        digitalWrite(PIN_LED_DIAG, HIGH);
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

static void enterLightSleep() {
    display_sleep();
    displayAsleep = true;


    gpio_wakeup_enable((gpio_num_t)PIN_BTN_UP, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)PIN_BTN_DOWN, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)PIN_BTN_SELECT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();


    esp_light_sleep_start();


    delay(100);
    buttons_init();
    resetActivityTimer();
}

static void checkIdleTimeout() {
    if (appState == STATE_WIFI_PORTAL) return;
    if (appState == STATE_BOOT) return;
    if (appState == STATE_ERROR) return;
    if (manualSleepLocked) return;
    if (!displayAsleep && (millis() - lastActivityMs) > DISPLAY_TIMEOUT_MS) {
        enterLightSleep();
    }
}

static bool handleManualSleepToggle() {

    bool comboPressed = buttons_isUpPressed() && buttons_isDownPressed();

    if (comboPressed && !sleepComboLatch) {
        sleepComboLatch = true;

        if (manualSleepLocked) {
            manualSleepLocked = false;
            resetActivityTimer();
        } else {
            manualSleepLocked = true;
            display_sleep();
            displayAsleep = true;
        }
        return true;
    }

    if (!comboPressed) {
        sleepComboLatch = false;
    }

    return false;
}



static void enterHome() {
    appState = STATE_HOME;
    homeIndex = 0;
    display_home(homeIndex);
}



static void enterFileMenu() {
    appState = STATE_FILE_MENU;

    int count = sd_scanFiles();


    menuSelectedIndex = 0;
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
    int count = sd_getFileCount();
    int totalItems = count + 1;

    if (menuSelectedIndex <= 0) {

        menuSelectedIndex = totalItems - 1;
        if (menuSelectedIndex >= MENU_LINES) {
            menuTopIndex = menuSelectedIndex - MENU_LINES + 1;
        } else {
            menuTopIndex = 0;
        }
    } else {
        menuSelectedIndex--;
        if (menuSelectedIndex < menuTopIndex) {
            menuTopIndex = menuSelectedIndex;
        }
    }
    updateFileMenu();
}

static void menuScrollDown() {
    int count = sd_getFileCount();


    if (count == 0) return;

    int totalItems = count + 1;
    if (menuSelectedIndex >= totalItems - 1) {

        menuSelectedIndex = 0;
        menuTopIndex = 0;
    } else {
        menuSelectedIndex++;
        if (menuSelectedIndex >= menuTopIndex + MENU_LINES) {
            menuTopIndex = menuSelectedIndex - MENU_LINES + 1;
        }
    }
    updateFileMenu();
}



static void updateReadingView() {
    int read = sd_readWrappedLines(scrollPosition, displayLines, READING_LINES);
    for (int i = 0; i < READING_LINES; i++) {
        displayLinePtrs[i] = (i < read) ? displayLines[i] : "";
    }
    display_reading(displayLinePtrs, read);
}

static void menuSelectFile() {
    int count = sd_getFileCount();
    int fileIndex = menuSelectedIndex - 1;
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
        scrollPosition = maxPos;
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
        scrollPosition = 0;
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
        scrollPosition = totalLines - READING_LINES;
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
        scrollPosition = 0;
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
    updateFileMenu();
}



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



static void enterSettings() {
    appState = STATE_SETTINGS;
    settingsIndex = 1;
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



void setup() {

    diagLedInit();
    diagBlink(1);


    disableRadios();
    setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);
    disableFlashLED();
    diagBlink(2);


    Serial.end();
    delay(50);


    buttons_init();
    buttons_readEarlyState();
    delay(100);
    diagBlink(3);


    display_init();
    display_boot();
    delay(1000);
    diagBlink(4);


    if (!sd_init()) {
        display_error("SD Card Error", "Insert & restart");
        appState = STATE_ERROR;
        diagBlink(10, 50, 50);
        return;
    }
    diagBlink(5);


    lastActivityMs = millis();
    enterHome();
}



void loop() {

    if (appState == STATE_WIFI_PORTAL) {
        if (millis() - portal_lastActivity() > WIFI_TIMEOUT_MS) {
            exitWifiPortal();
            return;
        }
        portal_handleClient();
    }


    ButtonEvent event = buttons_update();


    if (handleManualSleepToggle()) {
        delay(1);
        return;
    }


    if (manualSleepLocked) {
        if (!displayAsleep) {
            display_sleep();
            displayAsleep = true;
        }
        delay(1);
        return;
    }


    if (event != BTN_NONE) {
        resetActivityTimer();
        if (displayAsleep) return;
    }


    switch (appState) {


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
                        case 0: enterWifiPortal(); break;
                        case 1: enterFileMenu(); break;
                        case 2: enterSettings(); break;
                    }
                    break;
                default:
                    break;
            }
            break;


        case STATE_FILE_MENU:
            switch (event) {
                case BTN_UP_SHORT:
                    menuScrollUp();
                    break;
                case BTN_DOWN_SHORT:
                    menuScrollDown();
                    break;
                case BTN_UP_HELD:
                    menuScrollUp();
                    break;
                case BTN_DOWN_HELD:
                    menuScrollDown();
                    break;
                case BTN_SELECT_SHORT:
                    if (menuSelectedIndex == 0) {
                        enterHome();
                    } else {
                        menuSelectFile();
                    }
                    break;
                default:
                    break;
            }
            break;


        case STATE_READING:
            switch (event) {
                case BTN_UP_SHORT:
                    readingPageUp();
                    break;
                case BTN_DOWN_SHORT:
                    readingPageDown();
                    break;
                case BTN_UP_LONG:
                case BTN_UP_HELD:
                    readingLineUp();
                    break;
                case BTN_DOWN_LONG:
                case BTN_DOWN_HELD:
                    readingLineDown();
                    break;
                case BTN_SELECT_SHORT:
                    exitReading();
                    break;
                default:
                    break;
            }
            break;


        case STATE_WIFI_PORTAL:

            if (event == BTN_SELECT_SHORT) {
                exitWifiPortal();
            }

            break;


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
                        case 0: enterHome(); break;
                        case 1: showSystemInfo(); break;
                        case 2: showFileCount(); break;
                        case 3: showStorage(); break;
                    }
                    break;
                default:
                    break;
            }
            break;


        case STATE_SETTINGS_INFO:
        case STATE_SETTINGS_FILES:
        case STATE_SETTINGS_STORAGE:
            if (event == BTN_SELECT_SHORT) {
                enterSettings();
            }
            break;


        case STATE_ERROR:
            break;

        case STATE_BOOT:
            break;
    }


    checkIdleTimeout();


    delay(1);
}