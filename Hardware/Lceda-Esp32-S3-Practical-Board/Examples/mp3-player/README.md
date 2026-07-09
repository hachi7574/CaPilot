# 11 - MP3 音乐播放器

立创实战派 ESP32-S3 开发板多媒体示例 — MP3 音频播放器。

## 功能

- 初始化 LCD + LVGL 图形界面
- 挂载 SPIFFS 文件系统（存放 MP3 文件）
- 初始化 ES8311 音频编解码器
- 播放 SPIFFS 中存储的 MP3 音乐文件
- 屏幕显示播放状态与控制界面

## 学习要点

- `bsp_spiffs_mount()` SPIFFS 文件系统挂载
- `bsp_codec_init()` 音频编解码器初始化
- `mp3_player_init()` MP3 解码与播放流程
- LVGL 多媒体 UI 设计

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

需要预先将 MP3 文件烧录到 SPIFFS 分区中。

## 烧录 SPIFFS

```bash
idf.py spiffs-flash
```
