# ESP32-S3 多功能手持设备示例

本工程是立创 ESP32-S3 实战派开发板的综合演示固件。程序启动后先初始化硬件、挂载 SPIFFS、初始化音频编解码器并播放启动音效；播放完成后进入 LVGL 主界面。

## 当前实现的功能

- LCD 图形界面和 FT5x06 触摸输入
- QMI8658 姿态/运动监测，显示 X/Y/Z 数据和状态
- ES8311 音频播放，支持 SPIFFS 中的 PCM/MP3 音乐资源
- ES7210 麦克风输入接口初始化
- SD 卡挂载与文件浏览页面
- 摄像头初始化和实时画面显示
- Wi‑Fi 扫描、账号输入、连接和连接状态显示
- BLE HID 页面，用于蓝牙 HID 控制
- 开机任务与主界面任务分别固定到 Core 1 和 Core 0
- 周期性打印 DRAM、PSRAM 使用量

## 不包含的功能

本工程没有实现语音识别、唤醒词或离线语音命令。ESP-SR 依赖已移除，避免无意义地占用固件空间。

## 启动流程

```text
NVS 初始化
  -> I2C / PCA9557 初始化
  -> LCD、触摸和 LVGL 初始化
  -> SPIFFS 挂载
  -> I2S、ES8311、ES7210 初始化
  -> 播放 sword.pcm 启动音效
  -> 进入 LVGL 主界面
```

## 主界面入口

主界面由 `lv_main_page()` 创建，当前包含六个功能入口：

1. 运动监测
2. 音乐播放
3. SD 卡
4. 摄像头
5. Wi‑Fi
6. 蓝牙 HID

## 主要源文件

- `main/main.c`：系统启动、任务创建和内存监控
- `main/esp32_s3_szp.c`：板级外设、显示、摄像头、存储、音频和传感器驱动
- `main/app_ui.c`：LVGL 页面、按钮事件和各功能应用
- `main/bt/`：BLE HID 实现
- `spiffs/`：启动音频等运行时资源
- `partitions.csv`：应用、NVS、PHY 和 SPIFFS 分区布局

## 编译

使用 ESP-IDF v5.5.4、ESP32-S3 目标：

```powershell
idf.py set-target esp32s3
idf.py build
```

当前工程使用自定义目录布局，也可以使用项目配套的 ESP-Build-Skill 环境脚本执行 CMake/Ninja 构建。

## 烧录

除了应用、Bootloader 和分区表，还需要将 SPIFFS 资源写入 `storage` 分区。具体地址以 `partitions.csv` 和生成的分区表为准，不要使用固定的旧地址。
