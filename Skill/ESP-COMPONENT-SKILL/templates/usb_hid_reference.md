# USB HID 组件参考实现（TinyUSB）

基于 espressif/esp_tinyusb ^2.0.0，适用于立创实战派 ESP32-S3。

## sdkconfig 必需项

```ini
CONFIG_TINYUSB_HID_COUNT=1
```

## 关键代码结构

```c
#include "tinyusb.h"
#include "class/hid/hid_device.h"

// 1. 报告描述符（仅键盘）
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
};

// 2. 配置描述符
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

// 3. 必须实现的回调
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    return hid_report_descriptor;
}
uint16_t tud_hid_get_report_cb(...) { return 0; }
void tud_hid_set_report_cb(...) {}

// 4. 初始化
tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
tusb_cfg.descriptor.string = hid_string_descriptor;
tinyusb_driver_install(&tusb_cfg);

// 5. 发送按键（按下 + 释放）
uint8_t keycode[6] = {HID_KEY_A};
tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);  // 按下
vTaskDelay(pdMS_TO_TICKS(20));
tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);     // 释放

// 6. 检查连接
bool connected = tud_mounted();
```

## 参考组件

`B:/CaPilot/firmware/esp32_s3/keyboard_demo/components/capilot_usb_hid_keyboard/` 是已验证可用的实现，可直接参考。
