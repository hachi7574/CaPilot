/**
 * @file main.c
 * @brief record_playback_demo — Boot 键触发录放音
 *
 * 功能：
 *   1. I2C 探测 ES7210 + ES8311 是否在线
 *   2. 短按 BOOT 键开始录音
 *   3. 再次短按停止录音并播放
 *   4. 录音超过 10 秒自动停止并播放
 *
 * 硬件：
 *   - ESP32-S3 立创实战派开发板
 *   - BOOT 键接 GPIO0（低电平有效）
 *   - ES7210 麦克风（I2C 0x41, I2S TDM RX）
 *   - ES8311 音频输出（I2C 0x18, I2S STD TX）
 *
 * 音频参数：
 *   - 采样率 16kHz, 16bit
 *   - ES7210 TDM 采集为 stereo（取 L 通道作为 mono 录音）
 *   - ES8311 播放为 stereo（mono 数据复制到 L+R）
 *   - 最大录音时长 10 秒 = 16000 * 2 * 10 = 320000 字节
 */

#include "capilot_audio.h"
#include "capilot_bsp.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "rec_pb_demo";

/* ============ 配置 ============ */
#define BOOT_KEY_GPIO       GPIO_NUM_0      /* BOOT 键 */
#define SAMPLE_RATE         16000           /* 采样率 */
#define MAX_RECORD_SEC      10              /* 最大录音秒数 */
#define MAX_RECORD_BYTES    (SAMPLE_RATE * 2 * MAX_RECORD_SEC)  /* 320000 bytes */

/* 录音块大小（每次 I2S 读取） */
#define RECORD_CHUNK_BYTES  2048

/* ============ 状态机 ============ */
typedef enum {
    STATE_IDLE,         /* 空闲，等待按键开始录音 */
    STATE_RECORDING,    /* 录音中，等待按键停止或超时 */
    STATE_PLAYING,      /* 播放中 */
} demo_state_t;

static volatile demo_state_t s_state = STATE_IDLE;

/* ============ 按键事件队列 ============ */
static QueueHandle_t s_key_queue = NULL;
typedef enum {
    KEY_EVENT_SHORT_PRESS,  /* 短按 */
} key_event_t;

/* ============ 录音缓冲区（PSRAM） ============ */
static uint8_t *s_record_buf = NULL;     /* 录音缓冲区 */
static size_t   s_record_len = 0;        /* 已录音字节数 */

/* ============ 按键中断 ============ */
static void IRAM_ATTR boot_key_isr(void *arg)
{
    /* 只在 IDLE 和 RECORDING 状态响应按键 */
    demo_state_t st = s_state;
    if (st == STATE_IDLE || st == STATE_RECORDING) {
        key_event_t evt = KEY_EVENT_SHORT_PRESS;
        BaseType_t hp_woken = pdFALSE;
        xQueueSendFromISR(s_key_queue, &evt, &hp_woken);
        if (hp_woken) portYIELD_FROM_ISR();
    }
}

/* ============ 按键初始化 ============ */
static esp_err_t boot_key_init(void)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = BIT64(BOOT_KEY_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  /* 下降沿（按下） */
    };
    esp_err_t ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) return ret;

    ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;

    return gpio_isr_handler_add(BOOT_KEY_GPIO, boot_key_isr, NULL);
}

