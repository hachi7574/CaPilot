/**
 * @file main.c
 * @brief imu_demo - Gesture detection (PRD F5)
 *
 * Reads QMI8658 at 100Hz, detects 2 gesture types, outputs JSON.
 *
 * PRD F5:
 *   F5.1  Sample rate >= 100 Hz (vTaskDelayUntil for accuracy)
 *   F5.2  Flick: |gyr_z| > 400 deg/s -> flick (any direction)
 *   F5.3  Tilt:  |roll| > 30 deg held > 500 ms -> tilt_left / tilt_right
 *   F5.5  Output: {"type":"gesture","gesture":"flick","confidence":0.95}
 *
 * Startup:
 *   1. I2C init + IMU probe
 *   2. Gyro bias calibration (512 samples, ~5s, device must be stationary)
 *   3. Gesture detector init
 *   4. 100Hz sampling task
 *
 * IMU config: ACC 4g (default), GYR 512dps, ODR 250Hz
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_idf_version.h"
#include "capilot_bsp.h"
#include "capilot_imu.h"
#include "gesture_detector.h"

static const char *TAG = "imu_demo";

/* ============ Gyro bias (calibrated at startup) ============ */
static int16_t s_gyro_bias_x = 0;
static int16_t s_gyro_bias_y = 0;
static int16_t s_gyro_bias_z = 0;

/* ============ Gesture callback: clean JSON to serial (F5.5) ============ */
static void on_gesture(const gesture_event_t *event)
{
    printf("{\"type\":\"gesture\",\"gesture\":\"%s\",\"confidence\":%.2f}\n",
           gesture_type_to_string(event->type),
           event->confidence);
    fflush(stdout);
}

/* ============ Gyro bias calibration ============ */
static esp_err_t calibrate_gyro(void)
{
    /*
     * Read N samples while stationary, average gyro readings as bias.
     * Known issue: gyr_x has ~127 dps constant bias on this board.
     * Calibration removes this so flick thresholds are accurate.
     */
    const int N = 512;
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    int valid = 0;

    ESP_LOGI(TAG, "gyro calibration: keep device stationary (%d samples)...", N);

    for (int i = 0; i < N * 3; i++) {  /* allow retries, sensor may not be ready every cycle */
        capilot_imu_data_t data;
        esp_err_t ret = capilot_imu_read(&data);
        if (ret == ESP_OK) {
            sum_x += data.gyr_x;
            sum_y += data.gyr_y;
            sum_z += data.gyr_z;
            valid++;
            if (valid >= N) break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (valid < N / 2) {
        ESP_LOGE(TAG, "calibration failed: only %d/%d valid samples", valid, N);
        return ESP_FAIL;
    }

    s_gyro_bias_x = sum_x / valid;
    s_gyro_bias_y = sum_y / valid;
    s_gyro_bias_z = sum_z / valid;

    /* Convert to dps for human-readable log */
    float bx_dps = (float)s_gyro_bias_x / 64.0f;  /* 512dps range = 64 LSB/dps */
    float by_dps = (float)s_gyro_bias_y / 64.0f;
    float bz_dps = (float)s_gyro_bias_z / 64.0f;

    ESP_LOGI(TAG, "gyro bias: x=%+6.1f y=%+6.1f z=%+6.1f dps (%d samples)",
             bx_dps, by_dps, bz_dps, valid);
    return ESP_OK;
}

/* ============ 100Hz sampling task (F5.1) ============ */
static void sampling_task(void *arg)
{
    capilot_imu_data_t data;
    int sample_count = 0;
    int64_t last_status_us = esp_timer_get_time();
    int64_t last_rate_us = last_status_us;
    int rate_counter = 0;

    ESP_LOGI(TAG, "sampling task started (target 100Hz)");

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        /* vTaskDelayUntil ensures >=100Hz regardless of I2C read time */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));  /* 10ms = 100Hz */

        esp_err_t ret = capilot_imu_read_with_angle(&data);
        if (ret == ESP_OK) {
            /* Apply gyro bias calibration */
            data.gyr_x -= s_gyro_bias_x;
            data.gyr_y -= s_gyro_bias_y;
            data.gyr_z -= s_gyro_bias_z;

            /* Feed calibrated data to gesture detector */
            gesture_detector_feed(&data);

            sample_count++;
            rate_counter++;
        }
        /* ESP_ERR_INVALID_STATE = data not ready yet, just skip */

        /* Status + actual sampling rate every 5 seconds */
        int64_t now = esp_timer_get_time();
        if (now - last_status_us >= 5 * 1000 * 1000) {
            float elapsed_s = (now - last_rate_us) / 1000000.0f;
            float actual_hz = rate_counter / elapsed_s;
            ESP_LOGI(TAG, "rate=%.1fHz samples=%d roll=%.1f gyr_z=%d",
                     actual_hz, sample_count, data.angle_y, data.gyr_z);
            last_status_us = now;
            last_rate_us = now;
            rate_counter = 0;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== imu_demo (gesture detection, PRD F5) ===");
    ESP_LOGI(TAG, "ESP-IDF %s", esp_get_idf_version());

    /* Step 1: I2C bus */
    esp_err_t ret = capilot_bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Step 2: Probe IMU */
    if (!capilot_imu_is_present()) {
        ESP_LOGE(TAG, "No IMU detected! Check I2C wiring.");
        return;
    }
    ESP_LOGI(TAG, "IMU detected");

    /* Step 3: Init IMU with default config (4g ACC, 512dps GYR) */
    capilot_imu_config_t cfg = CAPILOT_IMU_CONFIG_DEFAULT();
    ret = capilot_imu_init_with_config(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IMU init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "IMU ready, driver='%s', acc=4g gyr=512dps",
             capilot_imu_get_driver_name());

    /* Step 4: Gyro bias calibration (device must be stationary) */
    if (calibrate_gyro() != ESP_OK) {
        ESP_LOGW(TAG, "gyro calibration failed, continuing with raw data");
    }

    /* Step 5: Init gesture detector */
    gesture_detector_init(on_gesture);

    /* Step 6: Start 100Hz sampling task */
    xTaskCreate(sampling_task, "imu_sample", 4096, NULL, 5, NULL);
}
