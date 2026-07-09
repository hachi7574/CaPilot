/**
 * @file gesture_detector.c
 * @brief Gesture detection implementation (PRD F5)
 *
 * Thresholds (PRD F5.2 ~ F5.3):
 *   Flick: |gyr_z| > 400 deg/s  (512dps range -> 25600 raw LSB)
 *   Tilt:  |roll| > 30 deg, held > 500 ms, hysteresis 15 deg
 *
 * IMU config assumed:
 *   GYR range = 512dps (64 LSB/(deg/s))
 *   ACC range = 4g (default, tilt uses angle not raw ACC)
 *
 * Confidence: 0.5 at threshold, 1.0 at 2x threshold.
 *
 * Note: Caller must bias-calibrate gyro before feeding data.
 */
#include "gesture_detector.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>

static const char *TAG = "gesture";

/* ============ Thresholds (raw LSB) ============ */

/* GYR 512dps range: 32768/512 = 64 LSB/(deg/s) */
#define GYR_LSB_PER_DPS     64.0f
#define FLICK_THRESHOLD_RAW 25600   /* 400 deg/s at 512dps */
#define FLICK_COOLDOWN_US   (500 * 1000)   /* 500 ms */

/* Tilt (degrees) */
#define TILT_THRESHOLD_DEG  30.0f
#define TILT_HYSTERESIS_DEG 15.0f
#define TILT_HOLD_US        (500 * 1000)   /* 500 ms */

/* ============ State ============ */

static gesture_callback_t s_callback = NULL;

/* Flick */
static int64_t s_flick_last_us = 0;

/* Tilt */
typedef enum {
    TILT_IDLE = 0,
    TILT_ARMED_R,
    TILT_ARMED_L,
    TILT_FIRED_R,
    TILT_FIRED_L,
} tilt_st_t;
static tilt_st_t s_tilt_st = TILT_IDLE;
static int64_t   s_tilt_armed_us = 0;

/* ============ Helpers ============ */

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* confidence: 0.5 at threshold, 1.0 at 2x threshold */
static float conf_calc(float value, float threshold)
{
    float excess = value - threshold;
    float ratio  = excess / threshold;
    return clampf(0.5f + 0.5f * ratio, 0.5f, 1.0f);
}

static void emit(gesture_type_t type, float confidence)
{
    if (s_callback) {
        gesture_event_t evt = { .type = type, .confidence = confidence };
        s_callback(&evt);
    }
}

/* ============ Flick detection (F5.2) ============ */

static void detect_flick(int16_t gyr_z)
{
    int64_t now = esp_timer_get_time();
    if (now - s_flick_last_us < FLICK_COOLDOWN_US) return;

    int32_t abs_gz = gyr_z < 0 ? -(int32_t)gyr_z : (int32_t)gyr_z;
    if (abs_gz > FLICK_THRESHOLD_RAW) {
        float conf = conf_calc((float)abs_gz, (float)FLICK_THRESHOLD_RAW);
        emit(GESTURE_FLICK, conf);
        s_flick_last_us = now;
    }
}

/* ============ Tilt detection (F5.3) ============ */

static void detect_tilt(float roll_deg)
{
    int64_t now = esp_timer_get_time();

    switch (s_tilt_st) {
    case TILT_IDLE:
        if (roll_deg > TILT_THRESHOLD_DEG) {
            s_tilt_st = TILT_ARMED_R;
            s_tilt_armed_us = now;
        } else if (roll_deg < -TILT_THRESHOLD_DEG) {
            s_tilt_st = TILT_ARMED_L;
            s_tilt_armed_us = now;
        }
        break;

    case TILT_ARMED_R:
        if (roll_deg < -TILT_HYSTERESIS_DEG) {
            s_tilt_st = TILT_ARMED_L;
            s_tilt_armed_us = now;
        } else if (roll_deg < TILT_HYSTERESIS_DEG) {
            s_tilt_st = TILT_IDLE;
        } else if (now - s_tilt_armed_us >= TILT_HOLD_US) {
            float conf = clampf(
                (roll_deg - TILT_THRESHOLD_DEG) / (90.0f - TILT_THRESHOLD_DEG) * 0.5f + 0.5f,
                0.5f, 1.0f);
            emit(GESTURE_TILT_RIGHT, conf);
            s_tilt_st = TILT_FIRED_R;
        }
        break;

    case TILT_ARMED_L:
        if (roll_deg > TILT_THRESHOLD_DEG) {
            s_tilt_st = TILT_ARMED_R;
            s_tilt_armed_us = now;
        } else if (roll_deg > -TILT_HYSTERESIS_DEG) {
            s_tilt_st = TILT_IDLE;
        } else if (now - s_tilt_armed_us >= TILT_HOLD_US) {
            float conf = clampf(
                (-roll_deg - TILT_THRESHOLD_DEG) / (90.0f - TILT_THRESHOLD_DEG) * 0.5f + 0.5f,
                0.5f, 1.0f);
            emit(GESTURE_TILT_LEFT, conf);
            s_tilt_st = TILT_FIRED_L;
        }
        break;

    case TILT_FIRED_R:
        if (roll_deg < TILT_HYSTERESIS_DEG) s_tilt_st = TILT_IDLE;
        break;

    case TILT_FIRED_L:
        if (roll_deg > -TILT_HYSTERESIS_DEG) s_tilt_st = TILT_IDLE;
        break;
    }
}

/* ============ Public API ============ */

void gesture_detector_init(gesture_callback_t callback)
{
    s_callback = callback;
    s_flick_last_us = 0;
    s_tilt_st = TILT_IDLE;
    s_tilt_armed_us = 0;
    ESP_LOGI(TAG, "init: flick>400dps, tilt>30deg@500ms");
}

void gesture_detector_feed(const capilot_imu_data_t *data)
{
    if (!data || !s_callback) return;

    detect_flick(data->gyr_z);
    detect_tilt(data->angle_y);   /* angle_y = roll (left/right tilt) */
}

const char *gesture_type_to_string(gesture_type_t type)
{
    static const char *names[] = {
        "flick",
        "tilt_left", "tilt_right",
    };
    if (type < GESTURE_TYPE_COUNT) return names[type];
    return "unknown";
}
