#include <stdio.h>
#include <string.h>
#include "ke_msg.h"
#include "ke_task.h"
#include "co_hci.h"
#include "co_math.h"
#include "nvrecord_ble.h"
//platform drv include
#include "tgt_hardware.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
//ble stack include
#include "hl_hci.h"
#include "gapc_msg.h"
#include "gatt_msg.h"
#include "l2cap_msg.h"
#include "data_path.h"
//app layer include
#include "app.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"        // Data Path Server Application Definitions
#endif //(BLE_APP_DATAPATH_SERVER)

#define PTS_SM_PIN_CODE 666666

#define PTS_MAX_TX_OCTETS   251
#define PTS_MAX_TX_TIME     2120
#define PTS_RESOLVING_LIST_MAX_NUM 4
#define PTS_DEVICE_NAME_MAX_LEN    24

/// States of APP task
enum appm_state
{
    /// Initialization state
    PTS_APPM_INIT,
    /// Database create state
    PTS_APPM_CREATE_DB,
    /// Ready State
    PTS_APPM_READY,
    /// Advertising state
    PTS_APPM_ADVERTISING,
    /// Scanning state
    PTS_APPM_SCANNING,
    /// connecting state
    PTS_APPM_CONNECTING,
    /// Connected state
    PTS_APPM_CONNECTED,
    //Connection Encryption
    PTS_APPM_ENCRYPTED,
    /// Number of defined states.
    PTS_APPM_STATE_MAX
};

ke_state_t ble_pts_state;
extern const struct pts_subtask_handlers pts_sec_handlers;
extern const struct app_subtask_handlers app_datapath_server_table_handler;
#ifdef CTKD_ENABLE
extern void app_bt_ctkd_connecting_mobile_handler(void);
#endif

/**********************************GAP PART*************************************************/
//adv_type: gap_ad_type
void pts_adv_data_add_element(send_adv_data_t *adv_param, uint8_t adv_type,
                             uint16_t uuid_type, const uint8_t *adv_data, uint8_t adv_len)
{
    adv_data_elem_t part_data;

    part_data.type = adv_type;
    if(uuid_type != 0){       //Contains UUID data
        part_data.data[0] = uuid_type & 0x00FF;
        part_data.data[1] = (uuid_type & 0xFF00) >> 8;
        memcpy(&part_data.data[2], adv_data, adv_len);
        part_data.len =  sizeof(part_data.type) + sizeof(uuid_type)+ adv_len;
    }else{                    //no contains UUID data
        memcpy(part_data.data, adv_data, adv_len);
        part_data.len =  sizeof(part_data.type) + adv_len;
    }

    memcpy(&adv_param->data[adv_param->len], &part_data,
              part_data.len+sizeof(part_data.len));

    adv_param->len += part_data.len+sizeof(part_data.len);
}

void pts_add_dev_to_rslv_list(uint8_t peer_addr_type, uint8_t *perr_addr)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T* nvrecord_ble_p = nv_record_blerec_get_ptr();
    struct gapm_list_set_ral_cmd *setReslovingListCmd = KE_MSG_ALLOC_DYN(GAPM_LIST_SET_CMD,
                                                         TASK_GAPM, TASK_BLE_PTS,
                                                         gapm_list_set_ral_cmd,
                                                         sizeof(gap_ral_dev_info_t));

    setReslovingListCmd->operation = GAPM_SET_RAL;
    setReslovingListCmd->size = 1;
    setReslovingListCmd->ral_info[0].priv_mode = 0;
    setReslovingListCmd->ral_info[0].addr.addr_type = peer_addr_type;
    memcpy(setReslovingListCmd->ral_info[0].addr.addr,  perr_addr, GAP_BD_ADDR_LEN);
    //setReslovingListCmd->ral_info[0].addr.addr_type = nvrecord_ble_p->ble_nv[0].pairingInfo.peer_addr.addr_type;
    //memcpy(setReslovingListCmd->ral_info[0].addr.addr,  nvrecord_ble_p->ble_nv[0].pairingInfo.peer_addr.addr, GAP_BD_ADDR_LEN);
    memcpy(setReslovingListCmd->ral_info[0].peer_irk,  nvrecord_ble_p->ble_nv[0].pairingInfo.IRK, 16);
    memcpy(setReslovingListCmd->ral_info[0].local_irk,  nvrecord_ble_p->self_info.ble_irk, 16);

    // Send the message
    ke_msg_send(setReslovingListCmd);

}
void pts_set_dev_config(uint8_t priv_en, uint8_t timeout)
{
        // Set Device configuration
        struct gapm_set_dev_config_cmd* cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                                           TASK_GAPM, TASK_BLE_PTS,
                                                           gapm_set_dev_config_cmd);
        // Set the operation
        cmd->operation = GAPM_SET_DEV_CONFIG;
        // Set the device role - Peripheral
        cmd->cfg.role      = GAP_ROLE_ALL;
        // Set Data length parameters
        cmd->cfg.sugg_max_tx_octets = PTS_MAX_TX_OCTETS;
        cmd->cfg.sugg_max_tx_time   = PTS_MAX_TX_TIME;
        cmd->cfg.pairing_mode = GAPM_PAIRING_LEGACY;

        cmd->cfg.pairing_mode |= GAPM_PAIRING_SEC_CON;

        if(priv_en)
        {
            //set priv addr
            cmd->cfg.renew_dur = timeout;
            cmd->cfg.privacy_cfg = GAPM_PRIV_CFG_PRIV_EN_BIT;
        }else{
            cmd->cfg.renew_dur = timeout;
            cmd->cfg.privacy_cfg = 0;
        }

        cmd->cfg.tx_pref_phy = GAP_PHY_LE_2MBPS;
        cmd->cfg.rx_pref_phy = GAP_PHY_LE_2MBPS;

        SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_MTU_EXCH, 1);
        SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_FEAT_EN, 1);
        SETF(cmd->cfg.att_cfg, GAPM_ATT_CLI_DIS_AUTO_EATT, 1);

        // load IRK
        nv_record_blerec_get_local_irk(cmd->cfg.irk.key);

        // Send message
        ke_msg_send(cmd);
}


static void pts_stop_activity(uint8_t actv_idx)
{
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                  TASK_GAPM, TASK_BLE_PTS,
                                                  gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(cmd);
}

static void pts_delete_activity(uint8_t actv_idx)
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_delete_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                      TASK_GAPM, TASK_BLE_PTS,
                                                      gapm_activity_delete_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_DELETE_ACTIVITY;
    cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(cmd);
}


//ble host pts adv function
static pts_adv_para_t adv_info = {
    .actv_idx     = 0xFF,
    .max_adv_evt  = 0,
    .duration     = 0,
    .adv_data     = {{0}, 0},
    .per_adv_data = {{0}, 0},
    .para = {
        .own_addr_type = GAPM_STATIC_ADDR,
        .type = GAPM_ADV_TYPE_LEGACY,
        .adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK,
        .adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC,
        .adv_param.with_flags = false,
        .adv_param.max_tx_pwr = 0xBA,
        .adv_param.filter_pol = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY,
        .adv_param.prim_cfg.adv_intv_min = PTS_BLE_ADVERTISING_INTERVAL,
        .adv_param.prim_cfg.adv_intv_max = PTS_BLE_ADVERTISING_INTERVAL,
        .adv_param.prim_cfg.chnl_map = 7,
        .adv_param.prim_cfg.phy = 1,
        .second_cfg.max_skip = 1,
        .second_cfg.phy = 1,
        .second_cfg.adv_sid = 1,
        .randomAddrRenewIntervalInSecond = 0,
        .period_cfg.interval_min = 0x0a,
        .period_cfg.interval_max = 0x0a,
    },
};

void pts_adv_creat(struct gapm_activity_create_adv_cmd *adv_para)
{

	struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
															  TASK_GAPM, TASK_BLE_PTS,
															  gapm_activity_create_adv_cmd);

    memcpy(p_cmd, adv_para, sizeof(struct gapm_activity_create_adv_cmd));

    // Set operation code
    p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

	// Send the message
	ke_msg_send(p_cmd);
}

void pts_adv_set_data(uint8_t adv_type, const uint8_t *adv_data, uint8_t data_len)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_BLE_PTS,
                                                           gapm_set_adv_data_cmd,
                                                           PTS_MAX_TX_OCTETS);

    // Fill the allocated kernel message
    p_cmd->operation = adv_type;
    p_cmd->actv_idx = adv_info.actv_idx;
    p_cmd->length = data_len;
    memcpy(p_cmd->data, adv_data, data_len);
    // Send the message
    ke_msg_send(p_cmd);
}

void pts_adv_enable(uint16_t duration, uint8_t max_adv_evt)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_BLE_PTS,
                                                         gapm_activity_start_cmd,
                                                         sizeof(gapm_adv_param_t));
    gapm_adv_param_t *adv_add_param = (gapm_adv_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = adv_info.actv_idx;
    adv_add_param->duration = duration;
    adv_add_param->max_adv_evt = max_adv_evt;

    // Send the message
    ke_msg_send(p_cmd);
}

void pts_adv_disenable()
{
    pts_stop_activity(adv_info.actv_idx);
}

void pts_adv_delete()
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    pts_delete_activity(adv_info.actv_idx);

    //clear adv_info
    adv_info.actv_idx = 0xFF;
}

pts_adv_para_t *pts_adv_get_para_index()
{
    return &adv_info;
}

void pts_adv_start()
{
    adv_info.para.adv_param.max_tx_pwr = btdrv_reg_op_txpwr_idx_to_rssidbm(adv_info.para.adv_param.max_tx_pwr);
    if(adv_info.adv_data.len == 0)
    {
            pts_set_default_adv_data();
    }

    if(adv_info.para.own_addr_type == GAPM_GEN_RSLV_ADDR)
    {
        pts_add_dev_to_rslv_list(adv_info.para.adv_param.peer_addr.addr_type,
                                        adv_info.para.adv_param.peer_addr.addr);
        osDelay(100);
        pts_set_dev_config(1, 10);
        osDelay(100);
        //see gapm_le_actv_get_hci_own_addr_type,The address type is controlled by privacy_cfg
        //To set ADV addr type to 2, this location must be set to GAPM_STATIC_ADDR
        adv_info.para.own_addr_type = GAPM_STATIC_ADDR;
    }

    // If you use an unresolvable address type, you need to set addr
    if(adv_info.para.own_addr_type == GAPM_GEN_NON_RSLV_ADDR)
    {
        adv_info.para.adv_param.local_addr.addr[0] = 0x10;
        adv_info.para.adv_param.local_addr.addr[1] = 0x1A;
        adv_info.para.adv_param.local_addr.addr[2] = 0x1B;
        adv_info.para.adv_param.local_addr.addr[3] = 0x1C;
        adv_info.para.adv_param.local_addr.addr[4] = 0x1D;
        adv_info.para.adv_param.local_addr.addr[5] = 0x3F;
    }

    if(adv_info.actv_idx == 0xFF)
    {
        pts_adv_creat(&adv_info.para);
        osDelay(20);
    }
    pts_adv_set_data(GAPM_SET_ADV_DATA, adv_info.adv_data.data, adv_info.adv_data.len);
    osDelay(20);
    pts_adv_enable(adv_info.duration, adv_info.max_adv_evt);
}

