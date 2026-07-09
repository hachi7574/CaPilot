@echo off
setlocal enabledelayedexpansion

set "IDF_PATH=A:\ESPIDF\Espressif5\frameworks\esp-idf-v5.5.4"
set "IDF_PYTHON_ENV_PATH=A:\ESPIDF\Espressif5\python_env\idf5.5_py3.11_env"
set "BASE=A:\ESPIDF\Espressif5\tools"

set "PATH=%BASE%\cmake\3.30.2\bin;%BASE%\ninja\1.12.1;%BASE%\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;%BASE%\ccache\4.11.1;%IDF_PYTHON_ENV_PATH%\Scripts;%PATH%"
set "ESP_ROM_ELF_DIR=%BASE%\esp-rom-elfs\20250120"

set "APP=B:\CaPilot\firmware\esp32_s3\imu_demo"
set "BUILD=%APP%\build"

if exist "%BUILD%" rmdir /s /q "%BUILD%"
mkdir "%BUILD%"

echo [CMAKE] Configuring...
"%BASE%\cmake\3.30.2\bin\cmake.exe" ^
  -G Ninja ^
  -DPYTHON_DEPS_CHECKED=1 ^
  -DPYTHON="%IDF_PYTHON_ENV_PATH%\Scripts\python.exe" ^
  -DESP_PLATFORM=1 ^
  -DIDF_TARGET=esp32s3 ^
  -DCMAKE_MAKE_PROGRAM="%BASE%\ninja\1.12.1\ninja.exe" ^
  -DCCACHE_ENABLE=0 ^
  -B "%BUILD%" ^
  -S "%APP%" ^
  2>"%APP%\logs\cmake_err.log" ^
  1>"%APP%\logs\cmake_out.log"

if errorlevel 1 (
  echo CMake FAILED
  type "%APP%\logs\cmake_err.log"
  exit /b 1
)

echo [NINJA] Building...
cd /d "%BUILD%"
"%BASE%\ninja\1.12.1\ninja.exe" ^
  2>"%APP%\logs\ninja_err.log" ^
  1>"%APP%\logs\ninja_out.log"

if errorlevel 1 (
  echo NINJA FAILED
  type "%APP%\logs\ninja_err.log"
  exit /b 1
)

echo BUILD OK
type "%APP%\logs\ninja_out.log" | findstr /i "imu_demo.bin Generated"
