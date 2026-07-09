# 09 - WiFi 扫描与连接

立创实战派 ESP32-S3 开发板网络示例 — 扫描并连接 WiFi 热点。

## 功能

- 初始化 NVS 闪存（WiFi 配置存储）
- 初始化 LCD + LVGL 图形界面
- 通过触摸屏交互完成 WiFi 扫描和连接操作
- 连接状态实时显示在屏幕上

## 学习要点

- WiFi  Station 模式配置
- `esp_wifi_scan_start()` 扫描热点
- `esp_wifi_connect()` 连接指定 AP
- NVS 存储 WiFi 凭据

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

烧录后通过屏幕操作扫描并选择 WiFi 网络进行连接。
