/*
 * ============================================================================
 *  PocketTXT — Button Handler Implementation
 * ============================================================================
 *  Handles debouncing, short/long press detection, combo detection,
 *  and fast scroll repeat for the two-button interface.
 *
 *  Buttons are active LOW (pulled HIGH by internal pull-ups).
 *  Uses GPIO13 (BTN_UP) and GPIO0 (BTN_DOWN) — both have internal pull-ups.
 *  NO external resistors needed.
 *
 *  ⚠️ GPIO0 note: If held LOW during power-on, ESP32 enters flash mode.
 *  Normal use is fine — just don't hold BTN_DOWN while powering on.
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
static bool comboFired = false;

// ─── Initialization ──────────────────────────────────────────────────────────

void buttons_init() {
    // GPIO13 — internal pull-up available
    pinMode(PIN_BTN_UP, INPUT_PULLUP);

    // GPIO0 — internal pull-up available (no external resistor needed)
    // Note: GPIO0 must be HIGH at boot for normal operation.
    // Using INPUT_PULLUP keeps it HIGH when not pressed = safe boot.
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);

    btnUp   = { PIN_BTN_UP,   false, true, false, 0, 0, false, 0 };
    btnDown = { PIN_BTN_DOWN, false, true, false, 0, 0, false, 0 };
    comboFired = false;
}

void buttons_readEarlyState() {
    // Read initial button states at boot (before other peripherals init).
    // In 1-bit SD_MMC mode, GPIO13 is free so no conflict with SD.
    btnUp.currentState = (digitalRead(PIN_BTN_UP) == LOW);
    btnDown.currentState = (digitalRead(PIN_BTN_DOWN) == LOW);
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

    // ── Combo detection (both held for COMBO_PRESS_MS) ──
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

    return BTN_NONE;
}

// ─── Raw State Getters ───────────────────────────────────────────────────────

bool buttons_isUpPressed() {
    return btnUp.currentState;
}

bool buttons_isDownPressed() {
    return btnDown.currentState;
}
