/*
 * ============================================================================
 *  PocketTXT — Button Handler Implementation
 * ============================================================================
 *  Handles debouncing, short/long press detection, combo detection,
 *  and fast scroll repeat for the three-button interface.
 *
 *  Buttons are active LOW (pulled HIGH by internal pull-ups).
 *    BTN_UP     = GPIO13  (internal pull-up)
 *    BTN_DOWN   = GPIO0   (internal pull-up) ⚠️ Don't hold during power-on
 *    BTN_SELECT = GPIO12  (internal pull-up) — boot-safe: active LOW = 3.3V
 *  NO external resistors needed.
 *
 *  GPIO12 BOOT SAFETY:
 *    GPIO12 is a strapping pin (selects flash voltage: LOW=3.3V, HIGH=1.8V).
 *    This is safe because:
 *      1. INPUT_PULLUP is configured in buttons_init(), which runs well
 *         after the boot strapping has already been sampled at reset.
 *      2. The button is active LOW — pressing it pulls GPIO12 to GND.
 *      3. Even if held during power-on: LOW = 3.3V flash = correct state.
 *      4. 1-bit SD_MMC mode does NOT use GPIO12 — no conflict.
 * ============================================================================
 */

#include "buttons.h"

// ─── Internal State ──────────────────────────────────────────────────────────

struct ButtonState {
    uint8_t pin;
    bool    currentState;       // Debounced state (true = pressed)
    bool    lastRawState;       // Last raw reading
    bool    wasPressed;         // Flag for edge detection
    unsigned long debounceTime; // Last state change timestamp
    unsigned long pressStart;   // When button was first pressed
    bool    longFired;          // Long press event already sent
    unsigned long lastRepeat;   // Last fast-scroll repeat time
};

static ButtonState btnUp;
static ButtonState btnDown;
static ButtonState btnSelect;
static bool comboFired = false;

// ─── Initialization ──────────────────────────────────────────────────────────

void buttons_init() {
    // GPIO13 — internal pull-up available
    pinMode(PIN_BTN_UP, INPUT_PULLUP);

    // GPIO0 — internal pull-up available (no external resistor needed)
    // Note: GPIO0 must be HIGH at boot for normal operation.
    // Using INPUT_PULLUP keeps it HIGH when not pressed = safe boot.
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);

    // GPIO12 — internal pull-up, set AFTER boot strapping is complete.
    // Strapping was already sampled at hardware reset; changing pin mode
    // here has zero effect on flash voltage selection. Safe.
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);

    btnUp     = { PIN_BTN_UP,     false, true, false, 0, 0, false, 0 };
    btnDown   = { PIN_BTN_DOWN,   false, true, false, 0, 0, false, 0 };
    btnSelect = { PIN_BTN_SELECT, false, true, false, 0, 0, false, 0 };
    comboFired = false;
}

void buttons_readEarlyState() {
    // Read initial button states at boot (before other peripherals init).
    // In 1-bit SD_MMC mode, GPIO13 and GPIO12 are free — no SD conflict.
    btnUp.currentState     = (digitalRead(PIN_BTN_UP) == LOW);
    btnDown.currentState   = (digitalRead(PIN_BTN_DOWN) == LOW);
    btnSelect.currentState = (digitalRead(PIN_BTN_SELECT) == LOW);
}

// ─── Internal: Debounce a single button ──────────────────────────────────────

static void debounceButton(ButtonState &btn) {
    bool rawState = (digitalRead(btn.pin) == LOW);  // Active LOW

    if (rawState != btn.lastRawState) {
        btn.debounceTime = millis();
    }
    btn.lastRawState = rawState;

    if ((millis() - btn.debounceTime) > DEBOUNCE_MS) {
        if (rawState != btn.currentState) {
            btn.currentState = rawState;

            if (btn.currentState) {
                // Button just pressed
                btn.pressStart = millis();
                btn.longFired  = false;
                btn.lastRepeat = 0;
            }
        }
    }
}

