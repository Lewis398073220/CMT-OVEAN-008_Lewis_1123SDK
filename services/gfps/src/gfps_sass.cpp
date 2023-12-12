/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#ifdef SASS_ENABLED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_bt.h"
#include "app_bt_func.h"
#ifdef IBRT
#include "app_tws_ibrt.h"
#include "app_ibrt_internal.h"
#include "earbud_profiles_api.h"
#include "earbud_ux_api.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#endif
#include "bluetooth.h"
#include "app_media_player.h"
#include "gfps_sass.h"
#include "bluetooth_ble_api.h"
#include "app_hfp.h"
#include "app_bt_media_manager.h"
#include "../utils/encrypt/aes.h"
#include "nvrecord_fp_account_key.h"
#include "audio_policy.h"
#include "app_audio_control.h"
#include "app_ui_api.h"

#include "gfps.h"

SassConnInfo sassInfo;
SassStateInfo sassAdv;

#ifdef __cplusplus
extern "C" {
#endif
bool gfps_sass_select_steal_device(const bt_bdaddr_t *conn_req_addr, bt_bdaddr_t *steal_addr);
#ifdef __cplusplus
}
#endif

void gfps_sass_update_active_info(uint8_t device_id);
void gfps_sass_switched_callback(uint8_t selectedId);
bool gfps_sass_is_need_ntf_status(uint8_t device_id);
bool gfps_sass_ntf_conn_status(uint8_t devId, bool isUseAdv, uint8_t *state);
void gfps_sass_update_switch_dev(uint8_t oldActive, uint8_t newActive);
void gfps_sass_set_resume_dev(uint8_t devId);
uint8_t gfps_sass_get_resume_dev(void);

POSSIBLY_UNUSED static ibrt_ext_conn_policy_cb_t sass_ibrt_callback = {
    .accept_incoming_conn_req_hook      = NULL,
    .accept_extra_incoming_conn_req_hook= gfps_sass_select_steal_device,
    .disallow_start_reconnect_hook      = NULL,
};

void gfps_sass_init()
{
    memset((void *)&sassInfo, 0, sizeof(SassConnInfo));
    sassInfo.reconnInfo.evt = 0xFF;
    sassInfo.activeId = 0xFF;
    sassInfo.lastActId = 0xFF;
    sassInfo.headState = SASS_HEAD_STATE_OFF;
    sassInfo.connAvail = SASS_CONN_AVAILABLE;
    sassInfo.preference = SASS_HFP_VS_A2DP_BIT;
    sassInfo.config.acceptNew = false;
    sassInfo.config.resume = true;
    sassInfo.config.resumeId = 0xFF;
#ifdef IBRT_V2_MULTIPOINT
    sassInfo.isMulti = true;
#endif
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        sassInfo.connInfo[i].connId = 0xFF;
        sassInfo.connInfo[i].devType = SASS_DEV_TYPE_INVALID;
    }

    memset((void *)&sassAdv, 0, sizeof(SassStateInfo));
    sassAdv.lenType = (3 << 4) + SASS_CONN_STATE_TYPE;
    SET_SASS_STATE(sassAdv.state, HEAD_ON, 1);
    SET_SASS_STATE(sassAdv.state, CONN_AVAILABLE, 1);

    app_ui_register_ext_conn_policy_callback(&sass_ibrt_callback);
    app_bt_audio_switch_a2dp_cmp_cb_init(gfps_sass_switched_callback);

    TRACE(1, "%s", __func__);
}

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
void gfps_sass_get_sync_info(uint8_t *buf, uint16_t *len)
{
    SassConnInfo *info = (SassConnInfo *)buf;
    memcpy((uint8_t *)info, (uint8_t *)&sassInfo, sizeof(SassConnInfo));
    *len = sizeof(SassConnInfo);
}

void gfps_sass_set_sync_info(uint8_t *buf, uint16_t len)
{
    SassConnInfo *info = (SassConnInfo *)buf;
    SassBtInfo temp[BT_DEVICE_NUM];
    bool updateHead = false;
    if (info->headState != sassInfo.headState)
    {
        updateHead = true;
    }

    info->headState = sassInfo.headState;
    memcpy((uint8_t *)&temp, (uint8_t *)&(sassInfo.connInfo), sizeof(SassBtInfo) * BT_DEVICE_NUM);
    memcpy((uint8_t *)&sassInfo, buf, sizeof(SassConnInfo));

    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (info->connInfo[i].connId != 0xFF)
        {
            for(int j= 0; j < BT_DEVICE_NUM; j++)
            {
                if (!memcmp((void *)temp[j].btAddr.address, \
                    (void *)info->connInfo[i].btAddr.address, sizeof(bt_bdaddr_t)))
                {
                    TRACE(3, "%s master id:%d, slave id:%d", __func__, sassInfo.connInfo[j].connId, temp[j].connId);
                    sassInfo.connInfo[j].connId = temp[j].connId;
                    break;
                }
            }
        }
    }

    if (updateHead)
    {
        gfps_sass_update_head_state(sassInfo.headState);
    }
}

void gfps_sass_sync_info(void)
{
    SassConnInfo info = {0};
    uint16_t len = 0;
    gfps_sass_get_sync_info((uint8_t *)&info, &len);
    tws_ctrl_send_cmd(APP_TWS_CMD_SEND_SASS_INFO, (uint8_t *)&info, len);

    TRACE(2, "%s len:%d", __func__, len);
}

void gfps_sass_role_switch_prepare(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (TWS_UI_SLAVE != app_ibrt_if_get_ui_role()&& p_ibrt_ctrl->init_done)
    {
        gfps_sass_sync_info();
    }
}
#endif

void gfps_sass_gen_session_nonce(uint8_t device_id)
{
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (sassInfo.connInfo[i].connId == device_id)
        {
            for(int n = 0; n < SESSION_NOUNCE_NUM; n++)
            {
                 sassInfo.connInfo[i].session[n] = (uint8_t)rand();
            }
            TRACE(1, "sass dev %d session nounce is:", device_id);
            DUMP8("%02x ", sassInfo.connInfo[i].session, 8);
            break;
        }
    }
}

bool gfps_sass_get_session_nonce(uint8_t device_id, uint8_t *session)
{
    bool ret = false;
    uint8_t zero[SESSION_NOUNCE_NUM] = {0};
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if ((sassInfo.connInfo[i].connId == device_id) && (memcmp(sassInfo.connInfo[i].session, zero, SESSION_NOUNCE_NUM)))
        {
            memcpy(session, sassInfo.connInfo[i].session, SESSION_NOUNCE_NUM);
            ret = true;
            break;
        }
    }

    return ret;
}

SassBtInfo *gfps_sass_get_free_handler()
{
    SassBtInfo *handler = NULL;
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (sassInfo.connInfo[i].connId == 0xFF)
        {
            handler = &(sassInfo.connInfo[i]);
            break;
        }
    }
    return handler;
}

SassBtInfo *gfps_sass_get_connected_dev(uint8_t id)
{
    SassBtInfo *handler = NULL;
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (sassInfo.connInfo[i].connId == id)
        {
            handler = &(sassInfo.connInfo[i]);
            break;
        }
    }
    return handler;
}

SassBtInfo *gfps_sass_get_connected_dev_by_addr(const bt_bdaddr_t *addr)
{
    SassBtInfo *handler = NULL;
    if (addr == NULL)
    {
        return NULL;
    }

    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (!memcmp(sassInfo.connInfo[i].btAddr.address, addr->address, sizeof(bt_bdaddr_t)))
        {
            handler = &(sassInfo.connInfo[i]);
            break;
        }
    }
    return handler;
}

SassBtInfo *gfps_sass_get_other_connected_dev(uint8_t id)
{
    SassBtInfo *info = NULL;
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if ((sassInfo.connInfo[i].connId != id) && (sassInfo.connInfo[i].connId != 0xFF))
        {
            info = (SassBtInfo *)&(sassInfo.connInfo[i]);
            break;
        }
    }
    return info;
}

bool gfps_sass_is_other_sass_dev(uint8_t id)
{
    bool ret = false;
    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(id);
    if (otherInfo)
    {
        ret = gfps_sass_is_sass_dev(otherInfo->connId);
    }
    return ret;
}

bool gfps_sass_is_key_valid(uint8_t *accKey)
{
    uint8_t empty[FP_ACCOUNT_KEY_SIZE] = {0};
    bool ret = false;
    if (!accKey)
        return false;

    if (memcmp(accKey, empty, FP_ACCOUNT_KEY_SIZE) && \
        (accKey[0] == SASS_NOT_IN_USE_ACCOUNT_KEY))
    {
        ret = true;
    }
    return ret;
}

void gfps_sass_get_adv_data(uint8_t *buf, uint8_t *len)
{
    *len =  (sassAdv.lenType >> 4) + 1;
    memcpy(buf, (uint8_t *)&sassAdv, *len);
    TRACE(1, "sass adv data, len:%d", *len);
    DUMP8("%02x ", buf, *len);
}

#ifdef SASS_SECURE_ENHACEMENT
void gfps_sass_encrypt_connection_state(uint8_t *iv,  uint8_t *inUseKey, 
                                            uint8_t *outputData, uint8_t *dataLen, bool LT, 
                                            bool isUseAdv, uint8_t *inputData)
{
    uint8_t sassBuf[SASS_ADV_LEN_MAX] = {0};//{0x35, 0x85, 0x38, 0x09};
    uint8_t sassLen = 0;
    uint8_t hkdfKey[16] = {0};
    char str[13] = "SASS-RRD-KEY";

    inUseKey[0] = NONE_IN_USET_ACCOUNT_KEY_HEADER;
    // HKDF(accountkey, NULL, UTF8("SASS-RRD-KEY"), 16);
    gfps_hdkf(NULL, 0, inUseKey, 16, (uint8_t *)str, 12, hkdfKey, 16);
    //TRACE(0, "gfps_hdkf is:");
    //DUMP8("%2x ", hkdfKey, 16);

    if (isUseAdv || !inputData)
    {
        gfps_sass_get_adv_data(sassBuf, &sassLen);
    }
    else
    {
        memcpy(sassBuf, inputData, SASS_ADV_LEN_MAX);
        sassLen = SASS_ADV_LEN_MAX;
    }

    if (LT)
    {
        AES128_CTR_encrypt_buffer(sassBuf, sassLen, hkdfKey, iv, &(outputData[1]));
        outputData[0] = (sassLen << 4) + APP_GFPS_RANDOM_RESOLVABLE_DATA_TYPE;
        *dataLen = (sassLen + 1);
    }
    else
    {
        AES128_CTR_encrypt_buffer(sassBuf + 1, sassLen-1, hkdfKey, iv, outputData);
        *dataLen = (sassLen - 1);
    }

}
#else
void gfps_sass_encrypt_adv_data(uint8_t *FArray, uint8_t sizeOfFilter, uint8_t *inUseKey, 
                                               uint8_t *outputData, uint8_t *dataLen,
                                               bool isUseAdv, uint8_t *inputData)
{
    uint8_t sassBuf[4] = {0};//{0x35, 0x85, 0x38, 0x09};
    uint8_t outBuf[16 + 1] = {0};
    uint8_t iv[16] = {0}; //{0x8c, 0xa9, 0X0c, 0x08, 0x1c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t sassLen = 0;
    uint8_t tempLen = *dataLen;

    if (isUseAdv || !inputData)
    {
        gfps_sass_get_adv_data(sassBuf, &sassLen);
    }
    else
    {
        memcpy(sassBuf, inputData, SASS_ADV_LEN_MAX);
    }

    memcpy(iv, FArray, sizeOfFilter);
    AES128_CTR_encrypt_buffer(sassBuf, sassLen, inUseKey, iv, outBuf + 1);
    TRACE(0, "encrypt connection state is:");
    DUMP8("%2x ", outBuf + 1, sassLen);
    outBuf[0] = (sassLen << 4) + APP_GFPS_RANDOM_RESOLVABLE_DATA_TYPE;
    memcpy(outputData + tempLen, outBuf, sassLen + 1);
    tempLen += (sassLen + 1);
    *dataLen = tempLen;
}

