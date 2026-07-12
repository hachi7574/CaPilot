/**
 * @file main.c
 * @brief CaPilot Keyboard Demo - 主程序（USB HID 版本）
 *
 * 功能：
 *   1. 初始化 I2C、PCA9557、LCD、LVGL 触摸屏
 *   2. 初始化 USB HID 键盘服务（TinyUSB，即插即用）
 *   3. 在屏幕上显示 3×4 数字键盘（0-9, *, #）
 *   4. 触摸数字按键即发送对应 HID 键码到电脑
 *   5. 按下开发板 BOOT 键（GPIO 0）发送回车键
 *
 * 硬件：立创实战派 ESP32-S3 开发板
 * 接线：USB-C 数据线连接到电脑（使用板载 USB OTG 接口）
 */
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "capilot_bsp.h"
#include "app_ui.h"
#include "capilot_usb_hid_keyboard.h"

static const char *TAG = "main";

/* BOOT 按键 GPIO（开发板板载按键） */
#define BOOT_BTN_GPIO       GPIO_NUM_0

/* 状态检查间隔（毫秒） */
#define STATUS_CHECK_MS     1000

/* 按键防抖间隔（毫秒） */
#define BTN_DEBOUNCE_MS     200

/* GPIO 中断队列 */
static QueueHandle_t gpio_evt_queue = NULL;

/* 上次按键时间（用于防抖） */
static int64_t last_btn_time_us = 0;

/* GPIO 中断回调 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/* BOOT 按键任务：发送回车键 */
static void boot_btn_task(void *arg)
{
    uint32_t io_num;
    bool last_connected = false;

    while (1) {
        /* 等待 GPIO 中断事件 */
        if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(STATUS_CHECK_MS)) == pdTRUE) {
            if (io_num == BOOT_BTN_GPIO) {
                int64_t now_us = esp_timer_get_time();
                if ((now_us - last_btn_time_us) < (BTN_DEBOUNCE_MS * 1000)) {
                    ESP_LOGD(TAG, "BOOT button debounce ignored");
                } else {
                    last_btn_time_us = now_us;
                    ESP_LOGI(TAG, "BOOT button pressed → sending ENTER key");

                    if (capilot_usb_hid_is_connected()) {
                        esp_err_t ret = capilot_usb_hid_send_key(HID_KEY_ENTER);
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "ENTER key sent successfully");
                        } else {
                            ESP_LOGE(TAG, "Failed to send ENTER: %s", esp_err_to_name(ret));
                        }
                    } else {
                        ESP_LOGW(TAG, "USB HID not mounted, ENTER ignored");
                    }
                }
            }
        }

        /* 周期性检查连接状态，更新 UI */
        bool now_connected = capilot_usb_hid_is_connected();
        if (now_connected != last_connected) {
            ESP_LOGI(TAG, "USB HID state changed: %s",
                     now_connected ? "MOUNTED" : "DISCONNECTED");
            app_ui_update_status(now_connected);
            last_connected = now_connected;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== CaPilot Keyboard Demo (USB HID) ===");
    ESP_LOGI(TAG, "Hardware: 立创实战派 ESP32-S3 开发板");
    ESP_LOGI(TAG, "Function: 3x4 数字键盘 + BOOT 键 = Enter");
    ESP_LOGI(TAG, "Connect USB-C cable to PC, no pairing needed");

    /* 1. 初始化 NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. 初始化 I2C 总线 */
    ESP_ERROR_CHECK(capilot_bsp_i2c_init());

    /* 3. 初始化 PCA9557 IO 扩展 */
    capilot_bsp_pca9557_init();

    /* 4. 初始化 LVGL + LCD + 触摸屏 + 背光 */
    capilot_bsp_lvgl_start();
    ESP_LOGI(TAG, "LCD and touch initialized");

    /* 5. 创建数字键盘 UI */
    app_ui_create();

    /* 6. 初始化 USB HID 键盘服务（无需蓝牙配对） */
    capilot_usb_hid_init();

    /* 7. 配置 BOOT 按键（GPIO 0）为下降沿中断 */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOOT_BTN_GPIO, gpio_isr_handler, (void *)BOOT_BTN_GPIO);

    /* 8. 创建 BOOT 按键处理任务 + 状态监控 */
    xTaskCreate(boot_btn_task, "boot_btn_task", 4 * 1024, NULL, 5, NULL);

    ESP_LOGI(TAG, "=== CaPilot Keyboard Demo Ready ===");
    ESP_LOGI(TAG, "通过 USB-C 数据线连接电脑，电脑会自动识别为键盘");
    ESP_LOGI(TAG, "触摸屏幕按键发送数字，按 BOOT 键发送回车");

    /* 主循环：打印内存使用情况 */
    while (1) {
        size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Memory: DRAM free=%zu, PSRAM free=%zu, USB mounted=%s",
                 free_dram, free_psram,
                 capilot_usb_hid_is_connected() ? "YES" : "NO");
        vTaskDelay(pdMS_TO_TICKS(10 * 1000));
    }
}
