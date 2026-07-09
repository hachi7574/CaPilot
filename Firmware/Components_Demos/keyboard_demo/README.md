# CaPilot Keyboard Demo — USB HID 数字键盘

基于 **立创实战派 ESP32-S3** 开发板的 USB HID 键盘演示工程。

## 功能

- 3×4 触摸数字键盘（0-9, *, #）
- 通过 USB-C 连接电脑，即插即用（TinyUSB），无需蓝牙配对
- 触摸按键即发送对应 HID 键码到电脑
- 按下开发板 **BOOT** 键（GPIO 0）发送回车键
- LCD 屏幕实时显示连接状态

## 硬件要求

- 立创实战派 ESP32-S3 开发板
- USB-C 数据线（连接板载 USB OTG 接口）

## 如何使用

1. 搭建 ESP-IDF 开发环境（推荐 v5.x）
2. 设置目标芯片为 `esp32s3`
3. 编译并烧录：

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

4. 通过 USB-C 将开发板连接到电脑，电脑会自动识别为 USB 键盘

## 项目结构

```
├── main/                    # 主程序
│   ├── main.c               # 入口，初始化各模块
│   ├── app_ui.c/h           # LVGL 用户界面
│   └── bsp.c/h              # 板级支持包（LCD、触摸、I2C）
├── components/
│   └── capilot_usb_hid_keyboard/  # USB HID 键盘组件
├── CMakeLists.txt
└── README.md
```
