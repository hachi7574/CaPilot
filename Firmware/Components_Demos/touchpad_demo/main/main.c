/**
 * @file main.c
 * @brief CaPilot Touchpad Demo - 主程序
 *
 * 功能：
 *   1. 初始化 I2C、PCA9557、LCD、LVGL 触摸屏
 *   2. 初始化 USB HID 鼠标服务（TinyUSB，即插即用）
 *   3. 全屏触摸板：滑动=鼠标移动，双击=左键单击，三击=左键双击
 *   4. BOOT 键（GPIO 0）= 鼠标右键单击
 *
 * 硬件：立创实战派 ESP32-S3 开发板
 * 接线：USB-C 数据线连接到电脑（板载 USB OTG 接口）
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
#include "capilot_usb_hid_mouse.h"

static const char *TAG = "main";

/* BOOT 按键 GPIO */
#define BOOT_BTN_GPIO       GPIO_NUM_0

/* 状态检查间隔 */
#define STATUS_CHECK_MS     1000

/* 按键防抖 */
#define BTN_DEBOUNCE_MS     200

/* GPIO 中断队列 */
static QueueHandle_t gpio_evt_queue = NULL;

/* 上次按键时间（防抖） */
static int64_t last_btn_time_us = 0;

/* GPIO 中断回调 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/* BOOT 按键任务：右键单击 */
static void boot_btn_task(void *arg)
{
    uint32_t io_num;
    bool last_connected = false;

    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(STATUS_CHECK_MS)) == pdTRUE) {
            if (io_num == BOOT_BTN_GPIO) {
                int64_t now_us = esp_timer_get_time();
                if ((now_us - last_btn_time_us) < (BTN_DEBOUNCE_MS * 1000)) {
                    ESP_LOGD(TAG, "BOOT button debounce ignored");
                } else {
                    last_btn_time_us = now_us;
                    ESP_LOGI(TAG, "BOOT pressed → Right Click");

                    if (capilot_usb_hid_mouse_is_connected()) {
                        esp_err_t ret = capilot_usb_hid_mouse_click(CAPOLOT_MOUSE_BTN_RIGHT);
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Right click sent");
                        } else {
                            ESP_LOGE(TAG, "Right click failed: %s", esp_err_to_name(ret));
                        }
                    } else {
                        ESP_LOGW(TAG, "USB not mounted, right click ignored");
                    }
                }
            }
        }

        /* 周期检查连接状态，更新 UI */
        bool now_connected = capilot_usb_hid_mouse_is_connected();
        if (now_connected != last_connected) {
            ESP_LOGI(TAG, "USB HID state: %s",
                     now_connected ? "MOUNTED" : "DISCONNECTED");
            app_ui_update_status(now_connected);
            last_connected = now_connected;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== CaPilot Touchpad Demo (USB HID Mouse) ===");
    ESP_LOGI(TAG, "Hardware: 立创实战派 ESP32-S3");
    ESP_LOGI(TAG, "Function: 滑动=移动, 双击=左键, 三击=左键双击, BOOT=右键");
    ESP_LOGI(TAG, "Connect USB-C cable to PC, no pairing needed");

    /* 1. NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. I2C */
    ESP_ERROR_CHECK(capilot_bsp_i2c_init());

    /* 3. PCA9557 */
    capilot_bsp_pca9557_init();

    /* 4. LCD + LVGL + 触摸 */
    capilot_bsp_lvgl_start();
    ESP_LOGI(TAG, "LCD and touch initialized");

    /* 5. 触摸板 UI */
    app_ui_create();

    /* 6. USB HID 鼠标 */
    capilot_usb_hid_mouse_init();

    /* 7. BOOT 键 → 右键 */
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

    /* 8. 按键任务 + 状态监控 */
    xTaskCreate(boot_btn_task, "boot_btn_task", 4 * 1024, NULL, 5, NULL);

    ESP_LOGI(TAG, "=== CaPilot Touchpad Ready ===");
    ESP_LOGI(TAG, "滑动 = 鼠标移动 | 双击 = 左键单击 | 三击 = 左键双击");
    ESP_LOGI(TAG, "按 BOOT 键 = 鼠标右键单击");

    /* 主循环：内存监控 */
    while (1) {
        size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Memory: DRAM=%zu, PSRAM=%zu, USB=%s",
                 free_dram, free_psram,
                 capilot_usb_hid_mouse_is_connected() ? "YES" : "NO");
        vTaskDelay(pdMS_TO_TICKS(10 * 1000));
    }
}
