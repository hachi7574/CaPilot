idf_component_register(
    SRCS "capilot_usb_headset.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_tinyusb" "tinyusb" "freertos"
)
