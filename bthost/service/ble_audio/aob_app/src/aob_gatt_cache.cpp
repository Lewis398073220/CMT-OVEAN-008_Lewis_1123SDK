/***************************************************************************
 *
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
#include "hal_trace.h"
#include "nvrecord_extension.h"
#include "gatt_service.h"
#include "prf_types.h"
#include "app_acc_mcc_msg.h"
#include "app_acc_tbc_msg.h"
#include "app_arc_vcs_msg.h"
#include "app_bap_uc_srv_msg.h"
#include "app_bap_capa_srv_msg.h"
#include "app_gaf_custom_api.h"

#include "ble_audio_earphone_info.h"

#include "aob_volume_api.h"
#include "aob_gatt_cache.h"

#ifdef BLE_BATT_ENABLE
#include "app_batt.h"
#endif

#ifndef BLE_STACK_NEW_DESIGN
#include "app_atc_csism_msg.h"
#include "gatt.h"
#include "gapc.h"
#include "app_bap_bc_deleg_msg.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "gatt_user.h"
#ifdef __cplusplus
}
#endif

#endif

/*
 *  Construct a simple data struct firstly,this struct need to refactor continuously
 *
*/
GATTC_SRV_ATTR_t *aob_gattc_find_valid_cache(uint8_t *peer_addr, bool alloc)
{
    NV_EXTENSION_RECORD_T *p_nv_rec_ext = nv_record_get_extension_entry_ptr();
    GATTC_SRV_ATTR_t *p_srv_attr = NULL;

    if (NULL == p_nv_rec_ext || NULL == peer_addr)
    {
        TRACE(1, "%s fail", __func__);
        return NULL;
    }

    uint8_t idx = 0;
    GATTC_NV_SRV_ATTR_t *gatt_srv_record = &(p_nv_rec_ext->srv_cache_attr);

    while (idx++ < BLE_GATT_CACHE_REC_NUM)
    {
        p_srv_attr = &gatt_srv_record->gatt_nv_arv_attr[idx - 1];
        DUMP8("%02x ", p_srv_attr->peer_addr, BD_ADDR_LEN);
        if (!memcmp(p_srv_attr->peer_addr, peer_addr, BD_ADDR_LEN))
        {
            TRACE(1, "%s success", __func__);
            return p_srv_attr;
        }
    }

    TRACE(2, "%s no cache found,alloc:%d, wrap_idx:%d", __func__, alloc, gatt_srv_record->wrap_idx);

    if (alloc)
    {
        p_srv_attr = &gatt_srv_record->gatt_nv_arv_attr[gatt_srv_record->wrap_idx % BLE_GATT_CACHE_REC_NUM];
        uint32_t lock = nv_record_pre_write_operation();
        memcpy(p_srv_attr->peer_addr, peer_addr, BD_ADDR_LEN);
        ++(gatt_srv_record->wrap_idx) %= BLE_GATT_CACHE_REC_NUM;
        nv_record_post_write_operation(lock);
        return p_srv_attr;
    }
    else
    {
        return NULL;
    }
}

void aob_gattc_del_all_nv_cache(void)
{
    NV_EXTENSION_RECORD_T *p_nv_rec_ext = nv_record_get_extension_entry_ptr();
    if (NULL == p_nv_rec_ext)
    {
        TRACE(1, "%s fail", __func__);
        return;
    }

    GATTC_NV_SRV_ATTR_t *gatt_srv_record = &(p_nv_rec_ext->srv_cache_attr);

    uint32_t lock = nv_record_pre_write_operation();
    for (uint32_t idx = 0; idx < BLE_GATT_CACHE_REC_NUM; idx++)
    {
        memset(&gatt_srv_record->gatt_nv_arv_attr[idx], 0xFF, sizeof(GATTC_SRV_ATTR_t));
    }

    gatt_srv_record->wrap_idx = 0;

    nv_record_update_runtime_userdata();
    nv_record_post_write_operation(lock);
}

