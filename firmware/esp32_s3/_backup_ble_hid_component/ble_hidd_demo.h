#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 启动 / 停止 BLE HID 服务 */
void bt_hid_start(void);
void bt_hid_end(void);

/* 连接状态（由 ble_hidd_demo.c 维护） */
extern uint16_t hid_conn_id;
extern bool sec_conn;

#ifdef __cplusplus
}
#endif
