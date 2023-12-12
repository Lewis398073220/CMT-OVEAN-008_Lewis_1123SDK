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

#include "spa_audio.h"
#include "hal_trace.h"

#include "kfifo.h"
#include "cmsis_os.h"
#include "heap_api.h"

enum{
    SPATIAL_AUDIO_ALGO_SIGNAL           = (1 << 0),
    SPATIAL_AUDIO_ALGO_SETTING_UPDATE   = (1 << 1),
};

#define SPATIAL_AUDIO_ALGO_FRAME_LEN                  (1024)
#define SPATIAL_AUDIO_ALGO_THREAD_STACK_SIZE          (1024)

#ifndef SPATIAL_AUDIO_ALGO_INPUT_16BIS
#define SPATIAL_AUDIO_ALGO_PCM_HANDLE_DATA_MAX_SIZE       (SPATIAL_AUDIO_ALGO_FRAME_LEN * 8)    // one frame size = frame_len * 4bytes(32 bites) * 2 chanels
#define SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE          (SPATIAL_AUDIO_ALGO_FRAME_LEN * 8 * 2)    //one frame size * two cache frames
#define SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE          (SPATIAL_AUDIO_ALGO_FRAME_LEN * 8 * 2)    //one frame size * two cache frames
#define SPATIAL_AUDIO_ALGO_HEAP_BUFFER_SIZE           (SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE + SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE + SPATIAL_AUDIO_ALGO_PCM_HANDLE_DATA_MAX_SIZE + 128)     //in fifo + out fifo + process buffer
#else
#define SPATIAL_AUDIO_ALGO_PCM_HANDLE_DATA_MAX_SIZE       (SPATIAL_AUDIO_ALGO_FRAME_LEN * 8)    // one frame size = frame_len * 4bytes(32 bites) * 2 chanels
#define SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE          (SPATIAL_AUDIO_ALGO_FRAME_LEN * 4 * 2)    //one frame size * two cache frames (16bits)
#define SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE          (SPATIAL_AUDIO_ALGO_FRAME_LEN * 8 * 2)    //one frame size * two cache frames (24bits)
#define SPATIAL_AUDIO_ALGO_HEAP_BUFFER_SIZE           (SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE + SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE + SPATIAL_AUDIO_ALGO_PCM_HANDLE_DATA_MAX_SIZE + 128)     //in fifo + out fifo + process buffer
#endif

static osThreadId spatial_audio_algo_tid = NULL;
static void spatial_audio_algo_thread(void const *argument);
osThreadDef(spatial_audio_algo_thread, osPriorityHigh, 1, SPATIAL_AUDIO_ALGO_THREAD_STACK_SIZE, "spatial_audio_algo_thread");

static struct kfifo g_spatial_audio_frame_resize_fifo_in;
static struct kfifo g_spatial_audio_frame_resize_fifo_out;
static unsigned char* g_spatial_audio_frame_resize_fifo_in_buffer = NULL;
static unsigned char* g_spatial_audio_frame_resize_fifo_out_buffer = NULL;
static unsigned char* g_spatial_audio_pcm_tmp_buffer = NULL;
//static unsigned char  g_AUDIO_thirdparty_algo_heap_buffer[AUDIO_SPATIAL_AUDIO_ALGO_HEAP_BUFFER_SIZE] = {0};
static unsigned char  *g_spatial_audio_algo_heap_buffer= NULL;
static unsigned int spatial_audio_algo_pcm_frame_putout_size = 0;
static unsigned int spatial_audio_algo_pcm_frame_putin_size = 0;
static heap_handle_t g_spatial_audio_heap = NULL;
osSemaphoreDef(g_spatial_audio_algo_wait_process_done);
static osSemaphoreId g_spatial_audio_algo_wait_process_done_id = NULL;

static unsigned char spatial_audio_algo_bits_output_byte = sizeof(int16_t);

static spa_audio_codec_cfg_t codec_cfg;

static ext_alloc_func_t ext_alloc = NULL;

static audio_algo_setting_conf_t spatial_audio_setting_cfg = {
    1,                              /* unsigned int conf_id;                 */
    NULL,                           /* unsigned char * conf_params_ptr;      */
    0,                              /* unsigned int conf_params_size;        */
    SPA_DATA_UPDATE_SOURCE_UNKNOWN, /* spa_data_update_source_e conf_source; */
    0,                              /* unsigned int conf_opt;                */
    SPA_AUDIO_CHANNEL_ID_LEFT,      /* spa_audio_chan_id_e chan_id;   */
};

