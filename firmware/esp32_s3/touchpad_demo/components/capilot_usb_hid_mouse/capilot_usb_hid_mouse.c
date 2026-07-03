/**
 * @file capilot_usb_hid_mouse.c
 * @brief CaPilot USB HID 鼠标组件 - 实现
 *
 * 基于 TinyUSB 栈，通过 ESP32-S3 原生 USB OTG 接口实现 USB HID 鼠标。
 * 报告描述符：标准鼠标（3 键 + X/Y + 滚轮）。
 */
#include "capilot_usb_hid_mouse.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

static const char *TAG = "capilot_mouse";

/* 按键按下与释放之间的间隔（毫秒） */
#define MOUSE_CLICK_DELAY_MS   20

/* ============ TinyUSB 描述符 ============ */

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/* HID 报告描述符：标准鼠标（左/右/中键 + X/Y + 滚轮） */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)),
};

/* 字符串描述符 */
const char *hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},         // 0: English
    "CaPilot",                    // 1: Manufacturer
    "CaPilot Touchpad",           // 2: Product
    "CaPilotTP01",                // 3: Serial
    "CaPilot Mouse Interface",    // 4: HID
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

esp_err_t capilot_usb_hid_mouse_init(void)
{
    ESP_LOGI(TAG, "Initializing CaPilot USB HID mouse");

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

    ESP_LOGI(TAG, "USB HID mouse initialized. Connect USB-C cable to PC.");
    return ESP_OK;
}

bool capilot_usb_hid_mouse_is_connected(void)
{
    return tud_mounted();
}

bool capilot_usb_hid_mouse_ready(void)
{
    return tud_mounted() && tud_hid_ready();
}

esp_err_t capilot_usb_hid_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel)
{
    if (!tud_mounted()) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 等待 HID 端点就绪，最多 3 次每次 1ms */
    for (int i = 0; i < 3 && !tud_hid_ready(); i++) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!tud_hid_ready()) {
        return ESP_ERR_TIMEOUT;
    }
    /* tud_hid_mouse_report(report_id, buttons, x, y, vertical, horizontal) */
    if (!tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, buttons, dx, dy, wheel, 0)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t capilot_usb_hid_mouse_move(int8_t dx, int8_t dy)
{
    /* 仅移动，按键状态为 0（释放） */
    return capilot_usb_hid_mouse_report(0, dx, dy, 0);
}

esp_err_t capilot_usb_hid_mouse_press(uint8_t button)
{
    return capilot_usb_hid_mouse_report(button, 0, 0, 0);
}

esp_err_t capilot_usb_hid_mouse_release(void)
{
    return capilot_usb_hid_mouse_report(0, 0, 0, 0);
}

esp_err_t capilot_usb_hid_mouse_click(uint8_t button)
{
    esp_err_t ret = capilot_usb_hid_mouse_press(button);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(MOUSE_CLICK_DELAY_MS));
    return capilot_usb_hid_mouse_release();
}
