/***************************************************************************
 *
 * Copyright 2015-2021 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_APP_SEC)

#include <string.h>
#include "co_utils.h"
#include "co_math.h"
#include "gapc_msg.h"      // GAP Controller Task API Definition
#include "gapc_le_msg.h"
#include "gap.h"            // GAP Definition
#include "gapc.h"           // GAPC Definition
#include "prf_types.h"

#include "app.h"            // Application API Definition
#include "app_sec.h"        // Application Security API Definition
#include "app_task.h"       // Application Manager API Definition
#include "nvrecord_ble.h"
#include "tgt_hardware.h"
#include "app_ble_core.h"
#include "bluetooth_bt_api.h"
#include "l2cap_api.h"
#ifdef CTKD_ENABLE
#include "l2cap_i.h"
#include "l2cap_smp.h"
#include "cmac.h"
#include "nvrecord_bt.h"
#include "ddbif.h"
#endif

#if (DISPLAY_SUPPORT)
#include "app_display.h"    // Display Application Definitions
#endif //(DISPLAY_SUPPORT)

#if (NVDS_SUPPORT)
#include "nvds.h"           // NVDS API Definitions
#endif //(NVDS_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"
#endif //(BLE_APP_AM0)

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#endif

#include "nvrecord_ble.h"
#include "tgt_hardware.h"
#include "nvrecord_extension.h"
#include "app_ble_core.h"

#if BLE_AUDIO_ENABLED
#include "aob_gatt_cache.h"
#include "aob_service_sync.h"
#include "ble_audio_earphone_info.h"
#endif

#ifdef _BLE_NVDS_
#define BLE_KEY_PRINT
BleDevicePairingInfo ble_save_info;
#endif

#ifdef IBRT
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#endif

#include "nvrecord_env.h"

#ifdef CTKD_ENABLE
extern void app_bt_ctkd_connecting_mobile_handler(void);
extern bt_bdaddr_t* app_bt_get_remote_device_address(uint8_t device_id);
extern uint8_t* bt_get_local_address(void);
#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
* API of is_ble_master_send_encrypt_req_enabled to configure whether master initiates 
* encryption or starts a new pairing for every ble connection (as master)
*/
static bool is_ble_master_send_encrypt_req_enabled = true;
bool app_sec_is_ble_master_send_encrypt_req_enabled(void)
{
    return is_ble_master_send_encrypt_req_enabled;
}

void app_sec_set_ble_master_send_encrypt_req_enabled_flag(int isEnabled)
{
    is_ble_master_send_encrypt_req_enabled = isEnabled;
}

/// Application Security Environment Structure
struct app_sec_env_tag app_sec_env;
set_rsp_dist_lk_bit_field_func dist_lk_set_cb = NULL;

smp_identify_addr_exch_complete indentify_addr_exch_cmp_cb = NULL;

bool app_sec_store_and_sync_ble_info(uint8_t *ble_save_info)
{
    BleDevicePairingInfo *pInfo = (BleDevicePairingInfo *)ble_save_info;
#if BLE_AUDIO_ENABLED
    aob_gattc_delete_nv_cache(pInfo->peer_addr.addr, 0);
#endif

    uint8_t status = nv_record_blerec_add(pInfo);

    if (!status)
    {
        // Update the bonding status in the environment
        app_sec_env.bonded = true;
#ifdef TWS_SYSTEM_ENABLED
#if (BLE_AUDIO_ENABLED == 0)
        app_ble_sync_ble_info();
        TRACE(0,"BLE NVDS SETUP SUCCESS!!!,the key is the newest");
#endif
#endif
        return true;
    }

    TRACE(0, "save ltk fail");
    return false;
}

#ifdef CTKD_ENABLE
/*****************************************************************************
* CTKD over classic BT
*****************************************************************************/
static const uint8_t tmp2[TMP2LEN] = {0x74, 0x6d, 0x70, 0x32};
static const uint8_t brle[BRLELEN] = {0x62, 0x72, 0x6c, 0x65};
static const uint8_t salt[SALTLEN] = {0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x6D, 0x70, 0x32};
static void function_h6_calc_ltk(uint8_t *iLtk, uint8_t* convertedLtk) //BLE LTK = h6(ILTK, “brle”)
{
    uint8_t ltk[LTKLEN];

    aes_cal_cmac(brle, BRLELEN, ltk, iLtk);

    for (uint32_t i = 0; i < BLE_LTK_SIZE; i++)
    {
        convertedLtk[i] = ltk[BLE_LTK_SIZE - (i + 1)];
    }
}

