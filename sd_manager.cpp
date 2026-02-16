/*  =========================================================================
 *  DEV_Darshan — SD Card Manager Implementation
 *  SD_MMC 1-bit mode — uses GPIO 2 (D0), GPIO 14 (CLK), GPIO 15 (CMD)
 *  =========================================================================
 */

#include <Arduino.h>
#include "sd_manager.h"
#include "config.h"
#include <SD_MMC.h>

// ── Private state ──────────────────────────────────────────────────────
static File _currentFile;
static bool _mounted = false;

// ═══════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════

bool sd_init() {
    if (_mounted) return true;

    /*  SD_MMC.begin(mountpoint, mode1bit, formatOnFail, frequency)
     *  1-bit mode frees GPIO 4, 12, 13 for our I2C and other use.
     *  We try up to 3 times because SD cards can be finicky on cold boot.
     */
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        if (SD_MMC.begin("/sdcard", true)) {   // true = 1-bit mode
            _mounted = true;
            Serial.printf("[SD] Mounted OK (attempt %d). Type: %d\n",
                          attempt + 1, SD_MMC.cardType());
            return true;
        }
        Serial.printf("[SD] Mount attempt %d failed\n", attempt + 1);
        delay(200);
    }
    Serial.println("[SD] Mount FAILED after 3 attempts");
    return false;
}

void sd_deinit() {
    if (_currentFile) _currentFile.close();
    SD_MMC.end();
    _mounted = false;
}

// ═══════════════════════════════════════════════════════════════════════
//  Directory listing — collects .txt files in root
// ═══════════════════════════════════════════════════════════════════════

uint8_t sd_listTxtFiles(char fileList[][32], uint8_t maxFiles) {
    if (!_mounted) return 0;

    File root = SD_MMC.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("[SD] Cannot open root dir");
        return 0;
    }

    uint8_t count = 0;
    File entry;
    while ((entry = root.openNextFile()) && count < maxFiles) {
        if (entry.isDirectory()) continue;

        const char* name = entry.name();
        // Filter .txt / .TXT files only
        size_t len = strlen(name);
        if (len < 5) continue;
        const char* ext = name + len - 4;
        if (strcasecmp(ext, ".txt") != 0) continue;

        // Strip leading '/' if present
        const char* display = name;
        if (display[0] == '/') display++;

        strncpy(fileList[count], display, MAX_FILENAME_LEN - 1);
        fileList[count][MAX_FILENAME_LEN - 1] = '\0';
        count++;
    }
    root.close();

    Serial.printf("[SD] Found %d .txt files\n", count);
    return count;
}

// ═══════════════════════════════════════════════════════════════════════
//  Streaming file reader — memory efficient, line-by-line
// ═══════════════════════════════════════════════════════════════════════

bool sd_openFile(const char* path) {
    sd_closeFile();

    // Build full path
    char fullPath[64];
    if (path[0] == '/') {
        strncpy(fullPath, path, sizeof(fullPath));
    } else {
        snprintf(fullPath, sizeof(fullPath), "/%s", path);
    }

    _currentFile = SD_MMC.open(fullPath, FILE_READ);
    if (!_currentFile) {
        Serial.printf("[SD] Failed to open: %s\n", fullPath);
        return false;
    }
    Serial.printf("[SD] Opened: %s (%lu bytes)\n", fullPath,
                  (unsigned long)_currentFile.size());
    return true;
}

void sd_closeFile() {
    if (_currentFile) {
        _currentFile.close();
    }
}

bool sd_isFileOpen() {
    return (bool)_currentFile;
}

/*  Read one line from the open file.
 *  Returns number of characters read (0 = empty line), -1 = EOF.
 *  Strips \r and \n from output. Null-terminates the buffer.
 */
int sd_readLine(char* buf, uint16_t maxLen) {
    if (!_currentFile || !_currentFile.available()) return -1;

    uint16_t i = 0;
    while (_currentFile.available() && i < maxLen - 1) {
        char c = (char)_currentFile.read();
        if (c == '\n') break;             // end of line
        if (c == '\r') continue;          // skip carriage return
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (int)i;
}

void sd_seekBeginning() {
    if (_currentFile) {
        _currentFile.seek(0);
    }
}

/*  Seek forward to a specific line number (0-indexed).
 *  Rewinds to start then skips N lines. Returns false if EOF reached early.
 */
bool sd_seekToLine(uint32_t lineNumber) {
    if (!_currentFile) return false;
    _currentFile.seek(0);

    char dummy[READ_BUF_SIZE];
    for (uint32_t i = 0; i < lineNumber; i++) {
        if (sd_readLine(dummy, sizeof(dummy)) < 0) return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════
//  Stats
// ═══════════════════════════════════════════════════════════════════════

uint64_t sd_totalBytes() {
    return _mounted ? SD_MMC.totalBytes() : 0;
}

uint64_t sd_usedBytes() {
    return _mounted ? SD_MMC.usedBytes() : 0;
}

// ═══════════════════════════════════════════════════════════════════════
//  Upload helper — save a blob to SD as a .txt file
// ═══════════════════════════════════════════════════════════════════════

bool sd_saveUploadedFile(const char* filename, const uint8_t* data, size_t len) {
    if (!_mounted) return false;

    char path[64];
    snprintf(path, sizeof(path), "/%s", filename);

    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
        Serial.printf("[SD] Cannot create file: %s\n", path);
        return false;
    }

    size_t written = f.write(data, len);
    f.close();

    Serial.printf("[SD] Saved %s (%u / %u bytes)\n", path,
                  (unsigned)written, (unsigned)len);
    return (written == len);
}
