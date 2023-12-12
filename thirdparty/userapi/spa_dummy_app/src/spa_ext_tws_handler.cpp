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

#if defined(SPA_AUDIO_BTH)
#include "hal_trace.h"
#include "app_ibrt_customif_cmd.h"
#include "spa_sens.h"
#include "spa_ctrl.h"

extern "C" int tws_ctrl_send_cmd(uint32_t cmd_code, uint8_t *p_buff, uint16_t length);
extern "C" bool app_ibrt_send_cmd_without_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);


void app_spa_tws_sens_data_sync_snd(unsigned char *p_buff, unsigned short length)
{
    tws_ctrl_send_cmd(APP_TWS_CMD_SPA_SENS_DATA_SYNC, p_buff, length);
}

void app_spa_tws_sens_data_sync_snd_handler(unsigned char *p_buff, unsigned short length)
{
    msg_header_t * pmsg = (msg_header_t*)p_buff;
    ASSERT(pmsg != NULL, "%s Invalid ptr",__func__);
    ASSERT(length == pmsg->len, "%s Invalid ptr %d %d ",__func__,length,pmsg->len);

    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SPA_SENS_DATA_SYNC,p_buff,length);
}

void app_spa_tws_sens_data_sync_rcv_handler(unsigned short rsp_seq, unsigned char *p_buff, unsigned short length)
{
    msg_header_t * pmsg = (msg_header_t*)p_buff;
    ASSERT(pmsg != NULL, "%s Invalid ptr",__func__);
    ASSERT(length == pmsg->len, "%s Invalid ptr",__func__);

    spa_ctrl_update_sens_data(p_buff,SPA_DATA_UPDATE_SOURCE_TWS);
}

#endif

