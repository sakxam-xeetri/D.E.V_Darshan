/*
 * ============================================================================
 *  D.E.V_Darshan — Button Handler Implementation
 * ============================================================================
 *  Handles debouncing, short/long/xlong press detection, and fast scroll
 *  repeat for the three-button interface.
 *
 *  Buttons are active LOW (pulled HIGH by internal pull-ups).
 *    BTN_UP     = GPIO13  (internal pull-up)
 *    BTN_DOWN   = GPIO0   (internal pull-up) ⚠️ Don't hold during power-on
 *    BTN_SELECT = GPIO12  (internal pull-up) — boot-safe
 *
 *  SELECT timing:
 *    Short release  → BTN_SELECT_SHORT  (Enter / Back, context-aware)
 *
 *  UP/DOWN timing:
 *    Short release  → BTN_UP/DOWN_SHORT (single step)
 *    Hold 800ms     → BTN_UP/DOWN_LONG  (fast scroll start)
 *    Each 150ms     → BTN_UP/DOWN_HELD  (fast scroll repeat)
 *
 *  NO external resistors needed.
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
    bool    xlongFired;         // Extra-long press event already sent (SELECT only)
    unsigned long lastRepeat;   // Last fast-scroll repeat time
};

static ButtonState btnUp;
static ButtonState btnDown;
static ButtonState btnSelect;

// ─── Initialization ──────────────────────────────────────────────────────────

void buttons_init() {
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);

    btnUp     = { PIN_BTN_UP,     false, true, false, 0, 0, false, false, 0 };
    btnDown   = { PIN_BTN_DOWN,   false, true, false, 0, 0, false, false, 0 };
    btnSelect = { PIN_BTN_SELECT, false, true, false, 0, 0, false, false, 0 };
}

void buttons_readEarlyState() {
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
                btn.xlongFired = false;
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

    // ── UP button (SCROLL_HOLD_MS threshold for fast scroll) ──
    if (btnUp.currentState) {
        if (!btnUp.longFired && (now - btnUp.pressStart) >= SCROLL_HOLD_MS) {
            btnUp.longFired = true;
            btnUp.lastRepeat = now;
            return BTN_UP_LONG;
        }
        if (btnUp.longFired && (now - btnUp.lastRepeat) >= FAST_SCROLL_INTERVAL) {
            btnUp.lastRepeat = now;
            return BTN_UP_HELD;
        }
    } else if (btnUp.wasPressed && !btnUp.currentState) {
        if (!btnUp.longFired) {
            btnUp.wasPressed = false;
            return BTN_UP_SHORT;
        }
        btnUp.wasPressed = false;
    }
    if (btnUp.currentState && !btnUp.wasPressed) {
        btnUp.wasPressed = true;
    }

    // ── DOWN button (SCROLL_HOLD_MS threshold for fast scroll) ──
    if (btnDown.currentState) {
        if (!btnDown.longFired && (now - btnDown.pressStart) >= SCROLL_HOLD_MS) {
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

    // ── SELECT button (short press only — context-aware Enter/Back) ──
    if (!btnSelect.currentState && btnSelect.wasPressed) {
        btnSelect.wasPressed = false;
        return BTN_SELECT_SHORT;
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
