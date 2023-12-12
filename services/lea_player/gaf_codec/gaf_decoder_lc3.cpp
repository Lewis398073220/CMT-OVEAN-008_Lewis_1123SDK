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
#include <string.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "app_utils.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "hal_location.h"
#include "audio_dump.h"
#include "app_overlay.h"
#include "lc3_process.h"
#include "gaf_codec_lc3.h"
#include "app_gaf_dbg.h"
#include "gaf_codec_common.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/
#define AOB_CALCULATE_CODEC_MIPS       0
#define LC3_AUDIO_DOWNLOAD_DUMP        0

/************************private type defination****************************/
typedef enum
{
    LC3_DEC_FREQ_SET_CHN_1_MIN = 0,
    LC3_DEC_FREQ_SET_1_8000_16000_10 = LC3_DEC_FREQ_SET_CHN_1_MIN,
    LC3_DEC_FREQ_SET_1_16000_16000_10,
    LC3_DEC_FREQ_SET_1_16000_32000_10,
    LC3_DEC_FREQ_SET_1_32000_32000_10,
    LC3_DEC_FREQ_SET_1_32000_48000_10,
    LC3_DEC_FREQ_SET_1_32000_64000_10,
    LC3_DEC_FREQ_SET_1_44100_88200_10,
    LC3_DEC_FREQ_SET_1_48000_32000_10,
    LC3_DEC_FREQ_SET_1_48000_96000_10,
    LC3_DEC_FREQ_SET_CHN_1_MAX,

    LC3_DEC_FREQ_SET_CHN_2_MIN = LC3_DEC_FREQ_SET_CHN_1_MAX,
    LC3_DEC_FREQ_SET_2_8000_32000_10 = LC3_DEC_FREQ_SET_CHN_2_MIN,
    LC3_DEC_FREQ_SET_2_16000_32000_10,
    LC3_DEC_FREQ_SET_2_16000_64000_10,
    LC3_DEC_FREQ_SET_2_32000_64000_10,
    LC3_DEC_FREQ_SET_2_32000_96000_10,
    LC3_DEC_FREQ_SET_2_32000_128000_10,
    LC3_DEC_FREQ_SET_2_44100_176400_10,
    LC3_DEC_FREQ_SET_2_48000_192000_10,

    LC3_DEC_FREQ_SET_MAX,
}lc3_decoder_freq_setting_e;

/************************private variable defination************************/
const codec_freq_setting_t lc3_dec_m55_freq_table[LC3_DEC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_26M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_26M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_26M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_52M},
    {1,         48000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_52M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_52M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_52M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_52M},
};

const codec_freq_setting_t lc3_dec_cp_freq_table[LC3_DEC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_52M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_52M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_78M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_78M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_78M},
};

const codec_freq_setting_t lc3_dec_m33_freq_table[LC3_DEC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_52M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_52M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_78M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_78M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_78M},
};

LC3_Dec_Info g_lc3_dec_info[PLAYBACK_INSTANCE_MAX];

#if AOB_CALCULATE_CODEC_MIPS
aob_codec_time_info_t aob_decode_time_info = {0};
#endif

/********************** function declaration*************************/

static void lc3_free(void *pool, void *data)
{
    return;
}

static uint32_t gaf_audio_lc3_decoder_set_freq(void *_codec_info,
                                    uint32_t base_freq, uint32_t core_type)
{
    CODEC_INFO_T* codec_info = (CODEC_INFO_T*)_codec_info;
    uint32_t lc3_dec_freq = base_freq;
    uint32_t sample_rate = codec_info->sample_rate;
    uint16_t frame_size = codec_info->frame_size * codec_info->num_channels;
    uint32_t bit_rate = (uint32_t)(frame_size * 8000 / codec_info->frame_ms);
    const codec_freq_setting_t *freq_table = NULL;
    uint32_t index = LC3_DEC_FREQ_SET_CHN_1_MIN;
    uint32_t indexMax = LC3_DEC_FREQ_SET_CHN_1_MAX;

    if (44100 == sample_rate){
        bit_rate = bit_rate * 44100 / 48000;
    }

    if (CP_CORE == core_type) {
        freq_table = lc3_dec_cp_freq_table;
    }
    else if (M55_CORE == core_type) {
        freq_table = lc3_dec_m55_freq_table;
    }
    else {
        freq_table = lc3_dec_m33_freq_table;
    }

    if (2 == codec_info->num_channels) {
        index = LC3_DEC_FREQ_SET_CHN_2_MIN;
        indexMax = LC3_DEC_FREQ_SET_MAX;
    }
    for (; index < indexMax; index++)
    {
        if (sample_rate == freq_table[index].sample_rate &&
            bit_rate == freq_table[index].bit_rate)
        {
            lc3_dec_freq = freq_table[index].freq;
        }
    }

    LOG_I("lc3 dec freq:%d bit_rate:%d", lc3_dec_freq, bit_rate);
    return lc3_dec_freq;
}

