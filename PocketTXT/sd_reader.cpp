/*
 * ============================================================================
 *  D.E.V_Darshan — SD Card Reader Implementation
 * ============================================================================
 *  Memory-efficient file reader using SD_MMC (1-bit mode).
 *
 *  Key design choices:
 *  - Files are NEVER loaded fully into RAM
 *  - Lines are read on-demand from SD and word-wrapped on the fly
 *  - A lightweight index maps wrapped line numbers to file byte positions
 *  - Bookmarks are stored in ESP32 NVS (Preferences library)
 *  - 1-bit SD_MMC mode frees GPIO4/12/13 and eliminates all pull resistors
 *
 *  SD_MMC 1-bit mode pins on ESP32-CAM:
 *    CLK=14, CMD=15, D0=2  (GPIO4/12/13 are FREE)
 * ============================================================================
 */

#include "sd_reader.h"
#include <SD_MMC.h>
#include <FS.h>
#include <Preferences.h>

// ─── Internal State ──────────────────────────────────────────────────────────

static bool mounted = false;
static File currentFile;
static bool fileOpen = false;

// File list storage
static char fileNames[MAX_FILES][MAX_FILENAME_LEN];
static int  fileCount = 0;

// Wrapped line index: maps wrapped line number → file byte offset
// Built on first access, enables random-access scrolling
#define MAX_LINE_INDEX 4096
static uint32_t lineIndex[MAX_LINE_INDEX];  // Byte positions
static int totalWrappedLines = -1;          // -1 = not yet counted

// Preferences for bookmarks
static Preferences prefs;

// ─── SD Mount/Unmount ────────────────────────────────────────────────────────

bool sd_init() {
    if (mounted) return true;

    for (int attempt = 0; attempt < SD_MOUNT_RETRIES; attempt++) {
        if (SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode (no extra resistors needed)
            // Verify card is present and accessible
            uint8_t cardType = SD_MMC.cardType();
            if (cardType != CARD_NONE) {
                mounted = true;
                return true;
            }
            SD_MMC.end();
        }
        delay(SD_RETRY_DELAY_MS);
    }

    mounted = false;
    return false;
}

void sd_deinit() {
    if (fileOpen) sd_closeFile();
    if (mounted) {
        SD_MMC.end();
        mounted = false;
    }
}

bool sd_isMounted() {
    return mounted;
}

// ─── File Listing ────────────────────────────────────────────────────────────

int sd_scanFiles() {
    fileCount = 0;
    if (!mounted) return 0;

    File root = SD_MMC.open("/");
    if (!root || !root.isDirectory()) {
        return 0;
    }

    File entry;
    while ((entry = root.openNextFile()) && fileCount < MAX_FILES) {
        if (entry.isDirectory()) continue;

        const char* name = entry.name();
        // Check for .txt extension (case insensitive)
        int len = strlen(name);
        if (len > 4) {
            const char* ext = name + len - 4;
            if (strcasecmp(ext, ".txt") == 0) {
                // Store filename without leading '/'
                const char* displayName = name;
                if (displayName[0] == '/') displayName++;

                strncpy(fileNames[fileCount], displayName, MAX_FILENAME_LEN - 1);
                fileNames[fileCount][MAX_FILENAME_LEN - 1] = '\0';
                fileCount++;
            }
        }
        entry.close();
    }
    root.close();

    // Sort alphabetically (simple bubble sort — max 50 files)
    for (int i = 0; i < fileCount - 1; i++) {
        for (int j = 0; j < fileCount - i - 1; j++) {
            if (strcasecmp(fileNames[j], fileNames[j + 1]) > 0) {
                char temp[MAX_FILENAME_LEN];
                memcpy(temp, fileNames[j], MAX_FILENAME_LEN);
                memcpy(fileNames[j], fileNames[j + 1], MAX_FILENAME_LEN);
                memcpy(fileNames[j + 1], temp, MAX_FILENAME_LEN);
            }
        }
    }

    return fileCount;
}

