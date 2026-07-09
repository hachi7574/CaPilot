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

**原因：** 立创实战派 handheld 例程的 `ble_hidd_demo.c` include 了 `esp_wifi.h`，但 HID 功能根本不需要 WiFi。

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

Esp-Build-Skill 的 `build.ps1 -Reconfigure` 参数已封装此操作。

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

---

## 12. QMI8658 驱动 I2C 频率太低导致数据未就绪率高

**症状：** 100kHz 下 ~80% 的读取返回 `ESP_ERR_INVALID_STATE`（状态寄存器 bit0/1 = 0）。

**原因：** QMI8658 输出速率 250Hz（4ms/次），I2C 100kHz 读 12 字节 ≈ 1ms，时序竞争导致主机在传感器更新窗口外读状态。

**修复：** I2C 频率提升到 400kHz（所有板载 I2C 设备均支持）。`capilot_bsp/include/capilot_bsp_i2c.h` 中 `CAPILOT_BSP_I2C_FREQ_HZ` 改为 `400000`。

---

## 13. `ESP_RETURN_ON_ERROR` 误报"数据未就绪"为错误

**症状：** `capilot_imu_read_with_angle()` 调用 `capilot_imu_read()`，后者返回 `ESP_ERR_INVALID_STATE`（数据未就绪）时，`ESP_RETURN_ON_ERROR` 打印错误日志。

**原因：** `ESP_ERR_INVALID_STATE` 是**正常状态**（传感器尚未更新数据），不是错误，不应触发 `ESP_RETURN_ON_ERROR`。

**修复：** 手动检查返回值，`ESP_ERR_INVALID_STATE` **静默传播**，让调用方决定重试策略。

---

## 14. QMI8658 陀螺仪有恒定零偏

**症状：** 静止放置时 `gyr_x` 恒定 ≈ 8137 LSB（约 127 dps），而 `gyr_y` / `gyr_z` ≈ 0。

**原因：** QMI8658 芯片差异，需要在上层做零偏校准（上电后静止采样 N 次取平均，后续读数减去偏置）。

**修复：** 在手势识别组件初始化时加 2 秒静止校准阶段。参见 `capilot_gesture`。

---

## 15. 状态寄存器检查应允许"任一个就绪"

**症状：** 早期代码检查 `(status & 0x03) == 0x03`（ACC 和 GYR 同时就绪），导致 ~40% 的读取被跳过。

**原因：** ACC 和 GYR 的更新时刻可能在 250Hz 周期内略有偏差，不会严格同时就绪。

**修复：** 改为 `(status & 0x03) != 0`（任一个就绪就可以读），读出后上层自行判断数据有效性。

---

## 16. I2C 读取连续寄存器需用专用连续读函数

**症状：** 直接用 `i2c_master_read()` 读 12 字节（AX_L..GZ_H）时，部分寄存器值错误。

**原因：** QMI8658 要求 `CTRL1` 设置 `0x40`（地址自动增加），且读取函数需支持连续读（无停止位）。

**修复：** 使用 `capilot_bsp_i2c_read()`（封装了 `i2c_master_transmit_read_rssi()` 连续读），并确保 `CTRL1 = 0x40`。

---

## 17. ESP-IDF v5.x 旧 I2C API 弃用警告

**症状：** 编译时出现警告：
```
W (604) i2c: This driver is an old driver, please migrate your application code to adapt `driver/i2c_master.h`
```

**原因：** ESP-IDF v5.x 引入了新 I2C Master API（在 `esp_driver_i2c` 组件中），旧 API（`driver/i2c.h`）已弃用。

**新 API 核心变化：**

| 旧 API | 新 API | 说明 |
|--------|--------|------|
| `i2c_param_config()` + `i2c_driver_install()` | `i2c_new_master_bus()` | 创建 I2C 总线 |
| `i2c_master_write_read_device()` | `i2c_master_transmit_receive()` | 写+读复合交易 |
| `i2c_master_write_to_device()` | `i2c_master_transmit()` | 只写交易 |
| 整数端口号（0/1） | 句柄（`i2c_master_bus_handle_t` / `i2c_master_dev_handle_t`） | 类型安全 |

**新 API 使用模板：**

```c
#include "driver/i2c_master.h"  // 注意：不是 driver/i2c.h

static i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_master_dev_handle_t s_dev_handle = NULL;

esp_err_t i2c_init(void) {
    i2c_master_bus_config_t bus_conf = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_1,
        .scl_io_num = GPIO_NUM_2,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_conf, &s_bus_handle), TAG, "I2C bus create failed");

    i2c_device_config_t dev_conf = {
        .device_address = 0x6A,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus_handle, &dev_conf, &s_dev_handle), TAG, "I2C device add failed");
    return ESP_OK;
}

// 写+读复合交易（最常用）
esp_err_t i2c_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(s_dev_handle, &reg, 1, data, len, -1);
}

// 只写交易
esp_err_t i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev_handle, buf, 2, -1);
}
```