#endif

void gfps_sass_update_adv_data()
{
    uint8_t tempDev = 0;
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        if (sassInfo.connInfo[i].connId != 0xFF)
        {
            if ((tempDev & SASS_DEV_TYPE_PHONEA) && \
                (sassInfo.connInfo[i].devType == SASS_DEV_TYPE_PHONEA))
            {
                tempDev |= SASS_DEV_TYPE_PHONEB;
            }
            else if (sassInfo.connInfo[i].devType != SASS_DEV_TYPE_INVALID)
            {
                tempDev |= sassInfo.connInfo[i].devType;
            }else
            {
            }
        }
    }
    sassAdv.devBitMap = tempDev;

    sassAdv.state = 0;
    SET_SASS_STATE(sassAdv.state, HEAD_ON, sassInfo.headState);
    SET_SASS_STATE(sassAdv.state, CONN_AVAILABLE, sassInfo.connAvail);
    SET_SASS_STATE(sassAdv.state, FOCUS_MODE, sassInfo.focusMode);
    SET_SASS_STATE(sassAdv.state, AUTO_RECONN, sassInfo.autoReconn);
    SET_SASS_CONN_STATE(sassAdv.state, CONN_STATE, sassInfo.connState);
}

void gfps_sass_set_sass_mode(uint8_t device_id, uint16_t sassVer, bool state)
{
    SassBtInfo *sInfo;
    //uint8_t empty[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t sKey[FP_ACCOUNT_KEY_SIZE] = {0};
    //bt_bdaddr_t invalidAddr = {0};
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        sInfo = &(sassInfo.connInfo[i]);
        if (sInfo->connId == device_id)
        {
            sInfo->ntfSassMode = true;
            if (!sassVer || !state) {
                sInfo->isSass = false;
            } else {
                sInfo->isSass = true;
            }
            TRACE(2, "sass ver:0x%4x, state:%d", sassVer, state);
            if (!state)
            {
                nv_record_fp_get_key_by_addr(sInfo->btAddr.address, sKey);
                nv_record_fp_delete_addr(sInfo->btAddr.address, sKey);
 
                if (!memcmp(sassInfo.inUseAddr.address, sInfo->btAddr.address, sizeof(bt_bdaddr_t)) || \
                    !memcmp(sassInfo.inuseKey, sInfo->accKey, FP_ACCOUNT_KEY_SIZE))
                {
                    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(device_id);
                    if (otherInfo && otherInfo->isSass && otherInfo->ntfSassMode)
                    {
                        if (gfps_sass_is_key_valid(otherInfo->accKey))
                        {
                            gfps_sass_set_inuse_acckey(otherInfo->accKey, &(otherInfo->btAddr));
                        }
                    }
                }
                memset(sInfo->accKey, 0, FP_ACCOUNT_KEY_SIZE);
            }
            else
            {
                if (gfps_sass_is_need_ntf_status(device_id))
                {
                    gfps_sass_ntf_conn_status(device_id, true, NULL);
                }
            }
            break;
        }
   }
}

//workaround for the rfcomm event delayed transport
void gfps_sass_set_default_sass_mode(uint8_t devId)
{
    uint8_t devKey[FP_ACCOUNT_KEY_SIZE] = {0};
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(devId);
    if (!sInfo)
    {
        return;
    }

    nv_record_fp_get_key_by_addr(sInfo->btAddr.address, devKey);
    if (gfps_sass_is_key_valid(devKey))
    {
        sInfo->isSass = true;
        if (!gfps_sass_is_key_valid(sInfo->accKey))
        {
            memcpy(sInfo->accKey, devKey, FP_ACCOUNT_KEY_SIZE);
        }
    }
    else
    {
        sInfo->isSass = false;
    }
}

void gfps_sass_set_multi_status(const bt_bdaddr_t               *addr, bool isMulti)
{
    SassEvtParam evtParam;
    SASS_CONN_AVAIL_E availstate;
    uint8_t num = app_ibrt_if_get_connected_mobile_count();
    uint8_t total = isMulti ? BT_DEVICE_NUM : 1;
    TRACE(3, "%s num:%d, total:%d", __func__, num, total);
    sassInfo.isMulti = isMulti;
    if (addr)
    {
        evtParam.devId = SET_BT_ID(app_bt_get_device_id_byaddr((bt_bdaddr_t *)addr));
    }
    else
    {
        evtParam.devId = 0;
    }
    
    evtParam.event = SASS_EVT_UPDATE_MULTI_STATE;
    if (num >= total)
    {
        availstate = SASS_CONN_NONE_AVAILABLE;
    }
    else
    {
        availstate = SASS_CONN_AVAILABLE;
    }
    evtParam.state.connAvail = availstate;
    gfps_sass_update_state(&evtParam);
}

bool gfps_sass_get_multi_status()
{
    TRACE(1, "sass is in multi-point state ? %d", sassInfo.isMulti);
    return sassInfo.isMulti;
}

void gfps_sass_switch_max_link(uint8_t device_id, uint8_t type)
{
    bool leAud = false;
#if BLE_AUDIO_ENABLED
    leAud = true;
#endif
    bt_bdaddr_t *mobile_addr = &(app_bt_get_device(GET_BT_ID(device_id))->remote);

    TRACE(3,"%s, type:%d, id:%d", __func__, type, GET_BT_ID(device_id));
    if ((type == SASS_LINK_SWITCH_TO_SINGLE_POINT) && (app_ibrt_if_get_connected_mobile_count() > 1))
    {
        struct BT_DEVICE_T *revDevice = NULL;
        for(int i = 0; i < BT_DEVICE_NUM; i++)
        {
            revDevice = app_bt_get_device(i);
            if (revDevice->acl_is_connected)
            {
                AppIbrtCallStatus callState;
                AppIbrtA2dpState a2dpState;
                app_ibrt_if_get_a2dp_state(&(revDevice->remote), &a2dpState);
                app_ibrt_if_get_hfp_call_status(&(revDevice->remote), &callState);
                if ((callState != IBRT_PROFILE_NO_CALL) || (a2dpState == IBRT_PROFILE_A2DP_STREAMING))
                {
                    mobile_addr = &(revDevice->remote);
                    break;
                }
            }
        }
    }

    if (type == SASS_LINK_SWITCH_TO_MULTI_POINT)
    {
        app_ui_change_mode_ext(leAud, true, mobile_addr);
    }
    else
    {
        app_ui_change_mode_ext(leAud, false, mobile_addr);
    }
}

bool gfps_sass_is_any_dev_connected()
{
    bool ret = false;
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        if (sassInfo.connInfo[i].connId != 0xFF) {
            ret = true;
            break;
        }
    }
    return ret;
}

bool gfps_sass_check_sass_mode(SassBtInfo *sInfo)
{
    bool ret = false;
    if (!sInfo)
    {
        return false;
    }

    if (sInfo->ntfSassMode && sInfo->isSass)
    {
        ret = true;
    }
    else if (sInfo->ntfSassMode && !sInfo->isSass)
    {
        ret = false;
    }
    else
    {
        uint8_t devKey[FP_ACCOUNT_KEY_SIZE] = {0};
        nv_record_fp_get_key_by_addr(sInfo->btAddr.address, devKey);
        if (gfps_sass_is_key_valid(devKey))
        {
            sInfo->isSass = true;
            ret = true;
            if (!gfps_sass_is_key_valid(sInfo->accKey))
            {
                memcpy(sInfo->accKey, devKey, FP_ACCOUNT_KEY_SIZE);
            }
        }
        else
        {
            sInfo->isSass = false;
            ret = false;
        }
    }
    return ret;
}

bool gfps_sass_is_sass_dev(uint8_t device_id)
{
    bool ret = false;
    SassBtInfo *sInfo;
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        sInfo = &(sassInfo.connInfo[i]);
        if (sInfo->connId == device_id)
        {
            ret = sInfo->isSass;
            break;
        }
    }
    return ret;
}

bool gfps_sass_is_truely_sass_dev(uint8_t device_id)
{
    bool ret = false;
    SassBtInfo *sInfo;
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        sInfo = &(sassInfo.connInfo[i]);
        if (sInfo->connId == device_id)
        {
            if (sInfo->isSass && sInfo->ntfSassMode)
            {
                ret = true;
            }
            break;
        }
    }
    return ret;
}

bool gfps_sass_is_there_sass_dev()
{
    bool ret = false;
    SassBtInfo *sInfo;
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        sInfo = &(sassInfo.connInfo[i]);
        if ((sInfo->connId != 0xFF) && sInfo->isSass)
        {
            ret = true;
            break;
        }
    }

    TRACE(1, "there is a sass dev ? %d", ret);
    return ret;
}

bool gfps_sass_is_there_in_use_dev()
{
    bool ret = false;
    SassBtInfo *sInfo;
    uint8_t devKey[16] = {0};
    for(int i = 0; i < BT_DEVICE_NUM; i++)
    {
        sInfo = &(sassInfo.connInfo[i]);
        if (sInfo->connId != 0xFF)
        {
            nv_record_fp_get_key_by_addr(sInfo->btAddr.address, devKey);
            if (sInfo->isSass || 
                (gfps_sass_is_key_valid(devKey) && \
                !memcmp(sassInfo.inuseKey, devKey, FP_ACCOUNT_KEY_SIZE)))
            {
                ret = true;
                break;
            }
        }
    }

    TRACE(1, "there is a in use key ? %d", ret);
    return ret;
}

void gfps_sass_set_custom_data(uint8_t data)
{
    sassAdv.cusData = data;
}

void gfps_sass_get_cap_data(uint8_t *buf, uint32_t *len)
{
    uint16_t sass_ver = SASS_VERSION;
    uint16_t capbility = 0;

    capbility |= SASS_STATE_ON_BIT;
#ifdef SUPPORT_DYNAMIC_MULTIPOINT
    capbility |= SASS_MULTIPOINT_BIT;
#endif
    if (gfps_sass_get_multi_status())
    {
        capbility |= SASS_MULTIPOINT_ON_BIT;
    }
    capbility |= SASS_ON_HEAD_BIT;
    if (gfps_sass_get_head_state())
    {
        capbility |= SASS_ON_HEAD_ON_BIT;
    }
    *len = NTF_CAP_LEN;
    buf[0] = (sass_ver >> 8) & 0xFF;
    buf[1] = (sass_ver & 0xFF);
    buf[2] = (capbility >> 8) & 0xFF;
    buf[3] = (capbility & 0xFF);
}

void gfps_sass_set_switch_pref(uint8_t pref)
{
    sassInfo.preference = pref;
}

uint8_t gfps_sass_get_switch_pref()
{
    TRACE(1, "set reconnect dev %d", sassInfo.preference);

    return sassInfo.preference;
}

void gfps_sass_set_active_dev(uint8_t device_id)
{
    if (sassInfo.activeId != device_id)
    {
        sassInfo.lastActId = sassInfo.activeId;
        sassInfo.activeId = device_id;
    }
}