static void function_calc_iLtk(uint8_t *linkKey, uint8_t *iLtk, bool ct2)
{
    uint8_t tmplinkKey[LINKKEYLEN];

    for(uint32_t i = 0; i < LINKKEYLEN; i++)
    {
        tmplinkKey[i]  = linkKey[LINKKEYLEN - (i + 1)];
    }

    TRACE(1, "%s,CT2:%d, tmpLinkKey:", __func__, ct2);
    DUMP8("%02x ", (uint8_t *)tmplinkKey, BLE_LTK_SIZE);

    if (ct2) {
        // both devices set CT2 = 1, ILTK = h7(SALT, Link Key)
        aes_cal_cmac(tmplinkKey, LINKKEYLEN, iLtk, salt);
    } else {
        // at least one device sets CT2 = 0, ILTK = h6(Link Key, "tmp2")
        aes_cal_cmac(tmp2, TMP2LEN, iLtk, tmplinkKey);
    }
}

static void clac_longTermKey(uint8_t *linkKey, BLE_ADDR_INFO_T *pBdAddr,
        uint8_t* pIrk, bool ct2, bool nvRole)
{
    uint8_t iLtk[16];
    APP_SEC_CTKD_INFO ctkdInfo;
    BleDevicePairingInfo *ble_save_info = NULL;
    memset(&ctkdInfo, 0, sizeof(BleDevicePairingInfo));
    ble_save_info = &ctkdInfo.i.leCTKDInfo;

    // Calculation
    function_calc_iLtk(linkKey, iLtk, ct2);
    function_h6_calc_ltk(iLtk, ble_save_info->LTK);

    // Fill in information
    ble_save_info->peer_addr.addr_type = pBdAddr->addr_type;
    memcpy(ble_save_info->peer_addr.addr, pBdAddr->addr, BLE_ADDR_SIZE);
    memcpy(ble_save_info->IRK, pIrk, BLE_IRK_SIZE);
    set_bit(ble_save_info->bond_info_bf, BONDED_STATUS_POS);
    set_bit(ble_save_info->bond_info_bf, BONDED_WITH_IRK_DISTRIBUTED);

    // Share or save info
    if (IBRT_SLAVE == nvRole) {
        tws_ctrl_send_cmd(APP_TWS_CMD_SHARE_CTKD_INFO,
                (uint8_t *)&ctkdInfo, sizeof(BleDevicePairingInfo) + 1);
    } else {
        app_sec_store_and_sync_ble_info((uint8_t *)ble_save_info);
    }
}

void app_sec_get_mobile_bt_link_key(uint8_t device_id, uint8_t *peerBtAddr, uint8_t *linkKey)
{
    btif_device_record_t mobileRecord;
    bt_status_t ret = nv_record_ddbrec_find((bt_bdaddr_t *)peerBtAddr, &mobileRecord);

    memset(linkKey, 0x00, BLE_LTK_SIZE);
    if (BT_STS_SUCCESS == ret)
    {
        if (!memcmp(mobileRecord.bdAddr.address, peerBtAddr,
                sizeof(mobileRecord.bdAddr.address)))
        {
            memcpy(linkKey, mobileRecord.linkKey, sizeof(mobileRecord.linkKey));
        }
    }
}

static inline void app_sec_fill_edr_smp_pairing_rsp_data(uint8_t *data)
{
    l2cap_smp_pairing_rsp_t *p_pairing_rsp = (l2cap_smp_pairing_rsp_t *)data;

    p_pairing_rsp->code = L2CAP_SMP_PAIRING_RSP_OPCODE;
#if defined (IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE)
    p_pairing_rsp->iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#else
    p_pairing_rsp->iocap = GAP_IO_CAP_DISPLAY_YES_NO;
#endif
    p_pairing_rsp->oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
    p_pairing_rsp->auth = GAP_AUTH_BOND|GAP_AUTH_CT2;
    p_pairing_rsp->key_size = 16;
    p_pairing_rsp->ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
    p_pairing_rsp->rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
}

static void app_sec_derive_ltk(uint8_t device_id,struct smp_identity *remote_dev)
{
    uint8_t linkkey[BLE_LTK_SIZE];
    bt_bdaddr_t* btAddr = NULL;
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    uint32_t nvRole = nvrecord_env->ibrt_mode.mode;


    btAddr = app_bt_get_remote_device_address(device_id);
    app_sec_get_mobile_bt_link_key(device_id, btAddr->address, linkkey);

    clac_longTermKey(linkkey, &remote_dev->pBdAddr, remote_dev->irk, true, nvRole);

    ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
    uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
    appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);

    if (indentify_addr_exch_cmp_cb)
    {
        indentify_addr_exch_cmp_cb((ble_bdaddr_t *)&remote_dev->pBdAddr);
    }
#ifdef GFPS_ENABLED
    gfps_ntf_ble_bond_over_bt(device_id, true);
#endif
}

