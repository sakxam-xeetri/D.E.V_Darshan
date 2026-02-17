/*
 * ============================================================================
 *  D.E.V_Darshan — WiFi Portal Implementation
 * ============================================================================
 *  Manages WiFi Access Point mode and HTTP web server for file upload.
 *
 *  Endpoints:
 *    GET  /        → Upload page (HTML from portal.h PROGMEM)
 *    POST /upload  → Multipart file upload handler (streams to SD)
 *    GET  /files   → JSON list of .txt files on SD
 *    GET  /usage   → JSON SD card usage info
 *
 *  RAM optimization:
 *    - HTML is in PROGMEM (flash), not RAM
 *    - File upload is streamed chunk-by-chunk to SD
 *    - WiFi is fully disabled on portal_stop()
 * ============================================================================
 */

#include "wifi_portal.h"
#include "portal.h"
#include "sd_reader.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SD_MMC.h>
#include <FS.h>

// ─── Internal State ──────────────────────────────────────────────────────────

static WebServer* server = nullptr;
static bool portalActive = false;
static File uploadFile;

// ─── HTTP Handlers ───────────────────────────────────────────────────────────

// Serve the main upload page
static void handleRoot() {
    server->send_P(200, "text/html", PORTAL_HTML);
}

// Handle file upload (multipart/form-data)
static void handleUpload() {
    HTTPUpload& upload = server->upload();

    switch (upload.status) {
        case UPLOAD_FILE_START: {
            String filename = upload.filename;

            // Validate file extension (server-side check)
            if (!filename.endsWith(".txt") && !filename.endsWith(".TXT")) {
                // Will be caught in handleUploadComplete
                return;
            }

            // Ensure filename starts with /
            if (!filename.startsWith("/")) {
                filename = "/" + filename;
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
                    // Remove oversized file
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
                // Clean up partial file
                SD_MMC.remove(uploadFile.name());
            }
            break;
        }
    }
}

// Called after upload completes to send response
static void handleUploadComplete() {
    HTTPUpload& upload = server->upload();

    // Check if file was valid
    if (!upload.filename.endsWith(".txt") && !upload.filename.endsWith(".TXT")) {
        server->send(400, "text/plain", "Only .txt files allowed");
        return;
    }

    // Check if file was written successfully
    String fullPath = "/" + upload.filename;
    if (SD_MMC.exists(fullPath)) {
        server->send(200, "text/plain", "OK");
    } else {
        server->send(500, "text/plain", "Failed to save file");
    }
}

// Return JSON list of .txt files
static void handleFileList() {
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

    server->on("/", HTTP_GET, handleRoot);
    server->on("/upload", HTTP_POST, handleUploadComplete, handleUpload);
    server->on("/files", HTTP_GET, handleFileList);
    server->on("/usage", HTTP_GET, handleUsage);

    server->begin();
    portalActive = true;

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
