# USB Headset Demo

ESP32-S3 USB UAC2 headset demo for the CaPilot practical board.

- Computer playback -> ES8311 speaker output (48 kHz, 16-bit stereo)
- ES7210 microphone -> computer recording input (48 kHz, 16-bit mono)
- BOOT (GPIO0) press/release -> HID Win+Alt press/release; holding BOOT keeps both modifiers held

Use the board's USB data connection, not a charge-only cable. This demo does not initialize the camera, display, touch, or LVGL.