static audio_algo_setting_conf_t spatial_audio_setting_cfg_new= {
    1,                              /* unsigned int conf_id;                 */
    NULL,                           /* unsigned char * conf_params_ptr;      */
    0,                              /* unsigned int conf_params_size;        */
    SPA_DATA_UPDATE_SOURCE_UNKNOWN, /* spa_data_update_source_e conf_source; */
    0,                              /* unsigned int conf_opt;                */
    SPA_AUDIO_CHANNEL_ID_LEFT,      /* spa_audio_chan_id_e chan_id;   */
};

static void inline spatial_audio_convert_16bit_to_24bit(int32_t *out, int16_t *in, int len)
{
    for (int i = len - 1; i >= 0; i--) {
        out[i] = ((int32_t)in[i] << 8);
    }
}

static void spatial_algo_thread_init(data_info_body_t *config,ext_alloc_func_t alloc)
{
    if (spatial_audio_algo_tid == NULL)
    {
        spatial_audio_algo_tid = osThreadCreate(osThread(spatial_audio_algo_thread), NULL);

        ASSERT(spatial_audio_algo_tid, "create spatial_audio_algo_tid thread fail");
    }

    if (g_spatial_audio_algo_wait_process_done_id == NULL)
    {
        g_spatial_audio_algo_wait_process_done_id = osSemaphoreCreate(osSemaphore(g_spatial_audio_algo_wait_process_done), 0);

        ASSERT(g_spatial_audio_algo_wait_process_done_id, "create g_spatial_audio_algo_wait_process_done_id os Semaphore fail");
    }

    if (g_spatial_audio_heap != NULL)
    {
//        heap_free(g_audio_thirdparty_heap, g_audio_frame_resize_fifo_in_buffer);
//        heap_free(g_audio_thirdparty_heap, g_audio_frame_resize_fifo_out_buffer);
//        heap_free(g_audio_thirdparty_heap, g_audio_pcm_tmp_buffer);

        g_spatial_audio_frame_resize_fifo_in_buffer = NULL;
        g_spatial_audio_frame_resize_fifo_out_buffer = NULL;
        g_spatial_audio_pcm_tmp_buffer = NULL;
        g_spatial_audio_heap = NULL;
    }

    ASSERT(alloc,"%s alloc external !!",__func__);

    ext_alloc = alloc;

    ext_alloc(&g_spatial_audio_algo_heap_buffer, SPATIAL_AUDIO_ALGO_HEAP_BUFFER_SIZE);
    g_spatial_audio_heap = heap_register(g_spatial_audio_algo_heap_buffer, SPATIAL_AUDIO_ALGO_HEAP_BUFFER_SIZE);

    ASSERT(g_spatial_audio_heap != NULL, "audio heap should not be null.");

    g_spatial_audio_frame_resize_fifo_in_buffer  = (unsigned char*)heap_malloc(g_spatial_audio_heap, SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE);
    g_spatial_audio_frame_resize_fifo_out_buffer = (unsigned char*)heap_malloc(g_spatial_audio_heap, SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE);
    g_spatial_audio_pcm_tmp_buffer               = (unsigned char*)heap_malloc(g_spatial_audio_heap, SPATIAL_AUDIO_ALGO_PCM_HANDLE_DATA_MAX_SIZE);

    ASSERT(g_spatial_audio_frame_resize_fifo_in_buffer  != NULL, "fifo in should not be null.");
    ASSERT(g_spatial_audio_frame_resize_fifo_out_buffer != NULL, "fifo out should not be null.");
    ASSERT(g_spatial_audio_pcm_tmp_buffer               != NULL, "process buffer should not be null.");

    kfifo_init(&g_spatial_audio_frame_resize_fifo_in,  g_spatial_audio_frame_resize_fifo_in_buffer,  SPATIAL_AUDIO_ALGO_PCM_FIFO_IN_MAX_SIZE);
    kfifo_init(&g_spatial_audio_frame_resize_fifo_out, g_spatial_audio_frame_resize_fifo_out_buffer, SPATIAL_AUDIO_ALGO_PCM_FIFO_OUT_MAX_SIZE);

    ASSERT(g_spatial_audio_frame_resize_fifo_in.buffer  != NULL, "fifo in buffer should not be null.");
    ASSERT(g_spatial_audio_frame_resize_fifo_out.buffer != NULL, "fifo out buffer should not be null.");
}