**性能优化技巧：**

对于频繁访问的 I2C 设备（如 PCA9557、IMU），**缓存 device handle**：
- 初始化时调用 `i2c_master_bus_add_device()` 创建句柄
- 后续所有交易复用同一句柄（零开销）
- 不要在每次交易时创建/删除句柄（开销大）

**Capilot 项目实践：**

`capilot_bsp` 组件已迁移到新 API（2026-07-05）：
- `capilot_bsp.c` — 总线初始化 + PCA9557 句柄缓存
- `capilot_bsp_i2c.c` — 通用 I2C 读写（带设备句柄缓存数组，最多 4 个设备）
- 组件 `CMakeLists.txt` 的 `REQUIRES` 已加 `esp_driver_i2c`

---

## 18. I2S 新版 driver `i2s_new_channel` 参数顺序是 `(tx, rx)` 不是 `(rx, tx)`

**症状：** 全双工模式下 `i2s_channel_read()` 报 `this channel is not rx channel`

**原因：** ESP-IDF 新版 I2S driver（`driver/i2s_std.h` / `driver/i2s_tdm.h`）的 `i2s_new_channel` 函数签名是：
```c
esp_err_t i2s_new_channel(const i2s_chan_config_t *chan_cfg,
                          i2s_chan_handle_t *tx_handle,   // 第一个是 TX
                          i2s_chan_handle_t *rx_handle);  // 第二个是 RX
```
很容易想当然写成 `(rx, tx)`，导致两个 handle 互换，read TX handle 报错。

**修复：** 参数顺序是 `(cfg, tx, rx)`：
```c
// 正确
i2s_new_channel(&chan_cfg, &s_tx_chan, &s_rx_chan);
// 错误（handle 互换）
i2s_new_channel(&chan_cfg, &s_rx_chan, &s_tx_chan);
```

---

## 19. I2S 全双工模式下 RX disable 会导致控制器时钟停止，TX write 超时

**症状：** 先录音（RX enable → disable），再播放（TX enable），`i2s_channel_write()` 立即超时返回 `ESP_ERR_TIMEOUT`

**原因：** 全双工模式下 TX 和 RX 共享同一个 I2S 控制器。控制器的时钟由两个 channel 共同维护——**只有至少一个 channel enable 时时钟才输出**。当 RX disable 后（且 TX 还没 enable），控制器时钟停止。虽然之后 TX enable，但 DMA 可能因时钟未稳定而无法搬运数据，导致 write 阻塞超时。

**修复：** 播放时保持 RX channel enable（即使不读数据），维持控制器时钟持续：
```c
esp_err_t playback_start(void) {
    // 全双工：播放前保持 RX enable，维持控制器时钟
    i2s_chan_handle_t rx = capilot_audio_get_shared_rx_chan();
    if (rx != NULL) {
        esp_err_t ret = i2s_channel_enable(rx);
        // ESP_ERR_INVALID_STATE 表示已 enable，忽略
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "rx keep-enable: %s", esp_err_to_name(ret));
        }
    }
    return i2s_channel_enable(s_tx_chan);
}
```

**关键点：** `i2s_channel_enable()` 对已 enable 的 channel 调用会返回 `ESP_ERR_INVALID_STATE`，这不是错误，需要静默忽略。

---

## 20. ES7210/ES8311 官方组件库用旧版 I2C API，与新版 i2c_master 不兼容

**症状：** 链接 `espressif/es7210` 或 `espressif/es8311` 组件库时，与 BSP 的新版 I2C API 冲突

**原因：** 乐鑫官方 `es7210` / `es8311` 组件库内部使用旧版 `driver/i2c.h`（`i2c_cmd_link_create` 等），而 CaPilot BSP 使用新版 `driver/i2c_master.h`（`i2c_master_bus_add_device` 等）。新旧 I2C API **不能在同一工程混用**（会重复安装 I2C 驱动，产生冲突）。

