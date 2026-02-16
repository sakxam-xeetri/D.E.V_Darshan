/*  =========================================================================
 *  DEV_Darshan — Pocket TXT eBook Reader
 *  =========================================================================
 *  Main sketch file (.ino) for Arduino IDE
 *
 *  Board:   ESP32-CAM (AI Thinker) — select "AI Thinker ESP32-CAM" in IDE
 *  Upload:  Use FTDI adapter (GPIO0 → GND during upload, then release)
 *
 *  Required Library (install via Library Manager):
 *    • "ESP8266 and ESP32 OLED driver for SSD1306 displays" by ThingPulse
 *
 *  State machine:
 *    BOOT → FILE_MENU → READING → (optional) WIFI_PORTAL
 *                ↑___________|
 *
 *  Controls:
 *    UP / DOWN   = navigate menu / scroll text
 *    SELECT      = open file / (in reader) back to menu
 *    SELECT long = toggle WiFi upload portal
 *  =========================================================================
 */

#include "config.h"
#include "display_manager.h"
#include "sd_manager.h"
#include "input_manager.h"
#include "wifi_portal_manager.h"
#include "power_manager.h"

// ═══════════════════════════════════════════════════════════════════════
//  Application states
// ═══════════════════════════════════════════════════════════════════════

enum class AppState : uint8_t {
    BOOT,
    FILE_MENU,
    READING,
    WIFI_PORTAL,
    ERROR_SCREEN
};

static AppState _state = AppState::BOOT;

// ── File menu state ────────────────────────────────────────────────────
static char      _fileNames[MAX_FILES][MAX_FILENAME_LEN];
static uint8_t   _fileCount    = 0;
static uint8_t   _menuSelected = 0;
static uint8_t   _menuTop      = 0;      // top visible index

// ── Reader state ───────────────────────────────────────────────────────
//  We maintain a cache of wrapped display lines. The reader loads
//  raw lines from SD and word-wraps them into screen-width chunks.

#define MAX_CACHED_LINES  256           // max wrapped lines we track offsets for
#define LINE_BUF_LEN      (CHARS_PER_LINE + 1)

static char     _displayLines[MAX_CACHED_LINES][LINE_BUF_LEN];
static uint16_t _totalDisplayLines = 0;
static uint16_t _viewTopLine       = 0;  // first visible wrapped line

// ═══════════════════════════════════════════════════════════════════════
//  Word-wrap engine — wraps raw text lines into CHARS_PER_LINE columns
// ═══════════════════════════════════════════════════════════════════════

/*  Takes a raw line and appends one or more wrapped lines to the
 *  _displayLines array. Returns the number of wrapped lines added.
 */
static uint16_t _wrapLine(const char* raw) {
    uint16_t added = 0;
    size_t len = strlen(raw);

    if (len == 0) {
        // Empty line → blank display line
        if (_totalDisplayLines < MAX_CACHED_LINES) {
            _displayLines[_totalDisplayLines][0] = '\0';
            _totalDisplayLines++;
            added++;
        }
        return added;
    }

    size_t pos = 0;
    while (pos < len && _totalDisplayLines < MAX_CACHED_LINES) {
        size_t remaining = len - pos;

        if (remaining <= CHARS_PER_LINE) {
            // Fits in one line
            strncpy(_displayLines[_totalDisplayLines], raw + pos, remaining);
            _displayLines[_totalDisplayLines][remaining] = '\0';
            _totalDisplayLines++;
            added++;
            break;
        }

        // Find a word boundary to break at
        size_t breakAt = CHARS_PER_LINE;
        while (breakAt > 0 && raw[pos + breakAt] != ' ') {
            breakAt--;
        }
        if (breakAt == 0) {
            breakAt = CHARS_PER_LINE;   // hard break if no space found
        }

        strncpy(_displayLines[_totalDisplayLines], raw + pos, breakAt);
        _displayLines[_totalDisplayLines][breakAt] = '\0';
        _totalDisplayLines++;
        added++;

        pos += breakAt;
        if (pos < len && raw[pos] == ' ') pos++;   // skip space at break
    }

    return added;
}