void gfps_sass_update_active_info(SassBtInfo *sInfo)
{
    if (!sInfo)
    {
        TRACE(1, "%s sass error:: there is no device info!!!", __func__);
    }
    uint8_t oldActive = gfps_sass_get_active_dev();
    uint8_t newActive = sInfo->connId;

    gfps_sass_set_active_dev(newActive);
    if (sInfo && sInfo->isSass && sInfo->ntfSassMode &&
        gfps_sass_is_key_valid(sInfo->accKey))
    {
        gfps_sass_set_inuse_acckey(sInfo->accKey, &(sInfo->btAddr));
    }

    gfps_sass_update_switch_dev(oldActive, newActive);
    sInfo->isActived = true;

    if (gfps_sass_get_resume_dev() == sInfo->connId)
    {
        gfps_sass_set_resume_dev(0xFF);
    }
}

uint8_t gfps_sass_get_active_dev()
{
    return sassInfo.activeId;
}

uint8_t gfps_sass_get_last_active_dev()
{
    return sassInfo.lastActId;
}

SassBtInfo *gfps_sass_get_inactive_dev()
{
    SassBtInfo *sInfo = NULL;
    for (int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if ((sassInfo.connInfo[i].connId != 0xFF) && \
            (sassInfo.activeId != sassInfo.connInfo[i].connId))
        {
            sInfo = &(sassInfo.connInfo[i]);
            break;
        }
    }
    return sInfo;
}

void gfps_sass_set_reconnecting_dev(bt_bdaddr_t *addr, uint8_t evt)
{
    memcpy(sassInfo.reconnInfo.reconnAddr.address, addr->address, sizeof(bt_bdaddr_t));
    sassInfo.reconnInfo.evt = evt;
    TRACE(0, "set reconnect dev");
}

bool gfps_sass_is_reconnect_dev(bt_bdaddr_t *addr)
{
    bool ret = false;
    if (addr && \
        !memcmp(sassInfo.reconnInfo.reconnAddr.address, addr->address, sizeof(bt_bdaddr_t)))
    {
        ret = true;
    }
    TRACE(1, "sass is reconnect dev ? %d", ret);
    return ret;
}

void gfps_sass_set_disconnecting_dev(bt_bdaddr_t *addr)
{
    if (addr)
    {
        memcpy(sassInfo.disconectingAddr.address, addr->address, sizeof(bt_bdaddr_t));
        TRACE(0, "disconnecting_dev is:");
        DUMP8("%2x ", addr->address, sizeof(bt_bdaddr_t));
    }
}

bool gfps_sass_is_disconnect_dev(bt_bdaddr_t *addr)
{
    bool ret = false;
    if (addr && \
        !memcmp(sassInfo.disconectingAddr.address, addr->address, sizeof(bt_bdaddr_t)))
    {
        ret = true;
    }
    TRACE(1, "sass is disconnect dev ? %d", ret);
    return ret;
}

uint8_t gfps_sass_get_hun_flag()
{
    return sassInfo.hunId;
}

void gfps_sass_set_hun_flag(uint8_t device_id)
{
    sassInfo.hunId = device_id;
}

bool gfps_sass_is_need_resume(bt_bdaddr_t *addr)
{
    bool ret = false;
    if (addr && 
        !memcmp(sassInfo.reconnInfo.reconnAddr.address, addr->address, sizeof(bt_bdaddr_t)) && \
        (sassInfo.reconnInfo.evt == SASS_EVT_SWITCH_BACK_AND_RESUME))
    {
        ret = true;
    }
    return ret;
}

bool gfps_sass_is_profile_connected(bt_bdaddr_t *addr)
{
    bool ret = false;
    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (!memcmp(sassInfo.connInfo[i].btAddr.address, addr->address, sizeof(bt_bdaddr_t)))
        {
            if (GET_PROFILE_STATE(sassInfo.connInfo[i].audState, CONNECTION, A2DP) && \
                GET_PROFILE_STATE(sassInfo.connInfo[i].audState, CONNECTION, HFP) && \
                GET_PROFILE_STATE(sassInfo.connInfo[i].audState, CONNECTION, AVRCP))
            {
                ret = true;
            }
            break;
        }
    }

    TRACE(1, "sass is profile connected ? %d", ret);
    return ret;
}

void gfps_sass_update_last_dev(bt_bdaddr_t *addr)
{
    if (addr)
    {
        memcpy(sassInfo.lastDev.address, addr->address, sizeof(bt_bdaddr_t));
    }
}

void gfps_sass_clear_last_dev()
{
    memset(sassInfo.lastDev.address, 0, sizeof(bt_bdaddr_t));
}

void gfps_sass_get_last_dev(bt_bdaddr_t *lastAddr)
{
    memcpy(lastAddr->address, sassInfo.lastDev.address, sizeof(bt_bdaddr_t));
}

void gfps_sass_clear_reject_hf_dev(uint8_t devId)
{
    if(app_bt_hf_get_reject_dev() == devId)
    {
        app_bt_hf_set_reject_dev(0xFF);
    }
}

SASS_CONN_STATE_E gfps_sass_get_state(SassBtInfo *sInfo, SASS_CONN_STATE_E entry)
{
    SASS_CONN_STATE_E state;
    uint8_t devId;
    SassBtInfo *otherInfo;

    if (!sInfo)
    {
       return sassInfo.connState;
    }

    devId = sInfo->connId;
    otherInfo = gfps_sass_get_other_connected_dev(devId);

    state = entry;
    if (otherInfo)
    {
        if ((entry == SASS_STATE_NO_CONNECTION) && (sassInfo.activeId == devId))
        {
            gfps_sass_update_active_info(otherInfo);
            TRACE(2, "change activeId to:%d as devId:%d disconnect", sassInfo.activeId, devId);
        }

        if (sassInfo.activeId == otherInfo->connId) {
            state = otherInfo->state;
        }
        else if (sassInfo.activeId == devId) {
            state = entry;
        }
        else if (otherInfo->state > entry)
        {
            if (((otherInfo->state == SASS_STATE_ONLY_A2DP ||
                 otherInfo->state == SASS_STATE_A2DP_WITH_AVRCP) &&
                (SET_BT_ID(app_bt_audio_get_curr_playing_a2dp()) == otherInfo->connId)) ||
                (otherInfo->state == SASS_STATE_HFP &&
                (SET_BT_ID(app_bt_audio_get_curr_playing_sco()) == otherInfo->connId)))
            {
                state = otherInfo->state;
                gfps_sass_update_active_info(otherInfo);                
                TRACE(2, "change activeId to:%d as devId:%d disconnect", sassInfo.activeId, devId);
            }
        }
        else {
        }
    }
    TRACE(4, "activeId:%d, devId:%d, entry:%d lastAct:%d", sassInfo.activeId, devId, entry, sassInfo.lastActId);

    return state;
}

SASS_CONN_STATE_E gfps_sass_set_seeker_state(SassBtInfo *info)
{
    SASS_CONN_STATE_E connState = SASS_STATE_NO_CONNECTION;
    if (info)
    {
        //remove connection state as sometimes sco comes first.
        if (GET_PROFILE_STATE(info->audState, AUDIO, HFP)) {
            connState = SASS_STATE_HFP;
        }else if (GET_PROFILE_STATE(info->audState, AUDIO, AVRCP)) {
            connState = SASS_STATE_A2DP_WITH_AVRCP;
        }else if (GET_PROFILE_STATE(info->audState, AUDIO, A2DP) && \
            GET_PROFILE_STATE(info->audState, CONNECTION, A2DP)) {
            connState = SASS_STATE_ONLY_A2DP;
        }else if (GET_PROFILE_STATE(info->audState, CONNECTION, HFP) || \
            GET_PROFILE_STATE(info->audState, CONNECTION, A2DP)  || \
            GET_PROFILE_STATE(info->audState, CONNECTION, AVRCP)) {
            connState = SASS_STATE_NO_DATA;
        }else {
            connState = SASS_STATE_NO_DATA;
        }
    }

    info->state = connState;
    return connState;
}

SASS_CONN_STATE_E gfps_sass_update_conn_state(SassBtInfo *info)
{
    SASS_CONN_STATE_E connState = SASS_STATE_NO_CONNECTION;
    gfps_sass_set_seeker_state(info);
    connState = gfps_sass_get_state(info, info->state);
    return connState;
}

void gfps_sass_set_pending_proc(SassPendingProc *pending)
{
    memcpy(&sassInfo.pending, pending, sizeof(SassPendingProc));
}

void gfps_sass_get_pending_proc(SassPendingProc *pending)
{
    memcpy(pending, &sassInfo.pending, sizeof(SassPendingProc));
}

void gfps_sass_clear_pending_proc(void)
{
    memset(&sassInfo.pending, 0, sizeof(SassPendingProc));
}

void gfps_sass_send_pause(SassBtInfo *sInfo)
{
    TRACE(3, "%s %p waitPausedone:%d", __func__, sInfo, sInfo ? sInfo->waitPauseDone : 0);
    if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
    {
        TRACE(2, "%s slave cannot exe sass switch", __func__);
        return;
    }

    if (sInfo && (!sInfo->waitPauseDone))
    {
        app_ibrt_if_a2dp_send_pause(GET_BT_ID(sInfo->connId));
        sInfo->waitPauseDone = true;
    }
}

void gfps_sass_switch_a2dp(uint8_t awayId, uint8_t destId, bool update)
{
    SassBtInfo *awayInfo = gfps_sass_get_connected_dev(awayId);
    SassBtInfo *destInfo = gfps_sass_get_connected_dev(destId);
    if (awayInfo)
    {
        gfps_sass_update_last_dev(&(awayInfo->btAddr));
    }

    if (!destInfo)
    {
        TRACE(2, "%s sass error:: invalid dest device id:%d", __func__, destId);
        return;
    }

    if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
    {
        TRACE(1, "%s sass error:: slave cannot exe sass switch", __func__);
        return;
    }

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
    if (app_tws_ibrt_tws_link_connected() && \
        (!app_ibrt_conn_is_profile_exchanged(&(destInfo->btAddr))))
    {
        TRACE(1, "%s waiting profile exchange done", __func__);
        SassPendingProc pending;
        pending.proc = SASS_PROCESS_SWITCH_ACTIVE_SRC;
        pending.param.activeId = destId;
        gfps_sass_set_pending_proc(&pending);
    }
    else
#endif
    {
        app_ibrt_if_switch_streaming_a2dp();
        gfps_sass_update_active_info(destInfo);

        if (update)
        {
            SassEvtParam evtParam;
            evtParam.devId = destId;
            evtParam.event = SASS_EVT_UPDATE_CONN_STATE;
            evtParam.state.connState = gfps_sass_update_conn_state(destInfo);
            gfps_sass_update_state(&evtParam);
        }
    }
}

void gfps_sass_exe_pending_switch_a2dp(uint8_t devId)
{
    SassPendingProc pending;
    if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
    {
        TRACE(2, "%s slave cannot exe sass switch", __func__);
        return;
    }

    uint8_t currId = SET_BT_ID(app_bt_audio_get_curr_playing_a2dp());

    gfps_sass_get_pending_proc(&pending);
    if ((currId != 0xFF) && (currId != devId) && \
        (pending.proc == SASS_PROCESS_SWITCH_ACTIVE_SRC) && \
        (pending.param.activeId == devId))
    {
        gfps_sass_clear_pending_proc();
        gfps_sass_switch_a2dp(currId, devId, true);
    }
}

void gfps_sass_switched_callback(uint8_t selectedId)
{
    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(SET_BT_ID(selectedId));
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(SET_BT_ID(selectedId));
    if (otherInfo && (otherInfo->connId != 0xFF) && \
        GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP) && \
        app_bt_is_a2dp_streaming(GET_BT_ID(otherInfo->connId)))
    {
       //app_ibrt_if_a2dp_send_pause(otherInfo->connId);
       gfps_sass_send_pause(otherInfo);
    }

    if (sInfo && sInfo->isNeedResume && (!GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP)))
    {
        app_bt_ibrt_audio_play_a2dp_stream(selectedId);
        sInfo->isNeedResume = false;
    }
}

