# imu_demo main component
idf_component_register(
    SRCS "main.c" "gesture_detector.c"
    INCLUDE_DIRS "."
    REQUIRES "capilot_bsp" "capilot_imu" "esp_timer"
)
