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
#if HID_ULL_ENABLE
#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hwtimer_list.h"
#include "rwble_config.h"

#include "gaf_ull_hid_stream.h"
#include "gaf_media_common.h"
#include "gaf_media_stream.h"
#include "gaf_media_sync.h"
#include "app_gaf_dbg.h"
#include "app_bap_data_path_itf.h"
#include "hci_transport.h"

extern "C" uint32_t btdrv_reg_op_cig_anchor_timestamp(uint8_t link_id);

#define ULL_HID_STREAM_HID_OUTPUT_BUF_SIZE        40

static uint32_t ull_hid_data_seq_num = 0;
static uint32_t record_tigger_type = 0;
static HWTIMER_ID gaf_ull_hid_timer_id = NULL;
static GAF_AUDIO_STREAM_ENV_T UllHidStreamEnv;

uint8_t ull_hid_data[ULL_HID_STREAM_HID_OUTPUT_BUF_SIZE] = {0};
ULL_HID_DATA_CHANGED_CB_T ull_hid_data_changed_cb = NULL;

void gaf_ull_hid_data_changed_register_cb(ULL_HID_DATA_CHANGED_CB_T cb)
{
    //Todo:get hid data
    ull_hid_data_changed_cb = cb;
}

static uint32_t gaf_ull_hid_upstream_calculate_trigger_timer(void)
{
    uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(UllHidStreamEnv.stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    uint32_t trigger_offset_timer = 100;
    if (latest_iso_bt_time > current_bt_time)
    {
        latest_iso_bt_time -= ((int32_t)(UllHidStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000));
    }
    uint32_t trigger_bt_time = latest_iso_bt_time + trigger_offset_timer;

    uint32_t timerPeriodUs;
    while (trigger_bt_time < current_bt_time+2000)
    {
        trigger_bt_time += UllHidStreamEnv.stream_info.captureInfo.isoIntervalUs;
    }

    timerPeriodUs = trigger_bt_time-current_bt_time;

    LOG_I("last anch %d us current bt time %d us trigger time %d us timer period %d us",
        latest_iso_bt_time, current_bt_time, trigger_bt_time, timerPeriodUs);

    return timerPeriodUs;
}

static uint32_t gaf_ull_hid_upstream_calculate_next_timer_point(uint32_t current_bt_time_us)
{
    static uint32_t latest_iso_bt_time = 0;
    uint32_t trigger_bt_time = 0;
    uint32_t trigger_offset_timer = 100;

    if (record_tigger_type == 0)
    {
        latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
            BLE_ISOHDL_TO_ACTID(UllHidStreamEnv.stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));

        trigger_bt_time = latest_iso_bt_time + trigger_offset_timer;
    }
    else
    {
        trigger_bt_time = latest_iso_bt_time + (int32_t)UllHidStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1250 + trigger_offset_timer;
    }
    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);

    while (gapUs < 0)
    {
        trigger_bt_time += ((int32_t)(UllHidStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1250));
        gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);
    }
    LOG_I("last anch %d us current bt time %d us trigger time %d us gapUs %d",
        latest_iso_bt_time, current_bt_time_us, trigger_bt_time, gapUs);
    return gapUs;
}

static void gaf_ull_hid_upstream_timer_handler_in_bt_thread(uint32_t timerPeriodUs)
{
    static uint8_t simulate_data = 0;
    uint32_t current_bt_time_us = gaf_media_sync_get_curr_time();
    simulate_data++;
    memset((uint8_t *)ull_hid_data, simulate_data, sizeof(ull_hid_data));

    if (ull_hid_data_changed_cb)
    {
        ull_hid_data_changed_cb(ull_hid_data);
    }
    DUMP8("%02x ", ull_hid_data, 4);
#ifdef IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
    if (simulate_data == 1) {
        hal_iomux_tportset(0);
    }
    if (simulate_data == 2) {
        hal_iomux_tportset(1);
    }
#endif
    bes_ble_bap_iso_dp_send_data(UllHidStreamEnv.stream_info.captureInfo.aseChInfo[0].ase_handle,
        ull_hid_data_seq_num++, ull_hid_data, sizeof(ull_hid_data), current_bt_time_us);
#ifdef IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
    //best patch to bes_hci_send_data when test;
    hal_iomux_tportclr(0);
    hal_iomux_tportclr(1);
#endif
}

