# capilot_audio — CaPilot 音频组件

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-^5.0-blue)](https://docs.espressif.com/projects/esp-idf/)
[![Target](https://img.shields.io/badge/Target-ESP32--S3-red)]()
[![Chips](https://img.shields.io/badge/Chips-ES7210%20%7C%20ES8311-green)]()

硬件无关的音频组件，提供统一的采集（麦克风）、播放（扬声器）、事件处理（VAD/Push-To-Talk）API。
内部通过驱动 vtable 机制解耦芯片细节，换芯片只需实现 vtable 并注册，无需修改上层调用代码。

## 目录

- [硬件依赖](#硬件依赖)
- [架构设计](#架构设计)
- [快速上手](#快速上手)
- [文件结构](#文件结构)
- [配置说明](#配置说明)
- [已知限制](#已知限制)
- [Demo 工程](#demo-工程)

---

## 硬件依赖

目标平台：**ESP32-S3 立创实战派开发板**

| 芯片 | 角色 | I2C 地址 | I2S 模式 | 引脚 |
|------|------|----------|----------|------|
| ES7210 | 4 通道 ADC 麦克风 | 0x41 | STD RX（全双工 master） | MCK=38, BCK=14, WS=13, DI=12 |
| ES8311 | 音频 DAC/Codec | 0x18 | STD TX（复用 I2S0） | DO=45 |
| PCA9557 | IO 扩展（PA_EN） | 0x19 | — | 控制 ES8311 功放使能（IO1） |

**前置组件**：`capilot_bsp`（提供 I2C 读写、PA 使能接口）

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│              调用方（应用 / Demo）                 │
└────────────────────┬────────────────────────────┘
                     │ 公共 API
┌────────────────────▼────────────────────────────┐
│              capilot_audio.h                      │
│  init / is_present / get_driver_name             │
├────────────────┬─────────────────┬───────────────┤
│ capture.h      │ playback.h      │ events.h      │
│ start/stop/    │ start/stop/     │ enable_vad/   │
│ read/callback  │ write/beep      │ enable_ptt    │
└───────┬────────┴────────┬────────┴───────────────┘
        │ vtable 调度      │
┌───────▼────────────────▼───────────────────────┐
│           capilot_audio_driver.h                  │
│  init/config/start/stop/read/write/is_present    │
├───────────────────┬─────────────────────────────┤
│  s_es7210_driver  │  s_es8311_driver             │
│  (capture.c)      │  (playback.c)                │
└───────────────────┴─────────────────────────────┘
        │                        │
┌───────▼────────┐      ┌───────▼────────┐
│   ES7210        │      │   ES8311        │
│   寄存器操作     │      │   寄存器操作     │
│   (I2C + I2S)   │      │   (I2C + I2S)   │
└────────────────┘      └────────────────┘
        │                        │
┌───────▼────────────────────────▼───────────────┐
│              capilot_bsp（I2C / PA）              │
└─────────────────────────────────────────────────┘
```

**设计要点**：

1. **芯片无关配置**：`capilot_audio_config_t` 用枚举（采样率/位深/通道/方向），不暴露寄存器值
2. **vtable 驱动接口**：新增芯片只需实现 `capilot_audio_driver_t` 并在 `capilot_audio.c` 的 `s_drivers[]` 注册
3. **自动探测**：`init_with_config()` 遍历已注册驱动，调 `is_present()` 探测在线芯片
4. **BSP I2C 复用**：驱动通过 `capilot_bsp_i2c_write_byte()` / `capilot_bsp_i2c_read()` 操作寄存器，不依赖旧版 I2C API

---

## 快速上手

```c
#include "capilot_audio.h"
#include "capilot_bsp.h"

void app_main(void)
{
    /* 1. 初始化 BSP（I2C + PCA9557） */
    capilot_bsp_i2c_init();
    capilot_bsp_pca9557_init();

    /* 2. 探测芯片（可选，init 时也会自动探测） */
    if (!capilot_audio_is_present()) {
        ESP_LOGE(TAG, "no audio chip");
        return;
    }

    /* 3. 初始化（默认 16kHz 16bit mono） */
    capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
    capilot_audio_init_with_config(&cfg);

    /* 4. 采集 */
    capilot_audio_capture_start();
    uint8_t buf[2048];
    capilot_audio_buffer_t audio_buf = { .data = buf, .length = sizeof(buf) };
    capilot_audio_capture_read(&audio_buf);
    capilot_audio_capture_stop();

    /* 5. 播放 */
    capilot_audio_playback_start();
    capilot_audio_playback_beep(1000, 500);  /* 1kHz, 500ms */
    capilot_audio_playback_stop();
}
```

---

## 文件结构

```
capilot_audio/
├── CMakeLists.txt              # 组件构建配置
├── idf_component.yml           # 依赖声明（仅 idf: "^5.0"）
├── README.md                   # 本文件（说明文档）
├── API.md                      # 接口文档
├── capilot_audio_internal.h    # 内部接口（全双工 TX channel 共享）
├── capilot_audio.c             # 组件初始化 + 驱动探测
├── capilot_audio_capture.c     # ES7210 采集驱动实现（I2S STD 全双工 master）
├── capilot_audio_playback.c    # ES8311 播放驱动实现（复用共享 TX channel）
├── capilot_audio_events.c      # 事件处理（VAD 占位 + PTT）
└── include/
    ├── capilot_audio.h         # 公共 API
    ├── capilot_audio_types.h   # 数据类型与配置枚举
    ├── capilot_audio_capture.h # 采集 API
    ├── capilot_audio_playback.h# 播放 API
    ├── capilot_audio_events.h  # 事件 API
    └── capilot_audio_driver.h  # 驱动 vtable 接口（内部）
```

---

## 配置说明

### 默认配置

```c
#define CAPILOT_AUDIO_CONFIG_DEFAULT() { \
    .sample_rate = CAPILOT_AUDIO_SAMPLE_RATE_16K,  /* 16 kHz */
    .bits        = CAPILOT_AUDIO_BITS_16,          /* 16-bit */
    .channels    = CAPILOT_AUDIO_CHANNELS_MONO,    /* 单声道 */
    .direction   = CAPILOT_AUDIO_DIR_BOTH,         /* 采集+播放 */
}
```

### 支持的采样率

| 采样率 | ES7210 | ES8311 | 备注 |
|--------|--------|--------|------|
| 8 kHz  | ✅     | ✅     | 需扩展系数表 |
| 16 kHz | ✅     | ✅     | 默认，语音识别常用 |
| 22 kHz | ❌     | ❌     | 需扩展系数表 |
| 32 kHz | ❌     | ❌     | 需扩展系数表 |
| 44.1 kHz | ❌   | ❌     | 需扩展系数表 |
| 48 kHz | ✅     | ✅     | 高品质音频 |

> 当前系数表只内置了 16kHz 和 48kHz。其他采样率需在 `s_coeff_table[]` 中添加对应系数。

### sdkconfig 要求

```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_SPIRAM=y                    # 录音缓冲区需要 PSRAM
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
```

---

## 已知限制

### 1. ~~I2S 控制器引脚共享~~（已解决）

ES7210 和 ES8311 现在共享 I2S0 全双工 channel（RX+TX），ES7210 作为 master 产生时钟，ES8311 复用 TX channel。GPIO 冲突警告已消除。

实现细节见 `capilot_audio_internal.h` 的 `capilot_audio_get_shared_tx_chan()`。

### 2. VAD 为占位实现

`capilot_audio_events_enable_vad()` 创建的任务不产生任何事件，未来需集成 `esp-sr` VAD 算法。

### 3. Capture 回调未实现

`capilot_audio_capture_set_callback()` 仅保存回调指针，未启动接收任务。当前需用轮询方式 `capilot_audio_capture_read()` 读取数据。

### 4. Beep 采样率固定

`capilot_audio_playback_beep()` 使用系数表第一项的采样率（16kHz）生成方波，若实际配置为 48kHz 会导致音调偏差。

---

## Demo 工程

| Demo | 路径 | 功能 |
|------|------|------|
| audio_demo | `Components_Demos/audio_demo/` | 芯片探测 + beep 播放 + 采集读取 |
| record_playback_demo | `Components_Demos/record_playback_demo/` | BOOT 键触发录放音（10秒自动播放） |

### record_playback_demo 使用方法

1. 烧录后看到 `ready, press BOOT to start recording`
2. 短按 BOOT 键 → 开始录音（串口打印 `[REC] start recording`）
3. 再次短按 BOOT 键 → 停止录音并播放
4. 若 10 秒内未按键 → 自动停止并播放
5. 播放结束后回到就绪状态，可再次录音
