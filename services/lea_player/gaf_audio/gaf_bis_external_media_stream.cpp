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
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "app_utils.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hwtimer_list.h"
#include "rwble_config.h"

#include "gaf_stream_dbg.h"
#include "gaf_bis_external_media_stream.h"
#include "gaf_media_common.h"
#include "gaf_media_stream.h"
#include "gaf_media_sync.h"

#include "bt_drv_reg_op.h"

#include "bes_aob_api.h"

#define GAF_BIS_EXTERNAL_STREAM_MAX 3
#define BIS_DIFF_ANCHOR_US                  4000    //current time diff to bis send anchor

typedef struct{
    bool       started;
    int        btc_buf_num_max;
    uint32_t   data_seq_num;
    uint32_t   prev_timestamp;
    gaf_bis_external_stream_param_t stream_param;
} gaf_bis_external_stream_info_t;

typedef struct
{
    uint8_t stream_num;
    HWTIMER_ID timer_id;
    gaf_bis_external_stream_info_t stream_info[GAF_BIS_EXTERNAL_STREAM_MAX];
} gaf_bis_external_stream_env_t;

static gaf_bis_external_stream_env_t bis_env = {0};

static uint32_t gaf_bis_external_stream_calculate_trigger_timer(gaf_bis_external_stream_info_t *stream_info)
{
    uint32_t latest_iso_bt_time = 0;
#ifndef WIFI_DONGLE
    latest_iso_bt_time = btdrv_reg_op_big_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(stream_info->stream_param.bis_hdl)); 
#endif
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();

    if (latest_iso_bt_time > current_bt_time)
    {
        latest_iso_bt_time -= ((int32_t)(stream_info->stream_param.send_interval_us));
    }

    uint32_t trigger_bt_time = latest_iso_bt_time+(stream_info->stream_param.big_sync_delay_us/
                                stream_info->stream_param.big_bn/2);
    uint32_t timerPeriodUs;
    while (trigger_bt_time < current_bt_time+2000)
    {
        trigger_bt_time += ((int32_t)(stream_info->stream_param.send_interval_us));
    }
    timerPeriodUs = trigger_bt_time-current_bt_time;
    LOG_I("last anch %d us current bt time %d us trigger time %d us timer period %d us",
        latest_iso_bt_time, current_bt_time, trigger_bt_time, timerPeriodUs);
    return timerPeriodUs;
}

#if 0

static uint32_t gaf_bis_external_stream_calculate_next_timer_point(
                                                gaf_bis_external_stream_info_t *stream_info,
                                                uint32_t current_bt_time_us)
{
    uint32_t latest_iso_bt_time = 0;
#ifndef WIFI_DONGLE
    latest_iso_bt_time = btdrv_reg_op_big_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(stream_info->stream_param.bis_hdl));
#endif
    if (latest_iso_bt_time > current_bt_time_us)
    {
        latest_iso_bt_time -= ((int32_t)(stream_info->stream_param.send_interval_ms*1000));
    }
    uint32_t trigger_bt_time = latest_iso_bt_time+(stream_info->stream_param.big_sync_delay_us/
                                stream_info->stream_param.big_bn/2);
    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);
    while (gapUs < 0)
    {
        trigger_bt_time += ((int32_t)(stream_info->stream_param.send_interval_ms*1000));
        gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, trigger_bt_time);
    }
    LOG_D("next timer pointer: last anch %d us current bt time %d us trigger time %d us timer period %d us",
        latest_iso_bt_time, current_bt_time_us, trigger_bt_time, gapUs);
    return gapUs;
}