**修复：** 不用官方组件库，从 [esp-bsp GitHub](https://github.com/espressif/esp-bsp) 提取寄存器初始化序列，用 BSP 的 I2C 读写函数直接操作寄存器：
```c
// 用 BSP I2C 代替官方组件库的 I2C
static esp_err_t es7210_write(uint8_t reg, uint8_t val) {
    return capilot_bsp_i2c_write_byte(0x41, reg, val);
}
```
组件 `idf_component.yml` 只声明 `idf: "^5.0"`，不声明外部依赖。

---

## 21. Demo 工程缺 sdkconfig.defaults 导致 target 默认 esp32

**症状：** 编译报 `fatal error: esp_lcd_touch.h: No such file or directory`，日志显示 `Building ESP-IDF components for target esp32`（而非 esp32s3）

**原因：** 新建 demo 工程如果没有 `sdkconfig.defaults` 指定 `CONFIG_IDF_TARGET`，ESP-IDF 默认 target 是 esp32。esp32 的组件不包含 `esp_lcd_touch.h`（这是 esp32s3 特有的触摸驱动），导致编译失败。

**修复：** 创建 `sdkconfig.defaults`：
```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
```
然后清除旧 build 产物重新配置（见坑点 #8）。

---

## 22. ES7210 从 TDM 改 STD 模式的寄存器配置

**症状：** 全双工模式下 ES7210 和 ES8311 需要统一用 STD 模式（不能用 TDM），但不知道怎么改

**原因：** ES7210 是 4 通道 ADC，默认用 TDM 模式。但 ESP32-S3 的 I2S 控制器在同一时间只能工作在一种模式（TDM 或 STD）。全双工要求 TX 和 RX 用相同模式。ES8311 不支持 TDM，所以 ES7210 必须改 STD。

**修复：** ES7210 只需改一个寄存器：
- `REG12 (SDP_INTERFACE2)`：从 `0x02`（TDM 使能）改为 `0x00`（TDM 禁止，标准 I2S）
- `REG11 (SDP_INTERFACE1)` 保持 `0x60`（I2S 格式 + 16bit）不变
- I2S 配置从 `i2s_tdm_config_t` + `i2s_channel_init_tdm_mode` 改为 `i2s_std_config_t` + `i2s_channel_init_std_mode`

开发板只焊了 2 个麦克风，TDM 的 4 通道用不上，STD stereo 完全够用。

---

## 23. Esp-Build-Skill build.ps1 路径写死，无法编译非标准目录

**症状：** `build.ps1 -AppName "xxx_demo"` 报 `The source directory "B:/CaPilot/Firmware/esp32_s3/xxx_demo" does not exist`

**原因：** `Esp-Build-Skill/scripts/build.ps1` 第 16 行路径写死：
```powershell
$appPath = Join-Path $ProjectDir "firmware\esp32_s3\$AppName"
```
只支持 `Firmware/esp32_s3/` 下的工程，不支持 `Firmware/Components_Demos/` 下的工程。

**修复（绕过）：** 直接用 cmake + ninja 编译，不走 build.ps1：
```powershell
. "B:\CaPilot\Skill\Esp-Build-Skill\scripts\setup-env.ps1"
$appPath = "B:\CaPilot\Firmware\Components_Demos\your_demo"
& $ESP_CMAKE -G Ninja -DPYTHON_DEPS_CHECKED=1 "-DPYTHON=$ESP_PYTHON" `
    -DESP_PLATFORM=1 "-DCMAKE_MAKE_PROGRAM=$ESP_NINJA" -DCCACHE_ENABLE=0 `
    -B "$appPath\build" -S $appPath
Set-Location "$appPath\build"
& $ESP_NINJA
```

**修复（根本）：** build.ps1 应支持 `-AppPath` 参数指定完整路径，而非硬编码 `firmware\esp32_s3\`。

---

## 24. 串口被占用时 esptool 报"信号灯超时"或"拒绝访问"

**症状：** 烧录时报 `Could not open COM8, the port is busy or doesn't exist` 或 `PermissionError(13, '拒绝访问。')`

**原因：**
1. 其他串口监控工具（Arduino IDE 串口监视器、PuTTY、idf.py monitor）占着 COM 口
2. 上一个 PowerShell `SerialPort` 对象未正确 Close/Dispose，句柄泄漏
3. USB 转串口芯片状态异常（需要拔插 USB 线）

**修复：**
1. 关闭所有占用 COM 口的工具
2. PowerShell `SerialPort` 用完必须 `$port.Close()`（最好用 `try/finally`）
3. 拔插 USB 线重置串口芯片
4. 如果不确定哪个进程占用，用 Process Explorer 搜索 "serial" 或对应 COM 口名

---

## 维护说明

- 新增坑点请追加到本文件末尾，保持 `## N. 标题 / 症状 / 原因 / 修复` 结构
- 坑点通用化后，可考虑贡献到 ESP-IDF 官方 issue
- 本文件是经验沉淀，不是临时笔记，请勿删除
