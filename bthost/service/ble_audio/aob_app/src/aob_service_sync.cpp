/*
 * @copyright Copyright (c) 2015-2021 BES Technic.
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
 */
/**
 ****************************************************************************************
 * @addtogroup AOB_APP
 * @{
 ****************************************************************************************
 */

/*****************************header include********************************/
#include "bluetooth_bt_api.h"
#include "bluetooth_ble_api.h"
#include "besaud_api.h"
#include "hal_trace.h"
#include "nvrecord_extension.h"
#include "app_gaf_define.h"
#include "app_gaf_custom_api.h"
#include "ble_audio_core.h"
#ifdef BLE_STACK_NEW_DESIGN
#include "app_ble.h"
#else
#include "gatt.h"
#include "app_ble_tws_sync.h"
#endif

#include "aob_service_sync.h"
#include "aob_gatt_cache.h"
#include "aob_mgr_gaf_evt.h"

#include "app_bap_uc_srv_msg.h"

bool app_ble_audio_support_sync_service()
{
    return false;
}

bool app_ble_audio_send_service_data(const bt_bdaddr_t *remote_addr, uint32_t svc_uuid, void *data)
{
    if (!app_ble_audio_support_sync_service())
    {
        return false;
    }

    if (!btif_besaud_is_connected())
    {
        return false;
    }

    TRACE(1, "ble audio profile_data_sync service: 0x%x", svc_uuid);
    uint8_t send_data_buf[BLE_TWS_SYNC_MAX_DATA_SIZE];
    uint32_t len = 0;

    memcpy(send_data_buf + len, remote_addr, sizeof(bt_bdaddr_t));
    len += 6;
    send_data_buf[len++] = (svc_uuid) & 0xff;
    send_data_buf[len++] = (svc_uuid >> 8) & 0xff;
    send_data_buf[len++] = (svc_uuid >> 16) & 0xff;
    send_data_buf[len++] = (svc_uuid >> 24) & 0xff;

    switch (svc_uuid)
    {
#ifndef BLE_STACK_NEW_DESIGN
        case GATT_SVC_GENERIC_MEDIA_CONTROL:
        case GATT_SVC_MEDIA_CONTROL:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_acc_mcc_bond_data_ind_t));
            len += sizeof(app_gaf_acc_mcc_bond_data_ind_t);
            break;
        case GATT_SVC_GENERIC_TELEPHONE_BEARER:
        case GATT_SVC_TELEPHONE_BEARER:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_acc_tbc_bond_data_ind_t));
            len += sizeof(app_gaf_acc_tbc_bond_data_ind_t);
            break;
#endif
        case GATT_SVC_VOLUME_CONTROL:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_arc_vcs_bond_data_ind_t));
            len += sizeof(app_gaf_arc_vcs_bond_data_ind_t);
            break;
        case GATT_SVC_AUDIO_STREAM_CTRL:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_bap_uc_srv_bond_data_ind_t));
            len += sizeof(app_gaf_bap_uc_srv_bond_data_ind_t);
            break;
        case GATT_SVC_PUBLISHED_AUDIO_CAPA:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_capa_srv_bond_data_ind_t));
            len += sizeof(app_gaf_capa_srv_bond_data_ind_t);
            break;
        case GATT_SVC_COORD_SET_IDENTIFICATION:
            memcpy(send_data_buf + len, data, sizeof(app_gaf_atc_csism_bond_data_ind_t));
            len += sizeof(app_gaf_atc_csism_bond_data_ind_t);
            break;
        case GATT_SVC_BCAST_AUDIO_SCAN:
            break;
        case GATT_SVC_AUDIO_INPUT_CONTROL:
            break;
        case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
            break;
        case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
            break;
        default:
            break;
    }

    ASSERT(len <= sizeof(send_data_buf), "send_data_buf tx=%d", len);
    DUMP8("%02x ", send_data_buf, len > 20 ? 20 : len);
    app_ble_tws_sync_send_cmd(BLE_TWS_SYNC_SHARE_SERVICE_INFO, 0, send_data_buf, len);

    return true;
}

