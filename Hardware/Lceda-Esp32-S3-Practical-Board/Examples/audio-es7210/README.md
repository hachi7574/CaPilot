# 04 - ES7210 音频录音

立创实战派 ESP32-S3 开发板外设示例 — 使用 ES7210 ADC 录制音频到 MicroSD 卡。

## 功能

- 通过 I2C 配置 ES7210 音频编解码器（ADC 模式）
- 使用 I2S TDM 接口接收 ES7210 的音频数据
- 将音频数据以 WAV 格式写入 MicroSD 卡（48kHz, 16-bit, 双声道）
- 默认录制 10 秒

## 学习要点

- `i2s_new_channel()` + `i2s_channel_init_tdm_mode()` 配置 I2S TDM 接收
- ES7210 驱动初始化与增益/偏置配置
- WAV 文件头结构封装
- I2S 音频数据流与文件写入的结合

## 硬件接线

| 接口 | 引脚 |
|------|------|
| I2C SDA | GPIO 1 |
| I2C SCL | GPIO 2 |
| I2S MCLK | GPIO 38 |
| I2S BCK | GPIO 14 |
| I2S WS | GPIO 13 |
| I2S DI | GPIO 12 |
| SD CLK | GPIO 47 |
| SD CMD | GPIO 48 |
| SD D0 | GPIO 21 |

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

插入 MicroSD 卡，录音完成后会在卡上生成 `/RECORD.WAV` 文件。
