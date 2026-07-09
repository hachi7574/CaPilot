/**
 * @file capilot_audio_types.h
 * @brief CaPilot Audio 组件 — 数据类型与配置枚举
 *
 * 所有音频芯片驱动共用这套类型定义。
 * 新芯片驱动只需实现 capilot_audio_driver_t vtable，无需修改本文件。
 *
 * 配置采用枚举类型，上层配置与芯片寄存器解耦。
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 采样率 ============ */
typedef enum {
    CAPILOT_AUDIO_SAMPLE_RATE_8K   = 8000,   /**<  8 kHz */
    CAPILOT_AUDIO_SAMPLE_RATE_16K  = 16000,  /**< 16 kHz */
    CAPILOT_AUDIO_SAMPLE_RATE_22K  = 22050,  /**< 22.05 kHz */
    CAPILOT_AUDIO_SAMPLE_RATE_32K  = 32000,  /**< 32 kHz */
    CAPILOT_AUDIO_SAMPLE_RATE_44K  = 44100,  /**< 44.1 kHz */
    CAPILOT_AUDIO_SAMPLE_RATE_48K  = 48000,  /**< 48 kHz */
} capilot_audio_sample_rate_t;

/* ============ 位深度 ============ */
typedef enum {
    CAPILOT_AUDIO_BITS_8  = 8,   /**<  8-bit */
    CAPILOT_AUDIO_BITS_16 = 16,  /**< 16-bit */
    CAPILOT_AUDIO_BITS_24 = 24,  /**< 24-bit */
    CAPILOT_AUDIO_BITS_32 = 32,  /**< 32-bit */
} capilot_audio_bits_t;

/* ============ 通道数 ============ */
typedef enum {
    CAPILOT_AUDIO_CHANNELS_MONO   = 1,  /**< 单声道 */
    CAPILOT_AUDIO_CHANNELS_STEREO = 2,  /**< 立体声 */
} capilot_audio_channels_t;

/* ============ 音频方向 ============ */
typedef enum {
    CAPILOT_AUDIO_DIR_CAPTURE  = 0,  /**< 采集（麦克风） */
    CAPILOT_AUDIO_DIR_PLAYBACK = 1,  /**< 播放（扬声器） */
    CAPILOT_AUDIO_DIR_BOTH     = 2,  /**< 同时采集和播放 */
} capilot_audio_direction_t;

/* ============ 音频配置 ============ */
typedef struct {
    capilot_audio_sample_rate_t sample_rate;  /**< 采样率 */
    capilot_audio_bits_t        bits;        /**< 位深度 */
    capilot_audio_channels_t   channels;    /**< 通道数 */
    capilot_audio_direction_t  direction;   /**< 音频方向 */
} capilot_audio_config_t;

/* ============ 音频数据缓冲区 ============ */
typedef struct {
    uint8_t *data;       /**< 音频数据指针 */
    size_t   length;      /**< 数据长度（字节） */
    int64_t  timestamp;   /**< 时间戳（微秒） */
} capilot_audio_buffer_t;

/* ============ 默认配置 ============ */
#define CAPILOT_AUDIO_CONFIG_DEFAULT() { \
    .sample_rate = CAPILOT_AUDIO_SAMPLE_RATE_16K, \
    .bits        = CAPILOT_AUDIO_BITS_16, \
    .channels    = CAPILOT_AUDIO_CHANNELS_MONO, \
    .direction   = CAPILOT_AUDIO_DIR_BOTH, \
}

#ifdef __cplusplus
}
#endif
