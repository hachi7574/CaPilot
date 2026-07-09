# CaPilot Touchpad Demo - 顶层 CMakeLists
# ESP-IDF v5.5.4
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "B:/CaPilot/Firmware/Components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(touchpad_demo)