void aob_gattc_delete_nv_cache(uint8_t *peer_addr, uint32_t svc_uuid)
{
    if (peer_addr == NULL)
    {
        return;
    }

    NV_EXTENSION_RECORD_T *p_nv_rec_ext = nv_record_get_extension_entry_ptr();
    if (NULL == p_nv_rec_ext)
    {
        return;
    }

    uint8_t rec_lid = 0xFF;
    GATTC_NV_SRV_ATTR_t *gatt_srv_record = &(p_nv_rec_ext->srv_cache_attr);
    for (uint32_t idx = 0; idx < BLE_GATT_CACHE_REC_NUM; idx++)
    {
        if (!memcmp(gatt_srv_record->gatt_nv_arv_attr[idx].peer_addr, peer_addr, BD_ADDR_LEN))
        {
            TRACE(1, "%s find record success, peer dev addr:", __func__);
            rec_lid = idx;
            DUMP8("%02x ", peer_addr, BD_ADDR_LEN);
            break;
        }
    }

    if (rec_lid == 0xFF)
    {
        TRACE(1, "%s fail", __func__);
        return;
    }

    uint32_t lock = nv_record_pre_write_operation();

    switch (svc_uuid)
    {
#ifndef BLE_STACK_NEW_DESIGN
        case GATT_SVC_MEDIA_CONTROL:
        case GATT_SVC_GENERIC_MEDIA_CONTROL:
            memset((uint8_t *)&gatt_srv_record->gatt_nv_arv_attr[rec_lid].acc_mcc_cache_data, 0xFF, sizeof(acc_mcc_nv_bond_data_t));
            break;
        case GATT_SVC_TELEPHONE_BEARER:
        case GATT_SVC_GENERIC_TELEPHONE_BEARER:
            memset((uint8_t *)&gatt_srv_record->gatt_nv_arv_attr[rec_lid].acc_tbc_cache_data, 0xFF, sizeof(acc_tbc_nv_bond_data_t));
#endif
            break;
        case GATT_SVC_VOLUME_CONTROL:
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].vcs_cli_cfg_bf                    = 0xFF;
            break;
        case GATT_SVC_AUDIO_STREAM_CTRL:
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].ascs_cli_cfg_bf                   = 0xFF;
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].ascs_ase_cli_cfg_bf               = 0xFF;
            break;
        case GATT_SVC_PUBLISHED_AUDIO_CAPA:
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].pacs_cli_cfg_bf                   = 0xFF;
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].pacs_pac_cli_cfg_bf               = 0xFF;
            break;
        case GATT_SVC_COORD_SET_IDENTIFICATION:
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].csis_cli_cfg_bf                   = 0xFF;
            gatt_srv_record->gatt_nv_arv_attr[rec_lid].csis_set_lid                      = 0xFF;
            break;
        case GATT_SVC_BCAST_AUDIO_SCAN:
        case GATT_SVC_AUDIO_INPUT_CONTROL:
        case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
        case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
        default:
            memset((uint8_t *)&gatt_srv_record->gatt_nv_arv_attr[rec_lid], 0xFF, sizeof(GATTC_SRV_ATTR_t));
            gatt_srv_record->wrap_idx = rec_lid;
    }
    nv_record_update_runtime_userdata();
    nv_record_post_write_operation(lock);
    TRACE(3, "%s success, rec_lid = %d, uuid = %02x", __func__, rec_lid, svc_uuid);
}

void aob_gattc_rebuild_cache(GATTC_NV_SRV_ATTR_t *record)
{

    GATTC_NV_SRV_ATTR_t *gattc_srv_record = record;
    memset((uint8_t *)gattc_srv_record, 0xFF, sizeof(GATTC_NV_SRV_ATTR_t));
    gattc_srv_record->wrap_idx = 0;
    TRACE(2, "gattc cache:rebuild success,start addr:%p, size = %d", record, sizeof(GATTC_NV_SRV_ATTR_t));
}

