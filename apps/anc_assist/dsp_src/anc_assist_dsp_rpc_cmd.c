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
#include "string.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "anc_assist_dsp.h"
#include "anc_assist_comm.h"
#include "app_rpc_api.h"

static void comm_dsp_anc_assist_send_data_to_bth_handler(uint8_t* ptr, uint16_t len)
{
    app_rpc_send_data_no_rsp(ANC_ASSIST_CORE, TASK_ANC_ASSIST_CMD_RSP, ptr, len);
}

static void comm_dsp_anc_assist_receive_data_from_bth_handler(uint8_t* ptr, uint16_t len)
{
    anc_assist_dsp_capture_cmd(ptr, len);
}

static void comm_dsp_anc_assist_tx_done_handler(uint16_t cmdCode, uint8_t* ptr, uint16_t len)
{
   // DUMP8("%02x ", ptr, len);
}

RPC_ANC_ASSIST_TASK_CMD_TO_ADD(TASK_ANC_ASSIST_CMD_RSP,
                                "ANC Assist CMD",
                                comm_dsp_anc_assist_send_data_to_bth_handler,
                                comm_dsp_anc_assist_receive_data_from_bth_handler,
                                0,
                                NULL,
                                NULL,
                                comm_dsp_anc_assist_tx_done_handler);

static void comm_dsp_anc_assist_receive_pcm_from_bth_handler(uint8_t* ptr, uint16_t len)
{
    anc_assist_dsp_capture_process(NULL, 0);
}

RPC_ANC_ASSIST_TASK_CMD_TO_ADD(TASK_ANC_ASSIST_DATA_NO_RSP,
                                "ANC Assist Data",
                                NULL,
                                comm_dsp_anc_assist_receive_pcm_from_bth_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);