void pts_set_default_adv_data()
{
    uint8_t adv_flgs = 0x1A;
    uint8_t *adv_name = NULL;

    adv_name = app_ble_get_dev_name();

    pts_adv_data_add_element(&adv_info.adv_data, GAP_AD_TYPE_FLAGS, 0, &adv_flgs, sizeof(adv_flgs));
    pts_adv_data_add_element(&adv_info.adv_data, GAP_AD_TYPE_COMPLETE_NAME, 0,
                                adv_name, strlen((char *)adv_name));
    memcpy(&adv_info.per_adv_data, &adv_info.adv_data, sizeof(send_adv_data_t));
}

//ble host pts scan function
static pts_scan_para_t scan_info = {
    .actv_idx = 0xFF,
    .para = {
        .type = GAPM_SCAN_TYPE_GEN_DISC,
        .prop = GAPM_SCAN_PROP_PHY_1M_BIT,
        .dup_filt_pol = 0,
        .scan_param_1m.scan_intv = 50,
        .scan_param_1m.scan_wd =50,
        .duration = 0,
        .period = 0,
    },
};

void pts_scan_creat(uint8_t own_addr_type)
{

	struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
															  TASK_GAPM, TASK_BLE_PTS,
															  gapm_activity_create_cmd);

	// Set operation code
	p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;
	p_cmd->own_addr_type = own_addr_type;

	// Send the message
	ke_msg_send(p_cmd);
}

void pts_scan_enable(gapm_scan_param_t *scan_para, pts_scan_report_cb scan_cb)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_BLE_PTS,
                                                         gapm_activity_start_cmd,
                                                         sizeof(gapm_scan_param_t));
    gapm_scan_param_t *scan_param = (gapm_scan_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = scan_info.actv_idx;
    memcpy(scan_param, scan_para, sizeof(gapm_scan_param_t));

    scan_info.scan_report_cb = scan_cb;

    // Send the message
    ke_msg_send(p_cmd);
}

void pts_scan_disenable()
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    pts_stop_activity(scan_info.actv_idx);
}


void pts_scan_delete()
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    pts_delete_activity(scan_info.actv_idx);

    //clear scan info
    scan_info.actv_idx = 0xFF;
    scan_info.scan_report_cb = NULL;
}

pts_scan_para_t *pts_scan_get_para_index()
{
    return &scan_info;
}

void pts_scan_start(uint8_t owr_addr_type, pts_scan_report_cb repot_cb)
{
    if(scan_info.actv_idx == 0xFF)
    {
        pts_scan_creat(owr_addr_type);
        osDelay(10);
    }
    pts_scan_enable(&scan_info.para, repot_cb);
}

//connect
static pts_conn_para_t conn_info = {
    .actv_idx = 0xFF,
    .own_addr_type = GAPM_STATIC_ADDR,
    .para = {
        .type = GAPM_INIT_TYPE_DIRECT_CONN_EST,
        .prop = GAPM_INIT_PROP_1M_BIT,
        .conn_to = 0,
        .scan_param_1m.scan_intv = 0x0200,
        .scan_param_1m.scan_wd = 0x0040,
        .scan_param_coded = {0},
        .conn_param_1m.conn_intv_min = 36,
        .conn_param_1m.conn_intv_max = 36,
        .conn_param_1m.conn_latency = 0,
        .conn_param_1m.supervision_to = 0x01F4,
        .conn_param_1m.ce_len_min = 0,
        .conn_param_1m.ce_len_max = 8,
        .conn_param_2m = {0},
        .conn_param_coded = {0},
        .peer_addr.addr = {0},
        .peer_addr.addr_type = GAPM_STATIC_ADDR,
    },
};

void pts_conn_creat(uint8_t own_addr_type)
{
    struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                         TASK_GAPM, TASK_BLE_PTS,
                                                         gapm_activity_create_cmd);
    p_cmd->operation = GAPM_CREATE_INIT_ACTIVITY;
    p_cmd->own_addr_type = own_addr_type;

    ke_msg_send(p_cmd);
}

void pts_conn_enable(gapm_init_param_t *param)
{

    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_BLE_PTS,
                                                         gapm_activity_start_cmd,
                                                         sizeof(gapm_init_param_t));
    gapm_init_param_t *init_param = (gapm_init_param_t *)(p_cmd->u_param);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = conn_info.actv_idx;
    memcpy(init_param, param, sizeof(gapm_init_param_t));


    ke_msg_send(p_cmd);
}

void pts_conn_disenable()
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    pts_stop_activity(conn_info.actv_idx);
}

void pts_conn_delete()
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    pts_delete_activity(conn_info.actv_idx);

    conn_info.actv_idx = 0xFF;
}

pts_conn_para_t *pts_conn_get_para_index()
{
    return &conn_info;
}

void pts_conn_start()
{
    if(conn_info.actv_idx == 0xFF)
    {
        pts_conn_creat(conn_info.own_addr_type);
        osDelay(10);
    }

    // If no peer address is specified, use the default peer address
    if ((conn_info.para.peer_addr.addr[0] == 0 )
        && (conn_info.para.peer_addr.addr[1] == 0 )
        && (conn_info.para.peer_addr.addr[2] == 0 )
        && (conn_info.para.peer_addr.addr[3] == 0 )
        && (conn_info.para.peer_addr.addr[4] == 0 )
        && (conn_info.para.peer_addr.addr[5] == 0 ))
    {
        memcpy(conn_info.para.peer_addr.addr, dongle_addr.addr, 6);
        conn_info.para.peer_addr.addr_type = dongle_addr.addr_type;
    }

    // If the resolvable address type is used,
    // you need to set the resolvable list of the controller
    if((conn_info.own_addr_type == GAPM_GEN_RSLV_ADDR)
        || (conn_info.para.peer_addr.addr_type == GAPM_GEN_RSLV_ADDR))
    {
        pts_add_dev_to_rslv_list(1, conn_info.para.peer_addr.addr);
        osDelay(150);
        pts_set_dev_config(1, 10);
        osDelay(150);
    }

	//patch GAP/CONN/PRDA/BV-02-C
	if(conn_info.para.peer_addr.addr_type == GAPM_GEN_NON_RSLV_ADDR)
    {
        conn_info.para.peer_addr.addr_type = GAPM_GEN_RSLV_ADDR;
    }

    pts_conn_enable(&conn_info.para);
}

//connect successful, Link information
struct gapc_le_connection_req_ind connected_info = {0};
uint8_t *pts_get_peer_addr()
{
    return connected_info.peer_addr.addr;
}

void pts_get_peer_dev_info(uint8_t operation)
{
    /// Start discovering
    struct gapc_get_info_cmd *cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                              TASK_GAPC,
                                              TASK_BLE_PTS,
                                              gapc_get_info_cmd);

    cmd->conidx = connected_info.conidx;
    cmd->operation = operation;
    ke_msg_send(cmd);
}

void pts_disconnect()
{
    struct gapc_disconnect_cmd *p_cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                               TASK_GAPC,
                                               TASK_BLE_PTS,
                                               gapc_disconnect_cmd);

    p_cmd->operation = GAPC_DISCONNECT;
    p_cmd->conidx = connected_info.conidx;
    p_cmd->reason = CO_ERROR_REMOTE_USER_TERM_CON;
    memset(&connected_info, 0, sizeof(struct gapc_le_connection_req_ind));

    ke_msg_send(p_cmd);
}

void pts_update_param(uint8_t conidx, struct gapc_conn_param *conn_param)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     TASK_GAPC,
                                                     TASK_BLE_PTS,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = conn_param->intv_min;
    cmd->intv_max   = conn_param->intv_max;
    cmd->latency    = conn_param->latency;
    cmd->time_out   = conn_param->time_out;
    cmd->conidx     = conidx;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}


void pts_gapc_cli_exchange_mtu()
{

    /// Start discovering
    struct gapc_exchange_mtu_req *cmd = KE_MSG_ALLOC(GAPC_EXCHANGE_MTU_CMD,
                                              TASK_GAPC,
                                              TASK_BLE_PTS,
                                              gapc_exchange_mtu_req);

    cmd->conidx = connected_info.conidx;
    ke_msg_send(cmd);
}


//BIG info
static pts_bis_para_t big_info = {
    .adv_para = &adv_info.para,
    .big_para = {
        .big_hdl = 0,
        .adv_hdl = 0,
        .num_bis = 1,
        .sdu_interval = 0x2710,
        .max_sdu = 0x28,
        .trans_latency = 0x0a,
        .rtn = 2,
        .phy = 1,
        .packing = 0,
        .framing = 0,
        .encryption = 0,
        .broadcast_code = {0},
        },
    .isoohci_data_path = NULL,
};

static void pts_big_hci_cmd_cmp_handler(uint16_t opcode, uint16_t event, void const *p_evt)
{
    switch(event){
        case PTS_BIG_CREAT_CMP:
            {
                struct hci_basic_cmd_cmp_evt * bis_evt = (struct hci_basic_cmd_cmp_evt *)p_evt;
                if(bis_evt->status == CO_ERROR_NO_ERROR){
                    TRACE(0, "[%s][%d][ERROR]: end creat big cmd OK!", __FUNCTION__, __LINE__);
                }
                else{
                    TRACE(0, "[%s][%d][ERROR]: end creat big cmd fail!", __FUNCTION__, __LINE__);
                }
                break;
            }
        case PTS_BIG_DATA_SETUP_CMP:
            {
                TRACE(0, "[%s][%d]: BIG DATA SETUP CMP!", __FUNCTION__, __LINE__);
                break;
            }
        default:
            break;
    }
}

