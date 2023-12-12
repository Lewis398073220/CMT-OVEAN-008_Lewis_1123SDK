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

#include "spa_ctrl.h"
#include "hal_trace.h"
#include "string.h"
#include "hal_aud.h"

static spa_ctrl_state_t g_spa_ctrl_st;
static spa_ctrl_state_t * const g_spa_ctrl_st_ptr = &g_spa_ctrl_st;

static void spa_ctrl_config_default(void)
{
    g_spa_ctrl_st.data_info.payload.data_channel = SPA_AUDIO_HANDLE_DATA_CHNL_NUM;
    g_spa_ctrl_st.data_info.payload.data_bits = SPA_AUDIO_HANDLE_DATA_BITS;
    g_spa_ctrl_st.data_info.payload.block_samples = SPA_AUDIO_HANDLE_BLOCK_SAMPLES;
    g_spa_ctrl_st.data_info.payload.sample_rate = SPA_AUDIO_HANDLE_BLOCK_SAMPLE_RATE;
}

int spa_ctrl_create(data_info_body_t * info, ext_alloc_func_t  alloc, const spa_algo_op_handler_t* algo_func,spa_algo_op_role_e role)
{
    if(!(g_spa_ctrl_st_ptr->inited)){
        spa_ctrl_config_default();
        if(info){
            memcpy((unsigned char*)&(g_spa_ctrl_st_ptr->data_info.payload),(unsigned char*)info,sizeof(data_info_t));
        }

        if(algo_func){
            spa_algo_op_handler_t *ptr = (spa_algo_op_handler_t*)algo_func;
            g_spa_ctrl_st_ptr->algo_op_handler.func_algo_run = ptr->func_algo_run;
            g_spa_ctrl_st_ptr->algo_op_handler.func_relay = ptr->func_relay;
        }

        memset((unsigned char*)&(g_spa_ctrl_st_ptr->debug_info),0,sizeof(spa_ctrl_debugging_statistic_info_t));

        g_spa_ctrl_st_ptr->algo_op_role = role;

        g_spa_ctrl_st_ptr->inited = true;
    }
    SPA_LOG_I("%s end",__func__);
    return 0;
}

int spa_ctrl_destroy(void)
{
    //TO DO
    return 0;
}

int spa_ctrl_reset(void)
{
    //TO DO
    return 0;
}

