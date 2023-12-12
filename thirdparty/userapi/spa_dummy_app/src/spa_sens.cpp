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
#include "hal_trace.h"
#include "string.h"

static spa_sens_state_t g_spa_sens_st;
static spa_sens_state_t * const g_spa_sens_st_ptr = &g_spa_sens_st;

static void spa_sens_config_default(void)
{
    g_spa_sens_st.data_info.payload.data_channel = SPA_SENS_HANDLE_DATA_CHNL_NUM;
    g_spa_sens_st.data_info.payload.data_bits = SPA_SENS_HANDLE_DATA_BITS;
    g_spa_sens_st.data_info.payload.block_samples = SPA_SENS_HANDLE_BLOCK_SAMPLES;
    g_spa_sens_st.data_info.payload.sample_rate = SPA_SENS_HANDLE_BLOCK_SAMPLE_RATE;
}


int spa_sens_create(data_info_t * info, ext_alloc_func_t  alloc, const spa_algo_op_handler_t* algo_func)
{
    if(!(g_spa_sens_st_ptr->inited)){
        spa_sens_config_default();
        if(info){
            memcpy((unsigned char*)&(g_spa_sens_st_ptr->data_info),(unsigned char*)info,sizeof(data_info_t));
        }

        if(algo_func){
            spa_algo_op_handler_t *ptr = (spa_algo_op_handler_t*)algo_func;
            g_spa_sens_st_ptr->algo_op_handler.func_algo_run = ptr->func_algo_run;
            g_spa_sens_st_ptr->algo_op_handler.func_relay = ptr->func_relay;
        }

        memset((unsigned char*)&(g_spa_sens_st_ptr->debug_info),0,sizeof(spa_sens_debugging_statistic_info_t));
        g_spa_sens_st_ptr->inited = true;
    }

    SPA_LOG_I("%s end",__func__);
    return 0;
}

int spa_sens_reset(void)
{
    return 0;
}

int spa_sens_update_sens_data(short * ptr, int data_src)
{
    return 0;
}

int spa_sens_proc(void)
{
    spa_algo_op_handler_t *_op_handler = &(g_spa_sens_st_ptr->algo_op_handler);
    data_sens_t *_data_sens = &(g_spa_sens_st_ptr->data_sens);
    spa_sens_debugging_statistic_info_t * _debug_info = &(g_spa_sens_st_ptr->debug_info);

    _debug_info->call_cnt_proc++;

    /* trace for case of normal processing*/
    if((_debug_info->call_cnt_proc % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
        _debug_info->call_tm_proc = hal_fast_sys_timer_get();
        SPA_LOG_I("[Update:] %s cnt %d tm %d",__func__,_debug_info->call_cnt_proc,_debug_info->call_tm_proc);
    }

    if(_op_handler->func_algo_run != NULL){
        //to do
    }

    if(_op_handler->func_relay != NULL){
        _debug_info->call_cnt_snd_sens++;

        int16_t tYPR = (_debug_info->call_cnt_snd_sens % 360) - 180;
        _data_sens->payload.data_Y = tYPR;
        _data_sens->payload.data_P = tYPR - 1;
        _data_sens->payload.data_R = tYPR + 1;

        if((_debug_info->call_cnt_snd_sens % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
            _debug_info->call_tm_snd_sens = hal_fast_sys_timer_get();
            SPA_LOG_I("[Update:] %s cnt %d tm %d",__func__,_debug_info->call_cnt_snd_sens,_debug_info->call_tm_snd_sens);
        }

        _data_sens->payload.data_opt = _debug_info->call_cnt_snd_sens;
        _data_sens->msg_head.len = sizeof(data_sens_t);

        if(_op_handler->func_relay != NULL){
            (*(_op_handler->func_relay))((unsigned char *)_data_sens,0);
        }
    }

    return 0;
}