static void pts_big_hci_cmd_set_creat(struct hci_le_create_big_cmd *para)
{
    struct hci_le_create_big_cmd* p_big_cmd = HL_HCI_CMD_ALLOC(HCI_LE_CREATE_BIG_CMD_OPCODE, hci_le_create_big_cmd);

    if(para){
        memcpy(p_big_cmd, para, sizeof(struct hci_le_create_big_cmd));
    }else{
        TRACE(0, "[%s][%d]: set_big_creat is NULL!", __FUNCTION__, __LINE__);
        return;
    }

    // Send the HCI command
    HL_HCI_CMD_SEND_TO_CTRL(p_big_cmd, PTS_BIG_CREAT_CMP, pts_big_hci_cmd_cmp_handler);

}

static void pts_big_hci_cmd_setup_iso_data(struct hci_le_setup_iso_data_path_cmd *para)
{
    struct hci_le_setup_iso_data_path_cmd* p_cmd = HL_HCI_CMD_ALLOC(HCI_LE_SETUP_ISO_DATA_PATH_CMD_OPCODE,
                                                                            hci_le_setup_iso_data_path_cmd);

    if(para){
        memcpy(p_cmd, para, sizeof(struct hci_le_setup_iso_data_path_cmd));
    }else{
        TRACE(0, "[%s][%d]: setup_iso_data is NULL!", __FUNCTION__, __LINE__);
        return;
    }

    // Send the HCI command
    HL_HCI_CMD_SEND_TO_CTRL(p_cmd, PTS_BIG_DATA_SETUP_CMP, pts_big_hci_cmd_cmp_handler);
}

void pts_big_hci_create_cmp_evt_handler(uint8_t evt_code, struct hci_le_create_big_cmp_evt const* p_evt)
{
    if(p_evt->status == CO_ERROR_NO_ERROR){
        memcpy(&big_info.big_creat_cmp, p_evt, sizeof(struct hci_le_create_big_cmp_evt const));

        struct hci_le_setup_iso_data_path_cmd big_data;
        big_data.conhdl = p_evt->conhdl[0];
        big_data.data_path_direction = 0;
        big_data.data_path_id = 0;
        big_data.ctrl_delay[0] = 6;
        big_data.codec_cfg_len = 0;
        pts_big_hci_cmd_setup_iso_data(&big_data);
    }
    else{
        TRACE(0, "[%s][%d][ERROR]: BIG creat fail!", __FUNCTION__, __LINE__);
    }
}

pts_bis_para_t *pts_big_get_para_index()
{
    return &big_info;
}

void pts_big_start()
{
    pts_adv_start();
    osDelay(1000);

    pts_big_hci_cmd_set_creat(&big_info.big_para);

    //get iso hci data path
    big_info.isoohci_data_path = data_path_itf_get(ISO_DP_ISOOHCI, ISO_SEL_TX);
}

void pts_send_iso_start(uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu)
{
    uint16_t conhdl;
    const struct data_path_itf *app_bap_tx_dp_itf;

    conhdl = big_info.big_creat_cmp.conhdl[0];
    app_bap_tx_dp_itf = big_info.isoohci_data_path;

    if (app_bap_tx_dp_itf && (NULL != app_bap_tx_dp_itf->cb_start))
    {
        app_bap_tx_dp_itf->cb_start(conhdl, sdu_interval, trans_latency, max_sdu);
    }
}
void pts_send_iso_stop()
{
    uint16_t conhdl;
    const struct data_path_itf *app_bap_tx_dp_itf;

    conhdl = big_info.big_creat_cmp.conhdl[0];
    app_bap_tx_dp_itf = big_info.isoohci_data_path;

    if (app_bap_tx_dp_itf && (NULL != app_bap_tx_dp_itf->cb_stop))
    {
        app_bap_tx_dp_itf->cb_stop(conhdl, 0);
    }
}

void pts_send_iso_data(uint16_t seq_num, uint8_t *payload, uint16_t payload_len, uint32_t ref_time)
{
    app_bap_dp_itf_send_data_directly(big_info.big_creat_cmp.conhdl[0], seq_num, payload, payload_len, ref_time);
}

/**********************************SM PART*************************************************/
pts_sm_para_t sm_info = {
    .pairing_oob = GAP_OOB_AUTH_DATA_NOT_PRESENT,
    .pairing_iocap = GAP_IO_CAP_DISPLAY_YES_NO,
    .pairing_auth  = GAP_AUTH_REQ_SEC_CON_BOND,
    .pairing_sec_level = GAP_SEC1_SEC_CON_PAIR_ENC,
    .pairing_key_dist  = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,
    .pairing_key_size = GAP_KEY_LEN,
    .pin_code = PTS_SM_PIN_CODE,
    .pairing_nv_get_key = true,
    .bonded_flag = false,
    .loc_irk = {0},
};

void pts_sm_init(void)
{
#ifdef _BLE_NVDS_
    NV_RECORD_PAIRED_BLE_DEV_INFO_T* nvrecord_ble_p = nv_record_blerec_get_ptr();
    memcpy(sm_info.loc_irk, nvrecord_ble_p->self_info.ble_irk, KEY_LEN);

    if (nv_record_ble_record_Once_a_device_has_been_bonded())
    {
        sm_info.bonded_flag = true;
    }
    else
    {
        sm_info.bonded_flag = false;
    }

    TRACE(2,"[%s] once bonded:%d", __func__, sm_info.bonded_flag);
#endif
}

uint8_t pts_sm_get_bond_status(void)
{
    return sm_info.bonded_flag;
}

void pts_sm_reset(void)
{
#ifdef _BLE_NVDS_
    memset(&sm_info.pts_save_info, 0, sizeof(BleDevicePairingInfo));
#endif
}

void pts_sm_pair_req()
{
    struct gapc_bond_cmd *cmd = KE_MSG_ALLOC(GAPC_BOND_CMD, TASK_GAPC,
                                         TASK_GAPC, gapc_bond_cmd);

    cmd->conidx = 0;
    cmd->operation = GAPC_BOND;

    cmd->pairing.oob       = sm_info.pairing_oob;
    cmd->pairing.iocap     = sm_info.pairing_iocap;
    cmd->pairing.auth      = sm_info.pairing_auth;
    cmd->pairing.ikey_dist = sm_info.pairing_key_dist;
    cmd->pairing.rkey_dist = sm_info.pairing_key_dist;
    cmd->pairing.key_size  = sm_info.pairing_key_size;
    cmd->sec_req_level     = sm_info.pairing_sec_level;

    ke_msg_send(cmd);
}

void pts_sm_send_security_req(uint8_t conidx, enum gap_auth authority)
{
    TRACE(1, "%s", __func__);
    // Send security request
    struct gapc_security_cmd *cmd = KE_MSG_ALLOC(GAPC_SECURITY_CMD,
                                                 TASK_GAPC,
                                                 TASK_GAPC,
                                                 gapc_security_cmd);

    cmd->operation = GAPC_SECURITY_REQ;

    cmd->auth = authority;
    cmd->conidx = conidx;
    // Send the message
    ke_msg_send(cmd);
}


bool pts_sm_send_encrypt_req(uint8_t conidx, uint8_t * securityInfo)
{
    BleDevicePairingInfo * deviceSecurityInfo = (BleDevicePairingInfo*)securityInfo;
    if (!deviceSecurityInfo)
    {
        return false;
    }

    struct gapc_encrypt_cmd *encryptCmd = KE_MSG_ALLOC(GAPC_ENCRYPT_CMD,
                                                        KE_BUILD_ID(TASK_GAPC, conidx), KE_BUILD_ID(TASK_GAPC, conidx),
                                                        gapc_encrypt_cmd);
    encryptCmd->conidx = conidx;
    encryptCmd->operation = GAPC_ENCRYPT;
    memcpy(encryptCmd->ltk.key.key, deviceSecurityInfo->LTK, sizeof(deviceSecurityInfo->LTK));
    encryptCmd->ltk.ediv = deviceSecurityInfo->EDIV;
    memcpy(encryptCmd->ltk.randnb.nb, deviceSecurityInfo->RANDOM, sizeof(deviceSecurityInfo->RANDOM));
    encryptCmd->ltk.key_size = sizeof(deviceSecurityInfo->LTK);

    ke_msg_send(encryptCmd);
    return true;
}


void pts_sm_set_pairing_para(int type, int param)
{
    // Support to change pairing info according pts case
    TRACE(0, "[%s][%d]: %x,%x", __FUNCTION__, __LINE__, type, param);
    switch (type) {
        case PTS_PAIRING_AUTH:
            sm_info.pairing_auth = param;
            break;
        case PTS_PAIRING_IOCAP:
            sm_info.pairing_iocap = param;
            break;
        case PTS_PAIRING_KEY_DIST:
            sm_info.pairing_key_dist = param;
            break;
        case PTS_PAIRING_NV_GET_KEY:
            sm_info.pairing_nv_get_key = param;
            break;
        case PTS_PAIRING_SEC_REQ_LEVEL:
            sm_info.pairing_sec_level = param;
            break;
        case PTS_PAIRING_KEY_SIZE:
            sm_info.pairing_key_size = param;
            break;
        case PTS_PAIRING_OOB:
            sm_info.pairing_oob = param;
            break;
        default:
            TRACE(0, "[%s][%d]ERROR: Invalid parameter type!", __FUNCTION__, __LINE__);
            break;
    }
}