SASS_CONN_STATE_E gfps_sass_get_conn_state()
{
    return sassInfo.connState;
}

void gfps_sass_set_conn_state(SASS_CONN_STATE_E state)
{
    TRACE(1, "set sass conn state:0x%0x", state);

    sassInfo.connState = state;
}

SASS_HEAD_STATE_E gfps_sass_get_head_state()
{
    return sassInfo.headState;
}

void gfps_sass_set_head_state(SASS_HEAD_STATE_E headstate)
{
    sassInfo.headState = headstate;
}

void gfps_sass_set_conn_available(SASS_CONN_AVAIL_E available)
{
    sassInfo.connAvail= available;
}

void gfps_sass_set_focus_mode(SASS_FOCUS_MODE_E focus)
{
    sassInfo.focusMode = focus;
}

void gfps_sass_set_auto_reconn(SASS_AUTO_RECONN_E focus)
{
    sassInfo.autoReconn = focus;
}

void gfps_sass_set_init_conn(uint8_t device_id, bool bySass)
{
    SassBtInfo *info = gfps_sass_get_connected_dev(device_id);
    if (info)
    {
        info->initbySass = bySass;
    }
}

void gfps_sass_set_inuse_acckey(uint8_t *accKey, bt_bdaddr_t *addr)
{
    if (accKey)
    {
        memcpy(sassInfo.inuseKey, accKey, FP_ACCOUNT_KEY_SIZE);
    }
    
    if (addr)
    {
        memcpy(sassInfo.inUseAddr.address, addr->address, sizeof(bt_bdaddr_t));
    }
}

void gfps_sass_set_inuse_acckey_by_dev(uint8_t device_id, uint8_t *accKey)
{
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo)
    {
        gfps_sass_set_inuse_acckey(accKey, &(sInfo->btAddr));

        memcpy(sInfo->accKey, accKey, FP_ACCOUNT_KEY_SIZE);
        sInfo->updated = true;
    }
    TRACE(1, "sass dev %d inuse acckey is:", device_id);
    DUMP8("0x%2x ", accKey, FP_ACCOUNT_KEY_SIZE);
}

bool gfps_sass_get_inuse_acckey(uint8_t *accKey)
{
    uint8_t nvKey[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t *kPointer = NULL;
    bool ret = false;

    if (!gfps_sass_is_key_valid(sassInfo.inuseKey))
    {
        SassBtInfo *sInfo = NULL;
        for(int i= 0; i < BT_DEVICE_NUM; i++)
        {
            sInfo = &(sassInfo.connInfo[i]);
            if (sInfo->connId != 0xFF)
            {
                if (sInfo->isSass && gfps_sass_is_key_valid(sInfo->accKey))
                {
                    kPointer = sInfo->accKey;
                    ret = true;
                }
                else 
                {
                    nv_record_fp_get_key_by_addr(sInfo->btAddr.address, nvKey);
                    if (gfps_sass_is_key_valid(nvKey))
                    {
                        kPointer = nvKey;
                        ret = true;
                    }
                }
                break;
            }
        } 
    }
    else
    {
        kPointer = sassInfo.inuseKey;
        ret = true;
    }

    if (ret)
    {
        memcpy(accKey, kPointer, FP_ACCOUNT_KEY_SIZE);
        TRACE(0, "get inuse acckey is:");
        DUMP8("%2x ", kPointer, FP_ACCOUNT_KEY_SIZE);
    }

    return ret;
}

bool gfps_sass_get_inuse_acckey_by_id(uint8_t device_id, uint8_t *accKey)
{
    bool ret = false;
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo && gfps_sass_is_key_valid(sInfo->accKey))
    {
        memcpy(accKey, sInfo->accKey, FP_ACCOUNT_KEY_SIZE);
        ret = true;
    }
    TRACE(1, "get sass dev %d inuse acckey is:", device_id);
    DUMP8("%2x ", accKey, FP_ACCOUNT_KEY_SIZE);
    return ret;
}

void gfps_sass_update_head_state(SASS_HEAD_STATE_E state)
{
    SassEvtParam evtParam;
    evtParam.devId = SET_BT_ID(0);
    evtParam.event = SASS_EVT_UPDATE_HEAD_STATE;
    evtParam.state.headState = state;
    gfps_sass_update_state(&evtParam);
}

SASS_REASON_E gfps_sass_get_switch_reason(uint8_t devId)
{
    SASS_REASON_E reason;
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(devId);
    if (sInfo == NULL)
    {
        return SASS_REASON_UNSPECIFIED;
    }

    if (GET_PROFILE_STATE(sInfo->audState, AUDIO, HFP)){
        reason = SASS_REASON_HFP;
    }else if (GET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP) || \
              GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP)) {
        reason = SASS_REASON_A2DP;
    }else {
        reason = SASS_REASON_UNSPECIFIED;
    }

    return reason;
}

SASS_DEV_TYPE_E gfps_sass_get_dev_type_by_cod(uint8_t *cod)
{
    SASS_DEV_TYPE_E type;
    uint32_t devCod = cod[0] + (cod[1] << 8) + (cod[2] << 16);
    TRACE(4, "%s type: 0x%02x%02x%02x", __func__, cod[0], cod[1], cod[2]);

    if ((devCod & COD_TYPE_PHONE) == COD_TYPE_PHONE)
    {
        type = SASS_DEV_TYPE_PHONEA;
    }
    else if ((devCod & COD_TYPE_LAPTOP) == COD_TYPE_LAPTOP)
    {
        type = SASS_DEV_TYPE_LAPTOP;
    }
    else if ((devCod & COD_TYPE_TABLET) == COD_TYPE_TABLET)
    {
        type = SASS_DEV_TYPE_TABLET;
    }
    else if ((devCod & COD_TYPE_TV) == COD_TYPE_TV)
    {
        type = SASS_DEV_TYPE_TV;
    }
    else
    {
        type = SASS_DEV_TYPE_PHONEA;//SASS_DEV_TYPE_INVALID;
    }

    return type;
}

void gfps_sass_check_if_need_reconnect(void)
{
    uint8_t invalidAddr[6] = {0};
    bt_bdaddr_t *reconnAddr = &(sassInfo.reconnInfo.reconnAddr);
#ifdef IBRT
    if (TWS_UI_SLAVE != app_ibrt_if_get_ui_role())
#endif
    {
        if (memcmp(reconnAddr->address, invalidAddr, sizeof(bt_bdaddr_t)) && \
            !app_bt_is_acl_connected_byaddr(reconnAddr))
        {
            TRACE(1, "%s try to reconnect dev", __func__);
            DUMP8("%02x ", reconnAddr->address, 6);
            app_ibrt_if_choice_mobile_connect((bt_bdaddr_t *)&(sassInfo.reconnInfo.reconnAddr));
        }
    }
}

void gfps_sass_check_if_need_hun(uint8_t device_id, uint8_t event)
{
    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(device_id);
    if (otherInfo == NULL)
    {
        return;
    }

    uint8_t activeId = gfps_sass_get_active_dev();

    if ((activeId != 0xFF) && (activeId == otherInfo->connId))
    {
        switch(event)
        {
            case BTIF_HF_EVENT_AUDIO_CONNECTED:
                if (GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP))
                {
                    gfps_sass_set_hun_flag(device_id);
                }
                break;

            default:
                break;
        }
    }
}

void gfps_sass_get_acckey_from_nv(uint8_t *addr, uint8_t *key)
{
    nv_record_fp_get_key_by_addr(addr, key);
}

void gfps_sass_set_need_ntf_status(uint8_t device_id, bool en)
{
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo)
    {
        sInfo->needNtfStatus = en;
    }
}

bool gfps_sass_is_need_ntf_status(uint8_t device_id)
{
    bool ret = false;
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo)
    {
        ret = sInfo->needNtfStatus;
    }
    return ret;
}

void gfps_sass_set_need_ntf_switch(uint8_t device_id, bool en)
{
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo)
    {
        sInfo->needNtfSwitch = en;
    }
}

bool gfps_sass_is_need_ntf_switch(uint8_t device_id)
{
    bool ret = false;
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(device_id);
    if (sInfo)
    {
        ret = sInfo->needNtfSwitch;
    }
    return ret;
}

void gfps_sass_send_session_nonce(uint8_t devId)
{
    FP_MESSAGE_STREAM_T req = {FP_MSG_GROUP_DEVICE_INFO, FP_MSG_DEVICE_INFO_SESSION_NONCE, 0, 8};
    gfps_sass_get_session_nonce(devId, req.data);
    gfps_send(devId, (uint8_t *)&req, FP_MESSAGE_RESERVED_LEN+8);
}

void gfps_sass_get_capability(uint8_t devId)
{
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_SASS,
         FP_MSG_SASS_GET_CAPBILITY,
         0,
         0};
    gfps_send(devId, ( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN);
}

void gfps_sass_ntf_capability(uint8_t devId)
{
    uint32_t dataLen = 0;
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_SASS,
         FP_MSG_SASS_NTF_CAPBILITY};
    gfps_sass_get_cap_data(req.data, &dataLen);
    req.dataLenHighByte = (uint8_t)(dataLen & 0xFF00);
    req.dataLenLowByte = (uint8_t)(dataLen & 0xFF);
    gfps_send(devId, ( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + dataLen);
}

void gfps_sass_set_capability(uint8_t devId,uint8_t *data)
{
    uint16_t sassVer =( data[0] << 8) | data[1];
    bool state = (data[2] & 0x80) ? true: false;
    gfps_sass_set_sass_mode(devId, sassVer, state);
}

void gfps_sass_set_multipoint_hdl(uint8_t devId, uint8_t *data)
{
    uint8_t enable = data[0];
    if(enable) {
        gfps_sass_switch_max_link(devId, SASS_LINK_SWITCH_TO_MULTI_POINT);
    }else {
        gfps_sass_switch_max_link(devId, SASS_LINK_SWITCH_TO_SINGLE_POINT);
    }

    TRACE(2,"%s swtich:%d",__func__, enable);
}

void gfps_sass_set_switch_pref_hdl(uint8_t devId, uint8_t *data)
{
    uint8_t pref = data[0];
    gfps_sass_set_switch_pref(pref);
}

void gfps_sass_ntf_switch_pref(uint8_t devId)
{
    uint16_t dataLen = 2;
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_SASS,
         FP_MSG_SASS_NTF_SWITCH_PREFERENCE,
         0,
         2};
    req.data[0] = gfps_sass_get_switch_pref();
    req.data[1] = 0;
    gfps_send(devId, ( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + dataLen);
}

bool gfps_sass_ntf_switch_evt(uint8_t devId, uint8_t reason)
{
    uint32_t nameLen = 0;
    bool ret = true;
    uint8_t *namePtr = NULL;
    uint8_t activeId = gfps_sass_get_active_dev();

    namePtr = nv_record_fp_get_name_ptr(&nameLen);
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_SASS,
         FP_MSG_SASS_NTF_SWITCH_EVT,
         0,
         (uint8_t)(2+nameLen)};

    req.data[0] = reason;
    req.data[1] = (devId == activeId) ? SASS_DEV_THIS_DEVICE : SASS_DEV_ANOTHER;
    if(namePtr)
    {
        memcpy(req.data + 2, namePtr, nameLen);
    }

    ret = gfps_send(devId, ( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + 2 + nameLen);
    if (ret == false)
    {
        gfps_sass_set_need_ntf_switch(devId, true);
    }
    else
    {
        gfps_sass_set_need_ntf_switch(devId, false);
    }
    return ret;
}