POSSIBLY_UNUSED static uint32_t aob_lc3_get_sysfreq(void)
{
    enum HAL_CMU_FREQ_T cpu_freq = hal_sysfreq_get();

    switch(cpu_freq)
    {
        case HAL_CMU_FREQ_32K:
            return 0;
            break;
        case HAL_CMU_FREQ_26M:
            return 26;
            break;
        case HAL_CMU_FREQ_52M:
            return 52;
            break;
        case HAL_CMU_FREQ_78M:
            return 78;
            break;
        case HAL_CMU_FREQ_104M:
            return 104;
            break;
        case HAL_CMU_FREQ_208M:
            return 208;
            break;
        default:
            return 0;
            break;
    }
}

FRAM_TEXT_LOC
static int aob_stream_lc3_decode_bits(LC3_Dec_Info *p_lc3_dec_info,
        void *input_bytes, int32_t num_bytes, void *output_samples, bool isPlc)
{
    int32_t bfi_ext = 0;
    int32_t err = LC3_API_OK;
    if (0 == num_bytes || isPlc)
    {
        bfi_ext = 1;
    }
#if AOB_CALCULATE_CODEC_MIPS
    uint32_t cpu_freq = 0;
    POSSIBLY_UNUSED uint32_t stime = 0, etime = 0;
    uint32_t use_time_in_us = 0, total_etime_in_ms = 0;
    if (!aob_decode_time_info.codec_started)
    {
        aob_decode_time_info.codec_started = true;
        aob_decode_time_info.total_time_in_us = 0;
        aob_decode_time_info.start_time_in_ms = hal_sys_timer_get();
    }

    stime = hal_sys_timer_get();
#endif

    err = p_lc3_dec_info->cb_decode_interlaced(p_lc3_dec_info, p_lc3_dec_info->scratch,
            input_bytes, num_bytes, output_samples, bfi_ext);

#if AOB_CALCULATE_CODEC_MIPS
    etime = hal_sys_timer_get();
    use_time_in_us = TICKS_TO_US(etime - stime);
    total_etime_in_ms = TICKS_TO_MS(etime - aob_decode_time_info.start_time_in_ms);
    aob_decode_time_info.total_time_in_us += use_time_in_us;
    cpu_freq = aob_lc3_get_sysfreq();
    if (total_etime_in_ms)
    {
        aob_decode_time_info.codec_mips =
            cpu_freq * (aob_decode_time_info.total_time_in_us / 1000) / total_etime_in_ms;
    }
    LOG_I("err %d ticks:%d time:%d us",
        err, (etime - stime), use_time_in_us);
    LOG_I("freq %d use:%d ms total:%d ms mips: %d M", cpu_freq,
        aob_decode_time_info.total_time_in_us/1000,
        total_etime_in_ms, aob_decode_time_info.codec_mips);
#endif
    return (int)err;
}

static void gaf_audio_lc3_decoder_buf_init(uint8_t instance_handle,
                                        void *_codec_info, void *alloc_cb)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    g_lc3_dec_info[instance_handle].cb_alloc = (CC_HEAP_ALLOC)alloc_cb;
    g_lc3_dec_info[instance_handle].cb_free = lc3_free;
}