static bool pts_sec_send_encrypt_req(uint8_t conidx, uint8_t * securityInfo)
{
    BleDevicePairingInfo * deviceSecurityInfo = (BleDevicePairingInfo*)securityInfo;
    if (!deviceSecurityInfo)
    {
        return false;
    }

    struct gapc_encrypt_cmd *encryptCmd = KE_MSG_ALLOC(GAPC_ENCRYPT_CMD,
                                                        KE_BUILD_ID(TASK_GAPC, conidx), KE_BUILD_ID(TASK_GAPC, conidx),
                                                        gapc_encrypt_cmd);
    encryptCmd->conidx = conidx;
    encryptCmd->operation = GAPC_ENCRYPT;
    memcpy(encryptCmd->ltk.key.key, deviceSecurityInfo->LTK, sizeof(deviceSecurityInfo->LTK));
    encryptCmd->ltk.ediv = deviceSecurityInfo->EDIV;
    memcpy(encryptCmd->ltk.randnb.nb, deviceSecurityInfo->RANDOM, sizeof(deviceSecurityInfo->RANDOM));
    encryptCmd->ltk.key_size = sizeof(deviceSecurityInfo->LTK);

    ke_msg_send(encryptCmd);
    return true;
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
static int pts_sm_bond_req_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_bond_req_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    TRACE(1, "[pts]Get bond req %d", param->request);

    struct gapc_bond_cfm *cfm = NULL;
    uint8_t conidx = param->conidx;

    if (GAPC_NC_EXCH != param->request)
    {
        // Prepare the GAPC_BOND_CFM message
        cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                 src_id, TASK_BLE_PTS,
                                 gapc_bond_cfm);

        cfm->conidx = param->conidx;
    }

    switch (param->request)
    {
        case (GAPC_PAIRING_REQ):
        {
            cfm->request = GAPC_PAIRING_RSP;
            cfm->accept  = true;
            cfm->data.pairing_feat.pairing_info.oob        = sm_info.pairing_oob;
            cfm->data.pairing_feat.pairing_info.auth       = sm_info.pairing_auth;
            cfm->data.pairing_feat.pairing_info.iocap      = sm_info.pairing_iocap;
            cfm->data.pairing_feat.pairing_info.ikey_dist  = sm_info.pairing_key_dist;
            cfm->data.pairing_feat.pairing_info.rkey_dist  = sm_info.pairing_key_dist;
            cfm->data.pairing_feat.pairing_info.key_size   = sm_info.pairing_key_size;
            cfm->data.pairing_feat.sec_req_level           = sm_info.pairing_sec_level;
        } break;

        case (GAPC_LTK_EXCH):
        {
            // Counter
            uint8_t counter;

            cfm->accept  = true;
            cfm->request = GAPC_LTK_EXCH;

            // Generate all the values
            cfm->data.ltk.ediv = (uint16_t)co_rand_word();

            for (counter = 0; counter < RAND_NB_LEN; counter++)
            {
                cfm->data.ltk.randnb.nb[counter] = (uint8_t)co_rand_word();
            }

            for (counter = 0; counter < GAP_KEY_LEN; counter++)
            {
                cfm->data.ltk.key.key[counter]    = (uint8_t)co_rand_word();
            }

#ifdef _BLE_NVDS_
#ifdef BLE_KEY_PRINT
            TRACE(0, "<==============>LTK IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk,16);
            TRACE(1, "<==============>EDIV IS: %04x:",cfm->data.ltk.ediv);
            TRACE(0, "<==============>RANDOM IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk.randnb.nb,8);
#endif
            /*
            * For legacy pairing, the distributed LTK is generated by both device through random numbers
            * The device that initiates encryption will decide which LTK to use through EDIV
            */
            sm_info.pts_save_info.LOCAL_EDIV = cfm->data.ltk.ediv;
            memcpy(&sm_info.pts_save_info.LOCAL_RANDOM, (uint8_t *)&cfm->data.ltk.randnb.nb, 8);
            memcpy(&sm_info.pts_save_info.LOCAL_LTK, (uint8_t *)&cfm->data.ltk, 16);

            sm_info.pts_save_info.bonded = false;
#endif
        } break;

        case (GAPC_IRK_EXCH):
        {
            cfm->accept  = true;
            cfm->request = GAPC_IRK_EXCH;

            // Load IRK
            memcpy(cfm->data.irk.key.key, sm_info.loc_irk, KEY_LEN);
            // load identity ble address
            memcpy(cfm->data.irk.addr.addr, ble_global_addr, BLE_ADDR_SIZE);
            cfm->data.irk.addr.addr_type = ADDR_PUBLIC;
        } break;

        case (GAPC_TK_EXCH):
        {
            cfm->accept  = true;
            cfm->request = GAPC_TK_EXCH;

            // Set the TK value
            memset(cfm->data.tk.key, 0, KEY_LEN);

            cfm->data.tk.key[0] = (uint8_t)((sm_info.pin_code & 0x000000FF) >>  0);
            cfm->data.tk.key[1] = (uint8_t)((sm_info.pin_code & 0x0000FF00) >>  8);
            cfm->data.tk.key[2] = (uint8_t)((sm_info.pin_code & 0x00FF0000) >> 16);
            cfm->data.tk.key[3] = (uint8_t)((sm_info.pin_code & 0xFF000000) >> 24);
        } break;
        case GAPC_NC_EXCH:
        {

            // Prepare the GAPC_BOND_CFM message
            struct gapc_bond_cfm *cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                                     TASK_GAPC, TASK_BLE_PTS,
                                                     gapc_bond_cfm);

            cfm->conidx  = conidx;
            cfm->request = GAPC_NC_EXCH;
            cfm->accept  = true;

            // Send the message
            ke_msg_send(cfm);
        } break;
        case GAPC_CSRK_EXCH:
        {
            cfm->accept = true;
            cfm->request = GAPC_CSRK_EXCH;
            memset(cfm->data.csrk.key,0,KEY_LEN);
        } break;
        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    if (GAPC_NC_EXCH != param->request)
    {
        // Send the message
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

static int pts_sm_security_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_security_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    uint8_t* peerBleAddr = NULL;
    uint8_t* deviceSecurityInfo = NULL;

    peerBleAddr = pts_get_peer_addr();
    deviceSecurityInfo = nv_record_ble_record_find_device_security_info_through_static_bd_addr(peerBleAddr);

    if (sm_info.pairing_nv_get_key && deviceSecurityInfo)
    {
        TRACE(1,"Master initiates encrypt req!!! conidx %d",conidx);
        pts_sec_send_encrypt_req(conidx, deviceSecurityInfo);
    }
    else
    {
        struct gapc_bond_cmd *cmd = KE_MSG_ALLOC(GAPC_BOND_CMD, TASK_GAPC,
                                     TASK_GAPC, gapc_bond_cmd);

        TRACE(1,"Master initiates pair req!!! conidx %d",conidx);
        cmd->conidx = conidx;
        cmd->operation = GAPC_BOND;
        cmd->pairing.oob       = sm_info.pairing_oob;
        cmd->pairing.auth      = sm_info.pairing_auth;
        cmd->pairing.iocap     = sm_info.pairing_iocap;
        cmd->pairing.ikey_dist = sm_info.pairing_key_dist;
        cmd->pairing.rkey_dist = sm_info.pairing_key_dist;
        cmd->pairing.key_size  = sm_info.pairing_key_size;
        cmd->sec_req_level     = sm_info.pairing_sec_level;

        ke_msg_send(cmd);
    }

    return (KE_MSG_CONSUMED);
}

static int pts_sm_bond_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_bond_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    switch (param->info)
    {
        case (GAPC_PAIRING_SUCCEED):
        {
            TRACE(1,"GAPC_PAIRING_SUCCEED");
            // Update the bonding status in the environment
            sm_info.bonded_flag = true;
#ifdef _BLE_NVDS_
            sm_info.pts_save_info.bonded = true;
            // Sometimes the device do not distribute IRK
            uint8_t nullAddr[BLE_ADDR_SIZE] = {0};

            if (!memcmp(sm_info.pts_save_info.peer_addr.addr, nullAddr, BLE_ADDR_SIZE)) {
                uint8_t *GetPeerAddr = NULL;
                GetPeerAddr = pts_get_peer_addr();
                memcpy(sm_info.pts_save_info.peer_addr.addr, GetPeerAddr, BLE_ADDR_SIZE);
            }

            if (!nv_record_blerec_add(&sm_info.pts_save_info))
            {
#ifdef CTKD_ENABLE
                clac_linkKey(sm_info.pts_save_info.LTK, sm_info.pts_save_info.peer_addr.addr, param->data.pairing.ct2);
                app_bt_ctkd_connecting_mobile_handler();
#endif
                gap_ral_dev_info_t devicesInfo[PTS_RESOLVING_LIST_MAX_NUM];
                uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
                appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
            }
#endif
            ke_state_set(TASK_BLE_PTS,PTS_APPM_ENCRYPTED);
            break;
        }
        case (GAPC_REPEATED_ATTEMPT):
        {
            pts_disconnect();
            break;
        }
        case (GAPC_IRK_EXCH):
        {
            TRACE(0,"Peer device IRK is:");
#ifdef _BLE_NVDS_
            DUMP8("%02x ", param->data.irk.key.key, BLE_IRK_SIZE);
            DUMP8("%02x ", (uint8_t *)&(param->data.irk.addr), BT_ADDR_OUTPUT_PRINT_NUM);
            memcpy(sm_info.pts_save_info.IRK, param->data.irk.key.key, BLE_IRK_SIZE);
            memcpy(sm_info.pts_save_info.peer_addr.addr, param->data.irk.addr.addr, BLE_ADDR_SIZE);
            sm_info.pts_save_info.peer_addr.addr_type = param->data.irk.addr.addr_type;
#endif
            break;
        }
        case (GAPC_PAIRING_FAILED):
        {
            TRACE(1,"GAPC_PAIRING_FAILED!!! Error code 0x%x", param->data.reason);
#ifdef _BLE_NVDS_
            //nv_record_ble_delete_entry(app_env.context[conidx].solvedBdAddr);
            sm_info.bonded_flag = false;
#endif
            break;
        }
        case GAPC_LTK_EXCH:
        {
#ifdef _BLE_NVDS_
            //TRACE(1,"isLocal %d", param->data.ltk.isLocal);
            TRACE(0,"Peer device LTK is:");

            DUMP8("%02x ", param->data.ltk.key.key, BLE_LTK_SIZE);
            TRACE(1,"EDIV %04x ", param->data.ltk.ediv);
            TRACE(0,"Peer device random number is:");
            DUMP8("%02x ", param->data.ltk.randnb.nb, BLE_ENC_RANDOM_SIZE);

            sm_info.pts_save_info.EDIV = param->data.ltk.ediv;
            memcpy(&sm_info.pts_save_info.RANDOM, (uint8_t *)&param->data.ltk.randnb.nb, BLE_ENC_RANDOM_SIZE);
            memcpy(&sm_info.pts_save_info.LTK, (uint8_t *)&param->data.ltk, BLE_LTK_SIZE);
#endif
            break;
        }
        default:
        {
            ASSERT_ERR(0);
            break;
        }
    }

    return (KE_MSG_CONSUMED);
}

static int pts_sm_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_encrypt_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    TRACE(1,"%s, master ask for LTK TO encrypt!!!!", __FUNCTION__);


    TRACE(0,"Random ble address solved. Can rsp enc req.");

    // Prepare the GAPC_ENCRYPT_CFM message
    struct gapc_encrypt_cfm *cfm = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM,
                                                src_id, TASK_BLE_PTS,
                                                gapc_encrypt_cfm);

    cfm->found    = false;
    cfm->conidx   = param->conidx;