void app_sec_handle_bredr_smp_pairing_req(uint8_t device_id,
    uint16_t conn_handle, uint16_t len, uint8_t *data)
{
    static struct smp_identity remote;

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    uint32_t nvRole = nvrecord_env->ibrt_mode.mode;

    enum l2cap_smp_code smpCmdCode = data[0];
    TRACE(2,"%s cmd:%d, role:%d", __func__, smpCmdCode, nvRole);

    switch (smpCmdCode)
    {
        case L2CAP_SMP_PAIRING_REQ_OPCODE:
        {
            uint8_t rsp_data[BLE_IRK_SIZE+2];

            // 0. initialize recv
            remote.recv = 0;

            // 1. response pair request
#if 0
            if ((!app_tws_is_connected()) && (IBRT_SLAVE == nvRole)) {
                l2cap_smp_pairing_failed_t *p_rsp_data = (l2cap_smp_pairing_failed_t *)rsp_data;
                p_rsp_data->code = L2CAP_SMP_PAIRING_FAILED_OPCODE;
                p_rsp_data->reason = L2CAP_SMP_ERR_PAIRING_NOT_SUPP;
                btif_l2cap_send_bredr_security_manager_rsp(device_id, conn_handle, 2, rsp_data);
                break;
            } else {
                app_sec_fill_edr_smp_pairing_rsp_data(rsp_data);
                btif_l2cap_send_bredr_security_manager_rsp(device_id, conn_handle, 7, rsp_data);
            }
#else
            app_sec_fill_edr_smp_pairing_rsp_data(rsp_data);
            btif_l2cap_send_bredr_security_manager_rsp(device_id, conn_handle, 7, rsp_data);
#endif

            // 2. send NV master's IRK
            uint8_t* pLocalIrk = NULL;
            if (IBRT_SLAVE == nvRole)
            {
                NV_EXTENSION_RECORD_T* pNvExtRec = nv_record_get_extension_entry_ptr();
                pLocalIrk = pNvExtRec->tws_info.ble_info.ble_irk;
            }
            else
            {
                pLocalIrk = appm_get_current_ble_irk();
            }

            rsp_data[0] = L2CAP_SMP_IDENTITY_INF_OPCODE;
            memcpy(&rsp_data[1], pLocalIrk, BLE_IRK_SIZE);
            btif_l2cap_send_bredr_security_manager_rsp(device_id, conn_handle, 1+BLE_IRK_SIZE, rsp_data);

            // 3. send local identity addr
            uint8_t* pLocalIdentityAddr = NULL;
            if (IBRT_SLAVE == nvRole)
            {
                pLocalIdentityAddr = nvrecord_env->ibrt_mode.record.bdAddr.address;
            }
            else
            {
                pLocalIdentityAddr = bt_get_local_address();
            }
            rsp_data[0] = L2CAP_SMP_ID_ADDR_INF_OPCODE;
            rsp_data[1] = ADDR_PUBLIC;
            memcpy(&rsp_data[2], pLocalIdentityAddr, BLE_ADDR_SIZE);
            btif_l2cap_send_bredr_security_manager_rsp(device_id, conn_handle, 2+BLE_ADDR_SIZE, rsp_data);
            break;
        }
        case L2CAP_SMP_IDENTITY_INF_OPCODE:
        {
            remote.recv++;
            memcpy(remote.irk, &data[1], sizeof(remote.irk));

            if(remote.recv == 2) {
                remote.recv = 0;
                app_sec_derive_ltk(device_id, &remote);
            }
            break;
        }
        case L2CAP_SMP_ID_ADDR_INF_OPCODE:
        {
            TRACE(0, "rem identity addr: type %d", data[1]);
            DUMP8("%02x ", &data[2], BLE_ADDR_SIZE);

            remote.recv++;
            remote.pBdAddr.addr_type = data[1];
            memcpy(remote.pBdAddr.addr, &data[2], sizeof(remote.pBdAddr.addr));

            if(remote.recv == 2) {
                remote.recv = 0;
                app_sec_derive_ltk(device_id, &remote);
            }
            break;
        }
        default:
            break;
    }

}

/*****************************************************************************
* CTKD over BLE
*****************************************************************************/
static const uint8_t tmp1[TMP1LEN] = {0x74,0x6d,0x70,0x31};
static const uint8_t lebr[LEBRLEN] = {0x6c,0x65,0x62,0x72};
static const uint8_t saltlebr[SALTLEN] =
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x74,0x6D,0x70,0x31};

extern void *app_bt_profile_active_store_ptr_get(uint8_t *bdAddr);
void app_sec_store_and_sync_bt_info(uint8_t *btInfo)
{
    APP_SEC_BT_CTKD_INFO *btCtkdInfo = (APP_SEC_BT_CTKD_INFO *)btInfo;
    btif_device_record_t record;
    bt_status_t status = BT_STS_FAILED;
    memcpy(record.bdAddr.address, btCtkdInfo->peerAddr, BLE_ADDR_SIZE);
    memcpy(record.linkKey, btCtkdInfo->linkKey , LINKKEYLEN);
    record.keyType = BTIF_UNAUTH_COMBINATION_KEY;
    TRACE(0, "%s", __func__);
    DUMP8("%02x ", btCtkdInfo, BLE_ADDR_SIZE + LINKKEYLEN);

    status = ddbif_add_record(&record);
    if (status != BT_STS_SUCCESS) {
        TRACE(0, "save link key fail");
    } else {
        btdevice_profile *btdevice_plf_p =
            (btdevice_profile *)app_bt_profile_active_store_ptr_get(record.bdAddr.address);
        nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_plf_p, true);
        nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_plf_p, true);
    }
}

