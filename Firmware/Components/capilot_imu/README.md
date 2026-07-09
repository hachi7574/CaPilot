# capilot_imu — CaPilot IMU 组件

芯片无关的六轴 IMU 驱动框架，支持运行时自动探测并切换芯片。

## 架构

```
include/
├─ capilot_imu.h           ← 公共 API（调用方只用这个）
├─ capilot_imu_types.h     ← 数据类型 + 量程/ODR 枚举（芯片无关）
└─ capilot_imu_driver.h    ← 驱动 vtable 接口（内部，新增芯片需实现）
capilot_imu.c              ← 调度层（注册驱动 + 分发调用）
capilot_imu_qmi8658.c     ← QMI8658 具体实现
```

## 公共 API（芯片无关）

```c
#include "capilot_imu.h"

/* 默认配置初始化（自动探测芯片） */
capilot_imu_init();

/* 或：指定配置 */
capilot_imu_config_t cfg = CAPILOT_IMU_CONFIG_DEFAULT();
cfg.acc_range = CAPILOT_IMU_ACC_RANGE_8G;
capilot_imu_init_with_config(&cfg);

/* 读取数据 */
capilot_imu_data_t data;
if (capilot_imu_read_with_angle(&data) == ESP_OK) {
    /* data.acc_x/y/z, data.gyr_x/y/z, data.angle_x/y/z */
}
```

## 新增 IMU 芯片步骤

**只需修改 2 个文件，不用动公共 API 或调用方代码。**

### 1. 创建驱动文件 `capilot_imu_xxx.c`

```c
/* capilot_imu_xxx.c */
#include "capilot_imu_driver.h"
#include "capilot_bsp_i2c.h"

static esp_err_t xxx_init(const capilot_imu_config_t *config) { ... }
static esp_err_t xxx_read(capilot_imu_data_t *data) { ... }
static esp_err_t xxx_read_with_angle(capilot_imu_data_t *data) { ... }
static bool     xxx_is_present(void) { ... }

/* 导出驱动实例 */
const capilot_imu_driver_t capilot_imu_driver_xxx = {
    .name = "xxx",
    .init = xxx_init,
    .read = xxx_read,
    .read_with_angle = xxx_read_with_angle,
    .is_present = xxx_is_present,
};
```

### 2. 在 `capilot_imu.c` 中注册

```c
/* capilot_imu.c 顶部，s_drivers[] 数组前 */
extern const capilot_imu_driver_t capilot_imu_driver_xxx;

/* s_drivers[] 数组中（探测顺序 = 数组顺序） */
static const capilot_imu_driver_t *s_drivers[] = {
    &capilot_imu_driver_xxx,        /* 优先探测 */
    &capilot_imu_driver_qmi8658,   /* 后备 */
};
```

### 3. 更新 `CMakeLists.txt`

```cmake
SRCS
    "capilot_imu.c"
    "capilot_imu_qmi8658.c"
    "capilot_imu_xxx.c"   /* 新增 */
```

### 4. 重建

```bash
# 清除缓存后重新配置
rm -rf build/ sdkconfig managed_components/
idf.py reconfigure
idf.py build
```

## 经验总结

开发过程中踩过的坑已统一归档到 [`../../../Doc/ESP-IDF-坑点记录.md`](../../../Doc/ESP-IDF-坑点记录.md)，编号 12～16。

## 文件清单

| 文件 | 用途 | 修改频率 |
|------|------|----------|
| `include/capilot_imu.h` | 公共 API | 极低（稳定后不改） |
| `include/capilot_imu_types.h` | 数据类型 + 枚举 | 低（新增量程/ODR 时改） |
| `include/capilot_imu_driver.h` | 驱动 vtable | 极低（接口稳定） |
| `capilot_imu.c` | 调度层 | 低（新增芯片时加 1 行注册） |
| `capilot_imu_qmi8658.c` | QMI8658 驱动 | 低（芯片驱动稳定后不改） |
| `CMakeLists.txt` | 构建配置 | 低（新增 .c 时改） |
| `README.md` | 本文档 | 中（经验积累时更新） |

## 许可证

内部项目组件，随 CaPilot 项目更新。
