/**
 * @file capilot_hid_keyboard.c
 * @brief CaPilot HID 键盘组件 - 高层 API 实现
 *
 * 封装底层 esp_hidd_send_keyboard_value()，提供易用的按键发送接口。
 */
#include "capilot_hid_keyboard.h"
#include "ble_hidd_demo.h"
#include "esp_hidd_prf_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "capilot_hid";

/* 默认按键按下与释放之间的间隔（毫秒） */
#define CAPOLOT_HID_KEY_DELAY_MS   20

esp_err_t capilot_hid_init(void)
{
    ESP_LOGI(TAG, "Initializing CaPilot HID keyboard service");
    bt_hid_start();
    ESP_LOGI(TAG, "CaPilot HID started. Waiting for BLE pairing...");
    return ESP_OK;
}

void capilot_hid_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing CaPilot HID keyboard service");
    bt_hid_end();
}

bool capilot_hid_is_connected(void)
{
    return sec_conn;
}

esp_err_t capilot_hid_press_key_combo(uint8_t modifier, uint8_t key_code)
{
    if (!sec_conn) {
        ESP_LOGW(TAG, "Not connected, cannot send key 0x%02X", key_code);
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t keys[1] = { key_code };
    esp_hidd_send_keyboard_value(hid_conn_id, (key_mask_t)modifier, keys, 1);
    return ESP_OK;
}

esp_err_t capilot_hid_press_key(uint8_t key_code)
{
    return capilot_hid_press_key_combo(0, key_code);
}

esp_err_t capilot_hid_release_all(void)
{
    if (!sec_conn) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 发送空报告释放所有按键 */
    uint8_t empty_keys[1] = { 0 };
    esp_hidd_send_keyboard_value(hid_conn_id, (key_mask_t)0, empty_keys, 0);
    return ESP_OK;
}

esp_err_t capilot_hid_send_key_combo(uint8_t modifier, uint8_t key_code)
{
    esp_err_t ret = capilot_hid_press_key_combo(modifier, key_code);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(CAPOLOT_HID_KEY_DELAY_MS));
    return capilot_hid_release_all();
}

esp_err_t capilot_hid_send_key(uint8_t key_code)
{
    return capilot_hid_send_key_combo(0, key_code);
}
