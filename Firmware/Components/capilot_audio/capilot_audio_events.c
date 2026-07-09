/**
 * @file capilot_audio_events.c
 * @brief CaPilot Audio 组件 — 音频事件处理实现
 *
 * 事件类型：
 *   - VAD（语音活动检测）— 占位实现，未来集成 esp-sr
 *   - Push To Talk（按键说话）— GPIO 中断触发
 *   - Typeless 语音输入 — 占位实现
 *
 * 当前状态：框架就绪，VAD 算法待集成 esp-sr。
 */

#include "capilot_audio_events.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static const char *TAG = "capilot_audio_evt";

/* ============ 内部状态 ============ */
static capilot_audio_event_callback_t s_event_callback = NULL;
static bool s_vad_enabled = false;
static bool s_push_talk_enabled = false;
static uint32_t s_push_talk_gpio = 0;
static TaskHandle_t s_vad_task_handle = NULL;

/* ============ VAD 处理任务 ============ */

static void vad_task(void *arg)
{
    /* TODO: 集成 esp-sr VAD 算法
     * 当前为占位实现，不产生任何事件。
     * 未来集成 esp-sr 后：
     *   1. 从 capilot_audio_capture_read() 获取音频帧
     *   2. 喂给 esp-sr VAD 模型
     *   3. 检测语音活动开始/结束
     *   4. 通过 s_event_callback 回调通知
     */
    ESP_LOGI(TAG, "VAD task started (placeholder)");
    while (s_vad_enabled) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    s_vad_task_handle = NULL;
    vTaskDelete(NULL);
}

/* ============ Push To Talk 中断 ============ */

static void IRAM_ATTR push_talk_isr_handler(void *arg)
{
    /* GPIO 中断：按键按下时触发 PUSH_TALK 事件 */
    if (s_event_callback != NULL) {
        s_event_callback(CAPILOT_AUDIO_EVENT_PUSH_TALK, NULL);
    }
}

/* ============ 公共 API 实现 ============ */

esp_err_t capilot_audio_events_init(void)
{
    ESP_LOGI(TAG, "audio events initialized");
    return ESP_OK;
}

esp_err_t capilot_audio_events_register_callback(capilot_audio_event_callback_t callback)
{
    s_event_callback = callback;
    return ESP_OK;
}

esp_err_t capilot_audio_events_enable_vad(bool enable)
{
    s_vad_enabled = enable;

    if (enable && s_vad_task_handle == NULL) {
        BaseType_t ret = xTaskCreate(vad_task, "vad_task", 4096, NULL, 5, &s_vad_task_handle);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "failed to create VAD task");
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGI(TAG, "VAD enabled");
    } else if (!enable && s_vad_task_handle != NULL) {
        /* 任务会在下次循环检查 s_vad_enabled 后自行退出 */
        ESP_LOGI(TAG, "VAD disabled");
    }

    return ESP_OK;
}

esp_err_t capilot_audio_events_enable_push_talk(bool enable, uint32_t button_gpio)
{
    s_push_talk_enabled = enable;
    s_push_talk_gpio = button_gpio;

    if (enable) {
        /* 配置 GPIO 为下降沿中断（按键按下） */
        gpio_config_t io_cfg = {
            .pin_bit_mask = BIT64(button_gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io_cfg), TAG, "gpio config");
        ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "isr service");
        ESP_RETURN_ON_ERROR(gpio_isr_handler_add(button_gpio,
            push_talk_isr_handler, NULL), TAG, "isr handler");
        ESP_LOGI(TAG, "Push To Talk enabled on GPIO %lu", (unsigned long)button_gpio);
    } else {
        gpio_isr_handler_remove(button_gpio);
        gpio_set_intr_type(button_gpio, GPIO_INTR_DISABLE);
        ESP_LOGI(TAG, "Push To Talk disabled");
    }

    return ESP_OK;
}
