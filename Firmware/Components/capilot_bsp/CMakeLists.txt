# capilot_bsp 组件 CMakeLists
# 立创实战派 ESP32-S3 板级支持包

# 条件链接 managed component
set(_capilot_reqs
    "freertos"
    "esp_driver_gpio"
    "esp_driver_i2c"
    "esp_driver_spi"
    "esp_driver_ledc"
    "esp_lcd"
)

if(CONFIG_CAPILOT_BSP_TOUCH_ENABLE)
    list(APPEND _capilot_reqs "esp_lcd_touch" "esp_lcd_touch_ft5x06")
endif()

if(CONFIG_CAPILOT_BSP_LVGL_ENABLE)
    list(APPEND _capilot_reqs "lvgl" "esp_lvgl_port")
endif()

idf_component_register(
    SRCS "capilot_bsp.c"
    INCLUDE_DIRS "." "include"
    REQUIRES ${_capilot_reqs}
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
)
