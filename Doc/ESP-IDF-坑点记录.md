# ESP-IDF v5.5.4 实战踩坑记录

> 基于 CaPilot 项目 keyboard_demo（USB HID 键盘组件）开发过程中实际遇到的坑。
> 适用于立创实战派 ESP32-S3 + ESP-IDF v5.5.4 工具链。

## 1. 组件 CMakeLists 漏写 REQUIRES

**症状：** `fatal error: esp_bt_defs.h: No such file or directory`

**原因：** ESP-IDF v5.5.4 不再隐式继承所有组件依赖，必须显式声明。

**修复：** 在组件 CMakeLists.txt 的 `REQUIRES` 中显式声明：

```cmake
REQUIRES "bt" "nvs_flash" "esp_driver_gpio" "esp_event"
```

---

## 2. ESP-IDF v5.5.4 头文件路径变化

**症状：** `fatal error: freertos/eventgroups.h: No such file or directory`

**修复：** v5.5.4 中路径是 `freertos/event_groups.h`（下划线，不是 eventgroups）。

如果代码没用到 event groups，直接删除 include 即可。

---

## 3. ESP-IDF v5.5.4 legacy I2C 驱动限制

**症状：** `esp_lcd_new_panel_io_i2c_v1(60): scl_speed_hz is not need to set in legacy i2c_lcd driver`

**原因：** `ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG()` 宏在 v5.5.4 中不允许设置 `scl_speed_hz`，速度由 `i2c_config_t.master.clk_speed` 决定。

**修复：** 删除 `tp_io_config.scl_speed_hz = 400000;` 这一行。

---

## 4. esp_timer_get_time() 未声明

**症状：** `implicit declaration of function 'esp_timer_get_time'`

**修复：** 头文件中补 `#include "esp_timer.h"`

---

## 5. bool 类型未定义

**症状：** `unknown type name 'bool'`

**修复：** 头文件中补 `#include <stdbool.h>`

---

## 6. 从厂商例程剥离代码时的无关依赖

**症状：** 编译 BLE HID 例程报 `esp_wifi.h: No such file`，但实际代码没用到 WiFi。

**原因：** 立创实战派 14-handheld 例程的 `ble_hidd_demo.c` include 了 `esp_wifi.h`，但 HID 功能根本不需要 WiFi。

**修复：** 逐个检查并删除无关 include：
- `esp_wifi.h` — HID 不需要
- `esp_event.h` — 如果没用 event loop 就删除
- `esp_lvgl_port.h` — 如果组件不直接用 LVGL 就删除

---

## 7. components/ 下所有子目录都会被编译

**症状：** 想禁用 BLE 组件，改名为 `_disabled_xxx_ble/`，仍然被编译。

**原因：** ESP-IDF 会扫描 `components/` 下所有含 `CMakeLists.txt` 的子目录，下划线前缀**不阻止**编译。

**修复：** 必须物理移出 `components/` 目录：

```bash
mv components/unused_component ../_backup_unused_component
```

---

## 8. sdkconfig 修改后未重新配置

**症状：** 修改 `sdkconfig.defaults` 后编译，配置没生效。

**原因：** CMake 缓存了上次的配置，必须清除缓存后重新配置。

**修复：** 删除 `build/`、`sdkconfig`、`managed_components/` 后重新执行 CMake 配置：

```powershell
if (Test-Path "build") { Remove-Item -Recurse -Force "build" }
if (Test-Path "sdkconfig") { Remove-Item -Force "sdkconfig" }
if (Test-Path "managed_components") { Remove-Item -Recurse -Force "managed_components" }
```

ESP-BUILD-SKILL 的 `build.ps1 -Reconfigure` 参数已封装此操作。

---

## 9. MOUSE_BUTTON_LEFT 宏与 TinyUSB 枚举冲突

**症状：** `error: expected identifier before numeric constant`（在自定义鼠标组件头文件行）

**原因：** TinyUSB 的 `class/hid/hid.h` 已定义枚举：
```c
enum {
  MOUSE_BUTTON_LEFT   = 0x01,
  MOUSE_BUTTON_RIGHT  = 0x02,
  MOUSE_BUTTON_MIDDLE = 0x04,
};
```
如果在自定义组件头文件里 `#define MOUSE_BUTTON_LEFT 0x01`，预处理后 TinyUSB 枚举变成 `0x01 = 0x01`，触发编译错误。

**修复：** 自定义宏加项目前缀避免冲突：
```c
#define CAPOLOT_MOUSE_BTN_LEFT    0x01
#define CAPOLOT_MOUSE_BTN_RIGHT   0x02
#define CAPOLOT_MOUSE_BTN_MIDDLE  0x04
```

---

## 10. PowerShell 5.1 脚本中文注释导致解析失败

**症状：** 执行 `.ps1` 脚本报 `"," 后面缺少表达式` 或 `无法加载文件...禁止运行脚本`。

**原因：**
1. Windows PowerShell 5.1 默认用 GBK 编码读取脚本，但脚本文件是 UTF-8，中文注释被误读为乱码，破坏语法解析。
2. 默认 ExecutionPolicy = Restricted，禁止运行任何 `.ps1`。

**修复：**
1. 调用脚本时加 `-ExecutionPolicy Bypass`：
   ```powershell
   powershell -ExecutionPolicy Bypass -File "xxx.ps1" -Arg ...
   ```
2. 脚本文件中的注释和字符串**只用 ASCII**，中文说明放在 `.md` 文档里，不放 `.ps1` 里。
3. 或保存为 UTF-8 with BOM 编码（PS 5.1 能正确识别 BOM）。

---

## 11. USB HID 连续发送报告被丢弃（鼠标移动失效）

**症状：** 触摸拖动时鼠标光标不动，但 USB 已枚举成功（`tud_mounted()` 返回 true）。

**原因：** `tud_mounted()` 只表示 USB 设备已被主机识别，不代表 HID 端点可写。连续发送鼠标移动报告时，上一个报告可能还在端点缓冲区中，`tud_hid_mouse_report()` 内部检查 `tud_hid_ready()` 返回 false → 报告被丢弃。如果调用方不检查返回值就更新坐标基准，丢失的位移就永远无法补偿。

**修复：**
1. 组件层：暴露 `capilot_usb_hid_mouse_ready()`（检查 `tud_mounted() && tud_hid_ready()`），并在 `report()` 中加最多 3×1ms 等待。
2. 调用层：PRESSING 事件中只有 `ready()` 返回 true 且 `report()` 返回 ESP_OK 时才更新 `last_x/last_y`；否则保留旧基准，让位移累加到下一帧。

```c
// 错误：不检查返回值就更新基准
mouse_report(0, dx, dy, 0);
last_x = point.x;  // 如果 report 失败，dx 就丢了

// 正确：只有成功才更新基准
if (mouse_ready() && mouse_report(0, dx, dy, 0) == ESP_OK) {
    last_x = point.x;
}
```

---

## 维护说明

- 新增坑点请追加到本文件末尾，保持 `## N. 标题 / 症状 / 原因 / 修复` 结构
- 坑点通用化后，可考虑贡献到 ESP-IDF 官方 issue
- 本文件是经验沉淀，不是临时笔记，请勿删除
