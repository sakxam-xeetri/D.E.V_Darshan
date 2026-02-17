/*
 * ============================================================================
 *  D.E.V_Darshan — WiFi Portal Header
 * ============================================================================
 *  Access Point mode with mobile-responsive file upload web server.
 * ============================================================================
 */

#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include "config.h"

// Start WiFi Access Point and HTTP server
// Returns true if AP started successfully
bool portal_start();

// Stop WiFi and HTTP server, disable radios
void portal_stop();

// Handle HTTP requests — call in main loop while portal is active
void portal_handleClient();

// Check if portal is currently active
bool portal_isActive();

#endif // WIFI_PORTAL_H
