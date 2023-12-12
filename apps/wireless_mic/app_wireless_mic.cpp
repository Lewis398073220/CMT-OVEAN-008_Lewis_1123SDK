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
#ifdef AOB_MOBILE_ENABLED
#ifdef WIRELESS_MIC
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "plat_types.h"
#include "audioflinger.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "app_audio.h"
#include "gaf_mobile_media_stream.h"
#include "aob_media_api.h"
#include "aob_mgr_gaf_evt.h"
#include "gaf_codec_lc3.h"
#include "gaf_media_common.h"
#include "ble_audio_ase_stm.h"

#include "app_wireless_mic.h"
#include "app_wireless_mic_opcode.h"
#include "buds.h"
#include "app_key.h"

// Enable Ase -> ISO Stream start -> Start Capture -> Send data
void app_wireless_mic_start_record(void)
{
    TRACE(1,"%s",__func__);
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        // SDU Size = (bit rate * duration) / 8000
        APP_GAF_BAP_SAMPLING_FREQ_32000HZ, 200, BES_BLE_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3,  BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT
    };

    for (uint32_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        // Only playback
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

void app_wireless_mic_opcode_test()
{
    uint8_t cmd_buf[1] = {WL_MIC_TEST_CMD};
    buds_send_data_via_write_command(0, cmd_buf, 1);
    buds_send_data_via_write_command(1, cmd_buf, 1);
}

static void app_wireless_mic_btn_down(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    // TODO: 
}

static const APP_KEY_HANDLE wl_mic_key_handle_cfg[] = {
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOWN},  "test", app_wireless_mic_btn_down, NULL},
};

static void app_wireless_mic_key_init(void)
{
    for (uint8_t i = 0; i< ARRAY_SIZE(wl_mic_key_handle_cfg); i++)
    {
        app_key_handle_registration(&wl_mic_key_handle_cfg[i]);
    }
}

void app_wireless_mic_init()
{
    TRACE(0, "%s", __func__);
    app_wireless_mic_key_init();
}

#endif /* WIRELESS_MIC */
#endif /* AOB_MOBILE_ENABLED */

