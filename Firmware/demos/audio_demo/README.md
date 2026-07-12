# audio_demo

ESP-IDF demo for the `capilot_audio` component on ESP32-S3.

## Build

Use the ESP-IDF 5.5.4 environment documented in [ESP-IDF-Environment.md](ESP-IDF-Environment.md).

```bash
export IDF_PYTHON_ENV_PATH=/home/hachi/.espressif/tools/python/v5.5.4/venv
. /home/hachi/.espressif/v5.5.4/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
```

## Local Components

This project uses ESP-IDF Component Manager local overrides instead of
`EXTRA_COMPONENT_DIRS` or project-local symlinks.

The local dependencies are declared in [main/idf_component.yml](main/idf_component.yml):

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

The paths are relative to `main/idf_component.yml`, so the demo can move with the
repository without depending on a machine-specific absolute path.
