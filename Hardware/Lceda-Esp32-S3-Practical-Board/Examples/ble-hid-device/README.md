# 10 - BLE HID 蓝牙键盘

立创实战派 ESP32-S3 开发板蓝牙示例 — BLE HID 蓝牙键盘设备。

## 功能

- 将 ESP32-S3 配置为 BLE HID 键盘设备
- 通过蓝牙与电脑/手机配对后发送键盘按键
- LCD 屏幕显示蓝牙连接状态
- 支持通过触摸屏和 BOOT 键触发按键

## 学习要点

- BLE HID 协议栈初始化（GATT Service 注册）
- HID 键盘报告描述符配置
- `esp_ble_gatts_create_attr_tab()` 创建属性表
- `esp_hidd_prf_api.h` 蓝牙 HID 配置文件接口

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

烧录后开发板会广播 BLE 信号，在电脑/手机蓝牙中搜索并配对即可使用。
