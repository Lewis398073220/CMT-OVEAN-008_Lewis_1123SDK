/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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

#include "rwip_config.h"

#if (BLE_APP_PRESENT)
#if (BLE_ANC_CLIENT)

#include "app_ancc.h"
#include "anc_common.h"
#include "app.h"        // Application Definitions
#include "app_task.h"   // application task definitions
#include "ancc_task.h"  // health thermometer functions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"  // Platform Definitions

#include "co_math.h"
#include "ke_timer.h"
#include "gapm_msg.h"


static app_anc_notification_count anc_info[BLE_CONNECTION_MAX];

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/
void app_ancc_init_info(void)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        memset(anc_info, 0, sizeof(app_anc_notification_count));
        anc_info[i].conidx = 0xFF;
    }
}

app_anc_notification_count *app_ancc_get_free_info()
{
    app_anc_notification_count *info = NULL;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (anc_info[i].conidx != 0xFF)
        {
            info = &(anc_info[i]);
            break;
        }
    }
    return info;
}

app_anc_notification_count *app_ancc_get_count(uint8_t conidx)
{
    app_anc_notification_count *info = NULL;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (anc_info[i].conidx == conidx)
        {
            info = &(anc_info[i]);
            break;
        }
    }
    return info;
}

void app_ancc_write_continue(uint8_t conidx)
{
    ancc_write_continue(conidx);
}

bool app_ancc_is_need_continue(uint8_t conidx)
{
    return ancc_is_need_wirte_continue(conidx);
}

void app_ancc_set_write_state(uint8_t conidx, bool state)
{
    ancc_set_wirte_state(conidx, state);
}

void app_ancc_add_ancc(void)
{
    BLE_APP_DBG("app_ancc_add_ancc");
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    /// Fill message
    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = (uint8_t)(SVC_SEC_LVL(AUTH) | ATT_UUID(128));
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_ANCC;
    req->start_hdl  = 0;

    /// Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Initialize application and enable ANCC profile.
 *
 ****************************************************************************************
 */
void app_ancc_enable(uint8_t conidx)
{
    BLE_FUNC_ENTER();

    if (!app_ancc_is_need_continue(conidx))
    {
        // Allocate the message
        struct ancc_enable_req *req = KE_MSG_ALLOC(ANCC_ENABLE_REQ,
                                                   KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), conidx),
                                                   TASK_APP,
                                                   ancc_enable_req);

        // Fill in the parameter structure
        req->conidx = conidx;
        // Send the message
        ke_msg_send(req);
        app_anc_notification_count *info = app_ancc_get_free_info();
        info->conidx = conidx;
    }
    else
    {
        app_ancc_write_continue(conidx);
    }
}

void app_ancc_disconnect_handle(uint8_t conidx)
{
    BLE_FUNC_ENTER();
    app_ancc_set_write_state(conidx, false);
    app_anc_notification_count *info = app_ancc_get_count(conidx);
    info->conidx = 0xFF;
}

void app_ancc_read(uint8_t conidx, uint16_t hdl)
{
    /// ask client to send read command
    anc_cli_rd_cmd_t *cmd = KE_MSG_ALLOC(ANCC_READ_CMD,
                                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), conidx),
                                         KE_BUILD_ID(TASK_APP, conidx),
                                         anc_cli_rd_cmd);

    cmd->conidx = conidx;
    cmd->hdl = hdl;
    ke_msg_send(cmd);
}

void app_ancc_write(uint8_t conidx, uint16_t hdl, uint16_t value_length, uint8_t *value)
{
    if (NULL == value)
    {
        return ;
    }
    /// ask client to send write command
    anc_cli_wr_cmd_t *cmd = (anc_cli_wr_cmd_t *)ke_msg_alloc(ANCC_WRITE_CMD,
                                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), conidx),
                                         KE_BUILD_ID(TASK_APP, conidx),
                                         sizeof(anc_cli_wr_cmd_t) + value_length);

    memset(cmd, 0, sizeof(anc_cli_wr_cmd_t));
    cmd->conidx = conidx;
    cmd->hdl = hdl;
    cmd->value_length = value_length;
    if (value_length > 0)
    {
        memcpy(cmd->value, value, value_length);
    }
    ke_msg_send(cmd);
}

void app_ancc_get_notification_info(uint8_t conidx, app_anc_get_ntf_attr_param *param)
{
    app_anc_info_param infoParam;
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
    if (ancc_env == NULL || param == NULL)
    {
        TRACE(1, "%s env is NULL!", __func__);
        return;
    }

    uint16_t len = param->attLen + sizeof(uint32_t) + sizeof(enum anc_cmd_id);
    uint16_t hdl = ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl;

    infoParam.cmdId = CmdIdGetNtfAttr;
    memcpy(infoParam.param, (uint8_t *)&(param->ntfId), sizeof(uint32_t));
    memcpy(infoParam.param + sizeof(uint32_t), param->att, param->attLen);
    app_ancc_write(conidx, hdl, len, (uint8_t *)&infoParam);
}

