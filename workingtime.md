# Battery Runtime & Safety Guide — D.E.V_Darshan

This document provides an overview of the estimated battery performance and safety guidelines for the D.E.V_Darshan project using a **3.7V 1100mAh Li-ion/Li-Po battery**.

## 🔋 Estimated Runtime
Based on the current components (ESP32-CAM, OLED, SD Card), the estimated runtimes are as follows:

### Full Cycle (4.2V to 3.3V)
| State | Activity | Estimated Current | Estimated Life |
| :--- | :--- | :--- | :--- |
| **Idle / Reading** | 80MHz CPU + OLED On | ~75 mA | **~14-15 Hours** |
| **Extreme** | 240MHz CPU + WiFi Portal | ~230 mA | **~4-5 Hours** |

### Mid-Range Use (3.7V to 3.3V)
Using only the bottom half of the capacity (~400mAh):
*   **Idle / Reading:** ~5.3 Hours
*   **Extreme:** ~1.7 Hours

---

## ⚠️ Battery Safety & Voltage Limits
To ensure the longevity of the battery and the safety of the device, follow these voltage guidelines:

| State | Voltage | Note |
| :--- | :--- | :--- |
| **Full Charge** | **4.2V** | Stop charging here. |
| **Nominal** | **3.7V** | Average operating voltage. |
| **Recharge Soon** | **3.3V - 3.4V** | Device should be plugged in. |
| **Cut-off (Danger)**| **3.0V** | **Absolute minimum.** Avoid dropping below this. |

### Best Practices:
1. **Charging:** Use a dedicated charging IC like the **TP4056**. Safe charging current for this battery is **550mA to 800mA**.
2. **Monitoring:** It is recommended to use a voltage divider on an ESP32 ADC pin to monitor the battery level in real-time.
3. **Heat & Swelling:** If the battery feels excessively hot during use or appears swollen, disconnect it immediately.
4. **Storage:** If not using the device for a long time, store the battery at its nominal voltage (~3.7V).