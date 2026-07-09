/**
 * @file capilot_audio_capture.h
 * @brief CaPilot Audio 组件 — 音频采集 API
 *
 * 使用方式：
 *   @code
 *   capilot_audio_capture_init();
 *   
 *   // 设置回调（当数据就绪时调用）
 *   capilot_audio_capture_set_callback(my_callback);
 *   
 *   capilot_audio_capture_start();
 *   
 *   // 或轮询读取
 *   capilot_audio_buffer_t buffer;
 *   if (capilot_audio_capture_read(&buffer) == ESP_OK) {
 *       // 处理 buffer.data
 *   }
 *   @endcode
 */

#pragma once

#include "capilot_audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 采集回调类型 ============ */
typedef void (*capilot_audio_capture_callback_t)(const capilot_audio_buffer_t *buffer);

/**
 * @brief 初始化音频采集
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_capture_init(void);

/**
 * @brief 开始音频采集
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_capture_start(void);

/**
 * @brief 停止音频采集
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_capture_stop(void);

/**
 * @brief 读取音频数据（非阻塞）
 * @param[out] buffer 输出音频数据缓冲区
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 无数据，其他表示硬件错误
 * @note   若返回 ESP_ERR_INVALID_STATE，表示暂无数据，调用方可重试
 */
esp_err_t capilot_audio_capture_read(capilot_audio_buffer_t *buffer);

/**
 * @brief 设置音频采集回调（当数据就绪时调用）
 * @param[in] callback 回调函数指针，NULL 表示取消回调
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_capture_set_callback(capilot_audio_capture_callback_t callback);

#ifdef __cplusplus
}
#endif
