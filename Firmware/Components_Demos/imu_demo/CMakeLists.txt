# imu_demo - capilot_imu component verification
# ESP-IDF v5.5.4
cmake_minimum_required(VERSION 3.16)

# Shared components directory
set(EXTRA_COMPONENT_DIRS "B:/CaPilot/Firmware/Components")

# Exclude components not needed by this demo (prevents managed_components download)
set(EXCLUDE_COMPONENTS "capilot_usb_hid_mouse" "capilot_usb_hid_keyboard")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(imu_demo)