// ═══════════════════════════════════════════════════════════════════════
//  Load file into wrapped line cache
// ═══════════════════════════════════════════════════════════════════════

static bool _loadFileIntoCache(const char* filename) {
    _totalDisplayLines = 0;
    _viewTopLine       = 0;

    if (!sd_openFile(filename)) return false;

    char rawBuf[READ_BUF_SIZE];
    while (_totalDisplayLines < MAX_CACHED_LINES) {
        int n = sd_readLine(rawBuf, sizeof(rawBuf));
        if (n < 0) break;        // EOF
        _wrapLine(rawBuf);
    }

    sd_closeFile();
    Serial.printf("[MAIN] Loaded %u display lines\n", _totalDisplayLines);
    return (_totalDisplayLines > 0);
}

// ═══════════════════════════════════════════════════════════════════════
//  Display helpers
// ═══════════════════════════════════════════════════════════════════════

static void _drawMenu() {
    const char* ptrs[LINES_PER_SCREEN];
    for (uint8_t i = 0; i < LINES_PER_SCREEN; i++) {
        if (_menuTop + i < _fileCount) {
            ptrs[i] = _fileNames[_menuTop + i];
        } else {
            ptrs[i] = "";
        }
    }
    display_fileMenu(ptrs, _fileCount, _menuSelected, _menuTop);
}

static void _drawReaderPage() {
    const char* ptrs[LINES_PER_SCREEN];
    uint8_t count = 0;
    for (uint8_t i = 0; i < LINES_PER_SCREEN; i++) {
        uint16_t idx = _viewTopLine + i;
        if (idx < _totalDisplayLines) {
            ptrs[i] = _displayLines[idx];
            count++;
        } else {
            ptrs[i] = "";
        }
    }
    display_readerPage(ptrs, count > 0 ? count : LINES_PER_SCREEN);
}

// ═══════════════════════════════════════════════════════════════════════
//  State handlers
// ═══════════════════════════════════════════════════════════════════════

static void _enterFileMenu() {
    _state = AppState::FILE_MENU;
    _menuSelected = 0;
    _menuTop      = 0;

    _fileCount = sd_listTxtFiles(_fileNames, MAX_FILES);

    if (_fileCount == 0) {
        display_message("No .txt files", "Upload via WiFi");
        _state = AppState::ERROR_SCREEN;
        return;
    }

    _drawMenu();
}

static void _handleMenuInput(BtnEvent ev) {
    switch (ev) {
        case BtnEvent::UP_SHORT:
        case BtnEvent::UP_REPEAT:
            if (_menuSelected > 0) {
                _menuSelected--;
                if (_menuSelected < _menuTop) _menuTop = _menuSelected;
                _drawMenu();
            }
            break;

        case BtnEvent::DOWN_SHORT:
        case BtnEvent::DOWN_REPEAT:
            if (_menuSelected < _fileCount - 1) {
                _menuSelected++;
                if (_menuSelected >= _menuTop + LINES_PER_SCREEN)
                    _menuTop = _menuSelected - LINES_PER_SCREEN + 1;
                _drawMenu();
            }
            break;

        case BtnEvent::SEL_SHORT: {
            display_message("Loading...");
            if (_loadFileIntoCache(_fileNames[_menuSelected])) {
                _state = AppState::READING;
                _drawReaderPage();
            } else {
                display_message("Read error!", _fileNames[_menuSelected]);
                delay(1500);
                _drawMenu();
            }
            break;
        }

        case BtnEvent::SEL_LONG:
            _state = AppState::WIFI_PORTAL;
            power_enableWiFi();
            wifi_portal_start();
            break;

        default:
            break;
    }
}