const char* sd_getFileName(int index) {
    if (index < 0 || index >= fileCount) return "";
    return fileNames[index];
}

int sd_getFileCount() {
    return fileCount;
}

// ─── Word Wrap Helper ────────────────────────────────────────────────────────
// Wraps a raw line into multiple display lines of CHARS_PER_LINE width.
// Returns number of wrapped lines produced.

static int wrapLine(const char* rawLine, char outLines[][CHARS_PER_LINE + 1],
                    int maxOut) {
    int produced = 0;
    int rawLen = strlen(rawLine);

    if (rawLen == 0) {
        // Empty line → produce one blank display line
        if (produced < maxOut) {
            outLines[produced][0] = '\0';
            produced++;
        }
        return produced;
    }

    int pos = 0;
    while (pos < rawLen && produced < maxOut) {
        // If remaining text fits in one line
        if (rawLen - pos <= CHARS_PER_LINE) {
            strncpy(outLines[produced], rawLine + pos, CHARS_PER_LINE);
            outLines[produced][rawLen - pos] = '\0';
            produced++;
            break;
        }

        // Find word break point
        int breakPos = CHARS_PER_LINE;

        // Look backwards for a space to break at word boundary
        int spacePos = breakPos;
        while (spacePos > 0 && rawLine[pos + spacePos] != ' ') {
            spacePos--;
        }

        if (spacePos > 0) {
            breakPos = spacePos;
        }
        // else: no space found — hard break at CHARS_PER_LINE

        memcpy(outLines[produced], rawLine + pos, breakPos);
        outLines[produced][breakPos] = '\0';
        produced++;

        pos += breakPos;
        // Skip the space at break point
        if (pos < rawLen && rawLine[pos] == ' ') pos++;
    }

    return produced;
}

// ─── File Open/Close ─────────────────────────────────────────────────────────

bool sd_openFile(const char* filename) {
    if (!mounted) return false;
    if (fileOpen) sd_closeFile();

    char path[MAX_FILENAME_LEN + 2];
    snprintf(path, sizeof(path), "/%s", filename);

    currentFile = SD_MMC.open(path, FILE_READ);
    if (!currentFile) return false;

    fileOpen = true;
    totalWrappedLines = -1;  // Force recount

    return true;
}

void sd_closeFile() {
    if (fileOpen) {
        currentFile.close();
        fileOpen = false;
        totalWrappedLines = -1;
    }
}

bool sd_isFileOpen() {
    return fileOpen;
}

// ─── Build Wrapped Line Index ────────────────────────────────────────────────
// Scans the entire file once and builds an index mapping wrapped line numbers
// to byte offsets. This enables O(1) random access for scrolling.

static void buildLineIndex() {
    if (!fileOpen) return;

    // Save current position to restore later
    size_t savedPos = currentFile.position();
    currentFile.seek(0);

    totalWrappedLines = 0;
    char rawLine[LINE_BUFFER_SIZE];
    char wrapped[8][CHARS_PER_LINE + 1];  // Temp buffer for wrap calculation

    // Record position of first wrapped line
    if (totalWrappedLines < MAX_LINE_INDEX) {
        lineIndex[0] = 0;
    }

    while (currentFile.available() && totalWrappedLines < MAX_LINE_INDEX) {
        size_t lineStart = currentFile.position();

        // Read one raw line
        int len = 0;
        while (currentFile.available() && len < LINE_BUFFER_SIZE - 1) {
            char c = currentFile.read();
            if (c == '\n') break;
            if (c == '\r') continue;  // Handle \r\n
            rawLine[len++] = c;
        }
        rawLine[len] = '\0';

        // Calculate how many wrapped lines this produces
        int wrappedCount = wrapLine(rawLine, wrapped, 8);

        // Store byte offset for each wrapped line
        for (int i = 0; i < wrappedCount && totalWrappedLines < MAX_LINE_INDEX; i++) {
            lineIndex[totalWrappedLines] = lineStart;
            totalWrappedLines++;
        }
    }

    // Restore file position
    currentFile.seek(savedPos);
}

