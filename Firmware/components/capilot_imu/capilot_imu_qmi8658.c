/**
 * @file capilot_imu_qmi8658.c
 * @brief QMI8658 六轴 IMU 驱动实现（capilot_imu_driver_t 接口）
 *
 * 寄存器格式（QMI8658A Datasheet Rev A, Page 31-32, Table 22）：
 *   CTRL2 [bit7=aST] [bits6:4=aFS] [bits3:0=aODR]
 *   CTRL3 [bit7=gST] [bits6:4=gFS] [bits3:0=gODR]
 *
 *   bit7 = Self-Test (0=disable, 1=enable) — 正常运行必须为 0
 *
 * ACC 量程 (aFS, bits[6:4]):
 *   000=±2g  001=±4g  010=±8g  011=±16g
 * GYR 量程 (gFS, bits[6:4]):
 *   000=±16dps  001=±32dps  010=±64dps  011=±128dps
 *   100=±256dps  101=±512dps  110=±1024dps  111=±2048dps
 *
 * ODR 编码 (bits[3:0])，Accel-only / 6DOF 模式：
 *   0011=1000/896.8Hz  0100=500/448.4Hz  0101=250/224.2Hz
 *   0110=125/112.1Hz    0111=62.5/56.05Hz  1000=31.25/28.025Hz
 *
 * 我们的配置 (6DOF 模式, CTRL7=0x03):
 *   CTRL2 = 0x15  → aST=0, aFS=001(±4g), aODR=0101(250Hz)
 *   CTRL3 = 0x55  → gST=0, gFS=101(±512dps), gODR=0101(224.2Hz)
 *
 * is_present() 用 BSP 通用函数（只需读一个寄存器）
 * init()/read() 用自己的 device handle（性能更好）
 */
#include "capilot_imu_driver.h"
#include "capilot_bsp_i2c.h"   /* BSP 通用 I2C 函数 */
#include "driver/i2c_master.h"    /* 新 I2C Master API（仅 init/read 用） */
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "capilot_imu_qmi8658";

/* ============ QMI8658 寄存器地址 ============ */
enum {
    QMI8658_WHO_AM_I = 0x00,
    QMI8658_CTRL1    = 0x02,
    QMI8658_CTRL2    = 0x03,
    QMI8658_CTRL3    = 0x04,
    QMI8658_CTRL7    = 0x08,
    QMI8658_STATUS0  = 0x2D,
    QMI8658_AX_L     = 0x33,   /* AX_L..GZ_H 共 12 字节 */
    QMI8658_RESET    = 0x60,
};

#define QMI8658_I2C_ADDR    0x6A
#define QMI8658_WHO_AM_I_VAL 0x05
#define QMI8658_I2C_TIMEOUT_MS 100

/* 自己的 I2C device handle（初始化时创建，后续复用） */
static i2c_master_dev_handle_t s_dev_handle = NULL;

/* ============ 量程/ODR → 寄存器值映射 ============ */

/* ACC range: bits [6:4] */
static uint8_t acc_range_to_reg(capilot_imu_acc_range_t range)
{
    static const uint8_t range_bits[] = { 0x00, 0x10, 0x20, 0x30 };
    return (range < 4) ? range_bits[range] : 0x10;
}

/* GYR range: bits [6:4] */
static uint8_t gyr_range_to_reg(capilot_imu_gyr_range_t range)
{
    static const uint8_t range_bits[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 };
    return (range < 8) ? range_bits[range] : 0x50;
}

/*
 * ODR: bits [3:0] (QMI8658A Datasheet Table 22)
 * 在 6DOF 模式下 (ACC+GYR 均启用), 实际 ODR 略低于标称值:
 *   0101 → 250Hz (Accel-only) / 224.2Hz (6DOF)
 *
 * 我们的枚举 → QMI8658 寄存器值:
 *   125Hz  → 0110 (125 / 112.1 Hz)
 *   250Hz  → 0101 (250 / 224.2 Hz)
 *   500Hz  → 0100 (500 / 448.4 Hz)
 *   1000Hz → 0011 (1000 / 896.8 Hz)
 *   2000Hz → 0010 (1793.6 Hz, 6DOF only)
 *   4000Hz → 0001 (3587.2 Hz, 6DOF only)
 */
static uint8_t odr_to_reg(capilot_imu_odr_t odr)
{
    static const uint8_t odr_bits[] = { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
    return (odr < 6) ? odr_bits[odr] : 0x05;
}

/* ============ 内部 I2C 读写（直接用新 API） ============ */
static esp_err_t qmi_read(uint8_t reg, uint8_t *data, size_t len)
{
    if (!s_dev_handle) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(s_dev_handle, &reg, 1, data, len, QMI8658_I2C_TIMEOUT_MS);
}

static esp_err_t qmi_write_byte(uint8_t reg, uint8_t val)
{
    if (!s_dev_handle) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev_handle, buf, 2, QMI8658_I2C_TIMEOUT_MS);
}

