/*
 * ============================================================================
 *  D.E.V_Darshan — Button Handler Header
 * ============================================================================
 *  Debounced three-button input with short press, long press detection.
 *    BTN_UP     = GPIO13  (scroll up / previous)
 *    BTN_DOWN   = GPIO0   (scroll down / next)
 *    BTN_SELECT = GPIO12  (enter / back / WiFi portal)
 *
 *  Navigation Rules:
 *    UP            → Move up
 *    DOWN          → Move down
 *    SELECT short  → Enter (if sub-options) / Back (if terminal screen)
 * ============================================================================
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "config.h"

// Initialize button pins (must be called AFTER boot strapping completes)
void buttons_init();

// Read early button state (before SD_MMC takes over shared pins)
void buttons_readEarlyState();

// Call every loop iteration — processes debounce and timing
ButtonEvent buttons_update();

// Get raw button state
bool buttons_isUpPressed();
bool buttons_isDownPressed();
bool buttons_isSelectPressed();

#endif // BUTTONS_H
