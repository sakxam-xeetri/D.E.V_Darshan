/*
 * ============================================================================
 *  D.E.V_Darshan — WiFi Portal Implementation
 * ============================================================================
 *  Manages WiFi Access Point mode and HTTP web server for the full-featured
 *  file management portal.
 *
 *  Endpoints:
 *    GET  /          → Portal page (HTML from portal.h PROGMEM)
 *    POST /upload    → Multipart file upload handler (streams to SD)
 *    GET  /files     → JSON list of .txt files (name, size, modified)
 *    GET  /usage     → JSON SD card usage info
 *    GET  /read      → Read file content for editor (streamed)
 *    POST /save      → Save file content from editor
 *    POST /rename    → Rename a file on SD card
 *    POST /delete    → Delete a file from SD card
 *    GET  /download  → Download a file with Content-Disposition header
 *    POST /create    → Create a new empty .txt file
 *    GET  /info      → JSON device information
 *
 *  RAM optimization:
 *    - HTML is in PROGMEM (flash), not RAM
 *    - File upload is streamed chunk-by-chunk to SD
 *    - File reading for download/editor uses streamFile()
 *    - WiFi is fully disabled on portal_stop()
 *
 *  Security:
 *    - File names sanitized (no directory traversal)
 *    - .txt extension enforced on all file operations
 *    - File size limits enforced (upload: 2MB, editor: 64KB)
 * ============================================================================
 */

#include "wifi_portal.h"
#include "portal.h"
#include "sd_reader.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SD_MMC.h>
#include <FS.h>

// ─── Constants ───────────────────────────────────────────────────────────────

#define MAX_EDIT_SIZE  65536   // 64KB max for web editor save operations

// ─── Internal State ──────────────────────────────────────────────────────────

static WebServer* server = nullptr;
static bool portalActive = false;
static File uploadFile;
static bool uploadSuccess = false;
static unsigned long lastPortalActivityMs = 0;

// ─── Filename Sanitization ───────────────────────────────────────────────────
// Prevents directory traversal, strips path components, ensures .txt extension.

static String sanitizeFileName(const String& input) {
    String name = input;

    // Remove any path separators
    name.replace("/", "");
    name.replace("\\", "");

    // Remove parent directory references
    name.replace("..", "");

    // Trim whitespace
    name.trim();

    // Reject empty names
    if (name.length() == 0) {
        return "";
    }

    // Ensure .txt extension
    if (!name.endsWith(".txt") && !name.endsWith(".TXT")) {
        name += ".txt";
    }

    // Prepend / for SD path
    return "/" + name;
}

// ─── HTTP Handlers ───────────────────────────────────────────────────────────

// Serve the main portal page (PROGMEM)
static void handleRoot() {
    lastPortalActivityMs = millis();
    server->send_P(200, "text/html", PORTAL_HTML);
}

// Handle file upload (multipart/form-data) — streamed to SD
static void handleUpload() {
    HTTPUpload& upload = server->upload();

    switch (upload.status) {
        case UPLOAD_FILE_START: {
            String filename = sanitizeFileName(upload.filename);

            // Validate filename (sanitizeFileName ensures .txt extension)
            if (filename.length() == 0) {
                return;
            }

            // Open file on SD for writing
            uploadFile = SD_MMC.open(filename, FILE_WRITE);
            if (!uploadFile) {
                return;
            }
            break;
        }

        case UPLOAD_FILE_WRITE: {
            if (uploadFile) {
                // Check file size limit
                if (uploadFile.size() + upload.currentSize > MAX_UPLOAD_SIZE) {
                    uploadFile.close();
                    SD_MMC.remove(uploadFile.name());
                    return;
                }
                // Stream chunk directly to SD card
                uploadFile.write(upload.buf, upload.currentSize);
            }
            break;
        }

        case UPLOAD_FILE_END: {
            if (uploadFile) {
                uploadFile.flush();
                uploadFile.close();
            }
            break;
        }

        case UPLOAD_FILE_ABORTED: {
            if (uploadFile) {
                uploadFile.close();
                SD_MMC.remove(uploadFile.name());
            }
            break;
        }
    }
}

// Called after upload completes to send response
static void handleUploadComplete() {
    HTTPUpload& upload = server->upload();

    // Get sanitized filename (same path used in handleUpload)
    String fullPath = sanitizeFileName(upload.filename);
    
    if (fullPath.length() == 0) {
        server->send(400, "text/plain", "Invalid filename");
        return;
    }

    // Check if file was written successfully
    if (SD_MMC.exists(fullPath)) {
        uploadSuccess = true;
        lastPortalActivityMs = millis();
        server->send(200, "text/plain", "OK");
    } else {
        server->send(500, "text/plain", "Failed to save file");
    }
}

