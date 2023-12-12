/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#if BLE_AUDIO_ENABLED
#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "bluetooth_bt_api.h"
#include "bluetooth_ble_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hwtimer_list.h"
#include "rwble_config.h"

#include "gaf_non_codec_stream.h"
#include "gaf_media_common.h"
#include "gaf_media_stream.h"
#include "gaf_media_sync.h"
#include "gaf_stream_dbg.h"

extern "C" uint32_t btdrv_reg_op_cig_anchor_timestamp(uint8_t link_id);

#define NON_CODEC_STREAM_OUTPUT_BUF_SIZE        14
#define SIMULATED_NON_CODEC_DATA_PATTER_ENABLED

static uint32_t non_codec_stream_data_seq_num = 0;
static HWTIMER_ID gaf_non_codec_timer_id = NULL;
static uint8_t non_audio_is_on[GAF_MAXIMUM_CONNECTION_COUNT] = {0};
static GAF_AUDIO_STREAM_ENV_T gpNonCodecStreamEnv;
static uint8_t non_codec_stream_output_buf[NON_CODEC_STREAM_OUTPUT_BUF_SIZE];

uint8_t g_head_tracking_data[NON_CODEC_STREAM_OUTPUT_BUF_SIZE] = {0};
HEAD_TRACKING_DATA_CHANGED_CB_T head_tracking_data_changed_cb = NULL;

void gaf_media_head_track_data_changed_register_cb(HEAD_TRACKING_DATA_CHANGED_CB_T cb)
{
    head_tracking_data_changed_cb = cb;
}

void gaf_media_head_tracking_data_changed_handler(uint8_t *headTrack_data)
{
    memcpy(headTrack_data, g_head_tracking_data, sizeof(g_head_tracking_data));
}

void gaf_non_codec_stream_update_output_data(uint8_t* pBuf, uint32_t bufLen)
{
    memcpy(non_codec_stream_output_buf, pBuf, (bufLen > NON_CODEC_STREAM_OUTPUT_BUF_SIZE)?NON_CODEC_STREAM_OUTPUT_BUF_SIZE:bufLen);
}

static uint32_t gaf_non_codec_upstream_calculate_trigger_timer(void)
{
    uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(gpNonCodecStreamEnv.stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    if (latest_iso_bt_time > current_bt_time)
    {
        latest_iso_bt_time -= ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000));
    }
    uint32_t trigger_bt_time = latest_iso_bt_time+(gpNonCodecStreamEnv.stream_info.captureInfo.cigSyncDelayUs/
                                gpNonCodecStreamEnv.stream_info.captureInfo.bnS2M/2);

    uint32_t timerPeriodUs;
    while (trigger_bt_time < current_bt_time+2000)
    {
        trigger_bt_time += gpNonCodecStreamEnv.stream_info.captureInfo.isoIntervalUs;
    }

    timerPeriodUs = trigger_bt_time-current_bt_time;

    LOG_I("last anch %d us current bt time %d us trigger time %d us timer period %d us",
        latest_iso_bt_time, current_bt_time, trigger_bt_time, timerPeriodUs);

    return timerPeriodUs;
}

static uint32_t gaf_non_codec_upstream_calculate_next_timer_point(uint32_t current_bt_time_us)
{
    uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(gpNonCodecStreamEnv.stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
    if (latest_iso_bt_time > current_bt_time_us)
    {
        latest_iso_bt_time -= ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000));
    }
    uint32_t trigger_bt_time = latest_iso_bt_time+(gpNonCodecStreamEnv.stream_info.captureInfo.cigSyncDelayUs/
                                gpNonCodecStreamEnv.stream_info.captureInfo.bnS2M/2);

    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);

    while (gapUs < 0)
    {
        trigger_bt_time += ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000));
        gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);
    }

    LOG_D("next timer pointer: last anch %d us current bt time %d us trigger time %d us timer period %d us",
        latest_iso_bt_time, current_bt_time_us, trigger_bt_time, gapUs);
    return gapUs;
}