static void spatial_audio_alog_process(unsigned char*  pcm_in,  unsigned char * pcm_out, unsigned int pcm_in_length,unsigned int pcm_out_legnth)
{
    /// thirdparty algo process here.
    /// SPA_LOG_I( "thirdparty algo process...");

    /**
     ** TODO:  call core function of audio processing module
     **/

#ifdef SPATIAL_AUDIO_ALGO_CP_ACCEL
    spatial_audio_algo_status_t status = {
        .reserved = 0xff,
    };
    spatial_audio_algo_cp_process((short *)pcm_in,NULL,pcm_in_length,(int *)&pcm_out_legnth,&status);
#ifdef SPATIAL_AUDIO_ALGO_INPUT_16BIS
    /*
        pls note that.if not convert 16 to 24bits in [audio_decoder_thirdparty_algo_cp_process]
        need to convert here.
        so by default, in the sdk view. here do convert.
    */
    spatial_audio_convert_16bit_to_24bit((int32_t *)pcm_out, (int16_t *)pcm_in, pcm_in_length / sizeof(int16_t));
#endif
#endif
}

static void spatial_audio_algo_thread(void const *argument)
{
    osEvent evt;
    
    while (1)
    {
        evt = osSignalWait(SPATIAL_AUDIO_ALGO_SIGNAL, osWaitForever);
        if (evt.status == osEventSignal)
        {
            //switch (evt.value.signals)

            /** case SPATIAL_AUDIO_ALGO_SETTING_UPDATE: */
            if (evt.value.signals & SPATIAL_AUDIO_ALGO_SETTING_UPDATE) {
                audio_algo_setting_conf_t *cfg_new = &spatial_audio_setting_cfg_new;
                audio_algo_setting_conf_t *cfg_old = &spatial_audio_setting_cfg;

                switch (cfg_new->conf_source) {
                    case SPA_DATA_UPDATE_SOURCE_BTH:
                        {
                            cfg_old->chan_id = cfg_new->chan_id;
                            SPA_LOG_I("[Update:] %s chan id udpate, ch_id[%d], opt[%d] ", __func__, cfg_old->chan_id, cfg_old->conf_opt);
                        }
                        break;
                    case SPA_DATA_UPDATE_SOURCE_TUNE:
                        {
                            if (cfg_new->conf_params_ptr != NULL){
                                cfg_old->conf_params_ptr = cfg_new->conf_params_ptr;
                                cfg_old->conf_params_size = cfg_new->conf_params_size;
                                SPA_LOG_I("[Update:] %s conf param update, ptr[%p], size[%d] ", __func__, cfg_old->conf_params_ptr, cfg_old->conf_params_size);
                            } else {
                                cfg_old->conf_id = cfg_new->conf_id;
                                cfg_old->conf_opt = cfg_new->conf_opt;
                                SPA_LOG_I("[Update:] %s conf id update, id[%d], opt[%d] ", __func__, cfg_old->conf_id, cfg_old->conf_opt);
                            }
                        }
                        break;
                    default:
                        spatial_audio_setting_cfg = spatial_audio_setting_cfg_new;
                } /* end of switch: cfg_new->conf_source */

                osSignalClear(spatial_audio_algo_tid, SPATIAL_AUDIO_ALGO_SETTING_UPDATE);
            } /* end of case: SPATIAL_AUDIO_ALGO_SETTING_UPDATE*/


            /** case SPATIAL_AUDIO_ALGO_SIGNAL: */
            if (evt.value.signals & SPATIAL_AUDIO_ALGO_SIGNAL) {

                if (kfifo_len(&g_spatial_audio_frame_resize_fifo_in) >= spatial_audio_algo_pcm_frame_putin_size)
                {
                    memset(g_spatial_audio_pcm_tmp_buffer,0,spatial_audio_algo_pcm_frame_putout_size);
                    kfifo_get(&g_spatial_audio_frame_resize_fifo_in, g_spatial_audio_pcm_tmp_buffer, spatial_audio_algo_pcm_frame_putin_size);
                    spatial_audio_alog_process(g_spatial_audio_pcm_tmp_buffer,g_spatial_audio_pcm_tmp_buffer,
                                               spatial_audio_algo_pcm_frame_putin_size,spatial_audio_algo_pcm_frame_putout_size);

                    kfifo_put(&g_spatial_audio_frame_resize_fifo_out, g_spatial_audio_pcm_tmp_buffer, spatial_audio_algo_pcm_frame_putout_size);
                }
                else
                {
                    SPA_LOG_I( "[%s] input not ready", __func__);
                    SPA_LOG_I( "in fifo len:%d, frame size:%d", kfifo_len(&g_spatial_audio_frame_resize_fifo_in), spatial_audio_algo_pcm_frame_putin_size);
                }

                osSemaphoreRelease(g_spatial_audio_algo_wait_process_done_id);

            } /* end of case: SPATIAL_AUDIO_ALGO_SIGNAL*/
        } /* end of switch: evt.status */
    } /* end of while(1) */
}