bool gfps_sass_ntf_conn_status(uint8_t devId, bool isUseAdv, uint8_t *state)
{
    uint8_t advlen, len, activeId;
    uint8_t account[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t outBuf[FP_ACCOUNT_KEY_SIZE] = {0};
    //uint8_t actKey[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t iv[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t devKey[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t memNonce[SESSION_NOUNCE_NUM] = {0};
    bool ret = true;
    bool devKeyVaild = false;
    SassBtInfo * otherInfo = NULL;
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_SASS,
         FP_MSG_SASS_NTF_CONN_STATUS,
         0,
         0};

    if (!gfps_sass_get_session_nonce(devId, iv))
    {
        ret = false;
        TRACE(0, "sass error:cannot get session nonce!!!");
        goto NTF_EXIT;
    }

    activeId = gfps_sass_get_active_dev();
    devKeyVaild = gfps_sass_get_inuse_acckey_by_id(devId, devKey);
    #if 0
    if (isUseAdv)
    {
        bool valid = gfps_sass_get_inuse_acckey(account);
        if (!valid && devKeyVaild)
        {
           memcpy(account, devKey, FP_ACCOUNT_KEY_SIZE);
        }
        else if (!valid && !devKeyVaild)
        {
            ret = false; 
            TRACE(0, "sass error:cannot get dev inuse account key!!!");
            goto NTF_EXIT;
        }
        else{
        }
    }
    else
    {
        if (!devKeyVaild)
        {
            ret = false; 
            TRACE(0, "sass error:cannot get dev inuse account key!!!");
            goto NTF_EXIT;
        }
        memcpy(account, devKey, FP_ACCOUNT_KEY_SIZE);
        DUMP8("%2x ", state, 4);  
    }
#else
    if (!devKeyVaild)
    {
        ret = false; 
        TRACE(0, "sass error:cannot get dev inuse account key!!!");
        goto NTF_EXIT;
    }
    else
    {
        memcpy(account, devKey, FP_ACCOUNT_KEY_SIZE);
    }

#endif
    for(int i = 0; i < SESSION_NOUNCE_NUM; i++)
    {
         memNonce[i] = (uint8_t)rand();
         iv[SESSION_NOUNCE_NUM + i] = memNonce[i];
    }

#ifdef SASS_SECURE_ENHACEMENT
    gfps_sass_encrypt_connection_state(iv, account, outBuf, &advlen, false, isUseAdv, state);
#else
    uint8_t adv[5] = {0};
    gfps_sass_get_adv_data(adv, &advlen);
    AES128_CTR_encrypt_buffer(adv+1, advlen-1, account, iv, outBuf, isUseAdv, state);
    advlen--;
#endif

    len = advlen + 1 + SESSION_NOUNCE_NUM;
    req.dataLenLowByte = len;

    otherInfo = gfps_sass_get_other_connected_dev(devId);
    if (!otherInfo || activeId == devId)
    {
        req.data[0] = SASS_DEV_IS_ACTIVE;
    }
    else if ((otherInfo && gfps_sass_is_sass_dev(otherInfo->connId)) || (activeId == 0xFF))
    {
        req.data[0] = SASS_DEV_IS_PASSIVE;
    }
    else
    {
        req.data[0] = SASS_DEV_IS_PSSIVE_WITH_NONSASS;
    }
    TRACE(4, "%s dev:%d active:%d req.data[0]:%d", __func__, devId, activeId, req.data[0]);

    memcpy(req.data + 1, outBuf, advlen);
    memcpy(req.data + advlen + 1, memNonce, SESSION_NOUNCE_NUM);
    ret = gfps_send(devId, (uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + len);  

NTF_EXIT:
    if (ret == false)
    {
        gfps_sass_set_need_ntf_status(devId, true);
    }
    else
    {
        gfps_sass_set_need_ntf_status(devId, false);
    }
    return ret;
}

void gfps_sass_get_conn_hdl(uint8_t devId)
{
    gfps_sass_ntf_conn_status(devId, true, NULL);
    TRACE(1,"%s",__func__);
}

void gfps_sass_set_init_conn(uint8_t devId, uint8_t *data)
{
    bool isSass = data[0];
    gfps_sass_set_init_conn(devId, isSass);
    TRACE(1,"connection is triggered by sass? %d", isSass);
}

void gfps_sass_set_custom_data(uint8_t devId, uint8_t *data)
{
    uint8_t param = data[0];
    uint8_t activeDev = gfps_sass_get_active_dev();
    if ((activeDev != 0xFF) && (activeDev != devId))
    {
        return;
    }

    SassEvtParam evtParam;
    evtParam.event = SASS_EVT_UPDATE_CUSTOM_DATA;
    evtParam.devId = devId;
    evtParam.state.cusData = param;
    gfps_sass_update_state(&evtParam);
}

void gfps_sass_set_drop_dev(uint8_t devId, uint8_t *data)
{
    //drop this device
    if(data[0] == 1)
    {
        SassBtInfo *info = gfps_sass_get_connected_dev(devId);
        if (info)
        {
            memcpy(sassInfo.dropDevAddr.address, info->btAddr.address, sizeof(bt_bdaddr_t));
        }
    }
}

void gfps_sass_set_resume_dev(uint8_t devId)
{
     sassInfo.config.resumeId = devId;
}

uint8_t gfps_sass_get_resume_dev(void)
{
     return sassInfo.config.resumeId;
}

bool gfps_sass_is_need_resume_after_call(void)
{
     return sassInfo.config.resume;
}

bool gfps_sass_is_accept_new_media(void)
{
     return sassInfo.config.acceptNew;
}

void gfps_sass_update_switch_dev(uint8_t oldActive, uint8_t newActive)
{
    //SASS_CONN_STATE_E state = gfps_sass_get_conn_state();
    if ((newActive == 0xFF) || (oldActive == newActive))
    {
        return;
    }

    SASS_REASON_E reason = gfps_sass_get_switch_reason(newActive);
    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(newActive);
    SassBtInfo *sInfo = gfps_sass_get_connected_dev(newActive);

    TRACE(3,"sass_ntf_switch oldActive:%d, newActive:%d, reason:%d", oldActive, newActive, reason);
    if ((oldActive != 0xFF) && (newActive != 0xFF))
    {
        gfps_sass_ntf_switch_evt(newActive, reason);
        if (otherInfo && (otherInfo->connId != 0xFF))
        {
            gfps_sass_ntf_switch_evt(otherInfo->connId, reason);
        }
    }
    else //old = 0xFF, new !=0xFF
    {
        //1. only 1 device; 2.there is no active history before
        if ((!otherInfo && !sInfo->isActived) || (otherInfo && !otherInfo->isActived))
        {
            gfps_sass_ntf_switch_evt(newActive, reason);
        }

        if (!gfps_sass_get_multi_status() && otherInfo && (otherInfo->connId != 0xFF))
        {
            gfps_sass_ntf_switch_evt(otherInfo->connId, reason);
        }
    }
}

void gfps_sass_check_nonsass_switch(uint8_t devId)
{
    SassPendingProc pending;
    uint8_t currId = SET_BT_ID(app_bt_audio_get_curr_playing_a2dp());

    gfps_sass_get_pending_proc(&pending);
    if ((currId != 0xFF) && (currId != devId) && \
        (pending.proc == SASS_PROCESS_SWITCH_ACTIVE_SRC) && \
        (pending.param.activeId == devId))
    {
        gfps_sass_switch_a2dp(currId, devId, true);
        gfps_sass_clear_pending_proc();
    }
}

void gfps_sass_check_if_switch_a2dp(SassBtInfo *sInfo, bool devIssass, SassBtInfo *otherInfo, bool otherIssass)
{
    uint8_t currId, devId;
    SassPendingProc pending;
    bool isDevMusic, isDevGame;

    if (sInfo == NULL)
    {
        return;
    }

    currId = SET_BT_ID(app_bt_audio_get_curr_playing_a2dp());
    devId = sInfo->connId;

    isDevMusic = GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP);
    isDevGame = (GET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP) & \
                    (!GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP)));

    if (otherInfo)
    {
        bool isOtherGame, isOtherCall;

        isOtherGame = (GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP) & \
                          (!GET_PROFILE_STATE(otherInfo->audState, AUDIO, AVRCP)));

        isOtherCall = GET_PROFILE_STATE(otherInfo->audState, AUDIO, HFP);
        TRACE(5, "%s otherIssass:%d devIssass:%d currId:%d iscall:%d", __func__, otherIssass, devIssass, currId, isOtherCall);

        if (isOtherCall && (isDevGame || isDevMusic))
        {
            gfps_sass_send_pause(sInfo);  
        }
        else if ((currId != BT_DEVICE_INVALID_ID) && (currId != devId))
        {
            gfps_sass_get_pending_proc(&pending);
            if (devIssass && otherIssass)
            {
                if ((pending.proc == SASS_PROCESS_SWITCH_ACTIVE_SRC) && \
                    (pending.param.activeId == devId))
                {
                    gfps_sass_clear_pending_proc();
                    gfps_sass_switch_a2dp(currId, devId, false);
                }
            }
            else if (!devIssass)
            {
                uint8_t devKey[16] = {0};
                memset(devKey, 0, 16);
                nv_record_fp_get_key_by_addr(sInfo->btAddr.address, devKey);
                 //workaround for the case a2dp event come first before gfps connection.
                if (!gfps_sass_is_key_valid(devKey)) //dev B is non-SASS
                {
                    //dev A is SASS or do not accept new media
                    if (otherIssass || !gfps_sass_is_accept_new_media())
                    {
                        gfps_sass_send_pause(sInfo);
                    }
                    else // dev A is non-SASS and accept new media
                    {
                        gfps_sass_send_pause(otherInfo);
                    }
                }
                else if (isDevMusic || (isDevGame && isOtherGame))
                {
                    SassPendingProc pending;
                    pending.proc = SASS_PROCESS_SWITCH_ACTIVE_SRC;
                    pending.param.activeId = sInfo->connId;
                    gfps_sass_set_pending_proc(&pending);
                }
                else
                {
                    TRACE(1, "%s wait a2dp or avrcp event or sass event!!", __func__);
                }
            }
            else//B is sass, and A is non-SASS
            {
                //as samsung cannot paused by avrcp
                if (!gfps_sass_is_accept_new_media())
                {
                    gfps_sass_send_pause(sInfo);
                }
                else
                {
                    gfps_sass_send_pause(otherInfo);
                }
            }
         }
         else
         {
             gfps_sass_update_active_info(sInfo);
         }
    }
    else
    {
        gfps_sass_update_active_info(sInfo);
    }
}

bool gfps_sass_select_steal_device(const bt_bdaddr_t *conn_req_addr, bt_bdaddr_t *steal_addr)
{
    SassBtInfo *sInfo= NULL;
    uint8_t activeId = gfps_sass_get_active_dev();
    bool ret = false;
    TRACE(2, "%s activeId:%d", __func__, activeId);
    if (activeId == 0xFF)
    {
        activeId = gfps_sass_get_last_active_dev();
    }

    if (activeId != 0xFF)
    {
        sInfo = gfps_sass_get_other_connected_dev(activeId);
        if (sInfo)
        {
            memcpy(steal_addr, sInfo->btAddr.address, sizeof(bt_bdaddr_t));
            ret = true;
        }
    }

    TRACE(1, "%s steal addr is:", __func__);
    DUMP8("%2x ", (uint8_t *)steal_addr, sizeof(bt_bdaddr_t));
    return ret;
}

