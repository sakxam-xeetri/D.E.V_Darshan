/*  =========================================================================
 *  DEV_Darshan — Input Manager (Header)
 *  Three physical buttons with debounce + long-press + repeat
 *  =========================================================================
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

// ── Button events ──────────────────────────────────────────────────────
enum class BtnEvent : uint8_t {
    NONE,
    UP_SHORT,
    UP_LONG,
    UP_REPEAT,
    DOWN_SHORT,
    DOWN_LONG,
    DOWN_REPEAT,
    SEL_SHORT,
    SEL_LONG
};

// ── Lifecycle ──────────────────────────────────────────────────────────
void      input_init();
BtnEvent  input_poll();     // call every loop iteration — returns event

#endif // INPUT_MANAGER_H