static void gaf_ull_hid_upstream_timer_handler(void *param)
{
    uint32_t lock;
    static uint32_t timerPeriodUs = 0;
    lock = int_lock();
    uint32_t current_bt_time_us = gaf_media_sync_get_curr_time();

    if (record_tigger_type == 0)
    {
        timerPeriodUs = gaf_ull_hid_upstream_calculate_next_timer_point(current_bt_time_us);
        record_tigger_type = 1;
    }
    else
    {
        timerPeriodUs = gaf_ull_hid_upstream_calculate_next_timer_point(current_bt_time_us);
        record_tigger_type = 0;
    }
    hwtimer_start(gaf_ull_hid_timer_id, US_TO_TICKS(timerPeriodUs));
    int_unlock(lock);
    app_bt_start_custom_function_in_bt_thread(timerPeriodUs, 0,
        (uint32_t)gaf_ull_hid_upstream_timer_handler_in_bt_thread);
}

static void gaf_ull_hid_upstream_start_trigger(void *param)
{
    hwtimer_update(gaf_ull_hid_timer_id, gaf_ull_hid_upstream_timer_handler, 0);
    uint32_t timerPeriodUs = gaf_ull_hid_upstream_calculate_trigger_timer();
    hwtimer_start(gaf_ull_hid_timer_id, US_TO_TICKS(timerPeriodUs));
}

static void gaf_ull_hid_upstream_start(void)
{
    LOG_I("gaf_ull_hid_upstream_start.");
    if (NULL == gaf_ull_hid_timer_id)
    {
        gaf_ull_hid_timer_id =
            hwtimer_alloc(gaf_ull_hid_upstream_timer_handler, NULL);
    }

    ull_hid_data_seq_num = 0;

    btif_me_write_bt_sleep_enable(0);

    hwtimer_update(gaf_ull_hid_timer_id, gaf_ull_hid_upstream_start_trigger, 0);
    hwtimer_start(gaf_ull_hid_timer_id, MS_TO_TICKS(50));

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_78M);
}

void gaf_ull_hid_data_update_upstream_start(app_bap_ascs_ase_t * p_bap_ase_info, uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &UllHidStreamEnv;
    pStreamEnv->stream_info.captureInfo.aseChInfo[0].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_bap_ase_info->cis_hdl);
    pStreamEnv->stream_info.captureInfo.cigSyncDelayUs = p_bap_ase_info->cig_sync_delay;
    pStreamEnv->stream_info.captureInfo.isoIntervalUs = p_bap_ase_info->iso_interval_us;
    pStreamEnv->stream_info.captureInfo.bnM2S = p_bap_ase_info->bn_m2s;
    pStreamEnv->stream_info.captureInfo.bnS2M = p_bap_ase_info->bn_s2m;
    pStreamEnv->stream_info.captureInfo.codec_info.frame_ms =
        gaf_stream_common_frame_duration_parse(p_bap_ase_info->p_cfg->param.frame_dur);
    pStreamEnv->stream_info.captureInfo.aseChInfo[0].ase_handle = p_bap_ase_info->cis_hdl;
    gaf_ull_hid_upstream_start();
}

void gaf_ull_hid_upstream_stop(uint8_t con_lid)
{
    if (gaf_ull_hid_timer_id)
    {
        hwtimer_free(gaf_ull_hid_timer_id);
        gaf_ull_hid_timer_id = NULL;
    }
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
}
#endif

/// @} APP