//BR/EDR link key = h6(ILK,"lebr")
static void function_h6_calc_linkKey(uint8_t *ilk, uint8_t *pBdAddr, uint8_t* convertedLtk)
{
    uint8_t LinkKey[LINKKEYLEN];

    aes_cal_cmac(lebr, LEBRLEN, LinkKey, ilk);

    for (uint32_t i = 0; i < LINKKEYLEN; i++)
    {
        convertedLtk[i] = LinkKey[LINKKEYLEN - (i + 1)];
    }
}

static void function_calc_ilk(uint8_t *ltk, uint8_t *ilk, bool ct2)
{
    uint8_t tmpLtk[BLE_LTK_SIZE];

    for(uint32_t i = 0; i < BLE_LTK_SIZE; i++)
    {
        tmpLtk[i]  = ltk[BLE_LTK_SIZE - (i + 1)];
    }

    TRACE(1, "%s,CT2:%d, tmpLtk:", __func__, ct2);
    DUMP8("%02x ", (uint8_t *)tmpLtk, BLE_LTK_SIZE);

    if (ct2) {
        // both devices set CT2 = 1, ILK = h7(SALT,LTK)
        aes_cal_cmac(tmpLtk, BLE_LTK_SIZE, ilk, saltlebr);
    } else {
        // at least one device sets CT2 = 0, ILK = h6(LTK,"tmp1")
        aes_cal_cmac(tmp1, TMP1LEN, ilk, tmpLtk);
    }
}

void clac_linkKey(uint8_t *ltk, uint8_t *pBdAddr, bool ct2)
{
    uint8_t ilk[16];
    APP_SEC_CTKD_INFO ctkdInfo;
    APP_SEC_BT_CTKD_INFO *btCtkdInfo = &ctkdInfo.i.btCTKDInfo;
    ctkdInfo.infoType = 1;
    memcpy(btCtkdInfo->peerAddr, pBdAddr, 6);

    // Calculation
    function_calc_ilk(ltk, ilk, ct2);
    function_h6_calc_linkKey(ilk, pBdAddr, btCtkdInfo->linkKey);

    // Share or save info
    if (IBRT_SLAVE == app_ibrt_if_get_ui_role()) {
        tws_ctrl_send_cmd(APP_TWS_CMD_SHARE_CTKD_INFO,
            (uint8_t *)&ctkdInfo, sizeof(APP_SEC_BT_CTKD_INFO) + 1);
    } else {
        app_sec_store_and_sync_bt_info((uint8_t *)btCtkdInfo);
    }
}
#endif

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_sec_init(void)
{
#ifdef _BLE_NVDS_
    if (nv_record_ble_record_Once_a_device_has_been_bonded())
    {
        app_sec_env.bonded = true;
    }
    else
    {
        app_sec_env.bonded = false;
    }

    TRACE(2,"[%s] once bonded:%d", __func__, app_sec_env.bonded);
#endif
}

bool app_sec_get_bond_status(void)
{
    return app_sec_env.bonded;
}

void app_sec_send_security_req(uint8_t conidx, uint8_t authority)
{
    TRACE(1, "%s", __func__);
    // Send security request
    struct gapc_security_cmd *cmd = KE_MSG_ALLOC(GAPC_SECURITY_CMD,
                                                 TASK_GAPC,
                                                 TASK_APP,
                                                 gapc_security_cmd);

    cmd->operation = GAPC_SECURITY_REQ;

    cmd->auth = authority;
    cmd->conidx = conidx;
    // Send the message
    ke_msg_send(cmd);
}

