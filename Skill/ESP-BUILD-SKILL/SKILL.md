---
name: desktopet-build
description: "ESP32-S3 固件编译、烧录、日志监控（脚本化，低 token）。使用场景：用户说'验证'/'编译'/'烧录'/'刷进去'/'看日志'/'build'/'flash'/'monitor'。"
agent_created: true
---

# ESP32-S3 编译烧录监控（脚本化）

ESP32-S3 项目的编译、烧录、串口监控已封装为脚本，AI 加载本 skill 后**调用脚本而非内联命令**，大幅降低 token 消耗。

## 触发

用户说"验证"、"编译"、"烧录"、"刷进去"、"看日志"、"build"、"flash"、"monitor"。

## 脚本位置

所有脚本在 `Skill/Esp-Build-Skill/scripts/` 下：

| 脚本 | 用途 |
|------|------|
| `setup-env.ps1` | 环境变量设置（被其他脚本自动 source） |
| `build.ps1` | 编译（cmake + ninja，支持 -Reconfigure -Log） |
| `flash.ps1` | 烧录（支持 -ComPort AUTO 自动检测、-WithStorage） |
| `monitor.py` | 串口监控（pyserial，支持指定秒数后退出） |
| `build-flash-monitor.ps1` | 一站式编译+烧录+监控 |

## AI 使用方式

加载本 skill 后，AI 直接用 PowerShell 工具调用脚本，**不要内联命令**。

### 编译

```powershell
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/build.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "keyboard_demo" -Log
```

- 修改 sdkconfig 后加 `-Reconfigure`
- 日志保存在 `Firmware/esp32_s3/{AppName}/logs/build_*.log`

### 烧录

```powershell
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/flash.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "keyboard_demo" -ComPort "COM8"
```

- `-ComPort AUTO` 自动检测最大 COM 号
- `-WithStorage` 额外烧录 storage.bin（SPIFFS 分区）
- Linux 串口为 `/dev/ttyUSB0`（或 `/dev/ttyACM0`）

### 串口监控

```bash
# Windows (bash):
python "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/monitor.py" COM8 15
# Linux:
python3 "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/monitor.py" /dev/ttyUSB0 15
# 第 3 个参数 = 监控秒数（省略 = 持续监控直到 Ctrl+C）
```

### 一站式（编译+烧录+监控）

```powershell
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/build-flash-monitor.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "keyboard_demo" -ComPort "COM8" -MonitorSec 15
```

- `-Reconfigure` 重新配置
- `-SkipMonitor` 跳过监控
- `-WithStorage` 烧录 SPIFFS

## 变量说明

- `{{PROJECT_ROOT}}` = 项目根（如 `B:\CaPilot` 或 `/home/user/CaPilot`）
- `{{APP_NAME}}` = `Firmware/esp32_s3/` 下的应用目录名（如 `keyboard_demo`、`desktopet`）
- AI 执行时扫描 `Firmware/esp32_s3/` 子目录自动确定 APP_NAME

## 故障排查

**找不到设备端口：** `-ComPort AUTO` 自动检测，或手动指定（如 COM8）。

**idf.py 找不到 / 子进程找不到工具链：** 已在脚本中改用 cmake + ninja 直接调用，绕过 idf.py 的 PATH 继承问题。

**烧录失败（设备忙）：** 关闭串口监控窗口后重试。

**ninja: build stopped: subcommand failed：** 加 `-Log` 保存日志，脚本会自动 grep `FAILED|fatal error|error:` 显示前 10 条错误。

**ESP-IDF v5.5.4 API 注意：**
- `esp_event_loop_get_default()` 不存在 → 用 `esp_event_loop_create_default()` + `esp_event_post()`
- SPIFFS 组件名为 `spiffs`
- 自定义分区表需 `CONFIG_PARTITION_TABLE_CUSTOM=y` + `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"`
