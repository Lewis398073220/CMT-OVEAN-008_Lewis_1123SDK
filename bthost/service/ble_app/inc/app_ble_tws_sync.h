#ifndef __BLE_AUDIO_TWS_CMD_HANDLER_H__
#define __BLE_AUDIO_TWS_CMD_HANDLER_H__

#include "bluetooth_bt_api.h"

typedef void (*app_ble_tws_sync_volume_callback)(void);
typedef void (*app_ble_tws_sync_volume_rec_callback)(uint8_t *buf, uint8_t buf_len);
typedef bool (*app_ble_tws_is_connected_t)(void);
typedef void (*app_ble_tws_cmd_send_via_ble_t)(uint8_t*, uint16_t);

// sync@APP_TWS_IBRT_MAX_DATA_SIZE
#define BLE_TWS_SYNC_MAX_DATA_SIZE  (672)

// sync see@ibrt_role_e
typedef enum
{
    APP_BLE_TWS_MASTER      =0,
    APP_BLE_TWS_SLAVE       =1,
    APP_BLE_TWS_UNKNOW      =0xff
} APP_BLE_TWS_SYNC_ROLE_E;

typedef enum
{
    BLE_TWS_SYNC_EXCH_BLE_AUDIO_INFO = 0,
    BLE_TWS_SYNC_REQ_TRIGGER_SYNC_CAPTURE,
    BLE_TWS_SYNC_EXCH_VOLUME_STATUS,
    BLE_TWS_SYNC_EXCH_VOLUME_OFFSET_STATUS,
    BLE_TWS_SYNC_SYNC_DEV_INFO,
    BLE_TWS_SYNC_SHARE_SERVICE_INFO,
    BLE_TWS_SYNC_CAPTURE_US_SINCE_LATEST_ANCHOR_POINT,
    //new customer cmd add here
} APP_BLE_TWS_SYNC_CMD_CODE_E;

typedef struct
{
    uint8_t  streamContext;
    uint32_t master_clk_cnt;
    uint16_t master_bit_cnt;	
    int32_t  usSinceLatestAnchorPoint;
    uint32_t triggertimeUs;
    uint8_t  reserve[6];
} AOB_TWS_SYNC_TRIGGER_CAPTURE_INFO_T;

typedef struct
{
    int32_t  usSinceLatestAnchorPoint;
    uint8_t  streamContext;	
    uint8_t  reserve[7];
} AOB_TWS_SYNC_US_SINCE_LATEST_ANCHOR_POINT_T;

#ifdef __cplusplus
extern "C"{
#endif

void aob_tws_handshake_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);
void aob_tws_handshake_rsp_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

void aob_tws_exchange_info_handler(uint16_t rsp_seq, uint8_t* data, uint16_t len);
void aob_tws_exchange_info_rsp_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

void app_ibrt_aob_tws_exch_ble_audio_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_aob_tws_exch_ble_audio_info_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_aob_tws_exch_ble_audio_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_aob_tws_exch_ble_audio_info_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void aob_tws_send_sync_capture_trigger_cmd(uint8_t *p_buff, uint16_t length);
void aob_tws_sync_capture_trigger_handler(uint16_t rsp_seq, uint8_t* data, uint16_t len);
void aob_tws_sync_capture_trigger_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void aob_tws_sync_capture_trigger_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void aob_tws_send_sync_us_since_latest_anchor_point_cmd(uint8_t *p_buff, uint16_t length);
void aob_tws_sync_us_since_latest_anchor_point_handler(uint16_t rsp_seq, uint8_t* data, uint16_t len);


void ble_audio_tws_sync_init(void);

int app_ble_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);
int app_ble_tws_sync_send_cmd(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint8_t high_priority, uint8_t *data, uint16_t data_len);
int app_ble_tws_sync_send_rsp(APP_BLE_TWS_SYNC_CMD_CODE_E code, uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ble_tws_sync_sniff_manager(uint8_t con_lid, bool streaming);
bool app_ble_tws_get_conn_state();
uint32_t app_ble_tws_get_conn_curr_ticks();

bool app_ble_tws_sync_volume(uint8_t *data, uint8_t data_len);
bool app_ble_tws_sync_volume_register(app_ble_tws_sync_volume_rec_callback receive_cb,
                                                   app_ble_tws_sync_volume_callback trigger_cb, 
                                                   app_ble_tws_sync_volume_callback offset_trigger_cb);
APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_ui_role();
APP_BLE_TWS_SYNC_ROLE_E app_ble_tws_get_tws_local_role();
int app_ble_tws_sync_release_trigger_channel(uint8_t chnl);
uint32_t app_ble_tws_sync_get_slave_time_from_master_time(uint32_t master_clk_cnt, uint16_t master_bit_cnt);
int app_ble_tws_sync_get_master_time_from_slave_time(uint32_t SlaveBtTicksUs, uint32_t* p_master_clk_cnt, uint16_t* p_master_bit_cnt);
uint8_t app_ble_tws_sync_get_avaliable_trigger_channel();
uint8_t app_ble_tws_sync_get_avaliable_trigger_channel();
int app_ble_tws_sync_write_ble_address(uint8_t *ble_addr);
int app_ble_tws_sync_ui_ascs_bond_ntf(void *data);
int app_ble_tws_sync_is_connected_register(app_ble_tws_is_connected_t cb);
int app_ble_tws_sync_release_cmd_semphore(void);
int app_ble_tws_sync_send_cmd_via_ble_register(app_ble_tws_cmd_send_via_ble_t cb);


#ifdef __cplusplus
}
#endif

#endif /*__BLE_AUDIO_TWS_CMD_HANDLER_H__*/