void app_ble_audio_recv_service_data(uint8_t *p_buff, uint16_t len)
{
    bt_bdaddr_t remote_addr;
    uint32_t offset = 0;
    uint32_t svc_uuid = 0xFFFFFFFF;

    memcpy((uint8_t *)&remote_addr, p_buff, sizeof(bt_bdaddr_t));
    offset += 6;
    svc_uuid = *((uint32_t *)(p_buff + offset));
    offset += 4;
    TRACE(1, "ble audio recv service uuid:0x%x", svc_uuid);

    ble_bdaddr_t remote_addr_tmp;
    memcpy(remote_addr_tmp.addr, remote_addr.address, BD_ADDR_LEN);
    remote_addr_tmp.addr_type = ADDR_PUBLIC;

#ifndef BLE_STACK_NEW_DESIGN
    uint8_t con_lid = ble_audio_get_mobile_sm_index_by_addr(&remote_addr_tmp);

    bool is_acc_disc_done = app_bap_uc_srv_acc_already_discovered(con_lid);

    switch (svc_uuid)
    {
        case GATT_SVC_GENERIC_MEDIA_CONTROL:
        case GATT_SVC_MEDIA_CONTROL:
            app_gaf_acc_mcc_bond_data_ind_t mcs_data;
            memcpy((uint8_t *)&mcs_data, p_buff + offset, sizeof(app_gaf_acc_mcc_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&mcs_data, sizeof(app_gaf_acc_mcc_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, mcs_data.mcs_info.uuid, (void *)&mcs_data);

            if (is_acc_disc_done && aob_gattc_cache_load(con_lid, remote_addr.address, GATT_SVC_GENERIC_MEDIA_CONTROL))
            {
                TRACE(1, "[%d]%s earbuds load uuid :%04x cache success", con_lid, __func__, mcs_data.mcs_info.uuid);
                app_acc_mcc_set_cfg(con_lid, 0, AOB_MGR_MC_CHAR_TYPE_PLAYER_NAME, 1);
            }
            break;
        case GATT_SVC_GENERIC_TELEPHONE_BEARER:
        case GATT_SVC_TELEPHONE_BEARER:
            app_gaf_acc_tbc_bond_data_ind_t tbs_data;
            memcpy((uint8_t *)&tbs_data, p_buff + offset, sizeof(app_gaf_acc_tbc_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&tbs_data, sizeof(app_gaf_acc_tbc_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, tbs_data.tbs_info.uuid, (void *)&tbs_data);

            if (is_acc_disc_done && aob_gattc_cache_load(con_lid, remote_addr.address, GATT_SVC_GENERIC_TELEPHONE_BEARER))
            {
                TRACE(1, "[%d]%s earbuds load uuid :%04x cache success", con_lid, __func__, tbs_data.tbs_info.uuid);
                app_acc_tbc_set_cfg(con_lid, 0, AOB_MGR_TB_CHAR_TYPE_PROV_NAME, 1);
            }
            break;
#else
    switch (svc_uuid)
    {
#endif
        case GATT_SVC_VOLUME_CONTROL:
            app_gaf_arc_vcs_bond_data_ind_t vcs_bond_data;
            memcpy((uint8_t *)&vcs_bond_data, p_buff + offset, sizeof(app_gaf_arc_vcs_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&vcs_bond_data, sizeof(app_gaf_arc_vcs_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, GATT_SVC_VOLUME_CONTROL, (void *)&vcs_bond_data);
            break;
        case GATT_SVC_AUDIO_STREAM_CTRL:
            app_gaf_bap_uc_srv_bond_data_ind_t ascs_data;
            memcpy((uint8_t *)&ascs_data, p_buff + offset, sizeof(app_gaf_bap_uc_srv_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&ascs_data, sizeof(app_gaf_bap_uc_srv_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, GATT_SVC_AUDIO_STREAM_CTRL, (void *)&ascs_data);
            break;
        case GATT_SVC_PUBLISHED_AUDIO_CAPA:
            app_gaf_capa_srv_bond_data_ind_t pacs_bond_data;
            memcpy((uint8_t *)&pacs_bond_data, p_buff + offset, sizeof(app_gaf_capa_srv_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&pacs_bond_data, sizeof(app_gaf_capa_srv_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, GATT_SVC_PUBLISHED_AUDIO_CAPA, (void *)&pacs_bond_data);
            break;
        case GATT_SVC_COORD_SET_IDENTIFICATION:
            app_gaf_atc_csism_bond_data_ind_t csis_data;
            memcpy((uint8_t *)&csis_data, p_buff + offset, sizeof(app_gaf_atc_csism_bond_data_ind_t));
            DUMP8("%02x ", (uint8_t *)&csis_data, sizeof(app_gaf_atc_csism_bond_data_ind_t));
            aob_gattc_rebuild_new_cache((uint8_t *)&remote_addr, GATT_SVC_COORD_SET_IDENTIFICATION, (void *)&csis_data);
            break;
        case GATT_SVC_BCAST_AUDIO_SCAN:
            break;
        case GATT_SVC_AUDIO_INPUT_CONTROL:
            break;
        case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
            break;
        case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
            break;
        default:
            break;
    }
}