#ifdef _BLE_NVDS_
    gapc_ltk_t ltk;
    bool ret;
    uint8_t *slaveGetPeerAddr = NULL;
    if(connected_info.peer_addr_type)
    {
        slaveGetPeerAddr = dongle_addr.addr;
    }
    else
    {
        slaveGetPeerAddr = pts_get_peer_addr();
    }

    ret = nv_record_ble_record_find_ltk(slaveGetPeerAddr, ltk.key.key, param->ediv);
    if(sm_info.pairing_nv_get_key && ret)
    {
        TRACE(0,"FIND LTK SUCCESSED!!!");
        DUMP8("%02x ", (uint8_t *)ltk.key.key, 16);

        cfm->found    = true;
        cfm->key_size = sm_info.pairing_key_size;
        memcpy(&cfm->ltk, ltk.key.key, sizeof(struct gap_sec_key));

    }
    else
    {
        TRACE(0,"FIND LTK failed!!!");
    }
#endif
    TRACE(2,"%s app_sec_env.bonded %d", __FUNCTION__, sm_info.bonded_flag);

    // Send the message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int pts_sm_encrypt_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_encrypt_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t encryptResult = param->encrypt_error_code;
    if (!encryptResult)   //GAP_ERR_NO_ERROR = 0x00
    {
        TRACE(2,"%s conidx 0x%02x pair_lvl %d", __func__, conidx, param->pairing_lvl);
        ke_state_set(TASK_BLE_PTS,PTS_APPM_ENCRYPTED);
    }
    else
    {
        struct gapc_bond_cmd *cmd = KE_MSG_ALLOC(GAPC_BOND_CMD, TASK_GAPC,
                                     TASK_GAPC, gapc_bond_cmd);

        TRACE(1,"%s: conidx %d", __FUNCTION__, conidx);
        cmd->conidx = conidx;
        cmd->operation = GAPC_BOND;
        cmd->pairing.oob       = sm_info.pairing_oob;
        cmd->pairing.auth      = sm_info.pairing_auth;
        cmd->pairing.iocap     = sm_info.pairing_iocap;
        cmd->pairing.ikey_dist = sm_info.pairing_key_dist;
        cmd->pairing.rkey_dist = sm_info.pairing_key_dist;
        cmd->pairing.key_size  = sm_info.pairing_key_size;
        cmd->sec_req_level     = sm_info.pairing_sec_level;

        ke_msg_send(cmd);

    }
    return (KE_MSG_CONSUMED);
}

static int pts_sm_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message
    return (KE_MSG_CONSUMED);
}


const struct ke_msg_handler pts_sm_msg_handler_list[] =
{
    {KE_MSG_DEFAULT_HANDLER,  (ke_msg_func_t)pts_sm_msg_dflt_handler},

    {GAPC_BOND_REQ_IND,       (ke_msg_func_t)pts_sm_bond_req_ind_handler},
    {GAPC_BOND_IND,           (ke_msg_func_t)pts_sm_bond_ind_handler},
    {GAPC_SECURITY_IND,       (ke_msg_func_t)pts_sm_security_ind_handler},
    {GAPC_ENCRYPT_REQ_IND,    (ke_msg_func_t)pts_sm_encrypt_req_ind_handler},
    {GAPC_ENCRYPT_IND,        (ke_msg_func_t)pts_sm_encrypt_ind_handler},
};

const struct pts_subtask_handlers pts_sec_handlers = {&pts_sm_msg_handler_list[0], ARRAY_LEN(pts_sm_msg_handler_list)};



/**********************************GATT PART*************************************************/
gatt_para_t gatt_para = {
    .conidx = 0,
    .dummy = 0,
    .pts_server = {
        .pts_user_lid = 0xFF,
        .server_att_nb = 0,
    },
    .pts_client = {
        .pts_user_lid = 0xFF,
    },
};

void pts_gatt_reg(uint8_t role)
{
    gatt_user_register_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_user_register_cmd);

    cmd->cmd_code = GATT_USER_REGISTER;
    cmd->dummy = gatt_para.dummy;
    cmd->pref_mtu = 40;
    cmd->prio_level = 0;
    cmd->role = role;
    gatt_para.role = role;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_unreg(uint8_t role)
{
    /// role unreg
    gatt_user_unregister_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_user_unregister_cmd);
    cmd->cmd_code = GATT_USER_UNREGISTER;
    cmd->dummy = gatt_para.dummy;
    if(role == PTS_GATT_ROLE_CLIENT){
        cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    }else if(role == PTS_GATT_ROLE_SERVER){
        cmd->user_lid = gatt_para.pts_server.pts_user_lid;
    }
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

/// info:@verbatim, see @gatt_svc_info_bf
///   7      6     5     4      3     2    1   0
/// +-----+-----+-----+------+-----+-----+---+---+
/// | RFU | UUID_TYPE | HIDE | DIS | EKS |  AUTH |
/// +-----+-----+-----+------+-----+-----+---+---+
/// @endverbatim
/// GATT Service information Bit Field

void pts_gatt_svc_add(uint8_t info , uint8_t* p_uuid , uint8_t nb_att_rsvd, uint8_t nb_att , gatt_att_desc_t* p_atts)
{
    /// server att add
    gatt_db_svc_add_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_db_svc_add_cmd,
                                                 sizeof(gatt_att_desc_t)*nb_att);

    cmd->cmd_code = GATT_DB_SVC_ADD;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_server.pts_user_lid;
    cmd->info = info;
    cmd->start_hdl = 0;
    cmd->nb_att_rsvd = nb_att_rsvd;
    cmd->nb_att = nb_att;
    if(gatt_para.pts_server.server_att_para[0].start_hdl == 0){
        gatt_para.pts_server.server_att_nb = 1;
        gatt_para.pts_server.server_att_para[0].number_hdl = nb_att_rsvd;
    }else{
        gatt_para.pts_server.server_att_nb = 2;
        gatt_para.pts_server.server_att_para[1].number_hdl = nb_att_rsvd;
    }

    switch(GETF(info, GATT_SVC_UUID_TYPE))
    {
        case GATT_UUID_16:   {memcpy(cmd->uuid, p_uuid, 2);}   break;
        case GATT_UUID_32:   {memcpy(cmd->uuid, p_uuid, 4);}   break;
        case GATT_UUID_128:  {memcpy(cmd->uuid, p_uuid, 8);}   break;
    }
    memcpy(cmd->atts,p_atts,sizeof(gatt_att_desc_t) * nb_att);

    /// send msg to GATT layer
    ke_msg_send(cmd);
    TRACE(0, "[bincao][%s][%d] user_lid=%d", __FUNCTION__, __LINE__,cmd->user_lid);
}

void pts_gatt_svc_remove(uint8_t server_att_nb)
{
    gatt_db_svc_remove_cmd_t *cmd =    KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_db_svc_remove_cmd);

    cmd->cmd_code = GATT_DB_SVC_REMOVE;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_server.pts_user_lid;
    cmd->start_hdl = gatt_para.pts_server.server_att_para[server_att_nb].start_hdl;

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_svc_ctrl(uint8_t server_att_nb, uint8_t enable, uint8_t visible)
{
    gatt_db_svc_ctrl_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_db_svc_ctrl_cmd,
                                                 0);

    cmd->cmd_code = GATT_DB_SVC_CTRL;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid =  gatt_para.pts_server.pts_user_lid;
    cmd->start_hdl = gatt_para.pts_server.server_att_para[server_att_nb].start_hdl;
    cmd->enable = enable;
    cmd->visible = visible;
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_mtu_updata()
{
    /// client mtu update
    gatt_cli_mtu_update_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                  TASK_GATT,
                                                  TASK_BLE_PTS,
                                                  gatt_cli_mtu_update_cmd);

    cmd->cmd_code = GATT_CLI_MTU_UPDATE;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_read(uint16_t hdl, uint16_t offset, uint16_t length)
{
    gatt_cli_read_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_read_cmd);

    cmd->cmd_code = GATT_CLI_READ;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->hdl = hdl;
    cmd->offset = offset;
    cmd->length = length;

    /// send msg to GATT layer
    TRACE(0, "[%s][%d]: %d,%d,%d", __FUNCTION__, __LINE__, hdl, offset, length);
    ke_msg_send(cmd);
}

void pts_gatt_cli_read_by_uuid(uint16_t start_hdl, uint16_t end_hdl,
                                       uint8_t uuid_type, uint8_t *uuid)
{
    gatt_cli_read_by_uuid_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_read_by_uuid_cmd);

    cmd->cmd_code = GATT_CLI_READ_BY_UUID;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl = end_hdl;
    cmd->uuid_type = uuid_type;

    if(uuid_type == GATT_UUID_16){
        memcpy(cmd->uuid, uuid, 2);
        DUMP8("%02x ", cmd->uuid, 2);
    }
    else if(uuid_type == GATT_UUID_32){
        memcpy(cmd->uuid, uuid, 4);
        DUMP8("%02x ", cmd->uuid, 4);
    }
    else if(uuid_type == GATT_UUID_128){
        memcpy(cmd->uuid, uuid, 16);
        DUMP8("%02x ", cmd->uuid, 16);
    }
    else{
        TRACE(0, "[%s][%d]: non uuid!", __FUNCTION__, __LINE__);
    }

    /// send msg to GATT layer
    TRACE(0, "[%s][%d]: %x,%x,%x,%x", __FUNCTION__, __LINE__, start_hdl, end_hdl, uuid_type, uuid[0]);

    ke_msg_send(cmd);
}

void pts_gatt_cli_read_multiple(uint8_t        nb_att, gatt_att_t *att_info)
{
    gatt_cli_read_multiple_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_read_multiple_cmd,
                                                 sizeof(gatt_att_t)*nb_att);

    cmd->cmd_code = GATT_CLI_READ_MULTIPLE;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->nb_att = nb_att;

    if(cmd->nb_att > 0){
        memcpy(cmd->atts, att_info, sizeof(gatt_att_t)*nb_att);
    }

    /// send msg to GATT layer
    TRACE(0, "[%s][%d]: %d", __FUNCTION__, __LINE__, nb_att);
    DUMP8("%02x ", cmd->atts, sizeof(gatt_att_t)*nb_att);
    ke_msg_send(cmd);
}

