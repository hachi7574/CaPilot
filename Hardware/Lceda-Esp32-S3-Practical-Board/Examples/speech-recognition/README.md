# 12 - 语音识别

立创实战派 ESP32-S3 开发板 AI 示例 — 离线语音识别。

## 功能

- 初始化 LCD + LVGL 图形界面
- 挂载 SPIFFS 文件系统
- 初始化 ES8311 音频编解码器（麦克风输入）
- 初始化 MP3 播放器（语音播报反馈）
- 运行离线语音识别引擎
- 屏幕显示识别结果，支持语音控制

## 学习要点

- ESP-SR（ESP Speech Recognition）语音识别框架
- 语音前端信号处理（AEC、VAD）
- 离线命令词识别（无需联网）
- 多任务协同：语音识别 + LVGL UI + 音频播放

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

需要通过 `idf.py spiffs-flash` 烧录语音模型和提示音频到 SPIFFS 分区。