bool aob_gattc_cache_save(uint8_t *peer_addr, uint32_t svc_uuid, void *gatt_data)
{
    if (NULL == peer_addr || NULL == gatt_data)
    {
        return false;
    }

    TRACE(1, "%s uuid = %02x start, peer dev addr:", __func__, svc_uuid);
    DUMP8("%02x ", peer_addr, BD_ADDR_LEN);

    GATTC_SRV_ATTR_t *gatt_srv_valid_rec = aob_gattc_find_valid_cache(peer_addr, true);
    if (NULL == gatt_srv_valid_rec)
    {
        return false;
    }

    NV_EXTENSION_RECORD_T *p_nv_rec_ext = nv_record_get_extension_entry_ptr();
    if (NULL == p_nv_rec_ext)
    {
        return false;
    }

    uint32_t lock = nv_record_pre_write_operation();

    switch (svc_uuid)
    {
#ifndef BLE_STACK_NEW_DESIGN
        case GATT_SVC_GENERIC_ATTRIBUTE:
        {
            gapc_bond_data_updated_t *p_gapc_bond_data = (gapc_bond_data_updated_t *)gatt_data;
            gatt_srv_valid_rec->gapc_cache_bond_data.gatt_start_hdl = p_gapc_bond_data->gatt_start_hdl;
            gatt_srv_valid_rec->gapc_cache_bond_data.gatt_end_hdl = p_gapc_bond_data->gatt_end_hdl;
            gatt_srv_valid_rec->gapc_cache_bond_data.svc_chg_hdl = p_gapc_bond_data->svc_chg_hdl;
            gatt_srv_valid_rec->gapc_cache_bond_data.srv_feat = p_gapc_bond_data->srv_feat;
            gatt_srv_valid_rec->gapc_cache_bond_data.cli_info = p_gapc_bond_data->cli_info;
            gatt_srv_valid_rec->gapc_cache_bond_data.srv_nb = gatt_user_nb_get(GATT_ROLE_SERVER);
        }
        break;
        case GATT_SVC_MEDIA_CONTROL:
        case GATT_SVC_GENERIC_MEDIA_CONTROL:
        {
            app_gaf_acc_mcc_bond_data_ind_t *gatt_bond_data  = (app_gaf_acc_mcc_bond_data_ind_t *)gatt_data;
            uint8_t media_lid  = gatt_bond_data->media_lid;

            if (ACC_MCS_DFT_MEDIA_NUM <= media_lid || 0 > media_lid)
            {
                TRACE(2, "%s media lid = %d is out of range via [0, ACC_MCS_DFT_MEDIA_NUM)", __func__, media_lid);
                break;
            }

            // First tbs is GMCS ,follows with some tbs in mobile, use dft value ACC_MCS_DFT_MEDIA_NUM
            if (gatt_srv_valid_rec->acc_mcc_cache_data.nb_media == 0xFF)
            {
                gatt_srv_valid_rec->acc_mcc_cache_data.nb_media           = 1;
            }
            else if (gatt_srv_valid_rec->acc_mcc_cache_data.nb_media != ACC_MCS_DFT_MEDIA_NUM)
            {
                ++(gatt_srv_valid_rec->acc_mcc_cache_data.nb_media)      %= ACC_MCS_DFT_MEDIA_NUM + 1;
            }
            else
            {
                TRACE(1, "%s Already get max ACC_MCS_DFT_MEDIA_NUM, just update not add new mcs rec", __func__);
            }

            memcpy((void *)&gatt_srv_valid_rec->acc_mcc_cache_data.mcs_info[media_lid], \
                   (uint8_t *)&gatt_bond_data->mcs_info, \
                   sizeof(nv_acc_mcc_mcs_info_t));
        }
        break;
        case GATT_SVC_TELEPHONE_BEARER:
        case GATT_SVC_GENERIC_TELEPHONE_BEARER:
        {
            app_gaf_acc_tbc_bond_data_ind_t *gatt_bond_data               = (app_gaf_acc_tbc_bond_data_ind_t *)gatt_data;

            uint8_t bearer_lid                                            = gatt_bond_data->bearer_lid;
            if (ACC_TBS_DFT_BEARER_NUM <= bearer_lid || 0 > bearer_lid)
            {
                TRACE(2, "%s bearer lid = %d is out of range via [0, ACC_TBS_DFT_BEARER_NUM)", __func__, bearer_lid);
                break;
            }

            if (gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer == 0xFF)
            {
                gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer          = 1;
            }
            else if (gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer != ACC_TBS_DFT_BEARER_NUM)
            {
                ++(gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer)      %= ACC_TBS_DFT_BEARER_NUM + 1;
            }
            else
            {
                TRACE(1, "%s Already get max ACC_TBS_DFT_BEARER_NUM, just update not add new tbs rec", __func__);
            }

            memcpy((void *)&gatt_srv_valid_rec->acc_tbc_cache_data.tbs_info[bearer_lid], \
                   (uint8_t *)&gatt_bond_data->tbs_info, \
                   sizeof(nv_acc_tbc_tbs_info_t));
        }
        break;
#endif
        case GATT_SVC_VOLUME_CONTROL:
        {
            app_gaf_arc_vcs_bond_data_ind_t *gatt_bond_data               = (app_gaf_arc_vcs_bond_data_ind_t *)gatt_data;
            gatt_srv_valid_rec->vcs_cli_cfg_bf                            = gatt_bond_data->cli_cfg_bf;
        }
        break;
        case GATT_SVC_AUDIO_STREAM_CTRL:
        {
            app_gaf_bap_uc_srv_bond_data_ind_t *gatt_bond_data            = (app_gaf_bap_uc_srv_bond_data_ind_t *)gatt_data;
            gatt_srv_valid_rec->ascs_cli_cfg_bf                           = gatt_bond_data->cli_cfg_bf;
            gatt_srv_valid_rec->ascs_ase_cli_cfg_bf                       = gatt_bond_data->ase_cli_cfg_bf;
        }
        break;
        case GATT_SVC_PUBLISHED_AUDIO_CAPA:
        {
            app_gaf_capa_srv_bond_data_ind_t *gatt_bond_data              = (app_gaf_capa_srv_bond_data_ind_t *)gatt_data;
            gatt_srv_valid_rec->pacs_cli_cfg_bf                           = gatt_bond_data->cli_cfg_bf;
            gatt_srv_valid_rec->pacs_pac_cli_cfg_bf                       = gatt_bond_data->pac_cli_cfg_bf;
        }
        break;
        case GATT_SVC_COORD_SET_IDENTIFICATION:
        {
            app_gaf_atc_csism_bond_data_ind_t *gatt_bond_data             = (app_gaf_atc_csism_bond_data_ind_t *)gatt_data;
            gatt_srv_valid_rec->csis_set_lid                              = gatt_bond_data->set_lid;
            gatt_srv_valid_rec->csis_cli_cfg_bf                           = gatt_bond_data->cli_cfg_bf;
        }
        break;
#ifdef BLE_BATT_ENABLE
        case GATT_SVC_BATTERY_SERVICE:
        {
            gatt_srv_valid_rec->batt_ntf_cfg_bf                           = *(uint8_t *)gatt_data;
        }
        break;
#endif
#ifndef BLE_STACK_NEW_DESIGN
        case GATT_CHAR_DB_HASH:
        {
            uint8_t invalid_hash[GATT_DB_HASH_NV_LEN];
            // 0xff is default value
            memset(invalid_hash, 0xff, GATT_DB_HASH_NV_LEN);
            TRACE(1, "%s database hash:", __func__);
            DUMP8("%02x ", gatt_data, GATT_DB_HASH_LEN);
            DUMP8("%02x ", gatt_srv_valid_rec->database_hash, GATT_DB_HASH_LEN);
            if ((memcmp(invalid_hash, gatt_srv_valid_rec->database_hash,  GATT_DB_HASH_LEN)) &&
                    (memcmp(gatt_data, gatt_srv_valid_rec->database_hash, GATT_DB_HASH_LEN)))
            {
                // databash hash changed means the GATT services have been added,removed or modified
                // So just clear the GATT cache of the peer device as the server
                memset(&gatt_srv_valid_rec->acc_mcc_cache_data, 0xFF, sizeof(acc_mcc_nv_bond_data_t));
                memset(&gatt_srv_valid_rec->acc_tbc_cache_data, 0xFF, sizeof(acc_tbc_nv_bond_data_t));
                memcpy(gatt_srv_valid_rec->peer_addr, peer_addr, BD_ADDR_LEN);
                memcpy(gatt_srv_valid_rec->database_hash, gatt_data, GATT_DB_HASH_LEN);
            }
        }
        break;
#endif
        case GATT_SVC_BCAST_AUDIO_SCAN:
        {
            gatt_srv_valid_rec->bass_cfg_bf                               = *(uint8_t *)gatt_data;
        }
        break;
        case GATT_SVC_AUDIO_INPUT_CONTROL:
        case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
        case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
        default:
            TRACE(1, "%s Err Svc: %04x", __func__, svc_uuid);
            nv_record_post_write_operation(lock);
            return true;
    }

    nv_record_update_runtime_userdata();

    nv_record_post_write_operation(lock);

    return true;
}

