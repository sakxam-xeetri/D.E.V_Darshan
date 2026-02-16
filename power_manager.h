/*  =========================================================================
 *  DEV_Darshan — Power Manager (Header)
 *  WiFi/BT disable, light-sleep helpers, battery estimation
 *  =========================================================================
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

// ── Lifecycle ──────────────────────────────────────────────────────────
void power_init();                  // disable WiFi + BT, configure ADC

// ── Radio control ──────────────────────────────────────────────────────
void power_disableRadios();         // WiFi OFF, BT OFF
void power_enableWiFi();            // re-enable WiFi (for portal)

// ── Battery ────────────────────────────────────────────────────────────
uint8_t  power_batteryPercent();    // 0-100 %
uint16_t power_batteryMV();        // raw millivolts (approximate)

// ── Sleep ──────────────────────────────────────────────────────────────
void power_lightSleep(uint32_t ms);       // timed light sleep
void power_deepSleep();                    // never returns until reset

#endif // POWER_MANAGER_H
