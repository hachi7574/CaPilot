# 05 - ES8311 音频播放

立创实战派 ESP32-S3 开发板外设示例 — 使用 ES8311 DAC 播放 PCM 音频。

## 功能

- 通过 I2C 初始化 ES8311 音频编解码器（DAC 模式）
- 使用 I2S 标准模式发送 PCM 音频数据
- 循环播放内置的《Canon》PCM 音乐
- 通过 PCA9557 IO 扩展芯片控制音频功放使能

## 学习要点

- `i2s_new_channel()` + `i2s_channel_init_std_mode()` 配置 I2S 标准发送
- ES8311 驱动初始化（时钟、采样率、音量配置）
- `i2s_channel_preload_data()` 预加载音频数据
- PCA9557 控制音频功放使能（`pa_en()`）
- 嵌入式音频数据链接（`_binary_xxx_start` 符号）

## 硬件接线

| 接口 | 引脚 |
|------|------|
| I2C SDA | GPIO 1 |
| I2C SCL | GPIO 2 |
| I2S MCLK | GPIO 38 |
| I2S BCK | GPIO 14 |
| I2S WS | GPIO 13 |
| I2S DO | GPIO 45 |

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

烧录后开发板将通过扬声器/耳机循环播放 PCM 音乐。
