/**
 * @file capilot_audio.c
 * @brief CaPilot Audio 组件 — 组件初始化和管理
 *
 * 负责：
 *   - 注册音频芯片驱动（ES7210、ES8311）
 *   - 自动探测在线的芯片并初始化
 *   - 提供统一的初始化接口
 */

#include <string.h>
#include "capilot_audio.h"
#include "capilot_audio_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "capilot_audio";

/* ============ 已注册驱动 ============ */
/* 由具体芯片驱动文件外部声明，此处引用 */
extern const capilot_audio_driver_t s_es7210_driver;
extern const capilot_audio_driver_t s_es8311_driver;

static const capilot_audio_driver_t *s_drivers[] = {
    &s_es7210_driver,  /* ES7210 麦克风 */
    &s_es8311_driver,  /* ES8311 音频输出 */
};

/* ============ 当前激活的驱动 ============ */
static const capilot_audio_driver_t *s_active_capture_driver = NULL;
static const capilot_audio_driver_t *s_active_playback_driver = NULL;

/* ============ 内部函数 ============ */

static esp_err_t probe_drivers(void)
{
    bool found_capture = false;
    bool found_playback = false;
    
    for (size_t i = 0; i < sizeof(s_drivers) / sizeof(s_drivers[0]); i++) {
        if (s_drivers[i]->is_present()) {
            ESP_LOGI(TAG, "found audio chip: %s", s_drivers[i]->name);
            
            /* 根据驱动名称判断是采集还是播放 */
            if (strcmp(s_drivers[i]->name, "es7210") == 0) {
                s_active_capture_driver = s_drivers[i];
                found_capture = true;
            } else if (strcmp(s_drivers[i]->name, "es8311") == 0) {
                s_active_playback_driver = s_drivers[i];
                found_playback = true;
            }
        }
    }
    
    if (!found_capture && !found_playback) {
        ESP_LOGE(TAG, "no audio chip found");
        return ESP_ERR_NOT_FOUND;
    }
    
    return ESP_OK;
}

/* ============ 公共 API ============ */

esp_err_t capilot_audio_init(void)
{
    return capilot_audio_init_with_config(NULL);
}

esp_err_t capilot_audio_init_with_config(const capilot_audio_config_t *config)
{
    esp_err_t ret = ESP_OK;
    capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
    
    if (config != NULL) {
        cfg = *config;
    }
    
    /* 探测在线的芯片 */
    ret = probe_drivers();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 初始化采集驱动 */
    if (s_active_capture_driver != NULL) {
        ret = s_active_capture_driver->init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "capture driver init failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "capture driver '%s' initialized", s_active_capture_driver->name);
    }
    
    /* 初始化播放驱动 */
    if (s_active_playback_driver != NULL) {
        ret = s_active_playback_driver->init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "playback driver init failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "playback driver '%s' initialized", s_active_playback_driver->name);
    }
    
    return ESP_OK;
}

bool capilot_audio_is_present(void)
{
    for (size_t i = 0; i < sizeof(s_drivers) / sizeof(s_drivers[0]); i++) {
        if (s_drivers[i]->is_present()) {
            return true;
        }
    }
    return false;
}

const char *capilot_audio_get_driver_name(void)
{
    if (s_active_capture_driver != NULL) {
        return s_active_capture_driver->name;
    }
    if (s_active_playback_driver != NULL) {
        return s_active_playback_driver->name;
    }
    return "(none)";
}
