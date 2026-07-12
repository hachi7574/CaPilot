# Demo Build Record

Date: 2026-07-13

## Environment

Followed `ESP-IDF-Environment.md`.

```bash
export IDF_PYTHON_ENV_PATH=/home/hachi/.espressif/tools/python/v5.5.4/venv
. /home/hachi/.espressif/v5.5.4/esp-idf/export.sh
idf.py --version
```

Confirmed:

- `IDF_PATH=/home/hachi/.espressif/v5.5.4/esp-idf`
- `IDF_PYTHON_ENV_PATH=/home/hachi/.espressif/tools/python/v5.5.4/venv`
- `ESP-IDF v5.5.4`
- Target: `esp32s3`

## Final Build Results

All demos under `Firmware/demos` built successfully after the fixes below.

| Demo | Final command | Result | App size |
| --- | --- | --- | --- |
| `audio_demo` | `idf.py update-dependencies reconfigure build` | PASS | `0x38f20` |
| `imu_demo` | `idf.py update-dependencies reconfigure build` | PASS | `0x37ab0` |
| `keyboard_demo` | `idf.py reconfigure build` | PASS | `0x874f0` |
| `record_playback_demo` | `idf.py set-target esp32s3 build` | PASS | `0x40c80` |
| `touchpad_demo` | `idf.py set-target esp32s3 build` | PASS | `0x821e0` |
| `usb_headset_demo` | `idf.py set-target esp32s3 build` | PASS | `0x48800` |

Bootloader size was `0x5160` for all demos.

## Issues Found And Fixed

### 1. `capilot_bsp` missing public managed-component requirements

Failure seen while building `keyboard_demo`:

```text
fatal error: esp_lcd_touch.h: No such file or directory
Compilation failed because capilot_bsp.h includes esp_lcd_touch.h,
provided by espressif__esp_lcd_touch, but that component was not in
the requirements list of capilot_bsp.
```

Root cause:

- `capilot_bsp/include/capilot_bsp.h` exposes touch/LVGL headers when `CONFIG_CAPILOT_BSP_TOUCH_ENABLE` or `CONFIG_CAPILOT_BSP_LVGL_ENABLE` is enabled.
- `Firmware/components/capilot_bsp/CMakeLists.txt` did not publish the managed components that provide those headers to consumers and to the BSP component compile step.

Fix:

- Added public requirements in `Firmware/components/capilot_bsp/CMakeLists.txt`:
  - `espressif__esp_lcd_touch`
  - `espressif__esp_lcd_touch_ft5x06`
  - `lvgl__lvgl`
  - `espressif__esp_lvgl_port`
- Added matching component-manager dependencies in `Firmware/components/capilot_bsp/idf_component.yml`:
  - `espressif/esp_lcd_touch`
  - `espressif/esp_lcd_touch_ft5x06`
  - `espressif/esp_lvgl_port`
  - `lvgl/lvgl`

Verification:

- `keyboard_demo`, `touchpad_demo`, `record_playback_demo`, `usb_headset_demo`, `audio_demo`, and `imu_demo` all built successfully after this change.

### 2. Stale `dependencies.lock` after BSP dependency changes

Failure seen during final recheck of `audio_demo` and `imu_demo`:

```text
Failed to resolve component 'espressif__esp_lcd_touch' required by
component 'capilot_bsp': unknown name.
```

Root cause:

- Existing lock files were generated before `capilot_bsp` declared the managed touch/LVGL dependencies.
- CMake saw the new BSP CMake requirements, but the stale lock file did not bring the managed components into the project.

Fix:

```bash
idf.py update-dependencies reconfigure build
```

Applied to:

- `Firmware/demos/audio_demo`
- `Firmware/demos/imu_demo`

The other demos generated or refreshed their lock files during `set-target esp32s3 build`.

## Warnings

The managed `espressif/esp_lvgl_port` component emits this warning in several builds:

```text
warning: 'esp_lcd_touch_get_coordinates' is deprecated:
This API will be removed in version 2.0.0. Use esp_lcd_touch_get_data instead!
```

This warning is from the managed dependency source and does not fail the builds.

## Log Files

Temporary logs for this run were written under `/tmp`:

- `/tmp/capilot-demo-build-audio_demo-final2.log`
- `/tmp/capilot-demo-build-imu_demo-final2.log`
- `/tmp/capilot-demo-build-keyboard_demo-retry2.log`
- `/tmp/capilot-demo-build-record_playback_demo.log`
- `/tmp/capilot-demo-build-touchpad_demo.log`
- `/tmp/capilot-demo-build-usb_headset_demo.log`
