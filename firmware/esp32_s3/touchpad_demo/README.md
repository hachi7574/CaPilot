# CaPilot Touchpad Demo — USB HID 触摸板

基于 **立创实战派 ESP32-S3** 开发板的 USB HID 鼠标（触摸板）演示工程。

## 功能

- 全屏触摸板：**滑动 = 鼠标移动**
- **双击 = 左键单击**
- **三击 = 左键双击**
- 按下开发板 **BOOT** 键 = 鼠标右键单击
- 通过 USB-C 连接电脑，即插即用（TinyUSB）
- LCD 屏幕实时显示连接状态与操作提示

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

4. 通过 USB-C 将开发板连接到电脑，电脑会自动识别为 USB 鼠标

## 项目结构

```
├── main/                    # 主程序
│   ├── main.c               # 入口，初始化各模块
│   ├── app_ui.c/h           # LVGL 用户界面
│   └── bsp.c/h              # 板级支持包
├── components/
│   └── capilot_usb_hid_mouse/     # USB HID 鼠标组件
├── CMakeLists.txt
└── README.md
```
