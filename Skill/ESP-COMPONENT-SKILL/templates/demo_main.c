/**
 * @file    demo_main.c
 * @brief   {{COMPONENT_NAME}} 组件独立 Demo
 *
 * 本 Demo 仅验证组件功能，不包含任何业务/UI/AI 逻辑。
 *
 * 依赖：
 *   - {{COMPONENT_NAME}} 组件
 *   - 硬件最小系统（BSP I2C/GPIO 初始化）
 */

#include "{{COMPONENT_NAME}}.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "[demo_{{COMPONENT_NAME}}]";

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  {{COMPONENT_NAME}} Component Demo");
    ESP_LOGI(TAG, "========================================");

    /* 1. 硬件最小系统初始化 */
    /* TODO: 初始化 I2C/SPI/UART 等通信总线 */
    // bsp_i2c_init();

    /* 2. 组件初始化（使用默认配置） */
    {{COMPONENT_NAME}}_config_t cfg = {{COMPONENT_NAME_UPPER}}_DEFAULT_CONFIG();
    esp_err_t err = {{COMPONENT_NAME}}_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "init OK");

    /* 3. 功能验证循环 */
    while (1) {
        if ({{COMPONENT_NAME}}_is_ready()) {
            /* TODO: 调用组件核心 API，打印结果 */
            // xxx_data_t data;
            // {{COMPONENT_NAME}}_read(&data);
            // ESP_LOGI(TAG, "data: ...");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* 4. 反初始化（正常情况下不会执行到这里，但保留以验证 deinit 逻辑） */
    {{COMPONENT_NAME}}_deinit();
}