static void _handleReaderInput(BtnEvent ev) {
    switch (ev) {
        case BtnEvent::DOWN_SHORT:
            if (_viewTopLine + LINES_PER_SCREEN < _totalDisplayLines) {
                _viewTopLine++;
                _drawReaderPage();
            }
            break;

        case BtnEvent::DOWN_REPEAT:
        case BtnEvent::DOWN_LONG:
            if (_viewTopLine + LINES_PER_SCREEN < _totalDisplayLines) {
                _viewTopLine += LINES_PER_SCREEN;
                if (_viewTopLine + LINES_PER_SCREEN > _totalDisplayLines) {
                    _viewTopLine = _totalDisplayLines - LINES_PER_SCREEN;
                }
                _drawReaderPage();
            }
            break;

        case BtnEvent::UP_SHORT:
            if (_viewTopLine > 0) {
                _viewTopLine--;
                _drawReaderPage();
            }
            break;

        case BtnEvent::UP_REPEAT:
        case BtnEvent::UP_LONG:
            if (_viewTopLine >= LINES_PER_SCREEN) {
                _viewTopLine -= LINES_PER_SCREEN;
            } else {
                _viewTopLine = 0;
            }
            _drawReaderPage();
            break;

        case BtnEvent::SEL_SHORT:
            _enterFileMenu();
            break;

        case BtnEvent::SEL_LONG:
            _state = AppState::WIFI_PORTAL;
            power_enableWiFi();
            wifi_portal_start();
            break;

        default:
            break;
    }
}

static void _handleWifiInput(BtnEvent ev) {
    if (ev == BtnEvent::SEL_SHORT || ev == BtnEvent::SEL_LONG) {
        wifi_portal_stop();
        power_disableRadios();
        display_message("WiFi OFF", "Returning...");
        delay(800);
        _enterFileMenu();
    }
}

static void _handleErrorInput(BtnEvent ev) {
    if (ev == BtnEvent::SEL_LONG) {
        _state = AppState::WIFI_PORTAL;
        power_enableWiFi();
        wifi_portal_start();
    } else if (ev == BtnEvent::SEL_SHORT) {
        _enterFileMenu();
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Arduino setup() and loop()
// ═══════════════════════════════════════════════════════════════════════

void setup() {
    // Brief delay for stable power-up (especially with reed switch + TP4056)
    delay(100);

    Serial.begin(115200);
    Serial.println();
    Serial.println("========================================");
    Serial.printf("  %s  v%s\n", PROJECT_NAME, FW_VERSION);
    Serial.println("  Pocket TXT eBook Reader");
    Serial.println("========================================");

    // ── Initialize subsystems in safe order ────────────────────────────
    // 1. Power first — disables WiFi/BT immediately to save power
    power_init();

    // 2. Display — I2C on GPIO 13/12 (no conflict with SD_MMC 1-bit)
    display_init();
    display_splash();
    delay(1200);

    // 3. Buttons
    input_init();

    // 4. SD card — SD_MMC 1-bit on GPIO 2/14/15
    display_message("Mounting SD...");
    if (!sd_init()) {
        display_message("SD Card Error!", "Insert & restart");
        _state = AppState::ERROR_SCREEN;
        Serial.println("[MAIN] SD init failed — entering error state");
        return;
    }

    // 5. Enter file menu
    _enterFileMenu();

    Serial.println("[MAIN] Setup complete");
}

void loop() {
    BtnEvent ev = input_poll();

    switch (_state) {
        case AppState::FILE_MENU:
            _handleMenuInput(ev);
            break;

        case AppState::READING:
            _handleReaderInput(ev);
            break;

        case AppState::WIFI_PORTAL:
            wifi_portal_loop();
            _handleWifiInput(ev);
            break;

        case AppState::ERROR_SCREEN:
            _handleErrorInput(ev);
            break;

        default:
            break;
    }

    // Small yield to prevent WDT and reduce power
    delay(5);
}
