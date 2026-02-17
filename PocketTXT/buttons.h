/*
 * ============================================================================
 *  PocketTXT — Button Handler Header
 * ============================================================================
 *  Debounced three-button input with short press, long press, combo detection.
 * ============================================================================
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "config.h"

// Initialize button pins
void buttons_init();

// Read early button state (before SD_MMC takes over GPIO13)
void buttons_readEarlyState();

// Call every loop iteration — processes debounce and timing
ButtonEvent buttons_update();

// Get raw button state (for combo detection)
bool buttons_isUpPressed();
bool buttons_isDownPressed();
bool buttons_isSelectPressed();

#endif // BUTTONS_H
