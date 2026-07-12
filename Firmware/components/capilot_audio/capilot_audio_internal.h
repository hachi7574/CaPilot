/**
 * @file capilot_audio_internal.h
 * @brief CaPilot Audio 组件内部接口（不对外暴露）
 *
 * 全双工 I2S channel 共享机制：
 *   ES7210 作为 I2S master 创建全双工 channel（RX+TX 共享 I2S0），
 *   ES8311 复用 ES7210 创建的 TX channel，避免两个 I2S 控制器
 *   争用相同的时钟引脚（MCK/BCK/WS）。
 *
 * 注意：此头文件仅供组件内部使用，不应被应用层引用。
 */

#pragma once

#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取共享的 I2S TX channel（全双工模式）
 *
 * ES7210 初始化时以全双工模式创建 I2S0 的 RX+TX channel，
 * TX channel 通过本函数暴露给 ES8311 播放驱动复用。
 *
 * @return i2s_chan_handle_t TX channel handle，NULL 表示未创建
 *         （ES7210 未初始化或不存在）
 */
i2s_chan_handle_t capilot_audio_get_shared_tx_chan(void);

/**
 * @brief 获取共享的 I2S RX channel（全双工模式）
 *
 * 全双工模式下 TX 和 RX 共享 I2S 控制器时钟。
 * 播放时需要保持 RX enable 以维持控制器时钟，否则 TX write 会超时。
 *
 * @return i2s_chan_handle_t RX channel handle，NULL 表示未创建
 */
i2s_chan_handle_t capilot_audio_get_shared_rx_chan(void);

#ifdef __cplusplus
}
#endif
