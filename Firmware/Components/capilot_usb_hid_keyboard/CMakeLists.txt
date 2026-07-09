# capilot_usb_hid_keyboard 组件 CMakeLists
# 基于 TinyUSB 实现 USB HID 键盘

idf_component_register(
    SRCS
        "capilot_usb_hid_keyboard.c"
    INCLUDE_DIRS
        "."
        "include"
)

# 抑制 TinyUSB 的常见警告
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
)
