/*  =========================================================================
 *  DEV_Darshan — Input Manager Implementation
 *  Three-button input with debounce, long-press detection, and auto-repeat
 *  =========================================================================
 */

#include <Arduino.h>
#include "input_manager.h"
#include "config.h"

// ── Per-button state machine ───────────────────────────────────────────
struct ButtonState {
    uint8_t  pin;
    bool     lastReading;
    bool     stableState;      // debounced state (true = pressed for active-LOW)
    uint32_t lastChangeMs;
    uint32_t pressStartMs;
    bool     longFired;        // long-press event already emitted?
    uint32_t lastRepeatMs;
};

static ButtonState _btns[3];

// ── Internal helpers ───────────────────────────────────────────────────

static void _initBtn(ButtonState& b, uint8_t pin) {
    b.pin          = pin;
    b.lastReading  = false;
    b.stableState  = false;
    b.lastChangeMs = 0;
    b.pressStartMs = 0;
    b.longFired    = false;
    b.lastRepeatMs = 0;

    // All buttons use ESP32 internal pull-ups (INPUT_PULLUP mode)
    // This works for GPIO 1, 3, and 16 — no external resistors needed!
    pinMode(pin, INPUT_PULLUP);
}

/*  Debounce one button. Returns:
 *  0 = no event, 1 = short press released, 2 = long press detected,
 *  3 = repeat tick while held
 */
static uint8_t _pollBtn(ButtonState& b) {
    bool raw = (digitalRead(b.pin) == LOW);  // active LOW
    uint32_t now = millis();

    // Debounce
    if (raw != b.lastReading) {
        b.lastChangeMs = now;
        b.lastReading  = raw;
    }

    if ((now - b.lastChangeMs) < DEBOUNCE_MS) return 0;

    // State changed after debounce
    if (raw != b.stableState) {
        b.stableState = raw;

        if (raw) {
            // Just pressed
            b.pressStartMs = now;
            b.longFired    = false;
            b.lastRepeatMs = now;
        } else {
            // Just released
            if (!b.longFired) {
                return 1;   // short press
            }
            // Long was already fired on hold — nothing on release
            return 0;
        }
    }

    // While held
    if (b.stableState) {
        uint32_t held = now - b.pressStartMs;

        // Long press detection
        if (!b.longFired && held >= LONG_PRESS_MS) {
            b.longFired    = true;
            b.lastRepeatMs = now;
            return 2;   // long press
        }

        // Auto-repeat after long press
        if (b.longFired) {
            uint32_t interval = (held < 2000) ? REPEAT_INITIAL_MS : REPEAT_FAST_MS;
            if ((now - b.lastRepeatMs) >= interval) {
                b.lastRepeatMs = now;
                return 3;   // repeat
            }
        }
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════

void input_init() {
    _initBtn(_btns[0], PIN_BTN_UP);
    _initBtn(_btns[1], PIN_BTN_DOWN);
    _initBtn(_btns[2], PIN_BTN_SELECT);
}

BtnEvent input_poll() {
    // Poll UP
    uint8_t ev = _pollBtn(_btns[0]);
    if (ev == 1) return BtnEvent::UP_SHORT;
    if (ev == 2) return BtnEvent::UP_LONG;
    if (ev == 3) return BtnEvent::UP_REPEAT;

    // Poll DOWN
    ev = _pollBtn(_btns[1]);
    if (ev == 1) return BtnEvent::DOWN_SHORT;
    if (ev == 2) return BtnEvent::DOWN_LONG;
    if (ev == 3) return BtnEvent::DOWN_REPEAT;

    // Poll SELECT
    ev = _pollBtn(_btns[2]);
    if (ev == 1) return BtnEvent::SEL_SHORT;
    if (ev == 2) return BtnEvent::SEL_LONG;

    return BtnEvent::NONE;
}
