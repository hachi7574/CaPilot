---
name: esp-component-create
description: "ESP-IDF 组件开发工作流：一键生成组件骨架、工程结构、模板引用、验证清单。坑点和编译烧录已外置。使用场景：用户说'做组件'/'封装 XX 功能'/'新建 ESP-IDF 工程'/'USB HID'/'BLE HID'。"
agent_created: true
---

# ESP-IDF 组件开发工作流

从零创建 ESP-IDF 可复用组件、集成到应用工程、编译验证。模板与脚本已外置，本 skill 只保留决策性内容。

## 触发

用户说"做组件"、"封装 XX 功能"、"新建 ESP-IDF 工程"、"USB HID"、"BLE HID"、"键盘外设"、"加个模块"。

## 工程结构标准

```
项目根/
├─ firmware/esp32_s3/
│  └─ {{APP_NAME}}/                 ← 应用工程
│     ├─ CMakeLists.txt             ← 顶层 project()
│     ├─ sdkconfig.defaults
│     ├─ partitions.csv
│     ├─ main/
│     │  ├─ CMakeLists.txt          ← REQUIRES 引用自定义组件
│     │  ├─ idf_component.yml
│     │  ├─ main.c / bsp.c / app_ui.c
│     ├─ components/
│     │  └─ {{component_name}}/
│     │     ├─ CMakeLists.txt       ← REQUIRES 显式声明依赖
│     │     ├─ idf_component.yml    ← 第三方依赖（可选）
│     │     ├─ include/xxx.h
│     │     └─ xxx.c
│     └─ logs/
└─ Skill/ESP-COMPONENT-SKILL/       ← 本 skill
```

## 一键创建组件骨架

```powershell
& "B:\CaPilot\Skill\ESP-COMPONENT-SKILL\scripts\scaffold-component.ps1" `
    -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComponentName "my_module"
```

脚本会自动生成 `CMakeLists.txt` / `idf_component.yml` / `include/xxx.h` / `xxx.c`（stub 实现）。
生成后需在 `main/CMakeLists.txt` 的 `REQUIRES` 中手动添加组件名。

## 模板文件位置

所有模板在 `Skill/ESP-COMPONENT-SKILL/templates/`：

| 模板 | 用途 |
|------|------|
| `component_CMakeLists.txt` | 组件 CMakeLists（含 REQUIRES 占位符） |
| `component_idf_component.yml` | 组件第三方依赖 |
| `component_header.h` | 公开头文件规范（含 `#pragma once` / `extern "C"`） |
| `main_CMakeLists.txt` | main 工程的 CMakeLists |
| `main_idf_component.yml` | main 工程依赖（lvgl/ft5x06） |
| `sdkconfig.defaults` | 立创实战派 ESP32-S3 默认配置 |
| `partitions.csv` | 无 SPIFFS 分区表 |
| `usb_hid_reference.md` | USB HID 组件参考实现（TinyUSB） |
| `ble_hid_checklist.md` | BLE HID 剥离步骤 + USB/BLE 选型表 |
| `bsp_checklist.md` | BSP 裁剪要点（保留/删除清单） |

## 编译 / 烧录 / 监控

**不要内联命令**，调用 ESP-BUILD-SKILL 的脚本：

```powershell
# 编译
& "B:\CaPilot\Skill\ESP-BUILD-SKILL\scripts\build.ps1" -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -Log

# 修改 sdkconfig 后加 -Reconfigure
& "B:\CaPilot\Skill\ESP-BUILD-SKILL\scripts\build.ps1" -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -Reconfigure -Log

# 烧录
& "B:\CaPilot\Skill\ESP-BUILD-SKILL\scripts\flash.ps1" -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "COM8"

# 串口监控（15 秒）
"C:/Users/Hachi/.workbuddy/binaries/python/versions/3.13.12/python.exe" "B:/CaPilot/Skill/ESP-BUILD-SKILL/scripts/monitor.py" COM8 15
```

## USB HID vs BLE HID 选型

桌面外设首选 **USB HID**：即插即用、零配对、<5ms 延迟、固件 544KB（BLE 992KB）、DRAM 多 90KB。
详细对比表见 `templates/ble_hid_checklist.md`。

## 验证清单

组件完成后逐项检查：

1. 编译通过（`ninja` 退出码 0）
2. 烧录成功（`Hash of data verified`）
3. 启动正常（无 `abort() was called`）
4. 硬件初始化（I2C/PCA9557/LCD/Touch 全部 `initialized successfully`）
5. 功能验证（触摸/按键事件在串口日志正确打印）
6. 连接状态（`USB mounted=YES` 或 `HID connected=YES`）
7. 端到端（电脑实际收到输入）

## 常见坑点

8 个实战坑点（REQUIRES 漏写、v5.5.4 头文件路径、I2C 限制、components 全编译等）已迁移到：

**`B:\CaPilot\Doc\ESP-IDF-坑点记录.md`**

遇到编译错误时优先查这个文档。
