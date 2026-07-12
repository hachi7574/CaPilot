# capilot_audio — 接口文档

> 本文档详细描述 `capilot_audio` 组件的所有公共 API。
> 类型定义见 `capilot_audio_types.h`，API 声明见各模块头文件。

## 目录

- [数据类型](#数据类型)
- [组件管理 API](#组件管理-api)
- [采集 API](#采集-api)
- [播放 API](#播放-api)
- [事件 API](#事件-api)
- [驱动 vtable 接口](#驱动-vtable-接口内部)
- [错误码](#错误码)

---

## 数据类型

### `capilot_audio_sample_rate_t`

采样率枚举。

| 值 | 含义 |
|----|------|
| `CAPILOT_AUDIO_SAMPLE_RATE_8K` | 8 kHz |
| `CAPILOT_AUDIO_SAMPLE_RATE_16K` | 16 kHz（默认） |
| `CAPILOT_AUDIO_SAMPLE_RATE_22K` | 22.05 kHz |
| `CAPILOT_AUDIO_SAMPLE_RATE_32K` | 32 kHz |
| `CAPILOT_AUDIO_SAMPLE_RATE_44K` | 44.1 kHz |
| `CAPILOT_AUDIO_SAMPLE_RATE_48K` | 48 kHz |

> 当前仅 16kHz 和 48kHz 内置时钟系数，其他需扩展 `s_coeff_table`。

### `capilot_audio_bits_t`

位深度枚举。

| 值 | 含义 |
|----|------|
| `CAPILOT_AUDIO_BITS_8` | 8-bit |
| `CAPILOT_AUDIO_BITS_16` | 16-bit（默认） |
| `CAPILOT_AUDIO_BITS_24` | 24-bit |
| `CAPILOT_AUDIO_BITS_32` | 32-bit |

### `capilot_audio_channels_t`

通道数枚举。

| 值 | 含义 |
|----|------|
| `CAPILOT_AUDIO_CHANNELS_MONO` | 单声道（默认） |
| `CAPILOT_AUDIO_CHANNELS_STEREO` | 立体声 |

### `capilot_audio_direction_t`

音频方向枚举。

| 值 | 含义 |
|----|------|
| `CAPILOT_AUDIO_DIR_CAPTURE` | 采集（麦克风） |
| `CAPILOT_AUDIO_DIR_PLAYBACK` | 播放（扬声器） |
| `CAPILOT_AUDIO_DIR_BOTH` | 同时采集和播放（默认） |

### `capilot_audio_config_t`

音频配置结构体。

```c
typedef struct {
    capilot_audio_sample_rate_t sample_rate;  // 采样率
    capilot_audio_bits_t        bits;         // 位深度
    capilot_audio_channels_t    channels;     // 通道数
    capilot_audio_direction_t   direction;    // 音频方向
} capilot_audio_config_t;
```

**默认配置宏**：

```c
capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
// 等价于 { 16kHz, 16bit, MONO, BOTH }
```

### `capilot_audio_buffer_t`

音频数据缓冲区。

```c
typedef struct {
    uint8_t *data;       // 数据指针（调用方分配）
    size_t   length;     // 数据长度（字节）；read 后更新为实际读取长度
    int64_t  timestamp;  // 时间戳（微秒），read 后填充
} capilot_audio_buffer_t;
```

**使用方式**：
- 采集：调用方分配 `data` 并设置 `length` 为缓冲区大小，`read` 后 `length` 更新为实际读取字节数
- 播放：调用方填充 `data` 和 `length`，`write` 将数据写入 I2S

---

## 组件管理 API

### `capilot_audio_init`

```c
esp_err_t capilot_audio_init(void);
```

使用默认配置初始化音频组件。自动探测已注册驱动，找到在线芯片并初始化。

**参数**：无

**返回值**：
| 值 | 含义 |
|----|------|
| `ESP_OK` | 成功 |
| `ESP_ERR_NOT_FOUND` | 无音频芯片在线 |
| 其他 | 硬件错误 |

### `capilot_audio_init_with_config`

```c
esp_err_t capilot_audio_init_with_config(const capilot_audio_config_t *config);
```

使用指定配置初始化音频组件。

**参数**：
| 参数 | 方向 | 说明 |
|------|------|------|
| `config` | in | 芯片无关配置，`NULL` 表示默认配置 |

**返回值**：同 `capilot_audio_init`

**流程**：
1. 遍历已注册驱动（ES7210、ES8311），调 `is_present()` 探测
2. 对在线的采集驱动调 `init(config)`（配置寄存器 + I2S 通道）
3. 对在线的播放驱动调 `init(config)`（配置寄存器 + I2S 通道 + PA 使能）

### `capilot_audio_is_present`

```c
bool capilot_audio_is_present(void);
```

检查是否有音频芯片在线（任一已注册驱动响应）。

**返回值**：`true` = 至少一片芯片在线

### `capilot_audio_get_driver_name`

```c
const char *capilot_audio_get_driver_name(void);
```

获取当前激活的驱动名称。优先返回采集驱动名，其次播放驱动名。

**返回值**：驱动名称字符串（如 `"es7210"`、`"es8311"`），未初始化返回 `"(none)"`

---

## 采集 API

### `capilot_audio_capture_init`

```c
esp_err_t capilot_audio_capture_init(void);
```

初始化采集模块（标记内部状态）。通常由 `capilot_audio_init_with_config()` 间接完成，无需单独调用。

### `capilot_audio_capture_start`

```c
esp_err_t capilot_audio_capture_start(void);
```

开始音频采集。使能 I2S RX 通道。

**返回值**：
| 值 | 含义 |
|----|------|
| `ESP_OK` | 成功 |
| `ESP_ERR_INVALID_STATE` | 采集通道未初始化 |

### `capilot_audio_capture_stop`

```c
esp_err_t capilot_audio_capture_stop(void);
```

停止音频采集。禁能 I2S RX 通道。

### `capilot_audio_capture_read`

```c
esp_err_t capilot_audio_capture_read(capilot_audio_buffer_t *buffer);
```

读取音频数据（阻塞式，最多等 100ms）。

**参数**：
| 参数 | 方向 | 说明 |
|------|------|------|
| `buffer` | out | 输出缓冲区，调用方需预分配 `data` 并设置 `length` |

**返回值**：
| 值 | 含义 |
|----|------|
| `ESP_OK` | 成功，`buffer->length` 更新为实际字节数 |
| `ESP_ERR_INVALID_STATE` | 未开始采集或参数无效 |
| `ESP_ERR_TIMEOUT` | 100ms 内无数据 |

**示例**：

```c
uint8_t data[2048];
capilot_audio_buffer_t buf = { .data = data, .length = sizeof(data) };
if (capilot_audio_capture_read(&buf) == ESP_OK) {
    // buf.length 是实际读取的字节数
    // buf.timestamp 是采集时间戳（微秒）
}
```

### `capilot_audio_capture_set_callback`

```c
esp_err_t capilot_audio_capture_set_callback(capilot_audio_capture_callback_t callback);
```

设置采集回调函数。当数据就绪时调用。

> **注意**：当前为占位实现，仅保存回调指针，不启动接收任务。请使用轮询方式 `capture_read()`。

**回调类型**：

```c
typedef void (*capilot_audio_capture_callback_t)(const capilot_audio_buffer_t *buffer);
```

---

## 播放 API

### `capilot_audio_playback_init`

```c
esp_err_t capilot_audio_playback_init(void);
```

初始化播放模块。通常由 `capilot_audio_init_with_config()` 间接完成。

### `capilot_audio_playback_start`

```c
esp_err_t capilot_audio_playback_start(void);
```

开始音频播放。使能 I2S TX 通道。

> **重要**：调用 `beep()` 或 `write()` 前必须先调用 `start()`，否则返回 `ESP_ERR_INVALID_STATE`。

### `capilot_audio_playback_stop`

```c
esp_err_t capilot_audio_playback_stop(void);
```

停止音频播放。禁能 I2S TX 通道。

### `capilot_audio_playback_write`

```c
esp_err_t capilot_audio_playback_write(const capilot_audio_buffer_t *buffer);
```

写入音频数据（阻塞式，最多等 100ms）。

**参数**：
| 参数 | 方向 | 说明 |
|------|------|------|
| `buffer` | in | 输入音频数据 |

**返回值**：
| 值 | 含义 |
|----|------|
| `ESP_OK` | 成功 |
| `ESP_ERR_INVALID_STATE` | 未开始播放或参数无效 |
| `ESP_ERR_TIMEOUT` | 100ms 内缓冲区满 |

**数据格式**：16-bit stereo 交错（L R L R ...），采样率与初始化配置一致。

### `capilot_audio_playback_beep`

```c
esp_err_t capilot_audio_playback_beep(uint16_t frequency, uint16_t duration_ms);
```

播放系统提示音（方波）。

**参数**：
| 参数 | 范围 | 说明 |
|------|------|------|
| `frequency` | 100–8000 Hz | 频率 |
| `duration_ms` | 10–10000 ms | 持续时间 |

**返回值**：
| 值 | 含义 |
|----|------|
| `ESP_OK` | 成功 |
| `ESP_ERR_INVALID_STATE` | 未调用 `playback_start()` |
| `ESP_ERR_NO_MEM` | 内存不足 |

**实现**：生成 16-bit stereo 方波数据（幅度 8000/32768），通过 I2S 写入。采样率取系数表第一项（16kHz）。

### `capilot_audio_playback_wait_done`

```c
esp_err_t capilot_audio_playback_wait_done(uint32_t timeout_ms);
```

等待播放完成。

**参数**：
| 参数 | 说明 |
|------|------|
| `timeout_ms` | 超时毫秒，0 = 无限等待 |

> **注意**：当前实现为简单延时（`vTaskDelay`），非真正等待 I2S DMA 完成。

---

## 事件 API

### 事件类型

```c
typedef enum {
    CAPILOT_AUDIO_EVENT_VAD_START,   // 语音活动开始
    CAPILOT_AUDIO_EVENT_VAD_END,     // 语音活动结束
    CAPILOT_AUDIO_EVENT_PUSH_TALK,   // Push To Talk 触发
    CAPILOT_AUDIO_EVENT_TYPELESS,    // Typeless 语音输入
} capilot_audio_event_type_t;
```

### 回调类型

```c
typedef void (*capilot_audio_event_callback_t)(
    capilot_audio_event_type_t event,
    void *user_data
);
```

### `capilot_audio_events_init`

```c
esp_err_t capilot_audio_events_init(void);
```

初始化事件模块。当前为空实现，预留接口。

### `capilot_audio_events_register_callback`

```c
esp_err_t capilot_audio_events_register_callback(capilot_audio_event_callback_t callback);
```

注册事件回调函数。所有事件（VAD/PTT）都会通过此回调通知。

### `capilot_audio_events_enable_vad`

```c
esp_err_t capilot_audio_events_enable_vad(bool enable);
```

启用/禁用 VAD（语音活动检测）。

> **注意**：当前为占位实现，创建的任务不产生任何事件。未来集成 `esp-sr`。

### `capilot_audio_events_enable_push_talk`

```c
esp_err_t capilot_audio_events_enable_push_talk(bool enable, uint32_t button_gpio);
```

启用/禁用 Push To Talk（按键说话）。

**参数**：
| 参数 | 说明 |
|------|------|
| `enable` | `true`=启用，`false`=禁用 |
| `button_gpio` | 按钮 GPIO 编号（下降沿触发） |

**实现**：配置 GPIO 为上拉输入 + 下降沿中断，按下时触发 `CAPILOT_AUDIO_EVENT_PUSH_TALK` 事件。

**注意**：中断服务例程中直接调用回调，回调函数应尽量短小（IRAM 安全）。

---

## 驱动 vtable 接口（内部）

> 以下接口供组件内部驱动实现使用，应用层无需关心。

### `capilot_audio_driver_t`

```c
typedef struct capilot_audio_driver_t {
    const char *name;                                    // 驱动名
    esp_err_t (*init)(const capilot_audio_config_t *config);
    esp_err_t (*config)(const capilot_audio_config_t *config);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    esp_err_t (*read)(capilot_audio_buffer_t *buffer);   // 采集驱动实现
    esp_err_t (*write)(const capilot_audio_buffer_t *buffer); // 播放驱动实现
    bool (*is_present)(void);
} capilot_audio_driver_t;
```

**新增芯片步骤**：
1. 新建 `capilot_audio_xxx.c`，实现上述 7 个函数
2. 定义 `const capilot_audio_driver_t s_xxx_driver = { ... }`
3. 在 `capilot_audio.c` 中 `extern` 声明并加入 `s_drivers[]`
4. 在 `probe_drivers()` 中按 `name` 分类（capture/playback）

**已注册驱动**：
| 驱动 | 名称 | 角色 | 实现文件 |
|------|------|------|----------|
| `s_es7210_driver` | `"es7210"` | 采集 | `capilot_audio_capture.c` |
| `s_es8311_driver` | `"es8311"` | 播放 | `capilot_audio_playback.c` |

---

## 错误码

| 错误码 | 含义 | 常见原因 |
|--------|------|----------|
| `ESP_OK` | 成功 | — |
| `ESP_ERR_NOT_FOUND` | 芯片不在线 | I2C 连接问题、地址错误 |
| `ESP_ERR_NOT_SUPPORTED` | 不支持的操作 | 采集驱动调 write，或播放驱动调 read |
| `ESP_ERR_INVALID_STATE` | 状态错误 | 未 start 就 read/write，或通道未初始化 |
| `ESP_ERR_NO_MEM` | 内存不足 | PSRAM 未启用或分配失败 |
| `ESP_ERR_TIMEOUT` | 超时 | I2S 读写超时（100ms） |

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2026-07-05 | 初版：ES7210 采集 + ES8311 播放 + 事件框架 |
