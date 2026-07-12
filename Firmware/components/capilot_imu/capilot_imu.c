/**
 * @file capilot_imu.c
 * @brief CaPilot IMU 组件 — 芯片无关调度层
 *
 * 职责：
 *   1. 维护已注册驱动列表（s_drivers[]）
 *   2. capilot_imu_init() 时自动探测并选择第一个在线的驱动
 *   3. 公共 API 转发调用到 s_active_driver
 *
 * 新增芯片：在 s_drivers[] 中添加 extern 声明 + 数组项即可。
 */
#include "capilot_imu.h"
#include "capilot_imu_driver.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "capilot_imu";

/* ============ 已注册驱动列表 ============ */
/* 新增芯片：在下方添加 extern 声明，并加入 s_drivers[] 数组 */

extern const capilot_imu_driver_t capilot_imu_driver_qmi8658;

static const capilot_imu_driver_t *s_drivers[] = {
    &capilot_imu_driver_qmi8658,
    /* TODO: 添加新芯片驱动在这里，例如：
     *  extern const capilot_imu_driver_t capilot_imu_driver_mpu6050;
     *  &capilot_imu_driver_mpu6050,
     */
};

static const capilot_imu_driver_t *s_active_driver = NULL;
static capilot_imu_config_t        s_config = CAPILOT_IMU_CONFIG_DEFAULT();

/* ============ 公共 API 实现（调度层） ============ */

esp_err_t capilot_imu_init(void)
{
    /* 如果已选定驱动，直接重新初始化 */
    if (s_active_driver != NULL) {
        ESP_LOGI(TAG, "re-init with driver '%s'", s_active_driver->name);
        return s_active_driver->init(&s_config);
    }

    /* 探测：找到第一个在线的驱动 */
    ESP_LOGI(TAG, "probing %d driver(s)...", (int)(sizeof(s_drivers) / sizeof(s_drivers[0])));
    for (size_t i = 0; i < sizeof(s_drivers) / sizeof(s_drivers[0]); i++) {
        const capilot_imu_driver_t *drv = s_drivers[i];
        if (drv->is_present()) {
            s_active_driver = drv;
            ESP_LOGI(TAG, "driver '%s' detected, initializing...", drv->name);
            esp_err_t ret = drv->init(&s_config);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "IMU ready (driver='%s')", drv->name);
            } else {
                ESP_LOGW(TAG, "driver '%s' init failed: %s", drv->name, esp_err_to_name(ret));
                s_active_driver = NULL;
            }
            return ret;
        }
    }

    ESP_LOGE(TAG, "no IMU driver detected (check I2C wiring / power)");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t capilot_imu_init_with_config(const capilot_imu_config_t *config)
{
    if (config != NULL) {
        s_config = *config;
    }
    return capilot_imu_init();
}

esp_err_t capilot_imu_read(capilot_imu_data_t *data)
{
    ESP_RETURN_ON_FALSE(s_active_driver != NULL, ESP_ERR_INVALID_STATE,
                        TAG, "IMU not initialized (call capilot_imu_init first)");
    return s_active_driver->read(data);
}

esp_err_t capilot_imu_read_with_angle(capilot_imu_data_t *data)
{
    ESP_RETURN_ON_FALSE(s_active_driver != NULL, ESP_ERR_INVALID_STATE,
                        TAG, "IMU not initialized (call capilot_imu_init first)");
    return s_active_driver->read_with_angle(data);
}

bool capilot_imu_is_present(void)
{
    for (size_t i = 0; i < sizeof(s_drivers) / sizeof(s_drivers[0]); i++) {
        if (s_drivers[i]->is_present()) {
            return true;
        }
    }
    return false;
}

const char *capilot_imu_get_driver_name(void)
{
    return (s_active_driver != NULL) ? s_active_driver->name : "(none)";
}
