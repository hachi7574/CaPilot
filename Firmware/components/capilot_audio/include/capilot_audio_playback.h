/**
 * @file capilot_audio_playback.h
 * @brief CaPilot Audio 组件 — 音频播放 API
 *
 * 使用方式：
 *   @code
 *   capilot_audio_playback_init();
 *   
 *   capilot_audio_playback_start();
 *   
 *   // 写入音频数据（非阻塞）
 *   capilot_audio_buffer_t buffer = { .data = my_data, .length = my_len };
 *   capilot_audio_playback_write(&buffer);
 *   
 *   // 或播放系统提示音
 *   capilot_audio_playback_beep(1000, 100);  // 1kHz, 100ms
 *   @endcode
 */

#pragma once

#include "capilot_audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化音频播放
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_playback_init(void);

/**
 * @brief 开始音频播放
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_playback_start(void);

/**
 * @brief 停止音频播放
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_playback_stop(void);

/**
 * @brief 写入音频数据（非阻塞）
 * @param[in] buffer 输入音频数据缓冲区
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 缓冲区满，其他表示硬件错误
 * @note   若返回 ESP_ERR_INVALID_STATE，表示缓冲区满，调用方可重试
 */
esp_err_t capilot_audio_playback_write(const capilot_audio_buffer_t *buffer);

/**
 * @brief 播放系统提示音（BEEP）
 * @param[in] frequency 频率（Hz）
 * @param[in] duration_ms 持续时间（毫秒）
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_playback_beep(uint16_t frequency, uint16_t duration_ms);

/**
 * @brief 等待播放完成
 * @param[in] timeout_ms 超时时间（毫秒），0 表示无限等待
 * @return ESP_OK 成功，ESP_ERR_TIMEOUT 超时，其他表示失败
 */
esp_err_t capilot_audio_playback_wait_done(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