void pts_gatt_cli_write(uint8_t      write_type, int hdl, uint16_t offset,
                                             int data_size, uint8_t *data)
{
    TRACE(0, "[%s][%d]: %d,%x,%d,%d,%p", __FUNCTION__, __LINE__, write_type, hdl, offset, data_size, data);
    gatt_cli_write_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_write_cmd,
                                                 data_size);

    cmd->cmd_code = GATT_CLI_WRITE;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->write_type = write_type;
    cmd->hdl = hdl;
    cmd->offset = offset;
    cmd->value_length = data_size;

    if(data != NULL){
        memcpy(cmd->value, data, data_size);
    }
    else{
        cmd->value[0] =0x01;
    }

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_write_reliable(uint8_t      write_type, int hdl, uint16_t offset,
                                             int data_size, uint8_t *data)
{
    gatt_cli_write_reliable_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_write_reliable_cmd);

    cmd->cmd_code = GATT_CLI_WRITE_RELIABLE;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->write_type = write_type;
    cmd->write_mode = GATT_WRITE_MODE_AUTO_EXECUTE;
    cmd->hdl = hdl;
    cmd->offset = offset;
    cmd->length = data_size;
    gatt_para.pts_client.write_reliable_data_len = data_size;
    TRACE(0, "[%s][%d]: ", __FUNCTION__, __LINE__);

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_write_reliable_cb(gatt_cli_att_val_get_req_ind_t *param)
{
    gatt_cli_att_val_get_cfm_t *cmd = KE_MSG_ALLOC(GATT_CFM,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_att_val_get_cfm);

    cmd->req_ind_code = GATT_CLI_ATT_VAL_GET;
    cmd->token = param->token;
    cmd->user_lid = param->user_lid;
    cmd->conidx = param->conidx;
    cmd->status = (param->max_length<gatt_para.pts_client.write_reliable_data_len);
    cmd->value_length = gatt_para.pts_client.write_reliable_data_len;
    cmd->value[0] =0x01;

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_discover_svc(uint8_t       disc_type, bool full, uint16_t start_hdl,
                                 uint16_t end_hdl, uint8_t uuid_type,  uint8_t *uuid)
{
    /// Start discovering
    gatt_cli_discover_svc_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_discover_svc_cmd);

    cmd->cmd_code = GATT_CLI_DISCOVER_SVC;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->disc_type = disc_type;
    cmd->full = full;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl = end_hdl;
    cmd->uuid_type= uuid_type;

    if(uuid_type == GATT_UUID_16){
        memcpy(cmd->uuid, uuid, 2);
    }
    else if(uuid_type == GATT_UUID_32){
        memcpy(cmd->uuid, uuid, 4);
    }
    else if(uuid_type == GATT_UUID_128){
        memcpy(cmd->uuid, uuid, 16);
    }
    else{
        TRACE(0, "[%s][%d]: non uuid!", __FUNCTION__, __LINE__);
    }

    TRACE(0, "[%s][%d]: %x, %x, %x, %x, %x, %x!", __FUNCTION__, __LINE__, disc_type,
               full, start_hdl, end_hdl, uuid_type, uuid[0]);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_discover_char(uint8_t       disc_type, uint16_t start_hdl,
                                 uint16_t end_hdl, uint8_t uuid_type,  uint8_t *uuid)
{
    /// Start discovering
    gatt_cli_discover_char_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_discover_char_cmd);

    cmd->cmd_code = GATT_CLI_DISCOVER_CHAR;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->disc_type = disc_type;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl = end_hdl;
    cmd->uuid_type= uuid_type;

    if(uuid_type == GATT_UUID_16){
        memcpy(cmd->uuid, uuid, 2);
    }
    else if(uuid_type == GATT_UUID_32){
        memcpy(cmd->uuid, uuid, 4);
    }
    else if(uuid_type == GATT_UUID_128){
        memcpy(cmd->uuid, uuid, 16);
    }
    else{
        TRACE(0, "[%s][%d]: non uuid!", __FUNCTION__, __LINE__);
    }

    TRACE(0, "[%s][%d]: %x, %x, %x, %x, %x!", __FUNCTION__, __LINE__, disc_type,
               start_hdl, end_hdl, uuid_type, uuid[0]);
    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_cli_discover_desc(uint16_t start_hdl,uint16_t end_hdl)
{
    /// Start discovering
    gatt_cli_discover_desc_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_cli_discover_desc_cmd);

    cmd->cmd_code = GATT_CLI_DISCOVER_DESC;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_client.pts_user_lid;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl = end_hdl;

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_srv_event_send(uint8_t start_hdl,uint8_t evt_type,int value_length,uint8_t *value)
{
    gatt_srv_event_send_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                  TASK_GATT,
                                                  TASK_BLE_PTS,
                                                  gatt_srv_event_send_cmd,
                                                  value_length);

    cmd->cmd_code = GATT_SRV_EVENT_SEND;
    cmd->conidx = gatt_para.conidx;
    cmd->dummy = gatt_para.dummy;
    cmd->user_lid = gatt_para.pts_server.pts_user_lid;
    cmd->evt_type = evt_type;
    cmd->hdl = start_hdl;
    cmd->value_length = value_length;
    if((cmd->value_length !=0)&&(value!=NULL)){
        memcpy(cmd->value, value, cmd->value_length);}
    /// send msg to GATT layer
    ke_msg_send(cmd);
    TRACE(0, "[%s][%d]: evt_type=%d,hdl=%x,length=%d", __FUNCTION__, __LINE__,evt_type,start_hdl,value_length);
}

void pts_gatt_cli_event_register(gatt_cli_svc_ind_t *param)
{
    gatt_cli_event_register_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                  TASK_GATT,
                                                  TASK_BLE_PTS,
                                                  gatt_cli_event_register_cmd);
    cmd->cmd_code = GATT_CLI_EVENT_REGISTER;
    cmd->conidx= param->conidx;
    cmd->dummy = param->dummy;
    cmd->user_lid = param->user_lid;
    cmd->start_hdl = param->hdl;
    cmd->end_hdl = param->hdl+param->nb_att-1;

    /// send msg to GATT layer
    ke_msg_send(cmd);
}

void pts_gatt_set_ser_att_datasize(uint8_t server_att_nb , uint8_t data_size){
    gatt_para.pts_server.server_att_para[server_att_nb].ser_att_value_len = data_size;
}

uint8_t pts_gatt_srv_get_nb_from_hdl(uint8_t            hdl){
    uint8_t server_att_nb = 0;

    if(gatt_para.pts_server.server_att_nb == 1){
        server_att_nb = 0;
    }else if(gatt_para.pts_server.server_att_nb > 1){
        for(int i=0;i<gatt_para.pts_server.server_att_nb ;i++){
            if((hdl >= gatt_para.pts_server.server_att_para[i].start_hdl)&&
                (hdl < gatt_para.pts_server.server_att_para[i].number_hdl+gatt_para.pts_server.server_att_para[i].start_hdl)){
                server_att_nb = i;
                break;
            }
            server_att_nb = 0xFF;
        }
    }else{
        TRACE(0, "[%s]: no server att!", __FUNCTION__);
    }
    return server_att_nb;
}

void pts_gatt_srv_att_read_get(gatt_srv_att_read_get_req_ind_t *param)
{
    gatt_srv_att_read_get_cfm_t *cfm = KE_MSG_ALLOC_DYN(GATT_CFM,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_srv_att_read_get_cfm,
                                                 param->max_length);

    cfm->req_ind_code = GATT_SRV_ATT_READ_GET;
    cfm->conidx = gatt_para.conidx;
    cfm->token = param->token;
    cfm->user_lid = param->user_lid;
    cfm->status = 0;
    cfm->att_length = ATT_VALUE_MAX_LENGTH;

    uint8_t server_att_nb = pts_gatt_srv_get_nb_from_hdl(param->hdl);
    if(server_att_nb == 0xFF){
        TRACE(0, "[%s]: start_hdl not found!,hdl=%x", __FUNCTION__, param->hdl);
        return;
    }
    gatt_att_para_t *server_att =(gatt_att_para_t *)&gatt_para.pts_server.server_att_para[server_att_nb];
    if((param->max_length+param->offset>=server_att->ser_att_value_len)&&(server_att->ser_att_value_len>param->offset)){
        cfm->value_length = server_att->ser_att_value_len - param->offset;
        if(cfm->value_length>0){
            memcpy(cfm->value, &server_att->ser_att_value[param->offset], cfm->value_length);}
    }else if(param->max_length+param->offset<server_att->ser_att_value_len){
        cfm->value_length = param->max_length;
        memcpy(cfm->value, &server_att->ser_att_value[param->offset], cfm->value_length);
    }else if(param->offset > server_att->ser_att_value_len){
        cfm->status = ATT_ERR_INVALID_OFFSET;
    }
    /// send msg to GATT layer
    ke_msg_send(cfm);
}

void pts_gatt_srv_att_info_get(gatt_srv_att_info_get_req_ind_t *param)
{
    gatt_srv_att_info_get_cfm_t *cfm = KE_MSG_ALLOC(GATT_CFM,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_srv_att_info_get_cfm);

    cfm->req_ind_code = GATT_SRV_ATT_INFO_GET;
    cfm->conidx = param->conidx;
    cfm->token = param->token;
    cfm->user_lid = param->user_lid;
    cfm->status = 0;
    cfm->att_length = ATT_VALUE_MAX_LENGTH;
    ke_msg_send(cfm);
}

void pts_gatt_srv_att_val_set(gatt_srv_att_val_set_req_ind_t *param)
{
    gatt_srv_att_val_set_cfm_t *cfm = KE_MSG_ALLOC(GATT_CFM,
                                                 TASK_GATT,
                                                 TASK_BLE_PTS,
                                                 gatt_srv_att_val_set_cfm);

    cfm->req_ind_code = GATT_SRV_ATT_VAL_SET;
    cfm->conidx = param->conidx;
    cfm->token = param->token;
    cfm->user_lid = param->user_lid;

    uint8_t server_att_nb = pts_gatt_srv_get_nb_from_hdl(param->hdl);
    if(server_att_nb == 0xFF){
        TRACE(0, "[%s]: start_hdl not found!,hdl=%x", __FUNCTION__, param->hdl);
        return;
    }
    gatt_att_para_t *server_att =(gatt_att_para_t *)&gatt_para.pts_server.server_att_para[server_att_nb];

    if(param->value_length+param->offset<=ATT_VALUE_MAX_LENGTH){
        cfm->status = 0;
        server_att->ser_att_value_len = param->value_length+param->offset;
        if(param->value_length>0){
            memcpy(&server_att->ser_att_value[param->offset],param->value,param->value_length);
        }
    }else{ cfm->status = 1;}
    /// send msg to GATT layer
    ke_msg_send(cfm);
}

