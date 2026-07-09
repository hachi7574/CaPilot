# 14 - 多功能手持设备

立创实战派 ESP32-S3 开发板综合示例 — 多功能手持设备固件。

## 功能

- 开机播放音乐（SPIFFS 中存储的启动音效）
- 播放完毕后进入 LVGL 主界面
- 集成式功能菜单（通过 `lv_main_page()` 切换）
- 实时内存监控（DRAM / PSRAM 使用率）
- 模块化 APP 架构，支持功能扩展

## 集成的模块

| 模块 | 功能 |
|------|------|
| LCD + LVGL | 图形用户界面 |
| 触摸屏 | 用户交互输入 |
| ES8311 音频 | 音乐播放与语音输出 |
| SPIFFS | 文件系统存储资源文件 |
| PCA9557 | IO 扩展控制外设电源 |
| QMI8658 | 姿态传感器 |

## 架构说明

- 使用 `EventGroup` 实现任务间同步（开机音乐播放完成 -> 进入主界面）
- 双核调度：音频播放固定在 Core 1，UI 任务在 Core 0
- `xTaskCreatePinnedToCore()` 指定任务核心亲和性

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

需要先通过 `idf.py spiffs-flash` 将开机音乐等资源烧录到 SPIFFS 分区。