int spa_ctrl_update_sens_data(unsigned char * ptr, int data_src)
{
    //TO DO
    data_sens_t *_sens_new = (data_sens_t*)ptr;
    data_info_t *_data_info = &(g_spa_ctrl_st_ptr->data_info);
    data_sens_t *_data_sens = &(g_spa_ctrl_st_ptr->data_sens);
    data_aud_t *_data_aud = &(g_spa_ctrl_st_ptr->data_aud);
    spa_ctrl_debugging_statistic_info_t * _debug_info = &(g_spa_ctrl_st_ptr->debug_info);

    (void)_sens_new;
    (void)_data_info;
    (void)_data_sens;
    (void)_data_aud;
    (void)_debug_info;

    /** check module status */
    if (g_spa_ctrl_st_ptr->inited == 0 ) return -1;

    /** update motion data from sensor-hub */
    if (data_src == SPA_DATA_UPDATE_SOURCE_SENS) {
        _debug_info->call_cnt_rcv_sens ++;
        if((_debug_info->call_cnt_rcv_sens % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
            _debug_info->call_tm_rcv_sens = hal_fast_sys_timer_get();

            SPA_LOG_I("[Update:] %s sens rcv %d tm %d",__func__,_debug_info->call_cnt_rcv_sens,
                _debug_info->call_tm_rcv_sens);

            /**
             ** TODO:  more processing for data from sensor hub if necesary
             **/
        }
        

        /** reject sensor-hub data on tws-slave dev */
        if (g_spa_ctrl_st_ptr->algo_op_role == SPA_ALGO_OP_SLAVE) return -2;

    } else if ( data_src == SPA_DATA_UPDATE_SOURCE_TWS) {
        _debug_info->call_cnt_rcv_relay ++;
        if((_debug_info->call_cnt_rcv_relay % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
            _debug_info->call_tm_rcv_relay = hal_fast_sys_timer_get();

            SPA_LOG_I("[Update:] %s relay rcv %d tm %d",__func__,_debug_info->call_cnt_rcv_relay,
                _debug_info->call_tm_rcv_relay);

            /**
             ** TODO:  more processing for data from paired tws earbud if necesary
             **/
         }
    }

    /** udpate sensor data */
    memcpy(_data_sens, _sens_new, sizeof(data_sens_t));

    return 0;
}

int spa_ctrl_update_cfg_data_info(unsigned char * ptr, int data_src)
{
    if(ptr){
        g_spa_ctrl_st_ptr->data_info = *((data_info_t*)ptr);
    }
    return 0;
}

void spa_ctrl_update_cfg_op_role(spa_algo_op_role_e role)
{
    g_spa_ctrl_st_ptr->algo_op_role = role;
}

int spa_ctrl_proc(unsigned char * data_ptr, int data_size, data_info_t *data_info_ptr, spa_algo_op_role_e role)
{
    if (g_spa_ctrl_st_ptr->inited == 0 ) return -1;
    data_info_t *_data_info = &(g_spa_ctrl_st_ptr->data_info);
    data_sens_t *_data_sens = &(g_spa_ctrl_st_ptr->data_sens);
    data_aud_t *_data_aud = &(g_spa_ctrl_st_ptr->data_aud);
    spa_algo_op_handler_t *_op_handler = &(g_spa_ctrl_st_ptr->algo_op_handler);
    spa_ctrl_debugging_statistic_info_t * _debug_info = &(g_spa_ctrl_st_ptr->debug_info);
    uint32_t relay_snd_interval = 10;
    uint32_t relay_snd_interval_cnt1 = (_data_info->payload.sample_rate / 10);
    uint32_t relay_snd_interval_cnt2 = (_data_info->payload.block_samples);
    if (relay_snd_interval_cnt2 != 0)
    {
        relay_snd_interval = (relay_snd_interval_cnt1 / relay_snd_interval_cnt2) + 1;
    }

    _debug_info->call_cnt_proc++;

    if(role != g_spa_ctrl_st_ptr->algo_op_role){
        g_spa_ctrl_st_ptr->algo_op_role = role;
    }

    /*relay sens data to another*/
    if((g_spa_ctrl_st_ptr->algo_op_role == SPA_ALGO_OP_MASTER) &&
        (_op_handler->func_relay != NULL) &&
        ((_debug_info->call_cnt_proc % relay_snd_interval) == 0)){
        _debug_info->call_cnt_snd_relay++;
        /* relay trace info update*/

        SPA_LOG_I("[Update:] %s relay snd %d tm %d",__func__,_debug_info->call_cnt_snd_relay,
            _debug_info->call_tm_snd_relay);

        if((_debug_info->call_cnt_snd_relay % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
            _debug_info->call_tm_snd_relay = hal_fast_sys_timer_get();
        }

        /* relay data sending*/
        _data_sens->payload.data_opt = _debug_info->call_cnt_snd_relay;
        _data_sens->msg_head.len = sizeof(data_sens_t);
        (*(_op_handler->func_relay))((unsigned char *)_data_sens,sizeof(data_sens_t));
    }

    if(_op_handler->func_algo_run != NULL){
        _debug_info->call_cnt_snd_aud++;
        if((_debug_info->call_cnt_snd_aud % SPA_LOG_STATISTIC_TRACE_CNT_INTERVAL_) == 0){
            _debug_info->call_tm_snd_aud = hal_fast_sys_timer_get();

            SPA_LOG_I("[Update:] %s audio snd %d tm %d",__func__,_debug_info->call_cnt_snd_aud,
                 _debug_info->call_tm_snd_aud);
        }

        memcpy((unsigned char*)&(_data_aud->payload.info),(unsigned char*)_data_info,sizeof(data_info_t));
        memcpy((unsigned char*)&(_data_aud->payload.sens),(unsigned char*)_data_sens,sizeof(data_sens_t));
        _data_aud->payload.buffer.data_ptr = data_ptr;
        _data_aud->payload.buffer.data_size = data_size;
        _data_aud->payload.buffer.data_opt = _debug_info->call_cnt_snd_aud;
        _data_aud->payload.buffer.algo_op_role = role;
        _data_aud->msg_head.len = sizeof(data_aud_t);

        (*(_op_handler->func_algo_run))((void*)_data_aud,sizeof(data_aud_t));
    }

    return 0;
}



