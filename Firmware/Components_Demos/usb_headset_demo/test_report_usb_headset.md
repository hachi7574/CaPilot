# USB Headset Demo Test Report

## Test environment

- Board: Lceda ESP32-S3 Practical Board
- Framework: ESP-IDF v5.5.4
- TinyUSB: espressif/esp_tinyusb 2.2.1 + TinyUSB 0.19.0~3
- Date: 2026-07-10

## Build verification

| ID | Check | Expected result | Actual result | Status |
| --- | --- | --- | --- | --- |
| B01 | Fresh CMake configuration | Dependencies resolve | UAC2/HID dependencies resolved | PASS |
| B02 | Firmware build | 0 errors | Ninja exit code 0 | PASS |
| B03 | Descriptor size validation | UAC2 emitted length matches TinyUSB configuration | `_Static_assert` passed | PASS |

## Hardware verification checklist

| ID | Check | Expected result | Status |
| --- | --- | --- | --- |
| H01 | USB enumeration | PC lists `CaPilot USB Headset` playback and microphone devices | Pending flash |
| H02 | Playback | PC audio plays through ES8311/speaker | Pending flash |
| H03 | Capture | PC records ES7210 microphone input | Pending flash |
| H04 | BOOT press | HID reports Win+Alt down | Pending flash |
| H05 | BOOT long hold | HID keeps Win+Alt held until BOOT is released | Pending flash |

## Notes

The demo intentionally does not initialize the camera, display, touch, or LVGL. USB requires a data-capable cable connected to the board's native USB data port.