static void gaf_non_codec_upstream_timer_handler_in_bt_thread(uint32_t current_bt_time_us)
{
#ifdef SIMULATED_NON_CODEC_DATA_PATTER_ENABLED
    static uint8_t simulate_data = 0;
    simulate_data++;
    memset((uint8_t *)non_codec_stream_output_buf, simulate_data, sizeof(non_codec_stream_output_buf));
#endif

    if (head_tracking_data_changed_cb)
    {
        head_tracking_data_changed_cb(non_codec_stream_output_buf);
    }

    bes_ble_bap_iso_dp_send_data(gpNonCodecStreamEnv.stream_info.captureInfo.aseChInfo[0].ase_handle,
        non_codec_stream_data_seq_num++, non_codec_stream_output_buf, sizeof(non_codec_stream_output_buf), current_bt_time_us);
    LOG_D("timer ts: %d FRAME MS %d", current_bt_time_us,
        (int8_t)gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms);

    uint32_t latest_iso_bt_time = btdrv_reg_op_cig_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(gpNonCodecStreamEnv.stream_info.captureInfo.aseChInfo[0].iso_channel_hdl));
    if (latest_iso_bt_time > current_bt_time_us)
    {
        latest_iso_bt_time -= ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000));
    }

    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(latest_iso_bt_time, current_bt_time_us);
    int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);

    LOG_D("anchor point: %d - gap: %d", latest_iso_bt_time, gapUs_abs);

    if (gapUs_abs < ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000))/2)
    {
        uint32_t timerPeriodUs = gaf_non_codec_upstream_calculate_next_timer_point(current_bt_time_us);
        LOG_D("set timer %d us", timerPeriodUs);
        hwtimer_start(gaf_non_codec_timer_id, US_TO_TICKS(timerPeriodUs));
    }
    else
    {
        uint32_t updated_bt_time = gaf_media_sync_get_curr_time();
        int32_t passedUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, updated_bt_time);
        LOG_D("passUs %d", passedUs);
        hwtimer_start(gaf_non_codec_timer_id, US_TO_TICKS(
            ((int32_t)(gpNonCodecStreamEnv.stream_info.captureInfo.codec_info.frame_ms*1000)) - passedUs));
    }
}

static void gaf_non_codec_upstream_timer_handler(void *param)
{
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    app_bt_start_custom_function_in_bt_thread(current_bt_time, 0,
        (uint32_t)gaf_non_codec_upstream_timer_handler_in_bt_thread);
}

static void gaf_non_codec_upstream_start_trigger(void *param)
{
    hwtimer_update(gaf_non_codec_timer_id, gaf_non_codec_upstream_timer_handler, 0);
    uint32_t timerPeriodUs = gaf_non_codec_upstream_calculate_trigger_timer();
    hwtimer_start(gaf_non_codec_timer_id, US_TO_TICKS(timerPeriodUs));
}

uint8_t gaf_stream_non_audio_stream_on(uint8_t con_lid)
{
    LOG_I("no le audio stream on.");
    return non_audio_is_on[con_lid];
}

void gaf_non_codec_upstream_start(void)
{
    if (NULL == gaf_non_codec_timer_id)
    {
        gaf_non_codec_timer_id =
            hwtimer_alloc(gaf_non_codec_upstream_timer_handler, NULL);
    }

    non_codec_stream_data_seq_num = 0;

    btif_me_write_bt_sleep_enable(0);

    hwtimer_update(gaf_non_codec_timer_id, gaf_non_codec_upstream_start_trigger, 0);
    hwtimer_start(gaf_non_codec_timer_id, MS_TO_TICKS(50));

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_26M);
}

void gaf_simulated_sensor_data_update_upstream_start(app_bap_ascs_ase_t * p_bap_ase_info, uint8_t ase_lid)
{
    GAF_AUDIO_STREAM_ENV_T* pStreamEnv = &gpNonCodecStreamEnv;
    pStreamEnv->stream_info.captureInfo.aseChInfo[0].iso_channel_hdl = BLE_ISOHDL_TO_ACTID(p_bap_ase_info->cis_hdl);
    pStreamEnv->stream_info.captureInfo.cigSyncDelayUs = p_bap_ase_info->cig_sync_delay;
    pStreamEnv->stream_info.captureInfo.isoIntervalUs = p_bap_ase_info->iso_interval_us;
    pStreamEnv->stream_info.captureInfo.bnM2S = p_bap_ase_info->bn_m2s;
    pStreamEnv->stream_info.captureInfo.bnS2M = p_bap_ase_info->bn_s2m;
    pStreamEnv->stream_info.captureInfo.codec_info.frame_ms =
        gaf_stream_common_frame_duration_parse(p_bap_ase_info->p_cfg->param.frame_dur);
    pStreamEnv->stream_info.captureInfo.aseChInfo[0].ase_handle = p_bap_ase_info->cis_hdl;
    non_audio_is_on[p_bap_ase_info->con_lid] = 1;
    gaf_non_codec_upstream_start();
}

void gaf_non_codec_upstream_stop(uint8_t con_lid)
{
    if (gaf_non_codec_timer_id)
    {
        hwtimer_free(gaf_non_codec_timer_id);
        gaf_non_codec_timer_id = NULL;
    }

    if (!gaf_stream_non_audio_stream_on(con_lid))
    {
        btif_me_write_bt_sleep_enable(1);
    }
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
}
#endif

/// @} APP