POSSIBLY_UNUSED static bool aob_gattc_compare_mcs_caching_from_nv(uint32_t svc_uuid, acc_mcc_nv_bond_data_t *mcc_nv_data, void *mcs_cache_share)
{
    app_gaf_acc_mcc_bond_data_ind_t *mcs_cache_data = (app_gaf_acc_mcc_bond_data_ind_t *)mcs_cache_share;

    if (ACC_MCS_DFT_MEDIA_NUM < mcc_nv_data->nb_media || 0 > mcc_nv_data->nb_media)
    {
        return false;
    }

    /// Whether nv cache is the newest
    if (memcmp(&mcs_cache_data->mcs_info, &mcc_nv_data->mcs_info[mcs_cache_data->media_lid], sizeof(nv_acc_mcc_mcs_info_t)))
    {
        return false;
    }

    return true;
}

POSSIBLY_UNUSED static bool aob_gattc_compare_tbs_caching_from_nv(uint32_t svc_uuid, acc_tbc_nv_bond_data_t *tbs_nv_data, void *tbs_cache_share)
{
    app_gaf_acc_tbc_bond_data_ind_t *tbs_cache_data = (app_gaf_acc_tbc_bond_data_ind_t *)tbs_cache_share;

    if (ACC_TBS_DFT_BEARER_NUM < tbs_nv_data->nb_bearer || 0 > tbs_nv_data->nb_bearer)
    {
        return false;
    }

    /// Whether nv cache is the newest
    if (memcmp(&tbs_cache_data->tbs_info, &tbs_nv_data->tbs_info[tbs_cache_data->bearer_lid], sizeof(nv_acc_tbc_tbs_info_t)))
    {
        return false;
    }

    return true;
}