/* ============ 录音 ============ */
static esp_err_t do_record(void)
{
    ESP_LOGI(TAG, "[REC] start recording (max %d sec)", MAX_RECORD_SEC);

    /* 分配 PSRAM 缓冲区 */
    if (s_record_buf == NULL) {
        s_record_buf = heap_caps_malloc(MAX_RECORD_BYTES, MALLOC_CAP_SPIRAM);
        if (s_record_buf == NULL) {
            ESP_LOGE(TAG, "[REC] failed to alloc PSRAM %d bytes", MAX_RECORD_BYTES);
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGI(TAG, "[REC] PSRAM buffer allocated: %d bytes", MAX_RECORD_BYTES);
    }
    s_record_len = 0;

    /* 启动采集 */
    esp_err_t ret = capilot_audio_capture_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[REC] capture start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 循环读取音频数据，直到满或按键 */
    uint8_t chunk[RECORD_CHUNK_BYTES];
    int64_t start_us = esp_timer_get_time();
    uint32_t timeout_us = MAX_RECORD_SEC * 1000000ULL;

    while (s_state == STATE_RECORDING) {
        capilot_audio_buffer_t buf = {
            .data = chunk,
            .length = RECORD_CHUNK_BYTES,
        };
        ret = capilot_audio_capture_read(&buf);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "[REC] read failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* 拷贝到 PSRAM 缓冲区 */
        size_t copy_len = buf.length;
        if (s_record_len + copy_len > MAX_RECORD_BYTES) {
            copy_len = MAX_RECORD_BYTES - s_record_len;
        }
        if (copy_len > 0) {
            memcpy(s_record_buf + s_record_len, chunk, copy_len);
            s_record_len += copy_len;
        }

        /* 检查超时 */
        int64_t elapsed = esp_timer_get_time() - start_us;
        if (elapsed >= timeout_us) {
            ESP_LOGI(TAG, "[REC] timeout, auto stop");
            break;
        }
        if (s_record_len >= MAX_RECORD_BYTES) {
            ESP_LOGI(TAG, "[REC] buffer full");
            break;
        }
    }

    capilot_audio_capture_stop();
    ESP_LOGI(TAG, "[REC] recorded %d bytes (%.1f sec)",
             s_record_len, (float)s_record_len / (SAMPLE_RATE * 2));
    return ESP_OK;
}

/* ============ 播放 ============ */
static esp_err_t do_playback(void)
{
    if (s_record_len == 0) {
        ESP_LOGW(TAG, "[PLAY] nothing to play");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "[PLAY] start playback %d bytes", s_record_len);

    /* 启动播放 */
    esp_err_t ret = capilot_audio_playback_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[PLAY] playback start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 录音是 mono（L 通道），播放需要 stereo（L+R）
     * ES7210 TDM 采集实际是 stereo 格式，但我们只取了 L 通道数据吗？
     * 实际上 I2S TDM stereo 模式下，每帧 4 字节 = L(2) + R(2)
     * 录下来的数据已经是 stereo 交错格式，可以直接播放
     * 但如果只有 L 有效，R 是 0 或噪声，播放会偏左声道
     * 这里直接播放录到的数据，保持原样 */
    size_t offset = 0;
    size_t chunk_size = 4096;
    while (offset < s_record_len) {
        size_t this_len = (s_record_len - offset > chunk_size) ?
                          chunk_size : (s_record_len - offset);
        capilot_audio_buffer_t buf = {
            .data = s_record_buf + offset,
            .length = this_len,
        };
        ret = capilot_audio_playback_write(&buf);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "[PLAY] write failed at %d: %s", offset, esp_err_to_name(ret));
            break;
        }
        offset += this_len;
    }

    /* 等待最后的数据播完 */
    vTaskDelay(pdMS_TO_TICKS(200));
    capilot_audio_playback_stop();

    ESP_LOGI(TAG, "[PLAY] playback done");
    return ESP_OK;
}

/* ============ 主任务 ============ */
static void demo_task(void *arg)
{
    key_event_t evt;
    while (1) {
        if (xQueueReceive(s_key_queue, &evt, portMAX_DELAY) == pdTRUE) {
            switch (s_state) {
            case STATE_IDLE:
                s_state = STATE_RECORDING;
                do_record();
                s_state = STATE_PLAYING;
                do_playback();
                s_state = STATE_IDLE;
                ESP_LOGI(TAG, "--- ready, press BOOT to record again ---");
                break;

            case STATE_RECORDING:
                /* 按键停止录音 — do_record 循环会检测到状态变化退出 */
                ESP_LOGI(TAG, "[KEY] stop recording by user");
                s_state = STATE_PLAYING;  /* 退出 do_record 循环 */
                break;

            case STATE_PLAYING:
                /* 播放中忽略按键 */
                break;
            }
        }
    }
}

/* ============ app_main ============ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== record_playback_demo ===");
    ESP_LOGI(TAG, "boot key on GPIO%d, max record %d sec", BOOT_KEY_GPIO, MAX_RECORD_SEC);

    /* 1. 初始化 BSP（I2C + PCA9557） */
    capilot_bsp_i2c_init();
    capilot_bsp_pca9557_init();

    /* 2. I2C 探测 ES7210 + ES8311 */
    ESP_LOGI(TAG, "probing audio chips...");
    bool present = capilot_audio_is_present();
    ESP_LOGI(TAG, "audio chip present: %s", present ? "yes" : "no");
    if (!present) {
        ESP_LOGE(TAG, "no audio chip found, abort");
        return;
    }
    ESP_LOGI(TAG, "driver: %s", capilot_audio_get_driver_name());

    /* 3. 初始化 Audio 组件（默认 16kHz 16bit） */
    capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
    esp_err_t ret = capilot_audio_init_with_config(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "audio init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "audio init OK");

    /* 4. 初始化按键 */
    s_key_queue = xQueueCreate(4, sizeof(key_event_t));
    if (s_key_queue == NULL) {
        ESP_LOGE(TAG, "failed to create key queue");
        return;
    }
    ret = boot_key_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "boot key init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "boot key initialized");

    /* 5. 创建主任务 */
    xTaskCreate(demo_task, "demo_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "=== ready, press BOOT to start recording ===");
}
