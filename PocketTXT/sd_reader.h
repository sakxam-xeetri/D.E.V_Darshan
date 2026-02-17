/*
 * ============================================================================
 *  D.E.V_Darshan — SD Card Reader Header
 * ============================================================================
 *  SD_MMC 1-bit mode file operations: mount, list, read, bookmarks.
 * ============================================================================
 */

#ifndef SD_READER_H
#define SD_READER_H

#include "config.h"

// Mount SD card using SD_MMC 4-bit mode. Returns true on success.
bool sd_init();

// Unmount SD card
void sd_deinit();

// Check if SD is mounted
bool sd_isMounted();

// ── File Listing ──
// Scan root directory for .txt files. Returns number found.
// Populates internal file list accessible via sd_getFileName().
int sd_scanFiles();

// Get filename at index (from last scan)
const char* sd_getFileName(int index);

// Get total number of .txt files found
int sd_getFileCount();

// ── File Reading ──
// Open a file for reading. Returns true on success.
bool sd_openFile(const char* filename);

// Close currently open file
void sd_closeFile();

// Check if a file is currently open
bool sd_isFileOpen();

// Get total line count of current file (wrapped for display width).
// This performs a full scan on first call, then caches the result.
int sd_getTotalWrappedLines();

// Read a batch of wrapped lines starting at the given wrapped-line index.
// Fills `outLines` with up to `count` lines. Returns actual lines filled.
// Each line is null-terminated and fits within CHARS_PER_LINE.
int sd_readWrappedLines(int startLine, char outLines[][CHARS_PER_LINE + 1],
                        int count);

// ── SD Card Info ──
uint64_t sd_getTotalBytes();
uint64_t sd_getUsedBytes();
uint64_t sd_getFreeBytes();

// ── Bookmarks ──
// Save current reading position for a file
void sd_saveBookmark(const char* filename, int wrappedLinePos);

// Load saved reading position for a file. Returns -1 if not found.
int sd_loadBookmark(const char* filename);

#endif // SD_READER_H
