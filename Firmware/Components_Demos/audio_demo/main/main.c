/**
 * @file main.c
 * @brief audio_demo — CaPilot Audio 组件功能验证
 *
 * 测试内容：
 * 1. I2C 探测 ES7210 + ES8311 是否在线
 * 2. 播放 1kHz beep 测试 ES8311
 * 3. 读取 100ms 音频数据测试 ES7210
 */

#include "capilot_audio.h"
#include "capilot_bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "audio_demo";

void app_main(void)
{
    ESP_LOGI(TAG, "=== audio_demo ===");

    /* 初始化 BSP（I2C + PCA9557） */
    capilot_bsp_i2c_init();
    capilot_bsp_pca9557_init();

    /* 检查音频芯片是否在线 */
    bool present = capilot_audio_is_present();
    ESP_LOGI(TAG, "audio chip present: %s", present ? "yes" : "no");
    if (!present) {
        ESP_LOGE(TAG, "no audio chip found, abort");
        return;
    }

    /* 初始化 Audio 组件 */
    capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
    esp_err_t ret = capilot_audio_init_with_config(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "audio init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "audio init OK, driver: %s", capilot_audio_get_driver_name());

    /* ---- 测试 1: 播放 beep ---- */
    ESP_LOGI(TAG, "test 1: playback beep 1000Hz 500ms");
    ret = capilot_audio_playback_start();
    if (ret == ESP_OK) {
        ret = capilot_audio_playback_beep(1000, 500);
        ESP_LOGI(TAG, "beep result: %s", ret == ESP_OK ? "OK" : esp_err_to_name(ret));
        capilot_audio_playback_stop();
    } else {
        ESP_LOGW(TAG, "playback start failed: %s", esp_err_to_name(ret));
    }
    vTaskDelay(pdMS_TO_TICKS(200));

    /* ---- 测试 2: 采集音频 ---- */
    ESP_LOGI(TAG, "test 2: capture 100ms");
    ret = capilot_audio_capture_start();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "capture start failed: %s", esp_err_to_name(ret));
    } else {
        /* 读取 3 帧数据 */
        for (int i = 0; i < 3; i++) {
            capilot_audio_buffer_t buf;
            uint8_t data[512];
            buf.data = data;
            buf.length = 512;

            ret = capilot_audio_capture_read(&buf);
            if (ret == ESP_OK) {
                /* 打印前 4 字节（1个立体声样本） */
                int16_t left = (int16_t)((data[1] << 8) | data[0]);
                int16_t right = (int16_t)((data[3] << 8) | data[2]);
                ESP_LOGI(TAG, "  frame %d: %d bytes, L=%d R=%d",
                         i, buf.length, left, right);
            } else {
                ESP_LOGW(TAG, "  frame %d read: %s", i, esp_err_to_name(ret));
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        capilot_audio_capture_stop();
        ESP_LOGI(TAG, "capture test done");
    }

    ESP_LOGI(TAG, "=== all tests done ===");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
