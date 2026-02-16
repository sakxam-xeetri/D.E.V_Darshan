/*  =========================================================================
 *  DEV_Darshan — Power Manager Implementation
 *  Radio control, battery monitoring, sleep modes
 *  =========================================================================
 *
 *  POWER BUDGET ESTIMATION (3.7 V 800 mAh Li-ion):
 *  ┌────────────────────────┬──────────────┬──────────────────────┐
 *  │  Mode                  │  Current     │  Est. Battery Life   │
 *  ├────────────────────────┼──────────────┼──────────────────────┤
 *  │  Reading (WiFi OFF)    │  ~45-55 mA   │  ~14-17 hours        │
 *  │  WiFi AP active        │  ~120-180 mA │  ~4-6 hours          │
 *  │  Light sleep           │  ~2-5 mA     │  ~160-400 hours      │
 *  │  Deep sleep            │  ~10 µA      │  ~years              │
 *  └────────────────────────┴──────────────┴──────────────────────┘
 *
 *  Breakdown (reading mode):
 *    ESP32 core @ 80 MHz  ≈ 30 mA
 *    SD card idle          ≈  1 mA  (brief spikes to ~40 mA on read)
 *    SSD1306 OLED          ≈ 10-15 mA
 *    Misc (LDO quiescent)  ≈  2 mA
 *    TOTAL                 ≈ 45-55 mA
 *
 *  WiFi TX bursts add 80-150 mA peaks → 470 µF cap is essential.
 *  =========================================================================
 */

#include <Arduino.h>
#include "power_manager.h"
#include "config.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_sleep.h>
#include <driver/adc.h>

// ═══════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════

void power_init() {
    power_disableRadios();

    // Turn off the ESP32-CAM onboard flash LED (GPIO 4, active HIGH)
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

    // Reduce CPU frequency to 80 MHz for lower power in reader mode
    setCpuFrequencyMhz(80);

    Serial.println("[PWR] Power manager initialized, radios OFF, CPU @ 80 MHz");
}

// ═══════════════════════════════════════════════════════════════════════
//  Radio control
// ═══════════════════════════════════════════════════════════════════════

void power_disableRadios() {
    // Disable WiFi completely
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();

    // Disable Bluetooth
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    // Release BT memory
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    Serial.println("[PWR] WiFi + BT disabled");
}

void power_enableWiFi() {
    // Re-init WiFi (BT stays off)
    esp_wifi_init(NULL);
    Serial.println("[PWR] WiFi re-enabled");
}

// ═══════════════════════════════════════════════════════════════════════
//  Battery monitoring
// ═══════════════════════════════════════════════════════════════════════
//
//  NOTE: The ESP32-CAM does NOT have a built-in battery voltage divider.
//  If you add a 100 kΩ / 100 kΩ voltage divider from VBAT to GPIO 33
//  (one of the few ADC1 pins accessible), you can read battery voltage.
//  GPIO 33 is used by the camera module (not populated in this project)
//  so it's available.  Without the divider, these return estimates.

uint16_t power_batteryMV() {
    // Read ADC on GPIO 33 (ADC1_CH5) if hardware divider is present
    // For now, return a placeholder — the reed switch power design
    // means the device is either ON (battery OK) or OFF.
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);

    uint32_t raw = 0;
    for (int i = 0; i < 16; i++) {
        raw += adc1_get_raw(ADC1_CHANNEL_5);
    }
    raw /= 16;

    // With 100k/100k divider: Vbat = ADC_voltage * 2
    // ADC with 11dB attenuation: ~0-3.3V range, 12-bit = 4095
    uint16_t mv = (uint16_t)((raw * 3300UL * 2) / 4095);
    return mv;
}

uint8_t power_batteryPercent() {
    uint16_t mv = power_batteryMV();

    if (mv >= BATTERY_MV_FULL)  return 100;
    if (mv <= BATTERY_MV_EMPTY) return 0;

    // Linear approximation (good enough for Li-ion mid-range)
    return (uint8_t)((mv - BATTERY_MV_EMPTY) * 100UL /
                     (BATTERY_MV_FULL - BATTERY_MV_EMPTY));
}

// ═══════════════════════════════════════════════════════════════════════
//  Sleep modes
// ═══════════════════════════════════════════════════════════════════════

void power_lightSleep(uint32_t ms) {
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
    esp_light_sleep_start();
}

void power_deepSleep() {
    // Deep sleep — only reset or external wakeup can recover
    Serial.println("[PWR] Entering deep sleep...");
    Serial.flush();
    esp_deep_sleep_start();
    // Never reaches here
}
