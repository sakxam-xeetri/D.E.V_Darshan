/*  =========================================================================
 *  DEV_Darshan — WiFi Portal Manager (Header)
 *  Temporary AP mode with mobile-responsive file upload
 *  =========================================================================
 */

#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include <Arduino.h>

// ── Lifecycle ──────────────────────────────────────────────────────────
void wifi_portal_start();       // enable AP + web server
void wifi_portal_loop();        // handle client requests (call in main loop)
void wifi_portal_stop();        // tear down AP + server, disable WiFi
bool wifi_portal_isActive();

#endif // WIFI_PORTAL_H
