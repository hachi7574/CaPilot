# ESP-IDF Environment

This firmware tree is verified with ESP-IDF 5.5.4 for `esp32s3`.

## Required Paths

ESP-IDF installation:

```text
/home/hachi/.espressif/v5.5.4/esp-idf
```

Python virtual environment:

```text
/home/hachi/.espressif/tools/python/v5.5.4/venv
```

Do not use this environment unless it has been rebuilt:

```text
/home/hachi/.espressif/python_env/idf5.5_py3.14_env
```

It was observed to fail during `export.sh` with:

```text
No module named 'rich'
```

## Activation

Run these commands before building demos:

```bash
export IDF_PYTHON_ENV_PATH=/home/hachi/.espressif/tools/python/v5.5.4/venv
. /home/hachi/.espressif/v5.5.4/esp-idf/export.sh
```

Confirm the environment:

```bash
echo "$IDF_PATH"
echo "$IDF_PYTHON_ENV_PATH"
idf.py --version
```

Expected values:

```text
IDF_PATH=/home/hachi/.espressif/v5.5.4/esp-idf
IDF_PYTHON_ENV_PATH=/home/hachi/.espressif/tools/python/v5.5.4/venv
```

## Build Example

```bash
cd /home/hachi/Project/CaPilot/Firmware/demos/audio_demo
idf.py set-target esp32s3
idf.py build
```
