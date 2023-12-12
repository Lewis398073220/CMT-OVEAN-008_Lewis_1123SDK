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

#include "hal_trace.h"
#include "hal_timer.h"
#include "spa_common.h"
#include "spa_ctrl.h"
#include "spa_sens.h"
#include "cmsis_os.h"
#include "spa_ext_sens_handler.h"
#include "spa_sens_main.h"

#define APP_SENSOR_HUB_SPA_SENS_PROC_TIMER_PERIOS_MS    (100)    //10 ms

void spatial_audio_sens_process_run_if(unsigned char * data_ptr, int data_size);
static void app_sensor_hub_spa_sens_proc_timer_start(void);
static void sensor_hub_spa_sens_proc_timer_handler(void const * param);
osTimerDef(sensor_hub_spa_sens_proc_timer, sensor_hub_spa_sens_proc_timer_handler);
static osTimerId sensor_hub_spa_sens_proc_timer_id = NULL;

static void sensor_hub_spa_sens_proc_timer_handler(void const * param)
{
    spatial_audio_sens_process_run_if(NULL,0);
	/**> note:Enable timer according to actual situation*/
    app_sensor_hub_spa_sens_proc_timer_start();
}

static int spatial_audio_sens_algo_post_handler(void * buf,unsigned int len)
{
    return 0;
}

static int spatial_audio_sens_relay_handler(void * buf,unsigned int len)
{
#if defined(SPA_AUDIO_SENS)
    app_spa_sens_data_snd((unsigned char *)buf,len);
#endif
    return 0;
}

static const spa_algo_op_handler_t algo_op_handler_instant = 
{
    .func_algo_run = spatial_audio_sens_algo_post_handler,
    .func_relay = spatial_audio_sens_relay_handler,
};


static void app_sensor_hub_spa_sens_proc_timer_init(void)
{
    SPA_LOG_I("%s",__func__);
    if(sensor_hub_spa_sens_proc_timer_id == NULL){
        sensor_hub_spa_sens_proc_timer_id = osTimerCreate(osTimer(sensor_hub_spa_sens_proc_timer), osTimerOnce, NULL);
    }
    osTimerStart(sensor_hub_spa_sens_proc_timer_id, APP_SENSOR_HUB_SPA_SENS_PROC_TIMER_PERIOS_MS);
}

POSSIBLY_UNUSED static void app_sensor_hub_spa_sens_proc_timer_start(void)
{
    osTimerStart(sensor_hub_spa_sens_proc_timer_id, APP_SENSOR_HUB_SPA_SENS_PROC_TIMER_PERIOS_MS);
}

static void app_sensor_hub_spa_sens_proc_timer_stop(void)
{
    osTimerStop(sensor_hub_spa_sens_proc_timer_id);
}

void spatial_audio_sens_process_init_if(unsigned int sample_rate, unsigned char sample_bits, unsigned char channel, unsigned int frame_len,void * alloc_func)
{
    spa_sens_create(NULL,(ext_alloc_func_t )NULL,&algo_op_handler_instant);
    app_sensor_hub_spa_sens_proc_timer_init();
}

void spatial_audio_sens_process_deinit_if(void)
{
    app_sensor_hub_spa_sens_proc_timer_stop();
}

void spatial_audio_sens_process_run_if(unsigned char * data_ptr, int data_size)
{
    spa_sens_proc();
}