static void gaf_bis_external_stream_timer_handler_in_bt_thread(uint32_t current_bt_time_us, uint32_t stream_ptr)
{
    uint16_t data_len = 0;
    uint8_t *send_data_buf = NULL;
    gaf_bis_external_stream_info_t *stream_info = (gaf_bis_external_stream_info_t *)stream_ptr;

    /// read data from user layer, and send data to BTC
    stream_info->prev_timestamp = current_bt_time_us;
    if (stream_info->stream_param.read_data_cb)
    {
        stream_info->stream_param.read_data_cb(stream_info->stream_param.stream_idx,
            &send_data_buf, &data_len);
    }

    if (send_data_buf)
    {
        bes_ble_bap_iso_dp_send_data(stream_info->stream_param.bis_hdl,
            stream_info->data_seq_num++, send_data_buf,
            data_len, current_bt_time_us);

        if (stream_info->stream_param.data_buf_free_cb)
        {
            stream_info->stream_param.data_buf_free_cb(stream_info->stream_param.stream_idx, send_data_buf);
        }
    }

    LOG_D("timer ts: %d FRAME MS %d", current_bt_time_us,
        (int8_t)stream_info->stream_param.send_interval_ms);

    /// Calculate the next timing interval
    uint32_t latest_iso_bt_time = 0;
#ifndef WIFI_DONGLE
    latest_iso_bt_time = btdrv_reg_op_big_anchor_timestamp(
        BLE_ISOHDL_TO_ACTID(stream_info->stream_param.bis_hdl));
#endif
    if (latest_iso_bt_time > current_bt_time_us)
    {
        latest_iso_bt_time -= ((int32_t)(stream_info->stream_param.send_interval_ms*1000));
    }
    int32_t gapUs = GAF_AUDIO_CLK_32_BIT_DIFF(latest_iso_bt_time, current_bt_time_us);
    int32_t gapUs_abs = GAF_AUDIO_ABS(gapUs);
    LOG_D("anchor point: %d - gap: %d", latest_iso_bt_time, gapUs_abs);
    if (gapUs_abs < ((int32_t)(stream_info->stream_param.send_interval_ms*1000))/2)
    {
        uint32_t timerPeriodUs = gaf_bis_external_stream_calculate_next_timer_point(stream_info, current_bt_time_us);
        LOG_D("set timer %d us", timerPeriodUs);

        hwtimer_start(stream_info->timer_id, US_TO_TICKS(timerPeriodUs));
    }
    else
    {
        uint32_t updated_bt_time = gaf_media_sync_get_curr_time();
        int32_t passedUs = GAF_AUDIO_CLK_32_BIT_DIFF(current_bt_time_us, updated_bt_time);
        LOG_D("passUs %d", passedUs);
        hwtimer_start(stream_info->timer_id, US_TO_TICKS(
            ((int32_t)(stream_info->stream_param.send_interval_ms*1000)) - passedUs));
    }
}

static void gaf_bis_external_stream_timer_handler(void *param)
{
    uint32_t current_bt_time = gaf_media_sync_get_curr_time();
    app_bt_start_custom_function_in_bt_thread(current_bt_time, (uint32_t)param,
        (uint32_t)gaf_bis_external_stream_timer_handler_in_bt_thread);
}

static void gaf_bis_external_stream_start_trigger(void *param)
{
    gaf_bis_external_stream_info_t *stream_info = (gaf_bis_external_stream_info_t *)param;

    hwtimer_update(stream_info->timer_id, gaf_bis_external_stream_timer_handler, param);
    uint32_t timerPeriodUs = gaf_bis_external_stream_calculate_trigger_timer(stream_info);
    hwtimer_start(stream_info->timer_id, US_TO_TICKS(timerPeriodUs));
}

#endif

static uint32_t gaf_bis_external_stream_calculate_ts(gaf_bis_external_stream_info_t *stream_info)
{
    uint32_t diff_time = 0;
    uint32_t current_bt_time = btdrv_reg_op_bt_time_to_bts(btdrv_reg_op_syn_get_curr_ticks(),624);
    uint32_t bis_anchor_time = 0;
#ifndef WIFI_DONGLE
    bis_anchor_time = btdrv_reg_op_big_anchor_timestamp(stream_info->stream_param.bis_hdl); 
#endif
    if(current_bt_time >= bis_anchor_time)
    {
        diff_time = bis_anchor_time + stream_info->stream_param.send_interval_us - current_bt_time;
        if(diff_time >= BIS_DIFF_ANCHOR_US){
            if(bis_anchor_time +stream_info->stream_param.big_sync_delay_us > current_bt_time){
                diff_time = bis_anchor_time + stream_info->stream_param.big_sync_delay_us - current_bt_time;
                current_bt_time = bis_anchor_time + stream_info->stream_param.big_sync_delay_us;
                osDelay(diff_time/1000);
            }
        }
        else{
            current_bt_time = bis_anchor_time + stream_info->stream_param.send_interval_us;
            osDelay((diff_time + stream_info->stream_param.big_sync_delay_us)/1000);
        }
    }
    else
    {
        diff_time = bis_anchor_time - current_bt_time;
        if(diff_time >= BIS_DIFF_ANCHOR_US){
            if(bis_anchor_time + stream_info->stream_param.big_sync_delay_us - stream_info->stream_param.send_interval_us > current_bt_time){
                diff_time = bis_anchor_time + stream_info->stream_param.big_sync_delay_us - stream_info->stream_param.send_interval_us - current_bt_time;
                current_bt_time = bis_anchor_time + stream_info->stream_param.big_sync_delay_us - stream_info->stream_param.send_interval_us;
                osDelay(diff_time/1000);
            }
        }
        else{
            current_bt_time = bis_anchor_time;
            osDelay((diff_time + stream_info->stream_param.big_sync_delay_us)/1000);
        }
    }

    return current_bt_time;
}

