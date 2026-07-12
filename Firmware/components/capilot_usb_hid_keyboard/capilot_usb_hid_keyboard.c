/**
 * @file capilot_usb_hid_keyboard.c
 * @brief CaPilot USB HID 键盘组件 - 实现
 *
 * 基于 TinyUSB 栈，通过 ESP32-S3 原生 USB OTG 接口实现 USB HID 键盘。
 * 即插即用，无需配对。
 */
#include "capilot_usb_hid_keyboard.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"

static const char *TAG = "capilot_usb_hid";

/* 默认按键按下与释放之间的间隔（毫秒） */
#define CAPOLOT_USB_HID_KEY_DELAY_MS   20

/* ============ TinyUSB 描述符 ============ */

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/* HID 报告描述符：仅键盘（简化，去掉鼠标） */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
};

/* 字符串描述符 */
const char *hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},         // 0: English
    "CaPilot",                    // 1: Manufacturer
    "CaPilot Keyboard",           // 2: Product
    "CaPilot0001",                // 3: Serials
    "CaPilot HID Interface",      // 4: HID
};

/* 配置描述符 */
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/* ============ TinyUSB HID 回调（必须实现） ============ */

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

/* ============ API 实现 ============ */

esp_err_t capilot_usb_hid_init(void)
{
    ESP_LOGI(TAG, "Initializing CaPilot USB HID keyboard");

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
#if (TUD_OPT_HIGH_SPEED)
    tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;
#endif

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "USB HID keyboard initialized. Connect USB-C cable to PC.");
    return ESP_OK;
}

bool capilot_usb_hid_is_connected(void)
{
    /* tud_mounted() 表示 USB 已枚举成功，主机已识别设备 */
    return tud_mounted();
}

esp_err_t capilot_usb_hid_press_key_combo(uint8_t modifier, uint8_t key_code)
{
    if (!tud_mounted()) {
        ESP_LOGW(TAG, "USB not mounted, cannot send key 0x%02X", key_code);
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t keycode[6] = {0};
    keycode[0] = key_code;
    /* Report ID = HID_ITF_PROTOCOL_KEYBOARD, modifier mask + keycode array */
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
    return ESP_OK;
}

esp_err_t capilot_usb_hid_release_all(void)
{
    if (!tud_mounted()) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 发送空报告释放所有按键 */
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
    return ESP_OK;
}

esp_err_t capilot_usb_hid_send_key_combo(uint8_t modifier, uint8_t key_code)
{
    esp_err_t ret = capilot_usb_hid_press_key_combo(modifier, key_code);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(CAPOLOT_USB_HID_KEY_DELAY_MS));
    return capilot_usb_hid_release_all();
}

esp_err_t capilot_usb_hid_send_key(uint8_t key_code)
{
    return capilot_usb_hid_send_key_combo(0, key_code);
}