void gfps_sass_hf_connect_handler(SassBtInfo *sInfo, SassBtInfo* otherInfo)
{
    if (!sInfo)
    {
        return;
    }

    uint8_t oldHfpState = GET_PROFILE_STATE(sInfo->audState, AUDIO, HFP);

    SET_PROFILE_STATE(sInfo->audState, AUDIO, HFP, 1);
    if (GET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP))
    {
        SET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP, 0);
    }

    if (GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP))
    {
        SET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP, 0);
    }

    if (otherInfo && (!GET_PROFILE_STATE(otherInfo->audState, AUDIO, HFP)))
    {
        if (!oldHfpState && (GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP) || \
            GET_PROFILE_STATE(otherInfo->audState, AUDIO, AVRCP)))
        {
            gfps_sass_send_pause(otherInfo);
            if (gfps_sass_is_need_resume_after_call())
            {
                gfps_sass_set_resume_dev(otherInfo->connId);
            }
        }
         gfps_sass_update_active_info(sInfo);
    }
    else
    {
        if (!otherInfo)
        {
            gfps_sass_update_active_info(sInfo);
        }
     }
}

void gfps_sass_hf_disconnect_handler(SassBtInfo *sInfo, SassBtInfo* otherInfo)
{
    uint8_t activeId = gfps_sass_get_active_dev();

    if (GET_PROFILE_STATE(sInfo->audState, AUDIO, HFP))
    {
        SET_PROFILE_STATE(sInfo->audState, AUDIO, HFP, 0);
        if (otherInfo)
        {
            if (GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP))
            {
                gfps_sass_update_active_info(otherInfo);
                gfps_sass_set_resume_dev(0xFF);                
            }
            else
            {
                if (gfps_sass_is_need_resume_after_call() && \
                    gfps_sass_get_resume_dev() == otherInfo->connId)
                {
                    app_bt_ibrt_audio_play_a2dp_stream(GET_BT_ID(otherInfo->connId));
                }
            }
        }
        else {
            if (activeId == sInfo->connId)
            {
                gfps_sass_set_active_dev(0xFF);
            }
        }
        sInfo->isCallsetup = false;
    }
}

void gfps_sass_profile_event_handler(uint8_t pro, uint8_t devId, bt_bdaddr_t *btAddr, 
                                                   uint8_t event, uint8_t *param)
{
    bool needUpdate = true;
    bool needResume = false;
    bool devIssass = false;
    bool otherIssass = false;
    SASS_CONN_STATE_E tempState;
    SASS_CONN_STATE_E updateState;
    SassBtInfo *sInfo = NULL;
    uint8_t oldActive = gfps_sass_get_active_dev();

    if (btAddr)
    {
        sInfo = gfps_sass_get_connected_dev_by_addr(btAddr);
        if (!sInfo)
        {
            gfps_sass_connect_handler(devId, (bt_bdaddr_t *)btAddr);
        }
    }

    sInfo = gfps_sass_get_connected_dev(devId);
    if (!sInfo)
    { 
        TRACE(5,"%s sass error:: there is no connection for devId:%d", __func__, devId);
        return;
    }
    TRACE(5,"%s id:%d event:%d pro:%d avtiveId:%d", __func__, devId, event,pro,oldActive);

    SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(devId);
    if (otherInfo)
    {
        otherIssass = gfps_sass_is_sass_dev(otherInfo->connId);
    }
    needResume = gfps_sass_is_need_resume(&(sInfo->btAddr));

    devIssass = gfps_sass_is_sass_dev(devId);

    if (pro == SASS_PROFILE_A2DP)
    {
        //SassBtInfo *otherInfo = NULL;
        SassPendingProc pending;
        switch(event)
        {
            case BTIF_A2DP_EVENT_STREAM_OPEN:
            case BTIF_A2DP_EVENT_STREAM_OPEN_MOCK:
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, A2DP, 1);
                if (gfps_sass_get_active_dev() == 0xFF || \
                    (otherInfo && !GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP) && \
                    !GET_PROFILE_STATE(otherInfo->audState, AUDIO, HFP) && \
                    !GET_PROFILE_STATE(otherInfo->audState, AUDIO, AVRCP)))
                {
                    //gfps_sass_set_active_dev(devId);
                }
                break;

            case BTIF_A2DP_EVENT_STREAM_CLOSED:
                SET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP, 0);
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, A2DP, 0);
                gfps_sass_get_pending_proc(&pending);
                if (pending.param.activeId == devId)
                {
                    gfps_sass_clear_pending_proc();
                }

                if (oldActive == devId)
                {
                    gfps_sass_set_active_dev(0xFF);
                }

                if (gfps_sass_get_resume_dev() == devId)
                {
                    gfps_sass_set_resume_dev(0xFF);
                }
                break;

            case BTIF_A2DP_EVENT_STREAM_STARTED:
            case BTIF_A2DP_EVENT_STREAM_STARTED_MOCK:
                if(!GET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP))
                {
                    SET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP, 1);
                    gfps_sass_check_if_switch_a2dp(sInfo, devIssass, otherInfo, otherIssass);
                }
                break;

            case BTIF_A2DP_EVENT_STREAM_SUSPENDED:
                SET_PROFILE_STATE(sInfo->audState, AUDIO, A2DP, 0);
                gfps_sass_get_pending_proc(&pending);
                if (pending.param.activeId == devId)
                {
                    gfps_sass_clear_pending_proc();
                }
                if (oldActive == devId)
                {
                    gfps_sass_set_active_dev(0xFF);
                }
                break;
            default:
                needUpdate = false;
                break;
        }
    }
    else if (pro == SASS_PROFILE_HFP)
    {
        switch(event)
        {
            case BTIF_HF_EVENT_CALLSETUP_IND:
                TRACE(2, "%s hfp state:%d", __func__, param ? param[0] : 0);
                if (!param)
                {
                    needUpdate = false;
                }
                else if (param[0])
                {
                    if (!GET_PROFILE_STATE(sInfo->audState, AUDIO, HFP))
                    {
                        sInfo->isCallsetup = true;
                    }
                    gfps_sass_hf_connect_handler(sInfo, otherInfo);
                }
                else
                {
                    if (sInfo->isCallsetup)
                    {
                        gfps_sass_hf_disconnect_handler(sInfo, otherInfo);
                    }
                }
                break;

            case BTIF_HF_EVENT_AUDIO_CONNECTED:
            case BTIF_HF_EVENT_RING_IND:
                sInfo->isCallsetup = false;
                gfps_sass_hf_connect_handler(sInfo, otherInfo);
                break;

            case BTIF_HF_EVENT_CALL_IND:
            case BTIF_HF_EVENT_AUDIO_DISCONNECTED:
                if (event == BTIF_HF_EVENT_CALL_IND && param && param[0])
                {
                    needUpdate = false;
                    break;
                }
                gfps_sass_hf_disconnect_handler(sInfo, otherInfo);
                break;

            case BTIF_HF_EVENT_SERVICE_DISCONNECTED:
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, HFP, 0);
                if (oldActive == devId)
                {
                    gfps_sass_set_active_dev(0xFF);
                }
                gfps_sass_clear_reject_hf_dev(devId);
                sInfo->isCallsetup = false;
                break;

            case BTIF_HF_EVENT_SERVICE_CONNECTED:
            case BTIF_HF_EVENT_SERVICE_MOCK_CONNECTED:
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, HFP, 1);
                sInfo->isCallsetup = false;
                break;

            default:
            needUpdate = false;
            break;
        }
    }
    else if (pro == SASS_PROFILE_AVRCP)
    {
        switch(event)
        {
            case BTIF_AVCTP_CONNECT_EVENT:
            case BTIF_AVCTP_CONNECT_EVENT_MOCK:
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, AVRCP, 1);
                sInfo->waitPauseDone = false;
                sInfo->isNeedResume = false;
                break;

            case BTIF_AVCTP_DISCONNECT_EVENT:
                SET_PROFILE_STATE(sInfo->audState, CONNECTION, AVRCP, 0);
                SET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP, 0);
                sInfo->waitPauseDone = false;
                sInfo->isNeedResume = false;
                break;

            case BTIF_AVRCP_EVENT_ADV_NOTIFY:
            case BTIF_AVRCP_EVENT_ADV_RESPONSE:
                TRACE(2, "%s avrcp state:%d", __func__, *param);
                if (*param == BTIF_AVRCP_MEDIA_PLAYING) {
                    if (!GET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP))
                    {
                        SET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP, 1);             
                        gfps_sass_check_if_switch_a2dp(sInfo, devIssass, otherInfo, otherIssass);
                    }
                    sInfo->waitPauseDone = false;
                    sInfo->isNeedResume = false;
                } else if ((*param == BTIF_AVRCP_MEDIA_PAUSED || *param == BTIF_AVRCP_MEDIA_STOPPED)) {
                    SET_PROFILE_STATE(sInfo->audState, AUDIO, AVRCP, 0);
                    sInfo->waitPauseDone = false;
                    sInfo->isNeedResume = false;
                } else {
                    needUpdate = false;
                }
                break;

            default:
                needUpdate = false;
                break;
        }
    }
    else
    {
        needUpdate = false;
        TRACE(1,"%s sass profile update error", __func__);
    }

    if (needUpdate)
    {
        uint8_t newActive;
        SassEvtParam evtParam;
        evtParam.devId = devId;

        tempState = gfps_sass_get_conn_state();
        updateState = gfps_sass_update_conn_state(sInfo);
        newActive = gfps_sass_get_active_dev();
        TRACE(4,"%s sass audState: 0x%0x, temp:0x%0x, state:0x%0x", __func__, sInfo->audState, tempState, updateState);

        if (tempState != updateState)
        {
            evtParam.event = SASS_EVT_UPDATE_CONN_STATE;
            evtParam.state.connState = updateState;
            gfps_sass_update_state(&evtParam);
        }
        else if ((tempState == updateState) && (oldActive != newActive))
        {
            evtParam.event = SASS_EVT_UPDATE_ACTIVE_DEV;
            gfps_sass_update_state(&evtParam);
        }
        else
        {
        }

    }

    if (needResume && GET_PROFILE_STATE(sInfo->audState, CONNECTION, A2DP) && \
        GET_PROFILE_STATE(sInfo->audState, CONNECTION, AVRCP))
    {
        app_bt_resume_music_player(GET_BT_ID(devId));
        memset(sassInfo.reconnInfo.reconnAddr.address, 0, sizeof(bt_bdaddr_t));
        sassInfo.reconnInfo.evt = 0xFF;
    }
}