// Return JSON list of .txt files with size and last modified timestamp
static void handleFileList() {
    lastPortalActivityMs = millis();
    File root = SD_MMC.open("/");
    if (!root) {
        server->send(500, "application/json", "[]");
        return;
    }

    String json = "[";
    bool first = true;
    File entry;

    while ((entry = root.openNextFile())) {
        if (entry.isDirectory()) continue;

        const char* name = entry.name();
        int len = strlen(name);
        if (len > 4) {
            const char* ext = name + len - 4;
            if (strcasecmp(ext, ".txt") == 0) {
                if (!first) json += ",";
                first = false;

                const char* displayName = name;
                if (displayName[0] == '/') displayName++;

                json += "{\"name\":\"";
                json += displayName;
                json += "\",\"size\":";
                json += String(entry.size());
                json += ",\"modified\":";
                json += String((unsigned long)entry.getLastWrite());
                json += "}";
            }
        }
        entry.close();
    }
    root.close();

    json += "]";
    server->send(200, "application/json", json);
}

// Return JSON SD usage info
static void handleUsage() {
    lastPortalActivityMs = millis();
    uint64_t total = SD_MMC.totalBytes();
    uint64_t used  = SD_MMC.usedBytes();
    uint64_t free  = total - used;

    String json = "{\"total\":";
    json += String((unsigned long)total);
    json += ",\"used\":";
    json += String((unsigned long)used);
    json += ",\"free\":";
    json += String((unsigned long)free);
    json += "}";

    server->send(200, "application/json", json);
}

// ─── New Endpoints: File Management ──────────────────────────────────────────

// Read file content for the web editor (streamed)
static void handleReadFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("name")) {
        server->send(400, "text/plain", "Missing file name");
        return;
    }

    String path = sanitizeFileName(server->arg("name"));
    if (path.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        server->send(404, "text/plain", "File not found");
        return;
    }

    // Stream file content directly — memory efficient
    server->streamFile(f, "text/plain");
    f.close();
}

// Save file content from the web editor
static void handleSaveFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("name")) {
        server->send(400, "text/plain", "Missing file name");
        return;
    }

    String path = sanitizeFileName(server->arg("name"));
    if (path.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    // Get the raw POST body (file content)
    String content = server->arg("plain");

    // Enforce editor size limit
    if (content.length() > MAX_EDIT_SIZE) {
        server->send(413, "text/plain", "Content too large (max 64KB for web editor)");
        return;
    }

    // Write content to SD card
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
        server->send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    size_t written = f.print(content);
    f.flush();
    f.close();

    if (written > 0 || content.length() == 0) {
        server->send(200, "text/plain", "OK");
    } else {
        server->send(500, "text/plain", "Write failed");
    }
}

// Rename a file on SD card
static void handleRenameFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("old") || !server->hasArg("new")) {
        server->send(400, "text/plain", "Missing old or new name");
        return;
    }

    String oldPath = sanitizeFileName(server->arg("old"));
    String newPath = sanitizeFileName(server->arg("new"));

    if (oldPath.length() == 0 || newPath.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    if (oldPath == newPath) {
        server->send(200, "text/plain", "OK");  // Same name, no-op
        return;
    }

    if (!SD_MMC.exists(oldPath)) {
        server->send(404, "text/plain", "Source file not found");
        return;
    }

    if (SD_MMC.exists(newPath)) {
        server->send(409, "text/plain", "A file with that name already exists");
        return;
    }

    if (SD_MMC.rename(oldPath, newPath)) {
        server->send(200, "text/plain", "OK");
    } else {
        server->send(500, "text/plain", "Rename failed");
    }
}

// Delete a file from SD card
static void handleDeleteFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("name")) {
        server->send(400, "text/plain", "Missing file name");
        return;
    }

    String path = sanitizeFileName(server->arg("name"));
    if (path.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    if (!SD_MMC.exists(path)) {
        server->send(404, "text/plain", "File not found");
        return;
    }

    if (SD_MMC.remove(path)) {
        server->send(200, "text/plain", "OK");
    } else {
        server->send(500, "text/plain", "Delete failed");
    }
}

// Download a file with appropriate Content-Disposition header
static void handleDownloadFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("name")) {
        server->send(400, "text/plain", "Missing file name");
        return;
    }

    String rawName = server->arg("name");
    String path = sanitizeFileName(rawName);
    if (path.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        server->send(404, "text/plain", "File not found");
        return;
    }

    // Extract the display filename (without leading /)
    String displayName = path;
    if (displayName.startsWith("/")) displayName = displayName.substring(1);

    // Set Content-Disposition for download behavior
    server->sendHeader("Content-Disposition",
                       "attachment; filename=\"" + displayName + "\"");
    server->streamFile(f, "application/octet-stream");
    f.close();
}

