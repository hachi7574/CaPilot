cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "B:/CaPilot/Firmware/Components")
set(EXCLUDE_COMPONENTS "capilot_usb_hid_mouse" "capilot_usb_hid_keyboard")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(usb_headset_demo)

# esp_tinyusb does not yet expose UAC2 in its Kconfig. The headset component
# provides the raw TinyUSB configuration, which must also be visible while the
# TinyUSB library itself is compiled.
idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_include_directories(${tinyusb_lib} BEFORE PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../Components/capilot_usb_headset/include")