void spatial_audio_codec_config(spa_audio_codec_cfg_t *cfg)
{
    if(cfg){
        codec_cfg = *cfg;
    }else{
        codec_cfg.block_samples = SPA_AUDIO_HANDLE_BLOCK_SAMPLES;
        codec_cfg.data_bits = SPA_AUDIO_HANDLE_DATA_BITS;
        codec_cfg.sample_rate = SPA_AUDIO_HANDLE_BLOCK_SAMPLE_RATE;
        codec_cfg.data_channel = SPA_AUDIO_HANDLE_DATA_CHNL_NUM;
    }

    unsigned short frame_len = codec_cfg.block_samples * codec_cfg.data_channel;
    if(codec_cfg.data_bits == 16){
        spatial_audio_algo_bits_output_byte = 2;
        frame_len *= 2;
    }else if(codec_cfg.data_bits == 24){
        spatial_audio_algo_bits_output_byte = 4;
        frame_len *= 4;
    }
    spatial_audio_algo_pcm_frame_putout_size = frame_len;
    spatial_audio_algo_pcm_frame_putin_size = frame_len;

#ifdef SPATIAL_AUDIO_ALGO_INPUT_16BIS
    if(spatial_audio_algo_bits_output_byte == sizeof(int)){
        spatial_audio_algo_pcm_frame_putin_size = frame_len /(sizeof(int)/sizeof(short));
    }
#endif

}

void app_spatial_audio_pcm_data_send_to_algo_thread(unsigned char* pcm_in, unsigned char *pcm_out,unsigned short pcm_in_length,unsigned short pcm_out_length)
{

    kfifo_put(&g_spatial_audio_frame_resize_fifo_in, pcm_in, pcm_in_length);
    osSignalSet(spatial_audio_algo_tid, SPATIAL_AUDIO_ALGO_SIGNAL);

    if (kfifo_len(&g_spatial_audio_frame_resize_fifo_out) >= pcm_out_length)
    {
        kfifo_get(&g_spatial_audio_frame_resize_fifo_out, pcm_out, pcm_out_length);
    }
    else
    {
        SPA_LOG_I( "[%s] output not ready", __func__);
        memset(pcm_out, 0x00, pcm_out_length);
        osSemaphoreWait(g_spatial_audio_algo_wait_process_done_id, osWaitForever);
    }

}

void app_spatial_audio_conf_notify_update(audio_algo_setting_conf_t * cfg)
{
    if(cfg != NULL){
        /**
         ** TODO:  process different configuration by source type if neccesary 
         **/

        spatial_audio_setting_cfg_new = *cfg;
        osSignalSet(spatial_audio_algo_tid, SPATIAL_AUDIO_ALGO_SETTING_UPDATE);
    }
}

void spatial_audio_algo_init(data_info_body_t *config,ext_alloc_func_t alloc)
{
    spatial_algo_thread_init(config,alloc);
    spatial_audio_codec_config(config);

    /**
     ** TODO:  create instance of audio processing module
     **/

#ifdef SPATIAL_AUDIO_ALGO_CP_ACCEL
    int in_pcm_len_chnl = spatial_audio_algo_pcm_frame_putin_size/config->data_channel;
    int out_pcm_len_chnl = spatial_audio_algo_pcm_frame_putout_size/config->data_channel;
    spatial_audio_algo_cp_init(in_pcm_len_chnl,out_pcm_len_chnl,config->data_channel);
#endif

}

void spatial_audio_algo_deinit(void)
{
    /**
     ** TODO:  destropy instance of audio processing module
     **/

#ifdef SPATIAL_AUDIO_ALGO_CP_ACCEL
    spatial_audio_algo_cp_deinit();
#endif
}
