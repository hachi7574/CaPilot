# audio_demo - capilot_audio component verification
# ESP-IDF v5.5.4
cmake_minimum_required(VERSION 3.16)

# Shared components directory
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../Components")

# Exclude components not needed by this demo
set(EXCLUDE_COMPONENTS "capilot_usb_hid_mouse" "capilot_usb_hid_keyboard")

set(PROJECT_IDF_PATH "/home/hachi/.espressif/v5.5.4/esp-idf")
set(PROJECT_IDF_TOOLS_PATH "/home/hachi/.espressif")
set(PROJECT_IDF_PYTHON_ENV "${PROJECT_IDF_TOOLS_PATH}/python_env/idf5.5_py3.14_env/bin")
set(PROJECT_IDF_PYTHON "${PROJECT_IDF_PYTHON_ENV}/python")
set(PROJECT_XTENSA_TOOLCHAIN "${PROJECT_IDF_TOOLS_PATH}/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin")

set(ENV{IDF_PATH} "${PROJECT_IDF_PATH}")
set(ENV{IDF_TOOLS_PATH} "${PROJECT_IDF_TOOLS_PATH}")
set(ENV{PATH} "${PROJECT_IDF_PYTHON_ENV}:${PROJECT_XTENSA_TOOLCHAIN}:$ENV{PATH}")
set(PYTHON "${PROJECT_IDF_PYTHON}" CACHE FILEPATH "Python interpreter for ESP-IDF" FORCE)
set(Python3_EXECUTABLE "${PROJECT_IDF_PYTHON}" CACHE FILEPATH "Python3 interpreter for ESP-IDF" FORCE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(audio_demo)
