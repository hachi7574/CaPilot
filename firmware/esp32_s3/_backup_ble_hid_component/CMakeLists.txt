# capilot_hid_keyboard 组件 CMakeLists
# 依赖蓝牙、NVS、GPIO 等 ESP-IDF 原生组件

idf_component_register(
    SRCS
        "ble_hidd_demo.c"
        "esp_hidd_prf_api.c"
        "hid_dev.c"
        "hid_device_le_prf.c"
        "capilot_hid_keyboard.c"
    INCLUDE_DIRS
        "."
        "include"
    REQUIRES
        "bt"
        "nvs_flash"
        "esp_driver_gpio"
        "esp_event"
)

# 抑制第三方代码的编译警告
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-const-variable
    -Wno-unused-but-set-variable
    -Wno-unused-variable
)
