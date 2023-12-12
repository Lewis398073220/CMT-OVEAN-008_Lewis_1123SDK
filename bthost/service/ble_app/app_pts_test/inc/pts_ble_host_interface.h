#ifndef __PTS_BLE_HOST_INTERFACE_H__
#define __PTS_BLE_HOST_INTERFACE_H__

#include "gapm_le.h"
#include "gapm_le_msg.h"
#include "gapc_le_msg.h"
#include "gatt_msg.h"
#include "nvrecord_extension.h"

#define PTS_ADV_DATA_MAX   50
#define PTS_TX_POWER_LEVEL 12
#define ATT_VALUE_MAX_LENGTH 66

//16 bit(Subcategory:0-5, Category:6-15)
// see:https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf
#define PTS_APPEARANCE_TYPE         0x0941

#define PTS_BLE_ADVERTISING_INTERVAL (160)

#define PTS_SLAVE_CONN_INTERVAL_MIN 100      //Used for PTS tests only
#define PTS_SLAVE_CONN_INTERVAL_MAX 300      //Used for PTS tests only

enum BIG_OPERATION{
    PTS_BIG_CREAT_CMP      = 0,
    PTS_BIG_DATA_SETUP_CMP = 1,
};

enum GATT_ROL{
    /// Client user role
    PTS_GATT_ROLE_CLIENT = 0x00,
    /// Server user role
    PTS_GATT_ROLE_SERVER = 0x01,
    /// Role not defined
    PTS_GATT_ROLE_NONE   = 0xFF,
};

enum set_pairing_para{
    PTS_PAIRING_IOCAP         = 0,
    PTS_PAIRING_AUTH          = 1,
    PTS_PAIRING_KEY_DIST      = 2,
    PTS_PAIRING_NV_GET_KEY    = 3,
    PTS_PAIRING_SEC_REQ_LEVEL = 4,
    PTS_PAIRING_KEY_SIZE      = 5,
    PTS_PAIRING_OOB           = 6,
};

typedef void(*pts_scan_report_cb)(struct gapm_ext_adv_report_ind *param);

struct pts_subtask_handlers
{
    /// Pointer to the message handler table
    const struct ke_msg_handler *p_msg_handler_tab;
    /// Number of messages handled
    uint16_t msg_cnt;
};

typedef struct send_adv_data_t{
    uint8_t data[PTS_ADV_DATA_MAX];
    uint8_t len;
}send_adv_data_t;

typedef struct adv_element {
    // Length of Type and Value fields for AD data type
    uint8_t len;
    // Type: «Service Data - 16-bit UUID»
    uint8_t type;
    uint8_t data[PTS_ADV_DATA_MAX];
}adv_data_elem_t;

//ADV Process Information 
typedef struct pts_adv_para {
    uint8_t actv_idx;
    uint8_t max_adv_evt;
    uint16_t duration;
    send_adv_data_t adv_data;
    send_adv_data_t per_adv_data;
    struct gapm_activity_create_adv_cmd para;
}pts_adv_para_t;

//SCAN Process Information
typedef struct pts_scan_para {
    uint8_t actv_idx;
    struct gapm_scan_param para;
    pts_scan_report_cb scan_report_cb;
}pts_scan_para_t;

//CONN Process Information
typedef struct pts_conn_para {
    uint8_t actv_idx;
    uint8_t own_addr_type;
    struct gapm_init_param para;
}pts_conn_para_t;

//BIS Process Information
typedef struct pts_bis_para{
    uint16_t conhdl;
    struct gapm_activity_create_adv_cmd *adv_para;
    struct hci_le_create_big_cmd big_para;
    struct hci_le_create_big_cmp_evt big_creat_cmp;
    const struct data_path_itf *isoohci_data_path;
}pts_bis_para_t;

typedef struct pts_sm_para {
    //SM pairing para
    uint8_t pairing_oob;
    uint8_t pairing_iocap;
    uint8_t pairing_auth;
    uint8_t pairing_sec_level;
    uint8_t pairing_key_dist;
    uint8_t pairing_key_size;
    // SM Generate a PIN Code- (Between 100000 and 999999)
    uint32_t pin_code;
    //SM FLOW FLAG
    uint8_t pairing_nv_get_key;
    uint8_t bonded_flag;
    //SM local key
    uint8_t loc_irk[KEY_LEN];

    #ifdef _BLE_NVDS_
    BleDevicePairingInfo pts_save_info;
    #endif
}pts_sm_para_t;

typedef struct gatt_att_para {
    uint16_t start_hdl;
    uint16_t number_hdl;
    uint8_t ser_att_value[ATT_VALUE_MAX_LENGTH];
    uint8_t ser_att_value_len;
}gatt_att_para_t;

typedef struct gatt_server_para{
    uint8_t pts_user_lid;
    uint8_t server_att_nb;
    gatt_att_para_t server_att_para[2];
}gatt_server_para_t;