// Create a new empty .txt file
static void handleCreateFile() {
    lastPortalActivityMs = millis();

    if (!server->hasArg("name")) {
        server->send(400, "text/plain", "Missing file name");
        return;
    }

    String path = sanitizeFileName(server->arg("name"));
    if (path.length() == 0) {
        server->send(400, "text/plain", "Invalid file name");
        return;
    }

    if (SD_MMC.exists(path)) {
        server->send(409, "text/plain", "File already exists");
        return;
    }

    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
        server->send(500, "text/plain", "Failed to create file");
        return;
    }
    f.close();

    server->send(200, "text/plain", "OK");
}

// Return JSON device information
static void handleInfo() {
    lastPortalActivityMs = millis();

    // Count .txt files
    int fileCount = 0;
    File root = SD_MMC.open("/");
    if (root) {
        File entry;
        while ((entry = root.openNextFile())) {
            if (!entry.isDirectory()) {
                const char* name = entry.name();
                int len = strlen(name);
                if (len > 4 && strcasecmp(name + len - 4, ".txt") == 0) {
                    fileCount++;
                }
            }
            entry.close();
        }
        root.close();
    }

    // Build JSON response
    String json = "{";
    json += "\"firmware\":\"" + String(FW_VERSION) + "\"";
    json += ",\"device\":\"" + String(DEVICE_NAME) + "\"";
    json += ",\"author\":\"" + String(DEVELOPER_NAME) + "\"";
    json += ",\"files\":" + String(fileCount);
    json += ",\"sdMounted\":";
    json += (SD_MMC.cardType() != CARD_NONE) ? "true" : "false";
    json += ",\"totalBytes\":" + String((unsigned long)SD_MMC.totalBytes());
    json += ",\"usedBytes\":" + String((unsigned long)SD_MMC.usedBytes());
    json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
    json += ",\"cpuFreq\":" + String(getCpuFrequencyMhz());
    json += ",\"ssid\":\"" + String(WIFI_SSID) + "\"";
    json += ",\"maxUpload\":" + String(MAX_UPLOAD_SIZE);
    json += ",\"wifiTimeout\":" + String(WIFI_TIMEOUT_MS);
    json += ",\"uptime\":" + String((unsigned long)(millis() / 1000));
    json += "}";

    server->send(200, "application/json", json);
}

// Handle unknown routes
static void handleNotFound() {
    server->send(404, "text/plain", "Not found");
}

// ─── Portal Start/Stop ───────────────────────────────────────────────────────

bool portal_start() {
    // Increase CPU for WiFi operations
    setCpuFrequencyMhz(NORMAL_CPU_MHZ);

    // Start WiFi in AP mode
    WiFi.mode(WIFI_AP);
    delay(100);

    bool success = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, 1);
    if (!success) {
        WiFi.mode(WIFI_OFF);
        setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);
        return false;
    }

    delay(200);  // Allow AP to stabilize

    // Create and configure web server
    if (server) {
        delete server;
    }
    server = new WebServer(80);

    // ── Route Registration ───────────────────────────────────────────────

    // Portal page
    server->on("/", HTTP_GET, handleRoot);

    // File upload (existing)
    server->on("/upload", HTTP_POST, handleUploadComplete, handleUpload);

    // File listing & SD info (existing, updated)
    server->on("/files", HTTP_GET, handleFileList);
    server->on("/usage", HTTP_GET, handleUsage);

    // File management (new)
    server->on("/read",     HTTP_GET,  handleReadFile);
    server->on("/save",     HTTP_POST, handleSaveFile);
    server->on("/rename",   HTTP_POST, handleRenameFile);
    server->on("/delete",   HTTP_POST, handleDeleteFile);
    server->on("/download", HTTP_GET,  handleDownloadFile);
    server->on("/create",   HTTP_POST, handleCreateFile);

    // Device info (new)
    server->on("/info", HTTP_GET, handleInfo);

    // 404 handler
    server->onNotFound(handleNotFound);

    server->begin();
    portalActive = true;
    uploadSuccess = false;
    lastPortalActivityMs = millis();

    return true;
}

void portal_stop() {
    if (server) {
        server->stop();
        server->close();
        delete server;
        server = nullptr;
    }

    // Full WiFi shutdown
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // Reduce CPU back to low power
    setCpuFrequencyMhz(LOW_POWER_CPU_MHZ);

    portalActive = false;
}

void portal_handleClient() {
    if (server && portalActive) {
        server->handleClient();
    }
}

bool portal_isActive() {
    return portalActive;
}

bool portal_uploadCompleted() {
    return uploadSuccess;
}

unsigned long portal_lastActivity() {
    return lastPortalActivityMs;
}
