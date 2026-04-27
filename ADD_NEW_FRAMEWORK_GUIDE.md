# Add a New Framework to This Project

This project currently uses PlatformIO with:

- Board: `esp32cam`
- Framework: `arduino`
- Source layout: `src_dir = .` (root folder is treated as source)

This guide shows how to safely add another framework (for example `espidf`) without breaking your current Arduino build.

## 1) Check Which Frameworks Are Supported

Run this in a terminal where PlatformIO CLI is available:

```bash
pio boards esp32cam
```

Look for the `Frameworks` field in the output.

## 2) Add a New Environment in `platformio.ini`

Keep your existing Arduino environment and add a second one for the new framework.

Example:

```ini
[platformio]
src_dir = .
include_dir = .

[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200
lib_deps =
    olikraus/U8g2
build_src_filter =
    +<*>
    -<frameworks/espidf/**>

[env:esp32cam_espidf]
platform = espressif32
board = esp32cam
framework = espidf
monitor_speed = 115200
build_src_filter =
    -<*>
    +<frameworks/espidf/**>
```

Why `build_src_filter` matters:

- Your project compiles from the root (`src_dir = .`).
- Without filtering, Arduino and ESP-IDF files can be compiled together and conflict.

## 3) Generate Files for the New Framework

### Option A: Quick manual generation (inside current project)

Create a folder:

```text
frameworks/espidf/
```

Create `frameworks/espidf/main.c` with a minimal app:

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    while (1) {
        printf("ESP-IDF app is running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### Option B: Auto-generate a clean template, then copy files

```bash
pio project init --project-dir framework_templates/espidf_template --board esp32cam --project-option "framework=espidf"
```

Then copy the generated starter files (like `src/main.c`) into `frameworks/espidf/` and adapt.

## 4) Build Each Environment

```bash
pio run -e esp32cam
pio run -e esp32cam_espidf
```

Upload with:

```bash
pio run -t upload -e esp32cam_espidf
```

## 5) Common Issues

1. `multiple definition` or entry-point errors:
   - Caused by mixing Arduino and ESP-IDF sources.
   - Fix with correct `build_src_filter`.

2. Missing PlatformIO command:
   - Open the PlatformIO CLI terminal in VS Code.
   - Or use PlatformIO IDE tasks/commands from Command Palette.

3. Missing dependencies in the new framework:
   - Arduino `lib_deps` do not automatically map to ESP-IDF components.
   - Add framework-specific components separately.

## 6) Recommended Workflow

1. Keep current `esp32cam` (Arduino) unchanged.
2. Add `esp32cam_espidf` as an experimental environment.
3. Get `pio run -e esp32cam_espidf` green.
4. Migrate features module-by-module only after successful build.

---

If you want, the next step is to also update `platformio.ini` in this repo automatically so both environments are ready right now.