static bool aob_gattc_is_srv_caching_vaild(uint8_t *peer_addr, uint32_t svc_uuid, void *gatt_data)
{
    GATTC_SRV_ATTR_t *gatt_cache = NULL;

    gatt_cache = aob_gattc_find_valid_cache(peer_addr, false);
    if (gatt_cache == NULL)
    {
        return false;
    }

    bool result = false;
    switch (svc_uuid)
    {
#ifndef BLE_STACK_NEW_DESIGN
        case GATT_SVC_MEDIA_CONTROL:
        case GATT_SVC_GENERIC_MEDIA_CONTROL:
        {
            result = aob_gattc_compare_mcs_caching_from_nv(svc_uuid, &gatt_cache->acc_mcc_cache_data, gatt_data);
            break;
        }
        case GATT_SVC_TELEPHONE_BEARER:
        case GATT_SVC_GENERIC_TELEPHONE_BEARER:
        {
            result = aob_gattc_compare_tbs_caching_from_nv(svc_uuid, &gatt_cache->acc_tbc_cache_data, gatt_data);
            break;
        }
#endif
        default:
            break;
    }

    return result;

#if 0
    if (svc_uuid == GATT_SVC_VOLUME_CONTROL)
    {
        if (gatt_cache->vcs_cli_cfg_bf == 0xFF)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    if (svc_uuid == GATT_SVC_AUDIO_STREAM_CTRL)
    {
        if ((gatt_cache->ascs_cli_cfg_bf == 0xFF) || (gatt_cache->ascs_ase_cli_cfg_bf != NULL))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    if (svc_uuid == GATT_SVC_PUBLISHED_AUDIO_CAPA)
    {
        if ((gatt_cache->pacs_cli_cfg_bf == 0xFF) || (gatt_cache->pacs_pac_cli_cfg_bf == 0xFF))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    if (svc_uuid == GATT_SVC_COORD_SET_IDENTIFICATION)
    {
    }
#endif
    return false;
}

bool aob_gattc_rebuild_new_cache(uint8_t *peer_addr, uint32_t svc_uuid, void *gatt_data)
{
    if (!aob_gattc_is_srv_caching_vaild(peer_addr, svc_uuid, gatt_data))
    {
        TRACE(1, "gattc cache:rebuild caching uuid = 0x%x", svc_uuid);
        aob_gattc_cache_save(peer_addr, svc_uuid, gatt_data);
    }
    else
    {
        TRACE(0, "gattc cache:caching already exist!");
    }

    return true;
}

bool aob_gattc_cache_load(uint8_t con_lid, uint8_t *peer_addr, uint32_t svc_uuid)
{
    bool result = false;

    do
    {
        GATTC_SRV_ATTR_t *gatt_srv_valid_rec = aob_gattc_find_valid_cache(peer_addr, false);
        if (NULL == gatt_srv_valid_rec)
        {
            result = false;
            break;
        }

        switch (svc_uuid)
        {
#ifndef BLE_STACK_NEW_DESIGN
            case GATT_SVC_MEDIA_CONTROL:
            case GATT_SVC_GENERIC_MEDIA_CONTROL:
            {
                uint8_t nb_media = gatt_srv_valid_rec->acc_mcc_cache_data.nb_media;
                if (ACC_MCS_DFT_MEDIA_NUM <  nb_media || 0 >= nb_media)
                {
                    TRACE(1, "%s nb_media = %d", __func__, nb_media);
                    result = false;
                    break;
                }
                DUMP8("%02x ", \
                      (uint8_t *)gatt_srv_valid_rec->acc_mcc_cache_data.mcs_info, \
                      sizeof(nv_acc_mcc_mcs_info_t) * nb_media);

                app_acc_mcc_restore_bond_data_req(con_lid, nb_media, \
                                                  (void *)&gatt_srv_valid_rec->acc_mcc_cache_data.mcs_info[0]);

                TRACE(1, "gattc cache:restore d(%d) nb_media: %d caching success!", con_lid, nb_media);
                result = true;
                break;
            }
            case GATT_SVC_TELEPHONE_BEARER:
            case GATT_SVC_GENERIC_TELEPHONE_BEARER:
            {
                uint8_t nb_bearer = gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer;
                if (ACC_TBS_DFT_BEARER_NUM < nb_bearer || 0 >= nb_bearer)
                {
                    TRACE(1, "%s nb_bearer = %d", __func__, nb_bearer);
                    result = false;
                    break;
                }
                DUMP8("%02x ", \
                      (uint8_t *)gatt_srv_valid_rec->acc_tbc_cache_data.tbs_info, \
                      sizeof(nv_acc_tbc_tbs_info_t) * nb_bearer);

                app_acc_tbc_restore_bond_data_req(con_lid, nb_bearer, \
                                                  (void *)&gatt_srv_valid_rec->acc_tbc_cache_data.tbs_info[0]);

                TRACE(1, "gattc cache:restore d(%d) nb_bearer: %d caching success!", con_lid, nb_bearer);
                result = true;
                break;
            }
#endif
            case GATT_SVC_VOLUME_CONTROL:
                app_arc_vcs_restore_bond_data_req(con_lid, \
                                                  gatt_srv_valid_rec->vcs_cli_cfg_bf);
                TRACE(1, "gattc cache:restore d(%d) vcs_cli_cfg_bf: %x caching success!", con_lid, gatt_srv_valid_rec->vcs_cli_cfg_bf);
                aob_vol_get_real_time_volume(con_lid);
                result = true;
                break;
            case GATT_SVC_AUDIO_STREAM_CTRL:
                app_bap_uc_srv_restore_bond_data_req(con_lid, \
                                                     gatt_srv_valid_rec->ascs_cli_cfg_bf, \
                                                     gatt_srv_valid_rec->ascs_ase_cli_cfg_bf);
                TRACE(1, "gattc cache:restore d(%d) ascs_cli_cfg_bf: %x, ascs_ase_cli_cfg_bf: %x caching success!", con_lid, \
                      gatt_srv_valid_rec->vcs_cli_cfg_bf, gatt_srv_valid_rec->ascs_ase_cli_cfg_bf);
                result = true;
                break;
            case GATT_SVC_PUBLISHED_AUDIO_CAPA:
                app_bap_capa_srv_restore_bond_data_req(con_lid, \
                                                       gatt_srv_valid_rec->pacs_cli_cfg_bf, \
                                                       gatt_srv_valid_rec->pacs_pac_cli_cfg_bf);
                TRACE(1, "gattc cache:restore d(%d) pacs_cli_cfg_bf: %x, pacs_pac_cli_cfg_bf: %x caching success!", con_lid, \
                      gatt_srv_valid_rec->pacs_cli_cfg_bf, gatt_srv_valid_rec->pacs_pac_cli_cfg_bf);
                result = true;
                break;
            case GATT_SVC_COORD_SET_IDENTIFICATION:
                app_atc_csism_restore_bond_data_req(con_lid, \
                                                    gatt_srv_valid_rec->csis_set_lid, \
                                                    /* TODO: default locked value is false*/
                                                    false, \
                                                    gatt_srv_valid_rec->csis_cli_cfg_bf);
                TRACE(1, "gattc cache:restore d(%d) csis_set_lid: %d, csis_cli_cfg_bf: %x caching success!", con_lid, \
                      gatt_srv_valid_rec->csis_set_lid, \
                      gatt_srv_valid_rec->csis_cli_cfg_bf);
                result = true;
                break;
#ifdef BLE_BATT_ENABLE
            case GATT_SVC_BATTERY_SERVICE:
                if (gatt_srv_valid_rec->batt_ntf_cfg_bf == 0xFF)
				{
                    break;
                }
                app_batt_srv_restore_bond_data_req(con_lid, gatt_srv_valid_rec->batt_ntf_cfg_bf);
                result = true;
                break;
#endif
            case GATT_SVC_BCAST_AUDIO_SCAN:
            {
                if (gatt_srv_valid_rec->bass_cfg_bf == 0xFF)
				{
                    break;
                }
#if APP_BLE_BIS_ASSIST_ENABLE
                app_bap_bc_deleg_restore_bond_data_req(con_lid, gatt_srv_valid_rec->bass_cfg_bf);
                result = true;
#else
                result = false;
#endif
            }
            break;
            case GATT_SVC_AUDIO_INPUT_CONTROL:
            case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
            case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
            default:
                result = false;
        }

        TRACE(2, "gattc cache:load device %d, %s uuid: %04x", con_lid, result ? "success" : "failed", svc_uuid);
        break;
    } while (1);

    return result;
}

bool aob_gattc_is_cache_item_exist(uint8_t *peer_addr, uint32_t svc_uuid)
{
    bool result = true;

    do
    {
        GATTC_SRV_ATTR_t *gatt_srv_valid_rec = aob_gattc_find_valid_cache(peer_addr, false);
        if (NULL == gatt_srv_valid_rec)
        {
            result = false;
            break;
        }

        switch (svc_uuid)
        {
#ifndef BLE_STACK_NEW_DESIGN
            case GATT_SVC_MEDIA_CONTROL:
            case GATT_SVC_GENERIC_MEDIA_CONTROL:
            {
                uint8_t nb_media = gatt_srv_valid_rec->acc_mcc_cache_data.nb_media;
                if (ACC_MCS_DFT_MEDIA_NUM <  nb_media || 0 >= nb_media)
                {
                    result = false;
                }
                break;
            }
            case GATT_SVC_TELEPHONE_BEARER:
            case GATT_SVC_GENERIC_TELEPHONE_BEARER:
            {
                uint8_t nb_bearer = gatt_srv_valid_rec->acc_tbc_cache_data.nb_bearer;
                if (ACC_TBS_DFT_BEARER_NUM < nb_bearer || 0 >= nb_bearer)
                {
                    result = false;
                }
                break;
            }
#endif
            case GATT_SVC_VOLUME_CONTROL:
                if (gatt_srv_valid_rec->vcs_cli_cfg_bf == 0xFF)
                {
                    result = false;
                }
                break;
            case GATT_SVC_AUDIO_STREAM_CTRL:
                if (gatt_srv_valid_rec->ascs_cli_cfg_bf == 0xFF ||
                        gatt_srv_valid_rec->ascs_ase_cli_cfg_bf == 0xFF)
                {
                    result = false;
                }
                break;
            case GATT_SVC_PUBLISHED_AUDIO_CAPA:
                if (gatt_srv_valid_rec->pacs_cli_cfg_bf == 0xFF ||
                        gatt_srv_valid_rec->pacs_pac_cli_cfg_bf == 0xFF)
                {
                    result = false;
                }
                break;
            case GATT_SVC_COORD_SET_IDENTIFICATION:
                if (gatt_srv_valid_rec->csis_set_lid == 0xFF ||
                        gatt_srv_valid_rec->csis_cli_cfg_bf == 0xFF)
                {
                    result = false;
                }
                break;
#ifdef BLE_BATT_ENABLE
            case GATT_SVC_BATTERY_SERVICE:
                if (gatt_srv_valid_rec->batt_ntf_cfg_bf == 0xFF)
                {
                    result = false;
                }
                break;
#endif
            case GATT_SVC_BCAST_AUDIO_SCAN:
            case GATT_SVC_AUDIO_INPUT_CONTROL:
            case GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT:
            case GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT:
            default:
                result = false;
        }

        TRACE(2, "gattc cache:check cache item is %sexist, uuid: %04x", result ? "" : "not ", svc_uuid);
        break;
    } while (1);

    return result;
}

void aob_gattc_display_cache_server(void)
{
    //if (!aob_gattc_support_nv_save_caching())
    //{
    //    return;
    //}

    NV_EXTENSION_RECORD_T *p_nv_rec_ext = nv_record_get_extension_entry_ptr();
    if (NULL == p_nv_rec_ext)
    {
        return;
    }

    uint8_t empty_addr[BD_ADDR_LEN];
    memset(empty_addr, 0xFF, BD_ADDR_LEN);
    GATTC_NV_SRV_ATTR_t *gatt_srv_record = &(p_nv_rec_ext->srv_cache_attr);
    for (uint32_t i = 0; i < BLE_GATT_CACHE_REC_NUM; i ++)
    {
        if (!memcmp(gatt_srv_record->gatt_nv_arv_attr[i].peer_addr, empty_addr, BD_ADDR_LEN))
        {
            continue;
        }
        TRACE(1, "DUMP CACHE START, RECORD ADDR:");
        DUMP8("%02x ", gatt_srv_record->gatt_nv_arv_attr[i].peer_addr, BD_ADDR_LEN);
#ifndef BLE_STACK_NEW_DESIGN
        /// MCS
        TRACE(1, "gattc cache:gattc_display nb_media = %d, dump sz = 2 * %d", \
              gatt_srv_record->gatt_nv_arv_attr[i].acc_mcc_cache_data.nb_media, sizeof(nv_acc_mcc_mcs_info_t));
        DUMP8("%02x ", \
              (uint8_t *)&gatt_srv_record->gatt_nv_arv_attr[i].acc_mcc_cache_data.mcs_info[0], \
              ACC_MCS_DFT_MEDIA_NUM * sizeof(nv_acc_mcc_mcs_info_t));
        /// TBS
        TRACE(1, "gattc cache:gattc_display nb_bearer = %d, dump sz = 2 * %d", \
              gatt_srv_record->gatt_nv_arv_attr[i].acc_tbc_cache_data.nb_bearer, sizeof(nv_acc_mcc_mcs_info_t));
        DUMP8("%02x ", \
              (uint8_t *)&gatt_srv_record->gatt_nv_arv_attr[i].acc_tbc_cache_data.tbs_info[0], \
              ACC_TBS_DFT_BEARER_NUM * sizeof(nv_acc_tbc_tbs_info_t));
#endif
        /// VCS
        TRACE(1, "gattc cache:gattc_display vcs_cli_cfg_bf = %x", gatt_srv_record->gatt_nv_arv_attr[i].vcs_cli_cfg_bf);
        /// ASCS
        TRACE(2, "gattc cache:gattc_display ascs_cli_cfg_bf = %x, ascs_ase_cli_cfg_bf = %x", \
              gatt_srv_record->gatt_nv_arv_attr[i].ascs_cli_cfg_bf, \
              gatt_srv_record->gatt_nv_arv_attr[i].ascs_ase_cli_cfg_bf);
        /// PACS
        TRACE(2, "gattc cache:gattc_display pacs_cli_cfg_bf = %x, pacs_pac_cli_cfg_bf = %x", \
              gatt_srv_record->gatt_nv_arv_attr[i].pacs_cli_cfg_bf, \
              gatt_srv_record->gatt_nv_arv_attr[i].pacs_pac_cli_cfg_bf);
        /// CSIS
        TRACE(2, "gattc cache:gattc_display csis_cli_cfg_bf = %x, csis_set_lid = %d", \
              gatt_srv_record->gatt_nv_arv_attr[i].csis_cli_cfg_bf, \
              gatt_srv_record->gatt_nv_arv_attr[i].csis_set_lid);

        // TODO: DUMP other svc cache data
    }
}

void aob_gattc_caching_unit_test()
{
#ifndef BLE_STACK_NEW_DESIGN
    /// Service bond data cache test
    uint8_t idx = 0;
    app_gaf_acc_mcc_bond_data_ind_t acc_mcc_bond_data;
    acc_mcc_bond_data.con_lid = 0;
    acc_mcc_bond_data.mcs_info.uuid = GATT_SVC_GENERIC_MEDIA_CONTROL;

    for (; idx < ACC_MCS_DFT_MEDIA_NUM; idx++)
    {
        acc_mcc_bond_data.media_lid = idx;
        acc_mcc_bond_data.mcs_info.svc_info.shdl = (idx + 1) * 100;
        acc_mcc_bond_data.mcs_info.svc_info.ehdl = (idx + 2) * 100 + APP_GAF_MC_CHAR_TYPE_MAX;

        for (uint32_t i = APP_GAF_MC_CHAR_TYPE_PLAYER_NAME; i < APP_GAF_MC_CHAR_TYPE_MAX; i ++)
        {
            acc_mcc_bond_data.mcs_info.char_info[i].val_hdl = i + (idx + 1) * 100;
        }

        for (uint32_t j = APP_GAF_MC_DESC_TYPE_CCC_PLAYER_NAME; j < APP_GAF_MC_DESC_TYPE_MAX; j ++)
        {
            acc_mcc_bond_data.mcs_info.desc_info[j].desc_hdl = j + (idx + 2) * 100;
        }

        aob_gattc_cache_save((uint8_t *)"\x00\x11\x22\x33\x44\x55", GATT_SVC_GENERIC_MEDIA_CONTROL, &acc_mcc_bond_data);

        GATTC_SRV_ATTR_t *p_rec = aob_gattc_find_valid_cache((uint8_t *)"\x00\x11\x22\x33\x44\x55", false);

        ASSERT(NULL != p_rec, "save instance is not exist, Line:%d", __LINE__);

        acc_mcc_nv_bond_data_t *p_acc_record = &p_rec->acc_mcc_cache_data;

        ASSERT(p_acc_record->nb_media != 0xFF, "nb_media Exp, Line:%d", __LINE__);

        DUMP8("%02x ", &p_acc_record->mcs_info[idx], sizeof(nv_acc_mcc_mcs_info_t));
        ASSERT(p_acc_record->mcs_info[idx].uuid == GATT_SVC_GENERIC_MEDIA_CONTROL, "uuid Exp, Line:%d", __LINE__);
        ASSERT(p_acc_record->mcs_info[idx].svc_info.shdl == (idx + 1) * 100, "start handle Exp, Line:%d", __LINE__);
        ASSERT(p_acc_record->mcs_info[idx].svc_info.ehdl == (idx + 2) * 100 + APP_GAF_MC_CHAR_TYPE_MAX, "end handle Exp, Line:%d", __LINE__);

        for (uint32_t i = APP_GAF_MC_CHAR_TYPE_PLAYER_NAME; i < APP_GAF_MC_CHAR_TYPE_MAX; i ++)
        {
            ASSERT(p_acc_record->mcs_info[idx].char_info[i].val_hdl == (i + (idx + 1) * 100), "char type Exp, Line = %d", __LINE__);
        }

        for (uint32_t j = APP_GAF_MC_DESC_TYPE_CCC_PLAYER_NAME; j < APP_GAF_MC_DESC_TYPE_MAX; j ++)
        {
            ASSERT(p_acc_record->mcs_info[idx].desc_info[j].desc_hdl == (j + (idx + 2) * 100),
                   "desc type Exp, %d, %d, Line = %d", p_acc_record->mcs_info[0].desc_info[j].desc_hdl, (j + (idx + 2) * 100), __LINE__);
        }
    }
#endif

    /// Client characteristic configuration cache test
    app_gaf_bap_uc_srv_bond_data_ind_t ascs_bond_data;
    ascs_bond_data.cli_cfg_bf = 0x01;
    ascs_bond_data.ase_cli_cfg_bf = 0x01;
    aob_gattc_cache_save((uint8_t *)"\x00\x11\x22\x33\x44\x55", GATT_SVC_AUDIO_STREAM_CTRL, &ascs_bond_data);

    GATTC_SRV_ATTR_t *p_rec = aob_gattc_find_valid_cache((uint8_t *)"\x00\x11\x22\x33\x44\x55", false);

    ASSERT(NULL != p_rec, "save instance is not exist, Line:%d", __LINE__);

    ASSERT(0xFF != p_rec->ascs_cli_cfg_bf, "save ascs_cli_cfg_bf Exp, Line:%d", __LINE__);
    ASSERT(0xFF != p_rec->ascs_ase_cli_cfg_bf, "save ascs_ase_cli_cfg_bf Exp, Line:%d", __LINE__);

#ifndef BLE_STACK_NEW_DESIGN
    /// Delete cache test
    aob_gattc_delete_nv_cache((uint8_t *)"\x00\x11\x22\x33\x44\x55", GATT_SVC_GENERIC_MEDIA_CONTROL);
    aob_gattc_delete_nv_cache((uint8_t *)"\x00\x11\x22\x33\x44\x55", GATT_SVC_AUDIO_STREAM_CTRL);

    p_rec = aob_gattc_find_valid_cache((uint8_t *)"\x00\x11\x22\x33\x44\x55", false);

    ASSERT(p_rec->acc_mcc_cache_data.nb_media == 0xFF, "nb_media Exp, Line:%d", __LINE__);
    ASSERT(p_rec->acc_mcc_cache_data.mcs_info[0].svc_info.shdl == 0xFFFF, "svc start hdl Exp, Line:%d", __LINE__);
    ASSERT(p_rec->acc_mcc_cache_data.mcs_info[0].svc_info.ehdl == 0xFFFF, "svc end hdl Exp, Line:%d", __LINE__);
    ASSERT(p_rec->acc_mcc_cache_data.mcs_info[1].svc_info.shdl == 0xFFFF, "svc start hdl Exp, Line:%d", __LINE__);
    ASSERT(p_rec->acc_mcc_cache_data.mcs_info[1].svc_info.ehdl == 0xFFFF, "svc end hdl Exp, Line:%d", __LINE__);
    ASSERT(p_rec->ascs_cli_cfg_bf == 0xFF, "ascs_cli_cfg_bf Exp, Line:%d", __LINE__);
    ASSERT(p_rec->ascs_ase_cli_cfg_bf == 0xFF, "ascs_ase_cli_cfg_bf Exp, Line:%d", __LINE__);
#endif
}
