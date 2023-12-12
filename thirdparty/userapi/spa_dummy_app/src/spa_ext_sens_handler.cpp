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

#include "spa_sens.h"
#include "spa_ctrl.h"
#include "hal_trace.h"

#include "app_sensor_hub.h"
#if defined(SPA_AUDIO_SENS)

void app_spa_sens_data_snd(unsigned char * pmsg, int opt)
{
    msg_header_t * tmsg = (msg_header_t*)pmsg;
    ASSERT(pmsg != NULL, "%s Invalid ptr",__func__);

    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_DATA_SYNC,pmsg,tmsg->len);
}

static void app_spa_sens_data_snd_handler(unsigned char * ptr, unsigned short len)
{
    app_core_bridge_send_data_without_waiting_rsp(MCU_SENSOR_HUB_TASK_CMD_DATA_SYNC,ptr,len);
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_DATA_SYNC,
                                "spa-sens-data-sync",
                                app_spa_sens_data_snd_handler,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL);
#endif /* SPA_AUDIO_SENS */

#if defined(SPA_AUDIO_BTH)

static void app_spa_sens_data_rcv_handler(unsigned char * ptr, unsigned short len)
{
    msg_header_t * pmsg = (msg_header_t*)ptr;
    ASSERT(pmsg != NULL, "%s Invalid ptr",__func__);
    ASSERT(len == pmsg->len, "%s Invalid ptr %d %d ",__func__,len,pmsg->len);

    spa_ctrl_update_sens_data(ptr, SPA_DATA_UPDATE_SOURCE_SENS);
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_DATA_SYNC,
                                "spa-sens-data-sync",
                                NULL,
                                app_spa_sens_data_rcv_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);

#endif