typedef struct gatt_client_para{
    uint8_t pts_user_lid;
    //the return MAX_PTU of cli_write_reliable MSG
    uint8_t write_reliable_data_len;
}gatt_client_para_t;

typedef struct gatt_para {
    uint8_t conidx;
    uint8_t dummy;
    uint8_t role;
    gatt_server_para_t pts_server;
    gatt_client_para_t pts_client;
}gatt_para_t;

/****************GAP MODULE***************************/
//adv interface
void pts_adv_data_add_element(send_adv_data_t *adv_param, uint8_t adv_type,
                     uint16_t uuid_type, const uint8_t *adv_data, uint8_t adv_len);

pts_adv_para_t *pts_adv_get_para_index();
void pts_set_default_adv_data();
void pts_adv_start();
void pts_adv_creat(struct gapm_activity_create_adv_cmd *adv_para);
void pts_adv_set_data(uint8_t adv_type, const uint8_t *adv_data, uint8_t data_len);
void pts_adv_enable(uint16_t duration, uint8_t max_adv_evt);
void pts_adv_disenable();
void pts_adv_delete();

//scan interface
pts_scan_para_t *pts_scan_get_para_index();
void pts_scan_start(uint8_t owr_addr_type, pts_scan_report_cb repot_cb);
void pts_scan_creat(uint8_t own_addr_type);
void pts_scan_enable(gapm_scan_param_t *scan_para, pts_scan_report_cb scan_cb);
void pts_scan_disenable();
void pts_scan_delete();

//Connection interface
pts_conn_para_t *pts_conn_get_para_index();
void pts_conn_start();
void pts_conn_creat(uint8_t own_addr_type);
void pts_conn_enable(gapm_init_param_t *param);
void pts_conn_disenable();
void pts_conn_delete();

//BIG interface
pts_bis_para_t *pts_big_get_para_index();
void pts_big_start();
void pts_send_iso_start(uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu);
void pts_send_iso_stop();
void pts_send_iso_data(uint16_t seq_num, uint8_t *payload,
                            uint16_t payload_len, uint32_t ref_time);


//Operate the interface after the link is established
void pts_disconnect();
void pts_get_peer_dev_info(uint8_t operation);
void pts_gapc_cli_exchange_mtu();
void pts_update_param(uint8_t conidx, struct gapc_conn_param *conn_param);
uint8_t *pts_get_peer_addr();

//SM interface
void pts_sm_init(void);
void pts_sm_reset(void);
void pts_sm_pair_req();
void pts_sm_send_security_req(uint8_t conidx, enum gap_auth authority);
bool pts_sm_send_encrypt_req(uint8_t conidx, uint8_t * securityInfo);
void pts_sm_set_pairing_para(int type, int param);
uint8_t pts_sm_get_bond_status(void);


/****************GATT MODULE***************************/
//GATT reg and unreg
void pts_gatt_reg(uint8_t role);
void pts_gatt_unreg(uint8_t role);

//GATT svc
void pts_gatt_svc_add(uint8_t info , uint8_t* p_uuid , uint8_t nb_att_rsvd,
                            uint8_t nb_att , gatt_att_desc_t* p_atts);
void pts_gatt_svc_remove(uint8_t server_att_nb);
void pts_gatt_svc_ctrl(uint8_t server_att_nb,uint8_t enable , uint8_t visible);
void pts_gatt_srv_event_send(uint8_t start_hdl,uint8_t evt_type,int value_length,uint8_t *value);
void pts_gatt_set_ser_att_datasize(uint8_t server_att_nb , uint8_t data_size);

//GATT cli
void pts_gatt_cli_mtu_updata();
void pts_gatt_cli_discover_svc(uint8_t       disc_type, bool full, uint16_t start_hdl,
                                 uint16_t end_hdl, uint8_t uuid_type,  uint8_t *uuid);
void pts_gatt_cli_discover_char(uint8_t       disc_type, uint16_t start_hdl,
                                 uint16_t end_hdl, uint8_t uuid_type,  uint8_t *uuid);
void pts_gatt_cli_discover_desc(uint16_t start_hdl,uint16_t end_hdl);

void pts_gatt_cli_read(uint16_t hdl, uint16_t offset, uint16_t length);
void pts_gatt_cli_read_by_uuid(uint16_t        start_hdl, uint16_t end_hdl,
                                       uint8_t uuid_type, uint8_t *uuid);
void pts_gatt_cli_read_multiple(uint8_t        nb_att, gatt_att_t *att_info);

void pts_gatt_cli_write(uint8_t      write_type, int hdl, uint16_t offset,
                                             int data_size, uint8_t *data);
void pts_gatt_cli_write_reliable(uint8_t      write_type, int hdl, uint16_t offset,
                                             int data_size, uint8_t *data);

/****************L2CAP MODULE***************************/
void pts_l2cap_add_psm(uint16_t psm);
void pts_l2cap_channel_creat(uint16_t psm);


#endif