/**********************************L2CAP PART*************************************************/
void pts_l2cap_add_psm(uint16_t psm)
{
    l2cap_coc_spsm_add_cmd_t *cmd = KE_MSG_ALLOC(L2CAP_CMD,
                                             TASK_L2CAP,
                                             TASK_BLE_PTS,
                                             l2cap_coc_spsm_add_cmd);

    cmd->cmd_code = L2CAP_COC_SPSM_ADD;
    cmd->dummy = 0;
    cmd->spsm = psm;
    cmd->sec_lvl_bf = 0;

    ke_msg_send(cmd);
}

void pts_l2cap_channel_creat(uint16_t psm)
{
    l2cap_coc_create_cmd_t *cmd = KE_MSG_ALLOC(L2CAP_CMD,
                                             TASK_L2CAP,
                                             TASK_BLE_PTS,
                                             l2cap_coc_create_cmd);

    cmd->cmd_code = L2CAP_COC_CREATE;
    cmd->dummy = 0;
    cmd->conidx = connected_info.conidx;
    cmd->nb_chan = 1;
    cmd->spsm = psm;
    cmd->local_rx_mtu = 512;

    ke_msg_send(cmd);
}

/********************************ENEVT HANDLE**********************************************************/

static uint8_t pts_get_handler(const struct pts_subtask_handlers *handler_list_desc,
                               ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list_desc->msg_cnt; 0 < counter; counter--)
    {
        struct ke_msg_handler handler
                = (struct ke_msg_handler)(*(handler_list_desc->p_msg_handler_tab + counter - 1));

        if ((handler.id == msgid) ||
            (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_BLE_PTS, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}


static int *pts_gapm_cmp_evt_handler(ke_msg_id_t const msgid,
				                            struct gapm_cmp_evt *param,
				                            ke_task_id_t const dest_id,
				                            ke_task_id_t const src_id)
{
    if(param->status != GAP_ERR_NO_ERROR){
        TRACE(0, "[%s][%d]: [error]status=%x, oper=%x", __FUNCTION__, __LINE__,
                                        param->status, param->operation);
        return KE_MSG_CONSUMED;
    }
   TRACE(0, "[%s][%d]: status=%x, oper=%x, actv_idx=%x", __FUNCTION__, __LINE__,
                                    param->status, param->operation, param->actv_idx);
	switch(param->operation){
        case GAPM_CREATE_ADV_ACTIVITY:
            adv_info.actv_idx = param->actv_idx;
            break;
        case GAPM_CREATE_SCAN_ACTIVITY:
            scan_info.actv_idx = param->actv_idx;
            break;
        case GAPM_CREATE_INIT_ACTIVITY:
            conn_info.actv_idx = param->actv_idx;
            break;
        case GAPM_START_ACTIVITY:
            break;
        case (GAPM_SET_ADV_DATA):
            if(GAPM_ADV_TYPE_PERIODIC == adv_info.para.type){
                pts_adv_set_data(GAPM_SET_PERIOD_ADV_DATA,adv_info.per_adv_data.data, adv_info.per_adv_data.len);
                break;
            }
            // FALLTHROUGH
        case (GAPM_SET_PERIOD_ADV_DATA):
            break;
        default:
            break;
	}

    return KE_MSG_CONSUMED;
}

static int pts_gapm_dev_addr_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_dev_bdaddr_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Indicate that a new random BD address set in lower layers
    TRACE(0, "New dev addr:");
    DUMP8("%02x ", param->addr.addr, GAP_BD_ADDR_LEN);

    return KE_MSG_CONSUMED;
}

//Note：param->addr.addr handle
static int pts_gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_addr_solved_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    /// Indicate that resolvable random address has been solved
#ifdef _BLE_NVDS_
    bool isSuccessful = nv_record_blerec_get_bd_addr_from_irk(param->addr.addr, param->irk.key);
    if (isSuccessful)
    {
        TRACE(0, "[CONNECT]Connected random address's original addr is:");
        DUMP8("%02x ", param->addr.addr, GAP_BD_ADDR_LEN);
    }
    else
#endif
    {
        TRACE(0, "[CONNECT]Resolving of the connected BLE random addr failed.");
    }

    ke_task_msg_retrieve(TASK_GAPC);
    ke_task_msg_retrieve(TASK_BLE_PTS);

    pts_adv_start();

    return KE_MSG_CONSUMED;
}


static int pts_gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_created_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    TRACE(0, "actv index %d, type=%d, tx_pwr=%d", p_param->actv_idx,
                            p_param->actv_type, p_param->tx_pwr);

    return (KE_MSG_CONSUMED);
}

static int pts_gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_stopped_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    TRACE(0, "actv index %d, type=%d", p_param->actv_idx,
                        p_param->actv_type);

    return (KE_MSG_CONSUMED);
}


static int *pts_gapm_adv_report_evt_handler(ke_msg_id_t const msgid,
                                                      struct gapm_ext_adv_report_ind *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id)
{
    if(scan_info.scan_report_cb){
        scan_info.scan_report_cb(param);
    }else{
        TRACE(0, "[%s][%d]ERROE: scan report callback is NULL!", __FUNCTION__, __LINE__);
    }

    return KE_MSG_CONSUMED;
}

static int pts_appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol = KE_MSG_CONSUMED;

    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            #if (BLE_APP_SEC)
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_BOND_DATA_UPDATE_IND))
            {
                // Call the Security Module
                msg_pol = pts_get_handler(&pts_sec_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_SEC)
            // else drop the message
        } break;

        case (TASK_ID_GATT):
        {
            // Service Changed - Drop
        } break;

        case (TASK_ID_DATAPATHPS):
        {
            // Call the Data Path Module
            msg_pol = pts_get_handler((const struct pts_subtask_handlers *)&app_datapath_server_table_handler
                                                                                       , msgid, param, src_id);
        } break;


        default:
        {
        } break;
    }

    return (msg_pol);
}


static int pts_gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;

    TRACE(0, "%s conidx 0x%02x op 0x%02x", __func__, conidx, param->operation);
    switch(param->operation)
    {
        case (GAPC_UPDATE_PARAMS):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                TRACE(0, "%s conidx=0x%02x op=0x%02x status=0x%02x", __func__, conidx,
                                                    param->operation, param->status);
            }
        } break;

        default:
        {
        } break;
    }

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_le_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_le_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    // Check if the received Connection Handle was valid
    ASSERT(conidx != GAP_INVALID_CONIDX, "%s invalid conidx 0x%x", __func__, conidx);

    TRACE(0, "[CONNECT]device info:");
    TRACE(0, "peer addr:");
    DUMP8("%02x ", param->peer_addr.addr, GAP_BD_ADDR_LEN);
    TRACE(0, "peer addr type:%d", param->peer_addr_type);
    TRACE(0, "connection index:%d, ", conidx);
    TRACE(0, "conn interval:%d, timeout:%d", param->con_interval, param->sup_to);

    pts_sm_reset();

    memcpy(&connected_info, param, sizeof(struct gapc_le_connection_req_ind const));

    // Send connection confirmation
    struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
            TASK_GAPC, TASK_BLE_PTS,
            gapc_connection_cfm);

    cfm->conidx = conidx;
    cfm->bond_data.pairing_lvl = pts_sm_get_bond_status() ? sm_info.pairing_auth : GAP_AUTH_REQ_NO_MITM_NO_BOND;
    SETB(cfm->bond_data.cli_feat, GAPC_CLI_ROBUST_CACHE_EN, 1);
    SETB(cfm->bond_data.cli_info, GAPC_CLI_SVC_CHANGED_IND_EN, 1);
    // Send the message
    ke_msg_send(cfm);

    // We are now in connected State
    ke_state_set(dest_id, PTS_APPM_CONNECTED);

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    TRACE(0, "[DISCONNECT] device info:");
    uint8_t conidx = param->conidx;
    TRACE(0, "connection index:%d, reason:0x%x", conidx, param->reason);

    if (CO_ERROR_TERMINATED_MIC_FAILURE == param->reason)
    {
        TRACE(1,"Delete security info!!! device:");
        DUMP8("%02x ",connected_info.peer_addr.addr,6);
        nv_record_ble_delete_entry(connected_info.peer_addr.addr);
    }

    // Go to the ready state
    ke_state_set(TASK_BLE_PTS, PTS_APPM_READY);
    memset(&connected_info, 0, sizeof(struct gapc_le_connection_req_ind));
    #if (BLE_DATAPATH_SERVER)
    app_datapath_server_disconnected_evt_handler(conidx);
    #endif

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_mtu_changed_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    TRACE(0, "MTU has been negotiated as %d conidx %d", param->mtu, conidx);

#if (BLE_APP_DATAPATH_SERVER)
    app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_peer_att_info_ind_handler(ke_msg_id_t const msgid,
                                                            struct gapc_peer_att_info_ind* param,
                                                            ke_task_id_t const dest_id,
                                                            ke_task_id_t const src_id)
{
    TRACE(0, "%s req = %d", __func__, param->req - GAPC_DEV_NAME);

    switch (param->req)
    {
        case GAPC_DEV_DB_HASH:
        {

        }
        break;
        case GAPC_DEV_NAME:
        case GAPC_DEV_APPEARANCE:
        default:
        ;
    }

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_peer_features_ind_handler(ke_msg_id_t const msgid,
                                          struct gapc_peer_features_ind* param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    TRACE(0, "Peer dev feature is:");
    DUMP8("0x%02x ", param->features, GAP_LE_FEATS_LEN);


    return (KE_MSG_CONSUMED);
}

static int pts_gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_get_dev_info_req_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    TRACE(0, "%s req %d token 0x%x", __FUNCTION__, param->req, param->token);
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                                 src_id, dest_id,
                                                                 gapc_get_dev_info_cfm,
                                                                 PTS_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->status = GAP_ERR_NO_ERROR;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            int8_t len = appm_get_dev_name(cfm->info.name.value, param->name_offset);
            if (len >= 0) {
                cfm->status = GAP_ERR_NO_ERROR;
                cfm->info.name.value_length = len;
            } else {
                cfm->info.name.value_length = 0;
                cfm->status = ATT_ERR_INVALID_OFFSET;
            }
            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            // Set the device appearance
            #if (BLE_APP_HT)
            // Generic Thermometer - TODO: Use a flag
            cfm->info.appearance = 728;
            #elif (BLE_APP_HID)
            // HID Mouse
            cfm->info.appearance = 962;
            #else
            // No appearance
            cfm->info.appearance = 0;
            #endif

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    src_id, dest_id,
                                                            gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            cfm->conidx = param->conidx;
            // Slave preferred Connection interval Min
            cfm->info.slv_pref_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_pref_params.con_intv_max = 10;
            // Slave preferred Connection latency
            cfm->info.slv_pref_params.latency  = 0;
            // Slave preferred Link supervision timeout
            cfm->info.slv_pref_params.conn_timeout    = 200;  // 2s (500*10ms)

            // Send message
            ke_msg_send(cfm);
        } break;

        default: /* Do Nothing */ break;
    }


    return (KE_MSG_CONSUMED);
}