// ─── Main Update — Call Every Loop ───────────────────────────────────────────

ButtonEvent buttons_update() {
    unsigned long now = millis();

    debounceButton(btnUp);
    debounceButton(btnDown);
    debounceButton(btnSelect);

    // ── Combo detection (UP+DOWN held for COMBO_PRESS_MS) ──
    // Note: SELECT is intentionally excluded from combo detection.
    // Combo remains UP+DOWN only to maintain backward compatibility.
    if (btnUp.currentState && btnDown.currentState) {
        // Both are pressed — check for combo long press
        unsigned long comboStart = max(btnUp.pressStart, btnDown.pressStart);
        if (!comboFired && (now - comboStart) >= COMBO_PRESS_MS) {
            comboFired = true;
            btnUp.longFired  = true;   // Suppress individual long presses
            btnDown.longFired = true;
            return BTN_BOTH_LONG;
        }
        return BTN_NONE;  // Still waiting for combo threshold
    }

    // Reset combo flag when either button is released
    if (!btnUp.currentState || !btnDown.currentState) {
        comboFired = false;
    }

    // ── UP button logic ──
    if (btnUp.currentState) {
        if (!btnUp.longFired && (now - btnUp.pressStart) >= LONG_PRESS_MS) {
            btnUp.longFired = true;
            btnUp.lastRepeat = now;
            return BTN_UP_LONG;
        }
        // Fast scroll repeat while held past long press
        if (btnUp.longFired && (now - btnUp.lastRepeat) >= FAST_SCROLL_INTERVAL) {
            btnUp.lastRepeat = now;
            return BTN_UP_HELD;
        }
    } else if (btnUp.wasPressed && !btnUp.currentState) {
        // Button just released — was it a short press?
        if (!btnUp.longFired) {
            btnUp.wasPressed = false;
            return BTN_UP_SHORT;
        }
        btnUp.wasPressed = false;
    }

    // Track press edge for UP
    if (btnUp.currentState && !btnUp.wasPressed) {
        btnUp.wasPressed = true;
    }

    // ── DOWN button logic ──
    if (btnDown.currentState) {
        if (!btnDown.longFired && (now - btnDown.pressStart) >= LONG_PRESS_MS) {
            btnDown.longFired = true;
            btnDown.lastRepeat = now;
            return BTN_DOWN_LONG;
        }
        if (btnDown.longFired && (now - btnDown.lastRepeat) >= FAST_SCROLL_INTERVAL) {
            btnDown.lastRepeat = now;
            return BTN_DOWN_HELD;
        }
    } else if (btnDown.wasPressed && !btnDown.currentState) {
        if (!btnDown.longFired) {
            btnDown.wasPressed = false;
            return BTN_DOWN_SHORT;
        }
        btnDown.wasPressed = false;
    }

    if (btnDown.currentState && !btnDown.wasPressed) {
        btnDown.wasPressed = true;
    }

    // ── SELECT button logic ──
    if (btnSelect.currentState) {
        if (!btnSelect.longFired && (now - btnSelect.pressStart) >= LONG_PRESS_MS) {
            btnSelect.longFired = true;
            return BTN_SELECT_LONG;
        }
        // No fast-scroll repeat for SELECT — it's a confirmation button
    } else if (btnSelect.wasPressed && !btnSelect.currentState) {
        if (!btnSelect.longFired) {
            btnSelect.wasPressed = false;
            return BTN_SELECT_SHORT;
        }
        btnSelect.wasPressed = false;
    }

    if (btnSelect.currentState && !btnSelect.wasPressed) {
        btnSelect.wasPressed = true;
    }

    return BTN_NONE;
}

// ─── Raw State Getters ───────────────────────────────────────────────────────

bool buttons_isUpPressed() {
    return btnUp.currentState;
}

bool buttons_isDownPressed() {
    return btnDown.currentState;
}

bool buttons_isSelectPressed() {
    return btnSelect.currentState;
}
