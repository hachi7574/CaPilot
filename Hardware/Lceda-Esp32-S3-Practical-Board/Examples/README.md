# 立创实战派 ESP32-S3 官方例程

> 14 个可直接运行的参考工程，涵盖开发板全部硬件外设。

| 目录 | 功能 | 关键外设 |
|------|------|---------|
| `boot-key/` | GPIO 按键中断检测 | BOOT 按键 (GPIO0) |
| `attitude/` | QMI8658 六轴姿态数据读取 | QMI8658 (I2C) |
| `micro-sd/` | MicroSD 卡 FAT 文件系统读写 | SDMMC 1 线模式 |
| `audio-es7210/` | ES7210 麦克风录音 → WAV 存 SD 卡 | ES7210 (I2S TDM), MicroSD |
| `audio-es8311/` | ES8311 PCM 音频播放 | ES8311 (I2S) |
| `lcd/` | LCD 图片显示 | ST7789 (SPI) |
| `lcd-camera/` | 摄像头实时画面 → LCD 预览 | OV2640 (DVP), LCD |
| `lcd-lvgl/` | LVGL 图形框架演示（基准测试/控件） | LCD, 触摸屏 (FT5x06) |
| `wifi-scan-connect/` | WiFi 扫描与连接（触摸交互） | WiFi, LVGL, 触摸屏 |
| `ble-hid-device/` | BLE HID 蓝牙键盘 | BLE, LCD, 触摸屏 |
| `mp3-player/` | MP3 音乐播放器（SPIFFS + ES8311） | ES8311, SPIFFS, LVGL |
| `speech-recognition/` | 离线语音命令词识别 | ESP-SR, ES8311, SPIFFS, LVGL |
| `human-face-detection/` | AI 人脸实时检测 | OV2640, ESP-DL, LCD |
| `handheld/` | 多功能综合固件（菜单/音频/姿态/蓝牙） | 全部外设集成 |

## 代码复用参考

开发 CaPilot 组件时优先从以下例程裁剪：

- **GPIO / 按键** → `boot-key/`
- **姿态传感器** → `attitude/`
- **音频输入** → `audio-es7210/`
- **音频输出** → `audio-es8311/`
- **LCD 基础驱动** → `lcd/`
- **LVGL + 触摸** → `lcd-lvgl/`
- **BLE HID** → `ble-hid-device/`
- **MP3 播放** → `mp3-player/`
- **语音识别** → `speech-recognition/`
