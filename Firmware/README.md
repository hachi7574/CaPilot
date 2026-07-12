# CaPilot Firmware

Firmware components and demos for ESP-IDF.

## Layout

```text
Firmware/
├── components/
│   ├── capilot_audio/
│   ├── capilot_bsp/
│   ├── capilot_imu/
│   ├── capilot_usb_headset/
│   ├── capilot_usb_hid_keyboard/
│   └── capilot_usb_hid_mouse/
└── demos/
    ├── audio_demo/
    ├── imu_demo/
    ├── keyboard_demo/
    ├── record_playback_demo/
    ├── touchpad_demo/
    └── usb_headset_demo/
```

## Component Dependencies

Demos use ESP-IDF Component Manager local overrides in each demo's
`main/idf_component.yml`. Local components are referenced from `Firmware/components`
with relative `override_path` entries, for example:

```yaml
dependencies:
  capilot_audio:
    version: "*"
    override_path: "../../../components/capilot_audio"
  capilot_bsp:
    version: "*"
    override_path: "../../../components/capilot_bsp"
  idf: "^5.0"
```

Avoid adding machine-specific absolute paths or `B:/...` paths in demo
`CMakeLists.txt` files.

## ESP-IDF Environment

Use the environment documented in [ESP-IDF-Environment.md](ESP-IDF-Environment.md).
