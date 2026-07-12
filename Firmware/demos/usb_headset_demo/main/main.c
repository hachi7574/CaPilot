#include <inttypes.h>
#include <stdint.h>

#include "capilot_audio.h"
#include "capilot_bsp.h"
#include "capilot_usb_headset.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "usb_headset_demo";

#define BOOT_KEY_GPIO GPIO_NUM_0
#define AUDIO_FRAME_MS 1
#define PLAYBACK_FRAME_BYTES 192  /* 48 kHz x 16-bit x stereo x 1 ms */
#define CAPTURE_STEREO_FRAME_BYTES 192
#define CAPTURE_MONO_FRAME_BYTES 96

static int16_t abs_i16(int16_t value)
{
    return value == INT16_MIN ? INT16_MAX : (value < 0 ? -value : value);
}

static int16_t clamp_i16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static void apply_playback_volume(int16_t *samples, size_t sample_count, uint16_t gain_q15)
{
    if (gain_q15 >= 32767) {
        return;
    }
    for (size_t i = 0; i < sample_count; ++i) {
        samples[i] = clamp_i16(((int32_t)samples[i] * gain_q15) >> 15);
    }
}

static void boot_key_task(void *arg)
{
    (void)arg;
    bool last_pressed = false;

    while (true) {
        bool pressed = gpio_get_level(BOOT_KEY_GPIO) == 0;
        if (pressed != last_pressed) {
            esp_err_t ret = capilot_usb_headset_set_win_alt(pressed);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "BOOT: Win+Alt %s", pressed ? "pressed" : "released");
            } else {
                ESP_LOGW(TAG, "BOOT: Win+Alt %s failed: %s",
                         pressed ? "press" : "release", esp_err_to_name(ret));
            }
            last_pressed = pressed;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void playback_task(void *arg)
{
    (void)arg;
    uint8_t frame[PLAYBACK_FRAME_BYTES];
    uint32_t usb_rx_bytes = 0;
    uint32_t i2s_tx_bytes = 0;
    int16_t playback_peak = 0;
    int64_t last_log_us = esp_timer_get_time();

    while (true) {
        uint16_t bytes = capilot_usb_headset_read_playback(frame, sizeof(frame));
        if (bytes > 0) {
            usb_rx_bytes += bytes;
            int16_t *samples = (int16_t *)frame;
            size_t sample_count = bytes / sizeof(int16_t);
            uint16_t gain_q15 = capilot_usb_headset_get_playback_gain_q15();
            apply_playback_volume(samples, sample_count, gain_q15);
            for (size_t i = 0; i < sample_count; ++i) {
                int16_t peak = abs_i16(samples[i]);
                if (peak > playback_peak) {
                    playback_peak = peak;
                }
            }
            capilot_audio_buffer_t audio = { .data = frame, .length = bytes };
            esp_err_t ret = capilot_audio_playback_write(&audio);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "speaker write: %s", esp_err_to_name(ret));
            } else {
                i2s_tx_bytes += bytes;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(AUDIO_FRAME_MS));
        }
        int64_t now_us = esp_timer_get_time();
        if (now_us - last_log_us >= 1000000) {
            ESP_LOGI(TAG, "playback stats: alt=%u usb_rx=%" PRIu32 " i2s_tx=%" PRIu32
                     " peak=%d mute=%u vol_db_q8_8=%d gain_q15=%u",
                     capilot_usb_headset_get_speaker_alt(), usb_rx_bytes, i2s_tx_bytes,
                     playback_peak, capilot_usb_headset_is_playback_muted(),
                     capilot_usb_headset_get_playback_volume_db_q8_8(),
                     capilot_usb_headset_get_playback_gain_q15());
            usb_rx_bytes = 0;
            i2s_tx_bytes = 0;
            playback_peak = 0;
            last_log_us = now_us;
        }
    }
}

static void capture_task(void *arg)
{
    (void)arg;
    int16_t stereo[CAPTURE_STEREO_FRAME_BYTES / sizeof(int16_t)];
    int16_t mono[CAPTURE_MONO_FRAME_BYTES / sizeof(int16_t)];
    uint32_t i2s_rx_bytes = 0;
    uint32_t usb_tx_bytes = 0;
    uint32_t usb_drop_bytes = 0;
    int16_t capture_peak = 0;
    int64_t last_log_us = esp_timer_get_time();

    while (true) {
        capilot_audio_buffer_t audio = {
            .data = (uint8_t *)stereo,
            .length = sizeof(stereo),
        };
        esp_err_t ret = capilot_audio_capture_read(&audio);
        if (ret != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(AUDIO_FRAME_MS));
            continue;
        }
        i2s_rx_bytes += audio.length;

        size_t samples = audio.length / (sizeof(int16_t) * 2);
        if (samples > sizeof(mono) / sizeof(mono[0])) {
            samples = sizeof(mono) / sizeof(mono[0]);
        }
        for (size_t i = 0; i < samples; ++i) {
            mono[i] = stereo[i * 2]; /* expose ES7210 left microphone as mono USB input */
            int16_t peak = abs_i16(mono[i]);
            if (peak > capture_peak) {
                capture_peak = peak;
            }
        }
        uint16_t requested = samples * sizeof(mono[0]);
        uint16_t written = capilot_usb_headset_write_capture(mono, requested);
        usb_tx_bytes += written;
        usb_drop_bytes += requested - written;

        int64_t now_us = esp_timer_get_time();
        if (now_us - last_log_us >= 1000000) {
            ESP_LOGI(TAG, "capture stats: alt=%u i2s_rx=%" PRIu32 " usb_tx=%" PRIu32 " usb_drop=%" PRIu32 " peak=%d",
                     capilot_usb_headset_get_microphone_alt(), i2s_rx_bytes, usb_tx_bytes,
                     usb_drop_bytes, capture_peak);
            i2s_rx_bytes = 0;
            usb_tx_bytes = 0;
            usb_drop_bytes = 0;
            capture_peak = 0;
            last_log_us = now_us;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== CaPilot USB headset demo ===");

    ESP_ERROR_CHECK(capilot_bsp_i2c_init());
    capilot_bsp_pca9557_init();

    capilot_audio_config_t audio_config = {
        .sample_rate = CAPILOT_AUDIO_SAMPLE_RATE_48K,
        .bits = CAPILOT_AUDIO_BITS_16,
        .channels = CAPILOT_AUDIO_CHANNELS_STEREO,
        .direction = CAPILOT_AUDIO_DIR_BOTH,
    };
    ESP_ERROR_CHECK(capilot_audio_init_with_config(&audio_config));
    ESP_ERROR_CHECK(capilot_audio_capture_start());
    ESP_ERROR_CHECK(capilot_audio_playback_start());

    gpio_config_t key_config = {
        .pin_bit_mask = BIT64(BOOT_KEY_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&key_config));
    ESP_ERROR_CHECK(capilot_usb_headset_init());

    xTaskCreate(playback_task, "usb_playback", 4096, NULL, 5, NULL);
    xTaskCreate(capture_task, "usb_capture", 4096, NULL, 5, NULL);
    xTaskCreate(boot_key_task, "boot_key", 2048, NULL, 4, NULL);

    ESP_LOGI(TAG, "Connect the USB data port to the computer.");
    ESP_LOGI(TAG, "The host will see a CaPilot headset and keyboard; hold BOOT for Win+Alt.");
}