bool app_sec_send_encrypt_req(uint8_t conidx, uint8_t * securityInfo)
{
    BleDevicePairingInfo * deviceSecurityInfo = (BleDevicePairingInfo*)securityInfo;
    if (!deviceSecurityInfo)
    {
        return false;
    }

    struct gapc_encrypt_cmd *encryptCmd = KE_MSG_ALLOC(GAPC_ENCRYPT_CMD,
                                                        TASK_GAPC,
                                                        TASK_APP,
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

void app_sec_send_pair_req(uint8_t conidx, struct gapc_pairing_feat * pair_feat)
{
    struct gapc_bond_cmd *cmd = KE_MSG_ALLOC(GAPC_BOND_CMD, TASK_GAPC,
                                             TASK_APP, gapc_bond_cmd);
    cmd->conidx = conidx;
    cmd->operation = GAPC_BOND;
    cmd->sec_req_level = pair_feat->sec_req_level;

    memcpy(&(cmd->pairing), &(pair_feat->pairing_info), sizeof(gapc_pairing_t));

    ke_msg_send(cmd);

}

#if defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT)
static void app_sec_send_cli_feat_en_cmd(uint8_t conidx)
{
    struct gapc_operation_cmd *cmd = KE_MSG_ALLOC(GAPC_CLI_FEAT_EN_CMD, TASK_GAPC,
                                                  TASK_APP, gapc_operation_cmd);
    cmd->conidx = conidx;
    cmd->operation = GAPC_CLI_FEAT_EN;

    ke_msg_send(cmd);
}
#endif

void app_sec_nc_exch_accept(uint8_t conidx, bool accept)
{
    TRACE(1, "%s %d conidx 0x%02x", __func__, accept, conidx);

    // Prepare the GAPC_BOND_CFM message
    struct gapc_bond_cfm *cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                             TASK_GAPC, TASK_APP,
                                             gapc_bond_cfm);

    cfm->accept  = accept;
    cfm->request = GAPC_NC_EXCH;
    cfm->conidx  = conidx;

    // Send the message
    ke_msg_send(cfm);
}

void app_sec_reg_dist_lk_bit_set_callback(set_rsp_dist_lk_bit_field_func callback)
{
    dist_lk_set_cb = callback;
}

void *app_sec_reg_dist_lk_bit_get_callback(void)
{
    return dist_lk_set_cb;
}

void app_sec_reg_smp_identify_info_cmp_callback(smp_identify_addr_exch_complete callback)
{
    indentify_addr_exch_cmp_cb = callback;
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
static int app_sec_bond_req_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_bond_req_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    TRACE(1, "Get bond req %d", param->request);

    struct gapc_bond_cfm *cfm = NULL;
    uint8_t conidx = param->conidx;

    if (GAPC_NC_EXCH != param->request)
    {
        // Prepare the GAPC_BOND_CFM message
        cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                 src_id, TASK_APP,
                                 gapc_bond_cfm);

        cfm->conidx = param->conidx;
    }

    switch (param->request)
    {
        case (GAPC_PAIRING_REQ):
        {
            cfm->request = GAPC_PAIRING_RSP;

            cfm->accept  = true;

            cfm->data.pairing_feat.pairing_info.auth        = BLE_AUTHENTICATION_LEVEL;

#ifdef CFG_SEC_CON
            if (param->data.auth_req & APP_SEC_AUTH_SC_BIT) {
#if defined (IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE) && !defined(GFPS_ENABLED)
                cfm->data.pairing_feat.pairing_info.iocap   = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
                cfm->data.pairing_feat.sec_req_level        = GAP_SEC1_NOAUTH_PAIR_ENC;
#else
#ifdef GFPS_ENABLED
                if (app_gfps_is_connected(conidx))
                {
                    cfm->data.pairing_feat.pairing_info.iocap   = GAP_IO_CAP_DISPLAY_YES_NO;
                    cfm->data.pairing_feat.sec_req_level        = GAP_SEC1_SEC_CON_PAIR_ENC;
                }
                else
#endif
                {
                    cfm->data.pairing_feat.pairing_info.iocap   = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
                    cfm->data.pairing_feat.sec_req_level        = GAP_SEC1_NOAUTH_PAIR_ENC;
                }
#endif
            } else {
                // security mode cant reached gapc_le_smp_is_sec_mode_reached
                cfm->data.pairing_feat.pairing_info.iocap   = GAP_IO_CAP_DISPLAY_YES_NO;
                cfm->data.pairing_feat.sec_req_level        = GAP_NO_SEC;
            }
#else
            cfm->data.pairing_feat.pairing_info.iocap       = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
            cfm->data.pairing_feat.sec_req_level            = GAP_SEC1_NOAUTH_PAIR_ENC;
#endif

            cfm->data.pairing_feat.pairing_info.key_size    = 16;
            cfm->data.pairing_feat.pairing_info.oob         = GAP_OOB_AUTH_DATA_NOT_PRESENT;

            cfm->data.pairing_feat.pairing_info.ikey_dist   = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
            cfm->data.pairing_feat.pairing_info.rkey_dist   = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
#if defined(CFG_SEC_CON) && defined(CTKD_ENABLE)
            if (dist_lk_set_cb)
            {
                cfm->data.pairing_feat.pairing_info.ikey_dist |= dist_lk_set_cb();
                cfm->data.pairing_feat.pairing_info.rkey_dist |= dist_lk_set_cb();
            }
            else
            {
                cfm->data.pairing_feat.pairing_info.ikey_dist |= GAP_KDIST_LINKKEY;
                cfm->data.pairing_feat.pairing_info.rkey_dist |= GAP_KDIST_LINKKEY;
            }
#endif

#if defined (BLE_AUDIO_DOLPHIN_COMPATIBLE_SUPPORT)
            app_sec_send_cli_feat_en_cmd(conidx);
#endif

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
            TRACE(0,"<==============>LTK IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk,16);
            TRACE(1,"<==============>EDIV IS: %04x:",cfm->data.ltk.ediv);
            TRACE(0,"<==============>RANDOM IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk.randnb.nb,8);
#endif
            /*
            * For legacy pairing, the distributed LTK is generated by both device through random numbers
            * The device that initiates encryption will decide which LTK to use through EDIV
            */
            ble_save_info.LOCAL_EDIV = cfm->data.ltk.ediv;
            memcpy(&ble_save_info.LOCAL_RANDOM, (uint8_t *)&cfm->data.ltk.randnb.nb, 8);
            memcpy(&ble_save_info.LOCAL_LTK, (uint8_t *)&cfm->data.ltk, 16);

            clr_bit(ble_save_info.bond_info_bf, BONDED_STATUS_POS);
#endif
        } break;

        case (GAPC_IRK_EXCH):
        {
            cfm->accept  = true;
            cfm->request = GAPC_IRK_EXCH;

            // Load IRK
            memcpy(cfm->data.irk.key.key, app_env.loc_irk, KEY_LEN);
            // load identity ble address
            memcpy(cfm->data.irk.addr.addr, ble_global_addr, BLE_ADDR_SIZE);
            cfm->data.irk.addr.addr_type = ADDR_PUBLIC;
        } break;

        case (GAPC_TK_EXCH):
        {
            // Generate a PIN Code- (Between 100000 and 999999)
            uint32_t pin_code = (100000 + (co_rand_word()%900000));

            cfm->accept  = true;
            cfm->request = GAPC_TK_EXCH;

            // Set the TK value
            memset(cfm->data.tk.key, 0, KEY_LEN);

            cfm->data.tk.key[0] = (uint8_t)((pin_code & 0x000000FF) >>  0);
            cfm->data.tk.key[1] = (uint8_t)((pin_code & 0x0000FF00) >>  8);
            cfm->data.tk.key[2] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
            cfm->data.tk.key[3] = (uint8_t)((pin_code & 0xFF000000) >> 24);
        } break;
        case GAPC_NC_EXCH:
        {
#ifdef BLE_SEC_ACCEPT_BY_CUSTOMER
            ble_event_t event;
            event.evt_type = BLE_CONNECT_NC_EXCH_EVENT;
            event.p.connect_nc_exch_handled.conidx = conidx;
            app_ble_core_global_handle(&event, NULL);
#else
            app_sec_nc_exch_accept(conidx, true);
#endif
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

void app_sec_reset_env_on_connection(void)
{
#ifdef _BLE_NVDS_
    memset(&ble_save_info, 0, sizeof(BleDevicePairingInfo));
#endif
}

void app_sec_start_pairing(uint8_t conidx)
{
    struct gapc_pairing_feat pair;
    TRACE(1,"Master initiates pair req!!! conidx %d",conidx);
#if defined(IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE)
    pair.pairing_info.iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#else
    pair.pairing_info.iocap = GAP_IO_CAP_DISPLAY_YES_NO;
#endif
    pair.pairing_info.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
    pair.pairing_info.auth = BLE_AUTHENTICATION_LEVEL;
    pair.pairing_info.key_size = 16;
    pair.pairing_info.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
    pair.pairing_info.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
#if defined(CFG_SEC_CON) && defined(CTKD_ENABLE)
    if (dist_lk_set_cb)
    {
        pair.pairing_info.ikey_dist |= dist_lk_set_cb();
        pair.pairing_info.rkey_dist |= dist_lk_set_cb();
    }
    else
    {
        pair.pairing_info.ikey_dist |= GAP_KDIST_LINKKEY;
        pair.pairing_info.rkey_dist |= GAP_KDIST_LINKKEY;
    }
#endif
    pair.sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;
    app_sec_send_pair_req(conidx, &pair);
}

static int app_sec_security_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_security_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    ble_bdaddr_t remote_addr = {{0}};
    app_ble_get_peer_solved_addr(conidx, &remote_addr);
    if (is_ble_master_send_encrypt_req_enabled)
    {
        uint8_t* deviceSecurityInfo =
            nv_record_ble_record_find_device_security_info_through_static_bd_addr(remote_addr.addr);
        if (deviceSecurityInfo)
        {
            TRACE(1,"Master initiates encrypt req!!! conidx %d",conidx);
            app_sec_send_encrypt_req(conidx, deviceSecurityInfo);
            return (KE_MSG_CONSUMED);
        }
    }
    
    {
        struct gapc_pairing_feat pair;
        TRACE(1,"Master initiates pair req!!! conidx %d",conidx);
#if defined(IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE)
        pair.pairing_info.iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#else
        pair.pairing_info.iocap = GAP_IO_CAP_DISPLAY_YES_NO;
#endif
        pair.pairing_info.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
        pair.pairing_info.auth = BLE_AUTHENTICATION_LEVEL;
        pair.pairing_info.key_size = 16;
        pair.pairing_info.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
        pair.pairing_info.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
#if defined(CFG_SEC_CON) && defined(CTKD_ENABLE)
        if (dist_lk_set_cb)
        {
            pair.pairing_info.ikey_dist |= dist_lk_set_cb();
            pair.pairing_info.rkey_dist |= dist_lk_set_cb();
        }
        else
        {
            pair.pairing_info.ikey_dist |= GAP_KDIST_LINKKEY;
            pair.pairing_info.rkey_dist |= GAP_KDIST_LINKKEY;
        }
#endif
        pair.sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;
        app_sec_send_pair_req(conidx, &pair);
    }

    return (KE_MSG_CONSUMED);
}

static int app_sec_bond_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_bond_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;
    switch (param->info)
    {
        case (GAPC_PAIRING_SUCCEED):
        {
            TRACE(1,"GAPC_PAIRING_SUCCEED");
#ifdef _BLE_NVDS_
            set_bit(ble_save_info.bond_info_bf, BONDED_STATUS_POS);
            // Sometimes the device do not distribute IRK
            uint8_t nullAddr[BLE_ADDR_SIZE] = {0};

            if (!memcmp(ble_save_info.peer_addr.addr, nullAddr, BLE_ADDR_SIZE)) {
                ble_bdaddr_t remote_addr = {{0}};
                app_ble_get_peer_solved_addr(conidx, &remote_addr);
                memcpy(ble_save_info.peer_addr.addr, remote_addr.addr, BLE_ADDR_SIZE);
            }

            uint8_t ret = app_sec_store_and_sync_ble_info((uint8_t *)&ble_save_info);

            if (ret)
            {
#ifdef CTKD_ENABLE
                if ((dist_lk_set_cb && dist_lk_set_cb()) || (!dist_lk_set_cb))
                {
                    clac_linkKey(ble_save_info.LTK, ble_save_info.peer_addr.addr, param->data.pairing.ct2);
                    app_bt_ctkd_connecting_mobile_handler();
                }
#endif
                ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
                uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
                appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
            }

#if BLE_AUDIO_ENABLED
            if (!app_ble_audio_support_sync_service() || !ble_audio_is_ux_slave())
            {
                aob_gattc_delete_nv_cache(app_env.context[conidx].solvedBdAddr, 0);
            }
#endif

#endif
            app_ble_on_bond_status_changed(conidx,true,0);
            ke_state_set(TASK_APP,APPM_ENCRYPTED);
            break;
        }
        case (GAPC_REPEATED_ATTEMPT):
        {
            app_ble_start_disconnect(conidx);
            break;
        }
        case (GAPC_IRK_EXCH):
        {
            TRACE(0,"Peer device IRK is:");
#ifdef _BLE_NVDS_
            DUMP8("%02x ", param->data.irk.key.key, BLE_IRK_SIZE);
            DUMP8("%02x ", (uint8_t *)&(param->data.irk.addr), BT_ADDR_OUTPUT_PRINT_NUM);
            app_env.context[conidx].isBdAddrResolvingInProgress = false;
            app_env.context[conidx].isGotSolvedBdAddr = true;
            memcpy(app_env.context[conidx].solvedBdAddr, param->data.irk.addr.addr, BLE_ADDR_SIZE);
            memcpy(ble_save_info.IRK, param->data.irk.key.key, BLE_IRK_SIZE);
            memcpy(ble_save_info.peer_addr.addr, param->data.irk.addr.addr, BLE_ADDR_SIZE);
            ble_save_info.peer_addr.addr_type = param->data.irk.addr.addr_type;
            memcpy(ble_save_info.peer_rpa_addr, app_env.context[conidx].bdAddr, BLE_ADDR_SIZE);
            if (app_env.context[conidx].addr_resolv_supp)
            {
                set_bit(ble_save_info.bond_info_bf, BONDED_WITH_IRK_DISTRIBUTED);
            }
            else
            {
                clr_bit(ble_save_info.bond_info_bf, BONDED_WITH_IRK_DISTRIBUTED);
            }
            TRACE(1, "Does Peer device support addr resolv %x", get_bit(ble_save_info.bond_info_bf, BONDED_WITH_IRK_DISTRIBUTED));
#endif
            app_exchange_remote_feature(conidx);
            break;
        }
        case (GAPC_PAIRING_FAILED):
        {
            TRACE(1,"GAPC_PAIRING_FAILED!!! Error code 0x%x", param->data.reason);
#ifdef _BLE_NVDS_
            nv_record_ble_delete_entry(app_env.context[conidx].solvedBdAddr);
#if BLE_AUDIO_ENABLED
            aob_gattc_delete_nv_cache(app_env.context[conidx].solvedBdAddr, 0);
#endif
            app_sec_env.bonded = false;
#endif
            ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
            uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
            appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
            app_ble_on_bond_failed(conidx);
            app_ble_on_bond_status_changed(conidx,false,param->data.reason);
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

            ble_save_info.EDIV = param->data.ltk.ediv;
            memcpy(&ble_save_info.RANDOM, (uint8_t *)&param->data.ltk.randnb.nb, BLE_ENC_RANDOM_SIZE);
            memcpy(&ble_save_info.LTK, (uint8_t *)&param->data.ltk, BLE_LTK_SIZE);
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

static int app_sec_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_encrypt_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    TRACE(1,"%s, master ask for LTK TO encrypt!!!!", __FUNCTION__);

    uint8_t conidx = param->conidx;

    if (app_env.context[conidx].connectStatus == BLE_CONNECTED)
    {
         appm_check_and_resolve_ble_address(conidx);
    }

    if (!app_env.context[conidx].isGotSolvedBdAddr)
    {
        TRACE(1,"%s, isGotSolvedBdAddr = %d", __FUNCTION__,
                                app_env.context[conidx].isGotSolvedBdAddr);
        return (KE_MSG_SAVED);
    }

    TRACE(0,"Random ble address solved. Can rsp enc req.");

    // Prepare the GAPC_ENCRYPT_CFM message
    struct gapc_encrypt_cfm *cfm = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM,
                                                src_id, TASK_APP,
                                                gapc_encrypt_cfm);

    cfm->found    = false;
    cfm->conidx   = param->conidx;

#ifdef _BLE_NVDS_
    gapc_ltk_t ltk;
    bool ret;
    ble_bdaddr_t remote_addr = {{0}};
    app_ble_get_peer_solved_addr(conidx, &remote_addr);
    ret = nv_record_ble_record_find_ltk(remote_addr.addr, ltk.key.key, param->ediv);
    if(ret)
    {
        cfm->found    = true;
        cfm->key_size = 16;
        memcpy(&cfm->ltk, ltk.key.key, sizeof(struct gap_sec_key));
    }
#endif

    // Send the message
    ke_msg_send(cfm);

    ble_event_t event;
    event.evt_type = BLE_ENCRYPT_LTK_REPORT_EVENT;
    event.p.le_conn_encrypt_ltk_handled.conidx = conidx;
    if (cfm->found == true)
    {
        event.p.le_conn_encrypt_ltk_handled.ltk_existed = true;
    }
    else
    {
        event.p.le_conn_encrypt_ltk_handled.ltk_existed = false;
    }
    app_ble_core_global_handle(&event, NULL);

    // encryption has been completed, we can start exchanging remote features now
    app_exchange_remote_feature(conidx);

    return (KE_MSG_CONSUMED);
}

static int app_sec_encrypt_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_encrypt_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t conidx = param->conidx;

    uint8_t encryptResult = param->encrypt_error_code;
    if (!encryptResult)   //GAP_ERR_NO_ERROR = 0x00
    {
        TRACE(2,"%s conidx 0x%02x pair_lvl %d", __func__, conidx, param->pairing_lvl);
        ke_state_set(TASK_APP,APPM_ENCRYPTED);

        app_ble_on_encrypt_success(conidx, param->pairing_lvl);
    }
    else
    {
        TRACE(2,"%s conidx 0x%02x encrypt_error_code %d", __func__, conidx, param->encrypt_error_code);
        struct gapc_pairing_feat pair;
#if defined(IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE)
        pair.pairing_info.iocap = GAP_IO_CAP_DISPLAY_ONLY;
#else
        pair.pairing_info.iocap = GAP_IO_CAP_DISPLAY_YES_NO;
#endif
        pair.pairing_info.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
        pair.pairing_info.auth = BLE_AUTHENTICATION_LEVEL;
        pair.pairing_info.key_size = 16;
        pair.pairing_info.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
        pair.pairing_info.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
#if defined(CFG_SEC_CON) && defined(CTKD_ENABLE)
        if (dist_lk_set_cb)
        {
            pair.pairing_info.ikey_dist |= dist_lk_set_cb();
            pair.pairing_info.rkey_dist |= dist_lk_set_cb();
        }
        else
        {
            pair.pairing_info.ikey_dist |= GAP_KDIST_LINKKEY;
            pair.pairing_info.rkey_dist |= GAP_KDIST_LINKKEY;
        }
#endif
        pair.sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;

        app_sec_send_pair_req(conidx, &pair);
    }
    return (KE_MSG_CONSUMED);
}

static int app_sec_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message
    return (KE_MSG_CONSUMED);
}

/*
* LOCAL VARIABLE DEFINITIONS
****************************************************************************************
*/
/// Default State handlers definition
const struct ke_msg_handler app_sec_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,  (ke_msg_func_t)app_sec_msg_dflt_handler},

    {GAPC_BOND_REQ_IND,       (ke_msg_func_t)app_sec_bond_req_ind_handler},
    {GAPC_BOND_IND,           (ke_msg_func_t)app_sec_bond_ind_handler},
    {GAPC_SECURITY_IND,       (ke_msg_func_t)app_sec_security_ind_handler},
    {GAPC_ENCRYPT_REQ_IND,    (ke_msg_func_t)app_sec_encrypt_req_ind_handler},
    {GAPC_ENCRYPT_IND,        (ke_msg_func_t)app_sec_encrypt_ind_handler},
};

const WEAK struct app_subtask_handlers app_sec_handlers = {&app_sec_msg_handler_list[0], ARRAY_LEN(app_sec_msg_handler_list)};

#endif //(BLE_APP_SEC)

/// @} APP
