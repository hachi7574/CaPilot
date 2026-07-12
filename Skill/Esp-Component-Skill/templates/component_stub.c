/* === 头文件 ========================================================== */
#include "{{COMPONENT_NAME}}.h"
#include "esp_log.h"

/* === 日志标签 ======================================================== */
static const char *TAG = "[{{COMPONENT_NAME}}]";

/* === 模块级变量 ====================================================== */
static bool s_initialized = false;

/* === 内部函数声明 ==================================================== */
/*
 * 在这里声明你的 static 内部函数：
 * static esp_err_t _do_something(void);
 */

/* === 公开 API 实现 =================================================== */

esp_err_t {{COMPONENT_NAME}}_init(const {{COMPONENT_NAME}}_config_t *cfg) {
    if (s_initialized) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!cfg) {
        ESP_LOGE(TAG, "config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    /* TODO: 初始化硬件 / 配置寄存器 */

    s_initialized = true;
    ESP_LOGI(TAG, "init OK");
    return ESP_OK;
}

esp_err_t {{COMPONENT_NAME}}_deinit(void) {
    if (!s_initialized) {
        return ESP_OK;
    }

    /* TODO: 释放资源 */

    s_initialized = false;
    ESP_LOGI(TAG, "deinit OK");
    return ESP_OK;
}

bool {{COMPONENT_NAME}}_is_ready(void) {
    return s_initialized;
}

/* === 内部函数实现 ==================================================== */

/*
 * static esp_err_t _do_something(void) {
 *     // 实现
 * }
 */
