/**
 * @file gesture_detector.h
 * @brief Gesture detection for CaPilot (PRD F5)
 *
 * Detects 2 gesture types from IMU data:
 *   - Flick: Z-axis angular velocity spike (>400 deg/s, any direction)
 *   - Tilt:  Y-axis roll >30 deg held >500 ms
 *
 * Usage:
 *   gesture_detector_init(my_callback);
 *   // in a 100Hz loop:
 *   gesture_detector_feed(&imu_data);
 *
 * Output via callback as JSON-ready events.
 */
#pragma once

#include "capilot_imu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GESTURE_FLICK      = 0,
    GESTURE_TILT_LEFT  = 1,
    GESTURE_TILT_RIGHT = 2,
    GESTURE_TYPE_COUNT
} gesture_type_t;

typedef struct {
    gesture_type_t type;
    float          confidence;  /* 0.0 ~ 1.0 */
} gesture_event_t;

typedef void (*gesture_callback_t)(const gesture_event_t *event);

/**
 * @brief Initialize gesture detector
 * @param callback  Called when a gesture is detected (must not be NULL)
 */
void gesture_detector_init(gesture_callback_t callback);

/**
 * @brief Feed one IMU sample to the detector
 * @param data  IMU sample (acc + gyr + angle). Call this at >=100Hz.
 * @note  Gyro values should be bias-calibrated before feeding.
 */
void gesture_detector_feed(const capilot_imu_data_t *data);

/**
 * @brief Convert gesture type to string (for JSON output)
 */
const char *gesture_type_to_string(gesture_type_t type);

#ifdef __cplusplus
}
#endif