int sd_getTotalWrappedLines() {
    if (totalWrappedLines < 0) {
        buildLineIndex();
    }
    return totalWrappedLines;
}

// ─── Read Wrapped Lines ──────────────────────────────────────────────────────
// Random-access read of wrapped display lines starting at `startLine`.
// Uses the index built by buildLineIndex() for efficient seeking.

int sd_readWrappedLines(int startLine, char outLines[][CHARS_PER_LINE + 1],
                        int count) {
    if (!fileOpen || count <= 0) return 0;

    // Ensure index is built
    if (totalWrappedLines < 0) buildLineIndex();

    if (startLine < 0) startLine = 0;
    if (startLine >= totalWrappedLines) return 0;

    // Seek to the byte position of the raw line containing startLine
    currentFile.seek(lineIndex[startLine]);

    int filled = 0;
    int currentWrappedLine = startLine;

    // We need to track which sub-line within a raw line we need
    // Find the raw line that contains startLine and which wrapped sub-line
    // it corresponds to

    char rawLine[LINE_BUFFER_SIZE];
    char wrapped[8][CHARS_PER_LINE + 1];

    // Re-scan from the byte position to figure out sub-line offset
    // The lineIndex stores the raw line start, but multiple wrapped lines
    // may share the same raw line start. We need to find the offset.

    while (filled < count && currentFile.available()) {
        size_t rawStart = currentFile.position();

        // Read one raw line
        int len = 0;
        while (currentFile.available() && len < LINE_BUFFER_SIZE - 1) {
            char c = currentFile.read();
            if (c == '\n') break;
            if (c == '\r') continue;
            rawLine[len++] = c;
        }
        rawLine[len] = '\0';

        // Wrap it
        int wrappedCount = wrapLine(rawLine, wrapped, 8);

        // Determine which sub-lines to copy
        for (int i = 0; i < wrappedCount && filled < count; i++) {
            if (currentWrappedLine >= startLine) {
                strncpy(outLines[filled], wrapped[i], CHARS_PER_LINE + 1);
                filled++;
            }
            currentWrappedLine++;
        }
    }

    return filled;
}

// ─── SD Card Info ────────────────────────────────────────────────────────────

uint64_t sd_getTotalBytes() {
    if (!mounted) return 0;
    return SD_MMC.totalBytes();
}

uint64_t sd_getUsedBytes() {
    if (!mounted) return 0;
    return SD_MMC.usedBytes();
}

uint64_t sd_getFreeBytes() {
    if (!mounted) return 0;
    return SD_MMC.totalBytes() - SD_MMC.usedBytes();
}

// ─── Bookmarks (NVS) ────────────────────────────────────────────────────────
// Uses the Preferences library to store bookmarks in ESP32's NVS.
// Key: first 14 chars of filename (NVS key max 15 chars)
// Value: wrapped line position (int)

static void makeBookmarkKey(const char* filename, char* key, size_t keySize) {
    // Create a short key from filename (max 14 chars for NVS)
    size_t len = strlen(filename);
    if (len > 14) len = 14;
    memcpy(key, filename, len);
    key[len] = '\0';

    // Replace dots and spaces with underscores for NVS compatibility
    for (size_t i = 0; i < len; i++) {
        if (key[i] == '.' || key[i] == ' ' || key[i] == '-') {
            key[i] = '_';
        }
    }
}

void sd_saveBookmark(const char* filename, int wrappedLinePos) {
    char key[16];
    makeBookmarkKey(filename, key, sizeof(key));

    prefs.begin("bookmarks", false);
    prefs.putInt(key, wrappedLinePos);
    prefs.end();
}

int sd_loadBookmark(const char* filename) {
    char key[16];
    makeBookmarkKey(filename, key, sizeof(key));

    prefs.begin("bookmarks", true);  // Read-only
    int pos = prefs.getInt(key, -1);
    prefs.end();

    return pos;
}