static void gaf_bis_external_stream_timer_handler_in_bt_thread(uint32_t param0, uint32_t param1)
{
    uint16_t data_len = 0;
    uint8_t *send_data_buf = NULL;
    int free_num=0;
    gaf_bis_external_stream_info_t *stream_info = NULL;

    for (int i=0; i<2; ++i)
    {
        free_num = bes_ble_bap_get_free_iso_packet_num();
        if (free_num < 2)
        {
            return;
        }

        for (int stream_idx=0; stream_idx < bis_env.stream_num; ++stream_idx)
        {
            stream_info = &bis_env.stream_info[stream_idx];
            if (!stream_info->data_seq_num)
            {
                stream_info->btc_buf_num_max = free_num;
            }
            /// read data from user layer, and send data to BTC
            if (stream_info->stream_param.read_data_cb)
            {
                stream_info->stream_param.read_data_cb(stream_info->stream_param.stream_idx,
                    &send_data_buf, &data_len, stream_info->btc_buf_num_max-free_num);
            }

            if (send_data_buf)
            {
                if (!stream_info->data_seq_num)
                {
                    stream_info->prev_timestamp = gaf_bis_external_stream_calculate_ts(stream_info);
                }
                else
                {
                    stream_info->prev_timestamp +=  stream_info->stream_param.send_interval_us;
                }

                bes_ble_bap_iso_dp_send_data(stream_info->stream_param.bis_hdl,
                    stream_info->data_seq_num++, send_data_buf,
                    data_len,  stream_info->prev_timestamp);

                if (stream_info->stream_param.data_buf_free_cb)
                {
                    stream_info->stream_param.data_buf_free_cb(stream_info->stream_param.stream_idx, send_data_buf);
                }
            }
            else
            {
                return;
            }
        }
    }
}

static void gaf_bis_external_stream_timer_handler(void *param)
{
    gaf_bis_external_stream_info_t *stream_info = (gaf_bis_external_stream_info_t *)param;
    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)gaf_bis_external_stream_timer_handler_in_bt_thread);

    hwtimer_start(bis_env.timer_id, US_TO_TICKS(stream_info->stream_param.send_interval_us/2));
}

static void gaf_bis_external_stream_start_trigger(void *param)
{
    gaf_bis_external_stream_info_t *stream_info = (gaf_bis_external_stream_info_t *)param;

    hwtimer_update(bis_env.timer_id, gaf_bis_external_stream_timer_handler, param);
    uint32_t timerPeriodUs = gaf_bis_external_stream_calculate_trigger_timer(stream_info);
    hwtimer_start(bis_env.timer_id, US_TO_TICKS(timerPeriodUs));
}

uint32_t gaf_bis_external_stream_get_timestamp(uint8_t stream_idx)
{
    if (!bis_env.stream_info[stream_idx].started)
    {
        return 0xFFFFFFFF;
    }

    return bis_env.stream_info[stream_idx].prev_timestamp;
}

void gaf_bis_external_stream_start(gaf_bis_external_stream_param_t *stream_param)
{
    uint8_t stream_idx = stream_param->stream_idx;
    gaf_bis_external_stream_info_t *stream_info = NULL;

    if (bis_env.stream_info[stream_idx].started)
    {
        return;
    }

    if (!bis_env.stream_num)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_26M);
        bt_adapter_write_sleep_enable(0);
    }

    stream_info = &bis_env.stream_info[stream_idx];
    stream_info->started = true;
    stream_info->data_seq_num = 0;
    stream_info->prev_timestamp = 0;
    memcpy(&stream_info->stream_param, stream_param, sizeof(gaf_bis_external_stream_param_t));
    if (!bis_env.stream_num)
    {
        bis_env.timer_id = hwtimer_alloc(gaf_bis_external_stream_start_trigger, stream_info);
        hwtimer_start(bis_env.timer_id, MS_TO_TICKS(50));
    }
    bis_env.stream_num++;
}

void gaf_bis_external_stream_stop(uint8_t stream_idx)
{
    gaf_bis_external_stream_info_t *stream_info = NULL;

    if (!bis_env.stream_info[stream_idx].started)
    {
        return;
    }

    bis_env.stream_num--;
    stream_info = &bis_env.stream_info[stream_idx];
    stream_info->started = false;
    stream_info->data_seq_num = 0;
    stream_info->prev_timestamp = 0;
    memset(&stream_info->stream_param, 0, sizeof(gaf_bis_external_stream_param_t));

    if (!bis_env.stream_num)
    {
        hwtimer_stop(bis_env.timer_id);
        hwtimer_free(bis_env.timer_id);
        bt_adapter_write_sleep_enable(1);
        app_sysfreq_req(APP_SYSFREQ_USER_AOB_CAPTURE, APP_SYSFREQ_32K);
    }
}
#endif
/// @} APP