static void gaf_audio_lc3_decoder_init(uint8_t instance_handle, void *_codec_info)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    CODEC_INFO_T *codec_info = (CODEC_INFO_T*)_codec_info;
    LC3_Dec_Info *p_lc3_dec_info = &g_lc3_dec_info[instance_handle];

    p_lc3_dec_info->sample_rate = codec_info ->sample_rate;
    p_lc3_dec_info->channels   = codec_info ->num_channels;
    p_lc3_dec_info->bitwidth   = codec_info ->bits_depth;
    p_lc3_dec_info->bitalign   = (p_lc3_dec_info->bitwidth == 24) ? 32 : 0;
    p_lc3_dec_info->frame_dms  = (uint8_t)(codec_info ->frame_ms * 10);
    p_lc3_dec_info->frame_size = codec_info ->frame_size * p_lc3_dec_info->channels;
    p_lc3_dec_info->bitrate    = 0;
    p_lc3_dec_info->plcMeth    = LC3_API_PLC_ADVANCED;
    p_lc3_dec_info->epmode     = LC3_API_EP_OFF;
    p_lc3_dec_info->is_interlaced = 1;

    int err = lc3_api_decoder_init(p_lc3_dec_info);
    ASSERT(LC3_API_OK == err, "%s err %d", __func__, err);

    LOG_I("[lc3]");
    LOG_I("Bits:             %d", p_lc3_dec_info->bitwidth);
    LOG_I("Bits align:       %d", p_lc3_dec_info->bitalign);
    LOG_I("Sample rate:      %d", p_lc3_dec_info->sample_rate);
    LOG_I("Channels:         %d", p_lc3_dec_info->channels);
    LOG_I("Frame samples:    %d", p_lc3_dec_info->frame_samples);
    LOG_I("Frame length:     %d", p_lc3_dec_info->frame_size);
    LOG_I("PLC mode:         %d", p_lc3_dec_info->plcMeth);
    LOG_I("Bitrate:          %d", p_lc3_dec_info->bitrate);

#if AOB_CALCULATE_CODEC_MIPS
    memset(&aob_decode_time_info, 0, sizeof(aob_codec_time_info_t));
#endif

    /* lc3 audio dump parameters init */
#if (LC3_AUDIO_DOWNLOAD_DUMP)
    uint32_t num_channels = codec_info->num_channels;
    uint32_t dump_frame_len = (uint32_t)(codec_info->sample_rate * codec_info->frame_ms/1000);
    auto bitalign = (codec_info->bits_depth == 24)? 32 : codec_info->bits_depth;
    audio_dump_init(dump_frame_len, bitalign/8, num_channels);
    TRACE(0, "dump_frame_len:%d, bitalign:%d", dump_frame_len, bitalign);
#endif

    LOG_I("%s end", __func__);
}

static void gaf_audio_lc3_decoder_deinit(uint8_t instance_handle)
{
    LOG_I("%s", __func__);
}

FRAM_TEXT_LOC
static int gaf_audio_lc3_decode(uint8_t instance_handle, void *codec_info,
            uint32_t inputDataLength, void *input, void *output, bool isPlc)
{
    int ret = LC3_API_ERROR;
    LC3_Dec_Info *p_lc3_dec_info = &g_lc3_dec_info[instance_handle];
    ret = aob_stream_lc3_decode_bits(p_lc3_dec_info, input, inputDataLength, output, isPlc);
 #if LC3_AUDIO_DOWNLOAD_DUMP
    auto frame_samples = p_lc3_dec_info->frame_samples;
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, output, frame_samples);
    audio_dump_run();
#endif

    if (LC3_API_OK == ret) {
        return CODEC_OK;
    }
    return CODEC_ERR;
}

static void gaf_audio_lc3_decoder_buf_deinit(uint8_t instance_handle)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    LC3_Dec_Info *p_lc3_dec_info = &g_lc3_dec_info[instance_handle];
    if(p_lc3_dec_info->cb_uninit) {
        p_lc3_dec_info->cb_uninit(p_lc3_dec_info);
    }
    p_lc3_dec_info->instance = NULL;
    p_lc3_dec_info->scratch = NULL;
    p_lc3_dec_info->enabled_frame_buff = NULL;
}

static GAF_DECODER_FUNC_LIST_T gaf_lc3_decoder_func_list =
{
    .decoder_init_buf_func = gaf_audio_lc3_decoder_buf_init,
    .decoder_init_func = gaf_audio_lc3_decoder_init,
    .decoder_deinit_func = gaf_audio_lc3_decoder_deinit,
    .decoder_decode_frame_func = gaf_audio_lc3_decode,
    .decoder_deinit_buf_func = gaf_audio_lc3_decoder_buf_deinit,
    .decoder_set_freq = gaf_audio_lc3_decoder_set_freq,
};

void gaf_audio_lc3_update_decoder_func_list(void *_dec_func_list)
{
    GAF_DECODER_FUNC_LIST_T **dec_func_list = (GAF_DECODER_FUNC_LIST_T **)_dec_func_list;
    *dec_func_list = &gaf_lc3_decoder_func_list;
}