void app_ancc_get_app_info(uint8_t conidx, app_anc_get_app_attr_param *param)
{
    app_anc_info_param infoParam;
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
    if (ancc_env == NULL || param == NULL)
    {
        TRACE(1, "%s env is NULL!", __func__);
        return;
    }

    uint16_t len = param->appIdLen + param->attLen + sizeof(enum anc_cmd_id);
    uint16_t hdl = ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl;

    infoParam.cmdId = CmdIdGetAppAttr;
    memcpy(infoParam.param, (uint8_t *)&(param->appId), param->appIdLen);
    memcpy(infoParam.param + param->appIdLen, param->att, param->attLen);
    app_ancc_write(conidx, hdl, len, (uint8_t *)&infoParam);
}

void app_ancc_get_message_detail(uint8_t conidx, uint32_t uid, app_anc_get_msg_detail detail)
{
    app_anc_get_ntf_attr_param ntfInfo;
    uint8_t att[32] = {0};
    uint8_t len = 0;

    if (detail.appId) {
        att[len++] = AttrIdAppId;
    }

    if (detail.title) {
        att[len++] = AttrIdTitle;
        att[len] = detail.titleLen;
        len += 2;
    }

    if(detail.subTitle) {
        att[len++] = AttrIdSubTitle;
        att[len] = detail.subtitleLen;
        len += 2;
    }

    if (detail.msg) {
        att[len++] = AttrIdMsg;
        att[len] = detail.msgLen;
        len += 2;
    }

    if (detail.msgSize) {
        att[len++] = AttrIdMsgSize;
    }

    if (detail.date) {
        att[len++] = AttrIdDate;
    }

    ntfInfo.ntfId = uid;
    ntfInfo.att = att;
    ntfInfo.attLen = len;
    app_ancc_get_notification_info(conidx, &ntfInfo);
}

void app_anc_parse_message_detail(uint8_t *data, uint32_t dataLen)
{
    uint32_t uid = (uint32_t)(data[1]);
    uint32_t len = sizeof(uint32_t) + sizeof(enum anc_cmd_id);
    uint8_t attrId;
    uint16_t attrLen = 0;
    TRACE(1, "Received ANC rsp ntf attr uid:%d", uid);

    while (dataLen > len)
    {
        attrId = data[len];
        attrLen = (uint16_t)(data[len+1]);
        len += (3+attrLen);
        TRACE(1, "Received ANC attr:%d data", attrId);
        DUMP8("%2x ", data+len-attrLen, attrLen);

    }
}

void app_anc_parse_app_detail(uint8_t *data, uint32_t dataLen)
{
    uint32_t uid = (uint32_t)(data[1]);
    uint32_t len = sizeof(enum anc_cmd_id);
    uint8_t attrId;
    uint16_t attrLen = 0;
    TRACE(1, "Received ANC rsp ntf attr uid:%d", uid);

    while (data[len] != '\0')
    {
        len++;
    }

    while (dataLen > len)
    {
        attrId = data[len];
        attrLen = (uint16_t)(data[len+1]);
        len += (3+attrLen);
        TRACE(1, "Received ANC attr:%d data", attrId);
        DUMP8("%2x ", data+len-attrLen, attrLen);

    }
}

void app_ancc_parse_notification_info(app_anc_srv_param *param)
{
    if (param == NULL || param->data == NULL)
    {
        TRACE(1, "%s param is NULL!", __func__);
        return;
    }

    uint8_t conidx = param->conidx;
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
    if (ancc_env == NULL)
    {
        TRACE(1, "%s env is NULL!", __func__);
        return;
    }

    if (param->hdl == ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl)
    {
        anc_notification_info *info = (anc_notification_info *)param->data;

        uint8_t flag = info->evtFlag;
        uint8_t categoryId = info->cateID;
        uint8_t count = info->cateCount;
        app_anc_notification_count *countinfo = app_ancc_get_count(conidx);
        if (countinfo)
        {
            uint8_t *pCount = (uint8_t *)&(countinfo->count);
            pCount[categoryId] = count;
        }
        TRACE(3, "Received ANC notification count:%d, id:%d, flag:%d", count, categoryId, flag);

        switch (info->evtID)
        {
            case EventIDNtfAdded:
            {
                TRACE(0, "Received ANC notification added.");
                app_anc_get_msg_detail detail = 
                    {
                        .appId = true,
                        .title = true,
                        .subTitle = true,
                        .msg = true,
                        .msgSize = true,
                        .date = true,
                        .titleLen = 16,
                        .subtitleLen = 16,
                        .msgLen = 128,
                    };
                app_ancc_get_message_detail(conidx, info->ntfUID, detail);
                break;
            }
            case EventIDNtfModified:
            {
                TRACE(0, "Received ANC notification modified.");
                break;
            }
            case EventIDNtfRemoved:
            {
                TRACE(0, "Received ANC notification removed.");
                break;
            }
            default:
                break;
        }
    }
    else if (param->hdl == ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl)
    {
         if (param->data[0] == CmdIdGetNtfAttr)
         {
             app_anc_parse_message_detail(param->data, param->len);
         }
         else if (param->data[0] == CmdIdGetAppAttr)
         {
             app_anc_parse_app_detail(param->data, param->len);
         }
         else
         {
             TRACE(0, "Received ANC data source error handle!");
         }
    }
    else
    {
        TRACE(0, "Received ANC notification error handle!");
    }
}

#endif  //BLE_ANC_CLIENT
#endif  //BLE_APP_PRESENT

/// @} APP