uint8_t gfps_sass_switch_src_evt_hdl(uint8_t device_id, uint8_t evt)
{
    uint8_t reason = SASS_STATUS_OK;
    uint8_t awayId, switchId, currentId, otherId = 0xFF;
    bool isDisconnect = false;
    SassBtInfo *info = gfps_sass_get_other_connected_dev(device_id);
    if (info)
    {
        otherId = info->connId;
    }

    if (evt & SASS_SWITCH_TO_CURR_DEV_BIT)
    {
        switchId = device_id;
        awayId = otherId;
    }
    else
    {
        switchId = otherId;
        awayId = device_id;
    }

    if (switchId != 0xFF)
    {
        SassBtInfo *tInfo = gfps_sass_get_connected_dev(switchId);
        currentId = SET_BT_ID(app_bt_audio_get_curr_playing_a2dp());
        TRACE(3, "sass switch src to %d from %d, other:%d, evt:0x%0x a2dp curr id:%d", switchId, awayId, otherId, evt, currentId);

        if (currentId == switchId || \
            (tInfo && gfps_sass_is_disconnect_dev(&(tInfo->btAddr)))) {
            return SASS_STATUS_REDUNTANT;
        } else {
            if (evt & SASS_DISCONN_ON_AWAY_DEV_BIT)
            {
                SassBtInfo *awayinfo = gfps_sass_get_connected_dev(awayId);
                if (awayinfo)
                {
                    gfps_sass_update_last_dev(&(awayinfo->btAddr));
                    gfps_sass_set_disconnecting_dev((bt_bdaddr_t *)&(awayinfo->btAddr));
                    app_ui_destroy_device((bt_bdaddr_t *)&(awayinfo->btAddr), false);
                }
                isDisconnect = true;
            }
            else if (app_bt_is_a2dp_streaming(GET_BT_ID(switchId)))
            {
                SassPendingProc pending;
                gfps_sass_get_pending_proc(&pending);
                if ((pending.proc == SASS_PROCESS_SWITCH_ACTIVE_SRC) && 
                    (pending.param.activeId == switchId))
                {
                    gfps_sass_clear_pending_proc();
                }
                gfps_sass_switch_a2dp(currentId, switchId, true);
                goto set_hf;
            }
            else
            {
                TRACE(1, "switchId:%d is not in streaming mode", switchId);
            }

            if (evt & SASS_RESUME_ON_SWITCH_DEV_BIT)
            {
                if (!isDisconnect && app_bt_is_a2dp_streaming(GET_BT_ID(awayId)))
                {
                    SassBtInfo *awayinfo = gfps_sass_get_connected_dev(awayId);
                    gfps_sass_send_pause(awayinfo);
                }
                app_bt_ibrt_audio_play_a2dp_stream(GET_BT_ID(switchId));
            }
            else
            {
                SassPendingProc pending;
                pending.proc = SASS_PROCESS_SWITCH_ACTIVE_SRC;
                pending.param.activeId = switchId;
                gfps_sass_set_pending_proc(&pending);
                //app_bt_audio_stop_a2dp_playing(currentId);
            }
       }
    }
    else
    {
        if ((awayId != 0xFF) && app_bt_is_a2dp_streaming(GET_BT_ID(awayId)))
        {
            app_bt_audio_stop_a2dp_playing(GET_BT_ID(awayId));
        }
    }

set_hf:
    if (evt & SASS_REJECT_SCO_ON_AWAY_DEV_BIT)
    {
        app_bt_hf_set_reject_dev(GET_BT_ID(awayId));
    }
    return reason;
}

uint8_t gfps_sass_switch_back_evt_hdl(uint8_t device_id, uint8_t evt)
{
    bt_bdaddr_t currAddr, lastAddr;
    uint8_t empty[6] = {0};

    if ((evt != SASS_EVT_SWITCH_BACK) && (evt != SASS_EVT_SWITCH_BACK_AND_RESUME))
    {
        return  SASS_STATUS_FAIL;
    }

    app_bt_get_device_bdaddr(GET_BT_ID(device_id), currAddr.address);
    gfps_sass_get_last_dev(&lastAddr);

    if (!memcmp(lastAddr.address, empty, sizeof(bt_bdaddr_t)))
    {
        TRACE(0, "sass switch back hdl last dev is NULL!");
    }
    else if (memcmp(lastAddr.address, currAddr.address, sizeof(bt_bdaddr_t)))
    {
        TRACE(0, "sass switch back to dev:");
        DUMP8("%02x ", lastAddr.address, 6);
        if (!app_bt_is_acl_connected_byaddr((bt_bdaddr_t *)&lastAddr))
        {
            uint8_t maxLink = gfps_sass_get_multi_status() ? BT_DEVICE_NUM : 1;            
            if (app_bt_count_connected_device() >= maxLink)
            {
                gfps_sass_set_reconnecting_dev(&lastAddr, evt);
                gfps_sass_set_disconnecting_dev(&currAddr);
                app_ui_destroy_device(&currAddr, false);
            }
            else
            {
                app_ibrt_if_choice_mobile_connect((bt_bdaddr_t *)&lastAddr);
            }
        }
        else if (evt == SASS_EVT_SWITCH_BACK_AND_RESUME && gfps_sass_is_profile_connected(&lastAddr))
        {
            SassBtInfo *lastInfo = gfps_sass_get_connected_dev_by_addr(&lastAddr);
            uint8_t currId = SET_BT_ID(app_bt_audio_get_curr_playing_a2dp());
            uint8_t lastId = lastInfo->connId;
            if (app_bt_is_a2dp_streaming(GET_BT_ID(lastId)) && \
                GET_PROFILE_STATE(lastInfo->audState, AUDIO, A2DP) && \
                (currId != lastId))
            {
                gfps_sass_switch_a2dp(currId, lastId, true);
                lastInfo->isNeedResume = true;
            }
            else
            {
                app_bt_ibrt_audio_play_a2dp_stream(GET_BT_ID(lastId));
            }
        }else {
            TRACE(0, "sass switch back already connect!!");
        }
    }
    else
    {
        TRACE(0, "sass switch back hdl disconnect itself");
        gfps_sass_set_disconnecting_dev(&currAddr);
        app_ui_destroy_device(&currAddr, false);
    }

    return SASS_STATUS_OK;
}

void gfps_sass_switch_src_hdl(uint8_t devId, uint8_t *data)
{
    uint8_t evt = data[0];
    uint8_t reason = SASS_STATUS_OK;
#ifdef IBRT
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (TWS_UI_SLAVE != app_ibrt_if_get_ui_role()&& p_ibrt_ctrl->init_done)
    {
        reason = gfps_sass_switch_src_evt_hdl(devId, evt);
    }
#endif

    if (reason == SASS_STATUS_OK)
    {
        gfps_send_msg_ack(devId, FP_MSG_GROUP_SASS, FP_MSG_SASS_SWITCH_ACTIVE_SOURCE);
    }
    else if (reason == SASS_STATUS_REDUNTANT)
    {
        gfps_send_msg_nak(devId, FP_MSG_NAK_REASON_REDUNDANT_ACTION, FP_MSG_GROUP_SASS,\
                                    FP_MSG_SASS_SWITCH_ACTIVE_SOURCE);
    }
    else
    {
        gfps_send_msg_nak(devId, FP_MSG_NAK_REASON_NOT_ALLOWED, FP_MSG_GROUP_SASS,\
                                    FP_MSG_SASS_SWITCH_ACTIVE_SOURCE);
    }
    TRACE(3,"%s evt:0x%0x, reason:%d",__func__, evt, reason);
}

void gfps_sass_switch_back_hdl(uint8_t devId, uint8_t *data)
{
    uint8_t evt = data[0];
    uint8_t reason = SASS_STATUS_OK;
#ifdef IBRT
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if(TWS_UI_SLAVE != app_ibrt_if_get_ui_role()&& p_ibrt_ctrl->init_done)
    {
        reason = gfps_sass_switch_back_evt_hdl(devId, evt);
    }
#endif

    if (reason == SASS_STATUS_OK)
    {
        gfps_send_msg_ack(devId, FP_MSG_GROUP_SASS, FP_MSG_SASS_SWITCH_BACK);
    }
    else
    {
        gfps_send_msg_nak(devId, FP_MSG_NAK_REASON_NOT_ALLOWED, FP_MSG_GROUP_SASS,\
                                    FP_MSG_SASS_SWITCH_BACK);
    }

    //gfps_sass_ntf_switch_evt(devId, SASS_REASON_UNSPECIFIED);
    TRACE(3,"%s evt:0x%0x, reason:%d",__func__, evt, reason);
}

void gfps_sass_remove_dev_handler(bt_bdaddr_t *addr)
{
    uint8_t maxLink = gfps_sass_get_multi_status() ? 2 : 1;
    uint8_t actDev = 0xFF;
    uint8_t otherKey[FP_ACCOUNT_KEY_SIZE] = {0};
    uint8_t *inUseKey = NULL;
    uint8_t *inUseAddr = NULL;
    SassBtInfo *sInfo = NULL;

    if (addr == NULL)
    {
        return;
    }

    if (!memcmp(sassInfo.reconnInfo.reconnAddr.address, (uint8_t *)addr, sizeof(bt_bdaddr_t)))
    {
        memset((uint8_t *)&(sassInfo.reconnInfo), 0, sizeof(sassInfo.reconnInfo));
    }

    if (!memcmp(sassInfo.disconectingAddr.address, (uint8_t *)addr, sizeof(bt_bdaddr_t)))
    {
        memset((uint8_t *)&(sassInfo.disconectingAddr), 0, sizeof(bt_bdaddr_t));
    }

    sInfo = gfps_sass_get_connected_dev_by_addr(addr);
    if (sInfo)
    {
        if (IS_BT_DEVICE(sInfo->connId))
        {
            gfps_sass_clear_reject_hf_dev(GET_BT_ID(sInfo->connId));
        }

        SassBtInfo *otherInfo = gfps_sass_get_other_connected_dev(sInfo->connId);
        if (sassInfo.activeId == sInfo->connId)
        {
            if (otherInfo && (GET_PROFILE_STATE(otherInfo->audState, AUDIO, AVRCP) || \
                GET_PROFILE_STATE(otherInfo->audState, AUDIO, A2DP) || \
                GET_PROFILE_STATE(otherInfo->audState, AUDIO, HFP)))
            {
                actDev = otherInfo->connId;
            }
            else
            {
                actDev = 0xFF;
            }
            gfps_sass_set_active_dev(actDev);
        }

        if (!memcmp(sassInfo.inUseAddr.address, (uint8_t *)addr, sizeof(bt_bdaddr_t)))
        {
            if (otherInfo)
            {
                if (gfps_sass_is_key_valid(otherInfo->accKey))
                {
                    inUseKey = otherInfo->accKey;
                    inUseAddr = otherInfo->btAddr.address;
                }
                else
                {
                    nv_record_fp_get_key_by_addr(otherInfo->btAddr.address, otherKey);
                    if (gfps_sass_is_key_valid(otherKey))
                    {
                        inUseKey = otherKey;
                        inUseAddr = otherInfo->btAddr.address;
                    }
                    else
                    {
                        TRACE(0, "sass warning:there is no in use device!!!");
                    }
                }
            }
            gfps_sass_set_inuse_acckey(inUseKey, (bt_bdaddr_t *)inUseAddr);
        }

        if (sassInfo.pending.param.activeId == sInfo->connId)
        {
            gfps_sass_clear_pending_proc();
        }

        memset(sInfo, 0, sizeof(SassBtInfo));
        sInfo->connId = 0xFF;
        sassInfo.connNum--;
    }

    if (sassInfo.connNum >= maxLink)
    {
        sassInfo.connAvail = SASS_CONN_NONE_AVAILABLE;
    }
    else
    {
        sassInfo.connAvail = SASS_CONN_AVAILABLE;
        if (sassInfo.connNum == 0) {
            sassInfo.connState = SASS_STATE_NO_CONNECTION;
            sassInfo.focusMode = SASS_CONN_NO_FOCUS;
        }
    }
    TRACE(1, "%s acckey is:", __func__);
    DUMP8("%02x ", sassInfo.connInfo[0].accKey, 16);
    DUMP8("%02x ", sassInfo.connInfo[1].accKey, 16);
}

