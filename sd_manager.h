/*  =========================================================================
 *  DEV_Darshan — SD Card Manager (Header)
 *  SD_MMC 1-bit mode on ESP32-CAM built-in slot
 *  =========================================================================
 */

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>

// ── Lifecycle ──────────────────────────────────────────────────────────
bool sd_init();                             // mount SD card; returns success
void sd_deinit();                           // unmount

// ── Directory listing ──────────────────────────────────────────────────
uint8_t sd_listTxtFiles(char fileList[][32], uint8_t maxFiles);

// ── File reader (streaming, memory-efficient) ──────────────────────────
bool    sd_openFile(const char* path);
void    sd_closeFile();
bool    sd_isFileOpen();
int     sd_readLine(char* buf, uint16_t maxLen);   // returns chars read, -1=EOF
void    sd_seekBeginning();
bool    sd_seekToLine(uint32_t lineNumber);         // seek forward N lines

// ── Stats ──────────────────────────────────────────────────────────────
uint64_t sd_totalBytes();
uint64_t sd_usedBytes();

// ── Upload helper ──────────────────────────────────────────────────────
bool sd_saveUploadedFile(const char* filename, const uint8_t* data, size_t len);

#endif // SD_MANAGER_H