/* ============ 原始读取（无角度计算） ============ */
static esp_err_t qmi8658_read(capilot_imu_data_t *data)
{
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is NULL");

    /*
     * 直接读数据寄存器，不检查 STATUS0。
     *
     * QMI8658 的 data-ready 位是脉冲型（置位后短暂自动清除），
     * 10ms 轮询窗口内经常错过，导致读取成功率仅 ~45%。
     * 数据寄存器始终保存最新测量值，直接读即可保证 100% 成功率。
     * 副作用是偶尔会读到与上次相同的值（传感器 ODR 282.9Hz > 轮询 100Hz，
     * 这种情况极少），对手势检测无影响。
     */
    int16_t buf[6];
    esp_err_t ret = qmi_read(QMI8658_AX_L, (uint8_t *)buf, 12);
    if (ret != ESP_OK) {
        return ret;
    }

    data->acc_x = buf[0];
    data->acc_y = buf[1];
    data->acc_z = buf[2];
    data->gyr_x = buf[3];
    data->gyr_y = buf[4];
    data->gyr_z = buf[5];
    return ESP_OK;
}

/* ============ 驱动接口实现 ============ */

static esp_err_t qmi8658_init(const capilot_imu_config_t *config)
{
    ESP_LOGI(TAG, "init (addr=0x%02X)", QMI8658_I2C_ADDR);

    /* 创建自己的 I2C device handle */
    if (!s_dev_handle) {
        i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)capilot_bsp_i2c_get_bus_handle();
        if (!bus_handle) {
            ESP_LOGE(TAG, "I2C bus not initialized (call capilot_bsp_i2c_init first)");
            return ESP_ERR_INVALID_STATE;
        }

        i2c_device_config_t dev_cfg = {
            .device_address = QMI8658_I2C_ADDR,
            .scl_speed_hz = CAPILOT_BSP_I2C_FREQ_HZ,
        };
        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C device add failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "I2C device handle created");
    }

    /* 等待 WHO_AM_I，最多重试 ~3s */
    uint8_t id = 0;
    for (int retry = 0; retry < 30; retry++) {
        esp_err_t ret = qmi_read(QMI8658_WHO_AM_I, &id, 1);
        if (ret == ESP_OK && id == QMI8658_WHO_AM_I_VAL) {
            break;
        }
        if (retry == 0) {
            ESP_LOGW(TAG, "WHO_AM_I read %s (id=0x%02X), retrying...",
                     esp_err_to_name(ret), id);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        id = 0;
    }
    if (id != QMI8658_WHO_AM_I_VAL) {
        ESP_LOGE(TAG, "not found (WHO_AM_I=0x%02X, expected 0x%02X)",
                  id, QMI8658_WHO_AM_I_VAL);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "OK (WHO_AM_I=0x%02X)", id);

    /* 复位 + 配置 */
    qmi_write_byte(QMI8658_RESET, 0xb0);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* CTRL2/CTRL3: bit7=0 (Self-Test disabled), bits[6:4]=range, bits[3:0]=ODR */
    uint8_t ctrl2 = odr_to_reg(config ? config->odr : CAPILOT_IMU_ODR_250HZ)
                   | acc_range_to_reg(config ? config->acc_range : CAPILOT_IMU_ACC_RANGE_4G);
    uint8_t ctrl3 = odr_to_reg(config ? config->odr : CAPILOT_IMU_ODR_250HZ)
                   | gyr_range_to_reg(config ? config->gyr_range : CAPILOT_IMU_GYR_RANGE_512DPS);

    qmi_write_byte(QMI8658_CTRL1, 0x40);  /* 地址自动增加 */
    qmi_write_byte(QMI8658_CTRL7, 0x03);  /* 使能 ACC + GYR */
    qmi_write_byte(QMI8658_CTRL2, ctrl2);
    qmi_write_byte(QMI8658_CTRL3, ctrl3);

    ESP_LOGI(TAG, "initialized (CTRL2=0x%02X, CTRL3=0x%02X)", ctrl2, ctrl3);
    return ESP_OK;
}

static esp_err_t qmi8658_read_with_angle(capilot_imu_data_t *data)
{
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is NULL");

    /* ESP_ERR_INVALID_STATE 是正常状态（数据未就绪），静默传播 */
    esp_err_t ret = qmi8658_read(data);
    if (ret != ESP_OK) return ret;

    /* 由加速度计算三轴倾角（度） */
    float ax = (float)data->acc_x;
    float ay = (float)data->acc_y;
    float az = (float)data->acc_z;
    float temp;

    /* 弧度→角度系数 180/π = 57.29578 */
    temp = ax / sqrtf(ay * ay + az * az);
    data->angle_x = atanf(temp) * 57.29578f;

    temp = ay / sqrtf(ax * ax + az * az);
    data->angle_y = atanf(temp) * 57.29578f;

    temp = sqrtf(ax * ax + ay * ay) / az;
    data->angle_z = atanf(temp) * 57.29578f;

    return ESP_OK;
}

static bool qmi8658_is_present(void)
{
    uint8_t id = 0;
    /* is_present() 在 init() 之前调用，此时 s_dev_handle 还是 NULL
     * 必须用 BSP 通用函数（它通过缓存的 device handle 或临时创建） */
    if (capilot_bsp_i2c_read(QMI8658_I2C_ADDR, QMI8658_WHO_AM_I, &id, 1) != ESP_OK) {
        return false;
    }
    return id == QMI8658_WHO_AM_I_VAL;
}

/* ============ 驱动实例（供调度层注册） ============ */
const capilot_imu_driver_t capilot_imu_driver_qmi8658 = {
    .name = "qmi8658",
    .init = qmi8658_init,
    .read = qmi8658_read,
    .read_with_angle = qmi8658_read_with_angle,
    .is_present = qmi8658_is_present,
};