void gfps_sass_add_dev_handler(uint8_t devId, bt_bdaddr_t *addr)
{
    uint8_t cod[3];
    uint8_t connNum = 0;
    SassBtInfo *btHdl;
    bool isSass = false;

    if (sassInfo.isMulti) {
        connNum = BT_DEVICE_NUM;
    }else {
        connNum = 1;
    }

    app_ui_register_ext_conn_policy_callback(&sass_ibrt_callback);

    for(int i= 0; i < BT_DEVICE_NUM; i++)
    {
        if (!memcmp(sassInfo.connInfo[i].btAddr.address, addr->address, sizeof(bt_bdaddr_t)))
        {
            TRACE(1,"%s has already connected!!!", __func__);
            return;
        }

        if (sassInfo.connInfo[i].connId == devId)
        {
            gfps_sass_remove_dev_handler(&(sassInfo.connInfo[i].btAddr));
        }
    }

    btHdl = gfps_sass_get_free_handler();
    if (!btHdl)
    {
        TRACE(1,"%s has no res for new connection!!!", __func__);
    }

    memset(btHdl, 0, sizeof(SassBtInfo));
    btHdl->connId = devId; 
    memcpy(btHdl->btAddr.address, addr, sizeof(bt_bdaddr_t));

    app_bt_get_remote_cod_by_addr(addr, cod);
    btHdl->devType = gfps_sass_get_dev_type_by_cod(cod);

    isSass = gfps_sass_check_sass_mode(btHdl);
    if (IS_BT_DEVICE(btHdl->connId))
    {
        gfps_sass_clear_reject_hf_dev(GET_BT_ID(btHdl->connId));
    }
    TRACE(5,"%s btHdl->devType:0x%0x num:%d isSass:%d devId:%d", 
            __func__, btHdl->devType, sassInfo.connNum, isSass, devId);
    sassInfo.connNum++;

    if (sassInfo.connNum == 1) {
        sassInfo.connState = SASS_STATE_NO_DATA;
        sassInfo.focusMode = SASS_CONN_NO_FOCUS;
        gfps_sass_update_last_dev(addr);
        if (isSass)
        {
            gfps_sass_set_inuse_acckey(btHdl->accKey, &(btHdl->btAddr));
        }
    }

    if (sassInfo.connNum < connNum){
        sassInfo.connAvail = SASS_CONN_AVAILABLE;
    }else{
        sassInfo.connAvail = SASS_CONN_NONE_AVAILABLE;
    }

    if (!memcmp(sassInfo.reconnInfo.reconnAddr.address, addr, sizeof(bt_bdaddr_t)))
    {
        sassInfo.autoReconn = SASS_AUTO_RECONNECTED;
        if (sassInfo.reconnInfo.evt != SASS_EVT_SWITCH_BACK_AND_RESUME)
        {
            sassInfo.reconnInfo.evt = 0xFF;
        }
    }

    for(int n = 0; n < SESSION_NOUNCE_NUM; n++)
    {
         btHdl->session[n] = (uint8_t)rand();
    }
}

void gfps_sass_del_dev_handler(bt_bdaddr_t *addr, uint8_t reason)
{
    gfps_sass_remove_dev_handler(addr);
    if (reason == BTIF_BEC_MAX_CONNECTIONS || reason == BTIF_BEC_LOCAL_TERMINATED)
    {
        gfps_sass_update_last_dev(addr);
    }
    else if (!gfps_sass_is_any_dev_connected())
    {
        gfps_sass_clear_last_dev();
    }
    else
    {
        TRACE(1,"%s don't update last dev", __func__);
    }
    bes_bt_me_write_access_mode(BTIF_BAM_CONNECTABLE_ONLY, 0);
    TRACE(2,"%s connNum: %d", __func__, sassInfo.connNum);
}

void gfps_sass_connect_handler(uint8_t device_id, const bt_bdaddr_t *addr)
{
    SassEvtParam evtParam;
    evtParam.event = SASS_EVT_ADD_DEV;
    evtParam.devId = device_id;
    memcpy(evtParam.addr.address, addr, sizeof(bt_bdaddr_t));
    gfps_sass_update_state(&evtParam);
}

void gfps_sass_disconnect_handler(uint8_t device_id, const bt_bdaddr_t *addr, uint8_t errCode)
{
    SassEvtParam evtParam;
    evtParam.event = SASS_EVT_DEL_DEV;
    evtParam.devId = device_id;
    evtParam.reason = errCode;
    memcpy(evtParam.addr.address, addr, sizeof(bt_bdaddr_t));
    gfps_sass_update_state(&evtParam);
}

void gfps_sass_update_state(SassEvtParam *evtParam)
{
    uint8_t devId = evtParam->devId;
    bool needUpdate = true;
    SassBtInfo *otherInfo = NULL, *sInfo = NULL;

    switch(evtParam->event)
    {
        case SASS_EVT_ADD_DEV:
        {
            gfps_sass_add_dev_handler(devId, &(evtParam->addr));
            break;
        }

        case SASS_EVT_DEL_DEV:
        {
            gfps_sass_del_dev_handler(&(evtParam->addr), evtParam->reason);
            break;
        }

        case SASS_EVT_UPDATE_CONN_STATE:
        {
            gfps_sass_set_conn_state(evtParam->state.connState);
            break;
        }

        case SASS_EVT_UPDATE_HEAD_STATE:
        {
            SASS_HEAD_STATE_E headstate = evtParam->state.headState;
            gfps_sass_set_head_state(headstate);
            break;
        }

        case SASS_EVT_UPDATE_FOCUS_STATE:
        {
            SASS_FOCUS_MODE_E focusstate = evtParam->state.focusMode;
            gfps_sass_set_focus_mode(focusstate);
            break;
        }

        case SASS_EVT_UPDATE_RECONN_STATE:
        {
            SASS_AUTO_RECONN_E reconnstate = evtParam->state.autoReconn;
            gfps_sass_set_auto_reconn(reconnstate);
            break;
        }

        case SASS_EVT_UPDATE_CUSTOM_DATA:
        {
            gfps_sass_set_custom_data(evtParam->state.cusData);
            break;
        }

        case SASS_EVT_UPDATE_MULTI_STATE:
        {
            gfps_sass_set_conn_available(evtParam->state.connAvail);
            break;
        }

        case SASS_EVT_UPDATE_ACTIVE_DEV:
        {
            needUpdate = false;
            break;
        }

        case SASS_EVT_UPDATE_INUSE_ACCKEY:
        {
#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
            if (TWS_UI_MASTER == app_ibrt_if_get_ui_role())
            {
                gfps_sass_sync_info();
            }
#endif
            break;
        }
        default:
        break;
    }

    gfps_sass_update_adv_data();
    gfps_sass_ntf_conn_status(devId, true, NULL);

    otherInfo  = gfps_sass_get_other_connected_dev(devId);
    sInfo  = gfps_sass_get_connected_dev(devId);

    if (otherInfo && (otherInfo->connId != 0xFF))
    {
        if ((evtParam->event == SASS_EVT_UPDATE_HEAD_STATE) || \
            (evtParam->event == SASS_EVT_UPDATE_ACTIVE_DEV) || \
            (!sInfo)|| \
            (sInfo && !sInfo->isSass)|| \
            (sInfo && sInfo->isSass && otherInfo->isSass && \
            !memcmp(sInfo->accKey, otherInfo->accKey, FP_ACCOUNT_KEY_SIZE)))
        {
            gfps_sass_ntf_conn_status(otherInfo->connId, true, NULL);
        }
        else if (sInfo->isSass && otherInfo->isSass && \
            memcmp(sInfo->accKey, otherInfo->accKey, FP_ACCOUNT_KEY_SIZE))
        {
            SASS_CONN_STATE_E providerState = gfps_sass_get_conn_state();
            if ((otherInfo->state != providerState) && (providerState > SASS_STATE_NO_AUDIO))
            {
                SassStateInfo seekerState;
                uint8_t len;
                gfps_sass_get_adv_data((uint8_t *)&seekerState, &len);
                SET_SASS_CONN_STATE(seekerState.state, CONN_STATE, otherInfo->state);
                gfps_sass_ntf_conn_status(otherInfo->connId, false, (uint8_t *)&seekerState);
            }
        }
    }

    if (needUpdate)
    {
        bes_ble_gap_refresh_adv_state(BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL);
    }
}

void gfps_sass_ind_inuse_acckey(uint8_t devId, uint8_t *data)
{
    char str[8] = "in-use";
    uint8_t auth[8] = {0};
    uint8_t nonce[16] = {0};
    uint8_t keyCount = 0;
    uint8_t accKey[16] = {0};
    uint8_t output[8] = {0};
    if (memcmp(data, str, 6))
    {
        TRACE(1, "%s data error!", __func__);
        return;
    }

    gfps_sass_get_session_nonce(devId, nonce);
    memcpy(nonce + 8, data + 6, 8);
    memcpy(auth, data + 14, 8);

    keyCount = nv_record_fp_account_key_count();
    for (int i = 0; i < keyCount; i++)
    {
        nv_record_fp_account_key_get_by_index(i, accKey);
        gfps_encrypt_messasge(accKey, nonce, data, 6, output);
        if (!memcmp(output, auth, 8))
        {
            bt_bdaddr_t btAddr;
            uint8_t initAcc[FP_ACCOUNT_KEY_SIZE];
            gfps_sass_get_inuse_acckey(initAcc);
            gfps_sass_set_inuse_acckey_by_dev(devId, accKey);
            if (app_bt_get_device_bdaddr(GET_BT_ID(devId), btAddr.address) && \
                gfps_sass_is_truely_sass_dev(devId))
            {
                nv_record_fp_update_addr(i, btAddr.address);
                if (gfps_sass_get_active_dev() == 0xFF)
                {
                    gfps_sass_set_inuse_acckey(accKey, &btAddr);
                }
            }

            if (!gfps_sass_is_other_sass_dev(devId) || \
                memcmp(initAcc, accKey, FP_ACCOUNT_KEY_SIZE) || \
                gfps_sass_is_need_ntf_status(devId))
            {
                SassEvtParam evtParam;
                evtParam.devId = devId;
                evtParam.event = SASS_EVT_UPDATE_INUSE_ACCKEY;
                gfps_sass_update_state(&evtParam);
            }

            if (gfps_sass_is_need_ntf_switch(devId))
            {
                SASS_REASON_E reason = gfps_sass_get_switch_reason(gfps_sass_get_active_dev());
                gfps_sass_ntf_switch_evt(devId, reason);
            }
            break;
        }
    }
}

void gfps_sass_handler(uint8_t devId, uint8_t evt, void *param)
{
    TRACE(3,"%s id:%d evt:0x%0x", __func__, devId, evt);

    switch(evt)
    {
        case FP_MSG_SASS_GET_CAPBILITY:
        {  
            gfps_sass_ntf_capability(devId);
            break;
        }

        case FP_MSG_SASS_NTF_CAPBILITY:
        {
            gfps_sass_set_capability(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_SET_MULTIPOINT_STATE:
        {
            gfps_sass_set_multipoint_hdl(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_SET_SWITCH_PREFERENCE:
        {
            gfps_sass_set_switch_pref_hdl(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_GET_SWITCH_PREFERENCE:
        {
            gfps_sass_ntf_switch_pref(devId);
            break;
        }
        case FP_MSG_SASS_SWITCH_ACTIVE_SOURCE:
        {
            gfps_sass_switch_src_hdl(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_SWITCH_BACK:
        {
            gfps_sass_switch_back_hdl(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_GET_CONN_STATUS:
        {
            gfps_sass_get_conn_hdl(devId);
            break;
        }
        case FP_MSG_SASS_NTF_INIT_CONN:
        {
            gfps_sass_set_init_conn(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_IND_INUSE_ACCOUNT_KEY:
        {
            gfps_sass_ind_inuse_acckey(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_SEND_CUSTOM_DATA:
        {
            gfps_sass_set_custom_data(devId, (uint8_t *)param);
            break;
        }
        case FP_MSG_SASS_SET_DROP_TGT:
        {
            gfps_sass_set_drop_dev(devId, (uint8_t *)param);
            break;
        }
        default:
        break;
    }
}

#endif