static int pts_gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Set Device configuration
    struct gapc_set_dev_info_cfm* cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                     gapc_set_dev_info_cfm);
    if(param->req == GAPC_DEV_NAME){
        struct gapm_set_name_cmd* cmd = KE_MSG_ALLOC_DYN(GAPM_SET_NAME_CMD, TASK_GAPM, TASK_BLE_PTS,
                                               gapm_set_name_cmd, param->info.name.value_length);

        cmd->operation = GAPM_SET_NAME;
        cmd->name_len = param->info.name.value_length;
        memcpy(cmd->name, param->info.name.value, cmd->name_len);
        ke_msg_send(cmd);

        // change device name
        cfm->status = GAP_ERR_NO_ERROR;
        cfm->req = param->req;
        cfm->conidx = param->conidx;
        cfm->token = param->token;
    }else{
        // Reject to change parameters
        cfm->status = GAP_ERR_REJECTED;
        cfm->req = param->req;
        cfm->conidx = param->conidx;
        cfm->token = param->token;
    }

    // Send message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_conn_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_param_update_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    bool accept = true;

    TRACE(0, "Receive the conn param update request: min %d max %d latency %d timeout %d",
          param->intv_min,
          param->intv_max,
          param->latency,
          param->time_out);

    struct gapc_param_update_cfm* cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM, src_id, dest_id,
                                            gapc_param_update_cfm);

    TRACE(0, "%s ret %d ", __func__, accept);

    cfm->accept = true;
    cfm->conidx = param->conidx;

    if (!accept)
    {
        // when a2dp or sco streaming is on-going
        //make sure ble cnt interval isnot less than 15ms, in order to prevent bt collision
        if (param->intv_min <= (uint16_t)(15/1.25))
        {
            cfm->accept = false;
        }
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int pts_gapc_conn_param_updated_handler(ke_msg_id_t const msgid,
                                          struct gapc_param_updated_ind* param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    TRACE(0, "Conidx %d BLE conn parameter is updated as interval %d timeout %d",
        conidx, param->con_interval, param->sup_to);

    return (KE_MSG_CONSUMED);
}


static int *pts_gatt_user_cmp_evt_handle(ke_msg_id_t const msgid,
                                gatt_cmp_evt_t *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    switch(param->cmd_code){
        case GATT_USER_REGISTER:
            {
                if(param->status == GAP_ERR_NO_ERROR){
                    if(gatt_para.role == PTS_GATT_ROLE_CLIENT){
                        gatt_para.pts_client.pts_user_lid = param->user_lid;
                    }else if(gatt_para.role == PTS_GATT_ROLE_SERVER){
                        gatt_para.pts_server.pts_user_lid = param->user_lid;
                    }
                    TRACE(0, "[%s][%d]: user_lid = %d", __FUNCTION__, __LINE__, param->user_lid);
                }
                break;
            }
        case GATT_CLI_DISCOVER_SVC:
            {
                if(param->status == GAP_ERR_NO_ERROR){
                    TRACE(0, "[%s][%d]: user_lid = %d", __FUNCTION__, __LINE__, param->user_lid);
                }
                break;
            }
        case GATT_DB_SVC_ADD:
            {
                if(param->status == GAP_ERR_NO_ERROR){
                    uint8_t server_att_nb = gatt_para.pts_server.server_att_nb;
                    gatt_para.pts_server.server_att_para[server_att_nb-1].start_hdl = ((gatt_db_svc_add_cmp_evt_t *)param)->start_hdl;
                    TRACE(0, "[%s][%d]: user_lid = %x,start_hdl=%x", __FUNCTION__, __LINE__, param->user_lid, ((gatt_db_svc_add_cmp_evt_t *)param)->start_hdl);
                   }else{
                        TRACE(0, "[%s][%d]: user_lid = %x,status=%x", __FUNCTION__, __LINE__, param->user_lid, param->status);
                   }
                break;
            }
        case GATT_DB_SVC_REMOVE:
            if(param->status == GAP_ERR_NO_ERROR){
                    uint8_t server_att_nb = gatt_para.pts_server.server_att_nb;
                    gatt_para.pts_server.server_att_para[server_att_nb-1].start_hdl = 0;}
            break;
        case GATT_DB_SVC_CTRL:
            if(param->status == GAP_ERR_NO_ERROR){
                TRACE(0, "[%s][%d]: user_lid = %d", __FUNCTION__, __LINE__, param->user_lid);}
            break;
        default:
            break;
    }

    return (KE_MSG_CONSUMED);
}

static int *pts_gatt_user_ind_evt_handler(ke_msg_id_t const msgid,
                                gatt_cli_inc_svc_ind_t *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    switch(param->ind_code){
    case GATT_CLI_SVC:
            pts_gatt_cli_event_register((gatt_cli_svc_ind_t *)param);//PTS can recv notification after discover
            TRACE(0, "[%s][%d]: ind_code = %d", __FUNCTION__, __LINE__, param->ind_code);
        break;
    case GATT_CLI_SVC_INFO:
            TRACE(0, "[%s][%d]: ind_code = %d", __FUNCTION__, __LINE__, param->ind_code);
        break;
    default:
            TRACE(0, "[%s][%d]: ind_code = %d", __FUNCTION__, __LINE__, param->ind_code);
        break;
    }

    return (KE_MSG_CONSUMED);
}

static int *pts_gatt_req_ind_evt_handler(ke_msg_id_t const msgid,
                                gatt_srv_att_info_get_req_ind_t *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    switch(param->req_ind_code){
        case GATT_CLI_ATT_VAL_GET:
            pts_gatt_cli_write_reliable_cb((gatt_cli_att_val_get_req_ind_t *)param);
            break;
        case GATT_CLI_ATT_EVENT:
            TRACE(0, "[%s][%d]: ind_code = %d len=%d", __FUNCTION__, __LINE__, param->req_ind_code,((gatt_cli_att_event_req_ind_t *)param)->value_length);
            break;
        case GATT_SRV_ATT_READ_GET:
            pts_gatt_srv_att_read_get((gatt_srv_att_read_get_req_ind_t *)param);
            TRACE(0, "[%s][%d]: ind_code = %d hdl=%d", __FUNCTION__, __LINE__, param->req_ind_code,((gatt_srv_att_read_get_req_ind_t *)param)->hdl);
            break;
        case GATT_SRV_ATT_VAL_SET:
            pts_gatt_srv_att_val_set((gatt_srv_att_val_set_req_ind_t *)param);
            TRACE(0, "[%s][%d]: ind_code = %d len=%d,hdl=%d", __FUNCTION__, __LINE__, param->req_ind_code,((gatt_srv_att_val_set_req_ind_t *)param)->value_length,((gatt_srv_att_val_set_req_ind_t *)param)->hdl);
            break;
        case GATT_SRV_ATT_INFO_GET:
            pts_gatt_srv_att_info_get((gatt_srv_att_info_get_req_ind_t *)param);
            break;
        default:
            TRACE(0, "[%s][%d]: ind_code = %d", __FUNCTION__, __LINE__, param->req_ind_code);
            break;
    }

    return (KE_MSG_CONSUMED);
}

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(ble_pts)
{
    /// please add in order of gapc_msg size
    // GATT messages
    {GATT_CMP_EVT,                 (ke_msg_func_t)pts_gatt_user_cmp_evt_handle},
    {GATT_IND,                     (ke_msg_func_t)pts_gatt_user_ind_evt_handler},
    {GATT_REQ_IND,                 (ke_msg_func_t)pts_gatt_req_ind_evt_handler},

    /// please add in order of gapm_msg size
    // GAPM messages
    {GAPM_CMP_EVT,                 (ke_msg_func_t)pts_gapm_cmp_evt_handler},
    {GAPM_DEV_BDADDR_IND,          (ke_msg_func_t)pts_gapm_dev_addr_ind_handler},
    {GAPM_ADDR_SOLVED_IND,         (ke_msg_func_t)pts_gapm_addr_solved_ind_handler},
    {GAPM_ACTIVITY_CREATED_IND,    (ke_msg_func_t)pts_gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND,    (ke_msg_func_t)pts_gapm_activity_stopped_ind_handler},
    {GAPM_EXT_ADV_REPORT_IND,      (ke_msg_func_t)pts_gapm_adv_report_evt_handler},


    /// please add in order of gapc_msg size
    // GAPC messages
    {GAPC_CMP_EVT,                 (ke_msg_func_t)pts_gapc_cmp_evt_handler},
    {GAPC_LE_CONNECTION_REQ_IND,   (ke_msg_func_t)pts_gapc_le_connection_req_ind_handler},
    {GAPC_DISCONNECT_IND,          (ke_msg_func_t)pts_gapc_disconnect_ind_handler},
    {GAPC_MTU_CHANGED_IND,         (ke_msg_func_t)pts_gapc_mtu_changed_ind_handler},
    {GAPC_PEER_ATT_INFO_IND,       (ke_msg_func_t)pts_gapc_peer_att_info_ind_handler},
    {GAPC_PEER_FEATURES_IND,       (ke_msg_func_t)pts_gapc_peer_features_ind_handler},
    {GAPC_GET_DEV_INFO_REQ_IND,    (ke_msg_func_t)pts_gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND,    (ke_msg_func_t)pts_gapc_set_dev_info_req_ind_handler},

    {GAPC_PARAM_UPDATE_REQ_IND,    (ke_msg_func_t)pts_gapc_conn_param_update_req_ind_handler},
    {GAPC_PARAM_UPDATED_IND,       (ke_msg_func_t)pts_gapc_conn_param_updated_handler},

    {KE_MSG_DEFAULT_HANDLER,       (ke_msg_func_t)pts_appm_msg_handler},


};




// Application task descriptor
const struct ke_task_desc TASK_BLE_PTS_APP = {ble_pts_msg_handler_tab, &ble_pts_state, 1, ARRAY_LEN(ble_pts_msg_handler_tab)};
