#ifndef  __BLE_MESH_APP_H
#define __BLE_MESH_APP_H

#define CID_ESP 0x02E5
static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

esp_err_t ble_mesh_init(void);
void server_send_to_client(uint16_t dst_addr);

#endif