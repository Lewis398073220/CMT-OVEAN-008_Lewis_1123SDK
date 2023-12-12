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
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_location.h"
#include "app_trace_rx.h"
#include "app_utils.h"
#include "plat_types.h"
#include "app_gaf_dbg.h"
#include "audio_dump.h"
#include "app_overlay.h"
#include "lc3_process.h"
#include "gaf_codec_lc3.h"
#include "gaf_codec_common.h"
#include "cc_stream_common.h"

/************************private macro defination***************************/
#define AOB_CALCULATE_CODEC_MIPS       0
#define LC3_AUDIO_UPLOAD_DUMP          0

/************************private type defination****************************/
typedef enum
{
    LC3_ENC_FREQ_SET_CHN_1_MIN = 0,
    LC3_ENC_FREQ_SET_1_8000_16000_10 = LC3_ENC_FREQ_SET_CHN_1_MIN,
    LC3_ENC_FREQ_SET_1_16000_16000_10,
    LC3_ENC_FREQ_SET_1_16000_32000_10,
    LC3_ENC_FREQ_SET_1_32000_32000_10,
    LC3_ENC_FREQ_SET_1_32000_48000_10,
    LC3_ENC_FREQ_SET_1_32000_64000_10,
    LC3_ENC_FREQ_SET_1_44100_88200_10,
    LC3_ENC_FREQ_SET_1_48000_32000_10,
    LC3_ENC_FREQ_SET_1_48000_96000_10,
    LC3_ENC_FREQ_SET_CHN_1_MAX,

    LC3_ENC_FREQ_SET_CHN_2_MIN = LC3_ENC_FREQ_SET_CHN_1_MAX,
    LC3_ENC_FREQ_SET_2_8000_32000_10 = LC3_ENC_FREQ_SET_CHN_2_MIN,
    LC3_ENC_FREQ_SET_2_16000_32000_10,
    LC3_ENC_FREQ_SET_2_16000_64000_10,
    LC3_ENC_FREQ_SET_2_32000_64000_10,
    LC3_ENC_FREQ_SET_2_32000_96000_10,
    LC3_ENC_FREQ_SET_2_32000_128000_10,
    LC3_ENC_FREQ_SET_2_44100_176400_10,
    LC3_ENC_FREQ_SET_2_48000_64000_10,
    LC3_ENC_FREQ_SET_2_48000_128000_10,
    LC3_ENC_FREQ_SET_2_48000_192000_10,

    LC3_ENC_FREQ_SET_MAX,
}encoder_freq_setting_e;
/************************private variable defination************************/
const codec_freq_setting_t lc3_enc_m55_freq_table[LC3_ENC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_26M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_52M},
    {1,         48000,      32000,      10,     APP_SYSFREQ_78M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_78M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      96000,      10,     APP_SYSFREQ_52M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_78M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_104M},
    {2,         48000,      64000,      10,     APP_SYSFREQ_208M},
    {2,         48000,      128000,     10,     APP_SYSFREQ_208M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_208M},
};

const codec_freq_setting_t lc3_enc_cp_freq_table[LC3_ENC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_78M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_78M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_78M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_78M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_78M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_78M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      96000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_104M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_104M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_104M},
};

const codec_freq_setting_t lc3_enc_m33_freq_table[LC3_ENC_FREQ_SET_MAX] =
{
    /* channels, sample_rate, bit_rate, frame_ms, freq */
    {1,         8000,       16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      16000,      10,     APP_SYSFREQ_52M},
    {1,         16000,      32000,      10,     APP_SYSFREQ_52M},
    {1,         32000,      32000,      10,     APP_SYSFREQ_78M},
    {1,         32000,      48000,      10,     APP_SYSFREQ_78M},
    {1,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {1,         44100,      88200,      10,     APP_SYSFREQ_78M},
    {1,         48000,      96000,      10,     APP_SYSFREQ_78M},

    {2,         8000,       32000,      10,     APP_SYSFREQ_78M},
    {2,         16000,      32000,      10,     APP_SYSFREQ_78M},
    {2,         16000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      64000,      10,     APP_SYSFREQ_78M},
    {2,         32000,      96000,      10,     APP_SYSFREQ_104M},
    {2,         32000,      128000,     10,     APP_SYSFREQ_104M},
    {2,         44100,      176400,     10,     APP_SYSFREQ_104M},
    {2,         48000,      192000,     10,     APP_SYSFREQ_104M},
};

LC3_Enc_Info g_lc3_enc_info[CAPTURE_INSTANCE_MAX];

#if AOB_CALCULATE_CODEC_MIPS
aob_codec_time_info_t aob_encode_time_info = {0};
#endif

/********************** function declaration*************************/
static void lc3_free(void *pool, void *data)
{
    return;
}

static uint32_t gaf_audio_lc3_encoder_set_freq(void *_codec_info,
                                    uint32_t base_freq, uint32_t core_type)
{
    CODEC_INFO_T* codec_info = (CODEC_INFO_T*)_codec_info;
    uint32_t lc3_enc_freq = base_freq;
    uint32_t sample_rate = codec_info->sample_rate;
    uint16_t frame_size = codec_info->frame_size * codec_info->num_channels;
    uint32_t bit_rate = (uint32_t)(frame_size * 8000 / codec_info->frame_ms);
    const codec_freq_setting_t *freq_table = NULL;
    uint32_t index = LC3_ENC_FREQ_SET_CHN_1_MIN;
    uint32_t indexMax = LC3_ENC_FREQ_SET_CHN_1_MAX;

    if (44100 == sample_rate)
    {
        bit_rate = bit_rate * 44100 / 48000;
    }

    if (CP_CORE == core_type) {
        freq_table = lc3_enc_cp_freq_table;
    }
    else if (M55_CORE == core_type) {
        freq_table = lc3_enc_m55_freq_table;
    }
    else {
        freq_table = lc3_enc_m33_freq_table;
    }

    if (2 == codec_info->num_channels) {
        index = LC3_ENC_FREQ_SET_CHN_2_MIN;
        indexMax = LC3_ENC_FREQ_SET_MAX;
    }
    for (; index < indexMax; index++)
    {
        if (sample_rate == freq_table[index].sample_rate &&
            bit_rate == freq_table[index].bit_rate)
        {
            lc3_enc_freq = freq_table[index].freq;
        }
    }

    LOG_I("lc3 enc freq:%d bit_rate:%d", lc3_enc_freq, bit_rate);
    return lc3_enc_freq;
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
static int aob_stream_lc3_encode_bits(void *lc3_enc_info, void *input_samples,
    void *output_bytes, int32_t num_bytes)
{
    int err = LC3_API_OK;
    LC3_Enc_Info *p_lc3_enc_info = (LC3_Enc_Info *)lc3_enc_info;
#if AOB_CALCULATE_CODEC_MIPS
    uint32_t cpu_freq = 0;
    uint32_t stime = 0, etime = 0;
    uint32_t use_time_in_us = 0, total_etime_in_ms = 0;
    stime = hal_sys_timer_get();

    if (!aob_encode_time_info.codec_started)
    {
        aob_encode_time_info.codec_started = true;
        aob_encode_time_info.total_time_in_us = 0;
        aob_encode_time_info.start_time_in_ms = hal_sys_timer_get();
    }
#endif

    err = p_lc3_enc_info->cb_encode_interlaced(p_lc3_enc_info, p_lc3_enc_info->scratch,
        input_samples, output_bytes, num_bytes);

#if AOB_CALCULATE_CODEC_MIPS
    etime = hal_sys_timer_get();
    use_time_in_us = TICKS_TO_US(etime - stime);
    aob_encode_time_info.total_time_in_us += use_time_in_us;
    total_etime_in_ms = TICKS_TO_MS(etime - aob_encode_time_info.start_time_in_ms);
    cpu_freq = aob_lc3_get_sysfreq();
    aob_encode_time_info.codec_mips = cpu_freq *
        (aob_encode_time_info.total_time_in_us / 1000) / total_etime_in_ms;
    LOG_I("err %d ticks:%d time:%d us", err, (etime - stime), use_time_in_us);
    LOG_I("freq %d use:%d ms total:%d ms mips: %d M", cpu_freq,
        aob_encode_time_info.total_time_in_us/1000, total_etime_in_ms,
        aob_encode_time_info.codec_mips);
#endif
    if (err != LC3_API_OK) {
        return CODEC_ERR;
    }
    return CODEC_OK;
}


static void gaf_audio_lc3_encoder_buf_init(uint8_t instance_handle,
                                        void *_codec_info, void *alloc_cb)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    g_lc3_enc_info[instance_handle].cb_alloc = (CC_HEAP_ALLOC)alloc_cb;
    g_lc3_enc_info[instance_handle].cb_free = lc3_free;
}

static void gaf_audio_lc3_encoder_init(uint8_t instance_handle, void *_codec_info)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    CODEC_INFO_T *codec_info = (CODEC_INFO_T*)_codec_info;
    LC3_Dec_Info *p_lc3_enc_info = &g_lc3_enc_info[instance_handle];
    int err = LC3_API_OK;

    p_lc3_enc_info->bitrate       = 0;
    p_lc3_enc_info->is_interlaced = 1;
    p_lc3_enc_info->epmode        = LC3_API_EP_OFF;
    p_lc3_enc_info->sample_rate   = codec_info->sample_rate;
    p_lc3_enc_info->channels      = codec_info->num_channels;
    p_lc3_enc_info->bitwidth      = codec_info->bits_depth;
    p_lc3_enc_info->bitalign      = (p_lc3_enc_info->bitwidth == 24) ? 32 : p_lc3_enc_info->bitwidth;
    p_lc3_enc_info->frame_dms     = (uint8_t)(codec_info->frame_ms * 10.f);
    p_lc3_enc_info->frame_size    = codec_info->frame_size * p_lc3_enc_info->channels;

    err = lc3_api_encoder_init(p_lc3_enc_info);
    ASSERT(LC3_API_OK == err, "%s err %d", __func__, err);

    CC_HEAP_ALLOC buf_alloc = (CC_HEAP_ALLOC)p_lc3_enc_info->cb_alloc;
    if (!p_lc3_enc_info->enabled_frame_buff) {
        p_lc3_enc_info->enabled_frame_buff = buf_alloc(NULL, p_lc3_enc_info->frame_size);
    }
    ASSERT(p_lc3_enc_info->instance, "%s/%d", __func__, __LINE__);
    ASSERT(p_lc3_enc_info->enabled_frame_buff, "%s/%d", __func__, __LINE__);
    ASSERT(p_lc3_enc_info->cb_encode_interlaced, "%s/%d", __func__, __LINE__);
    ASSERT(p_lc3_enc_info->frame_samples, "%s/%d", __func__, __LINE__);
    ASSERT(p_lc3_enc_info->frame_size, "%s/%d", __func__, __LINE__);

    LOG_I("[lc3]");
    LOG_I("Encoder size:     %d", p_lc3_enc_info->instance_size);
    LOG_I("Scratch size:     %d", p_lc3_enc_info->scratch_size);
    LOG_I("Sample rate:      %d", p_lc3_enc_info->sample_rate);
    LOG_I("Channels:         %d", p_lc3_enc_info->channels);
    LOG_I("Frame samples:     %d", p_lc3_enc_info->frame_samples);
    LOG_I("Frame length:     %d", p_lc3_enc_info->frame_size);
    LOG_I("Output format:    %d bits", p_lc3_enc_info->bitwidth);
    LOG_I("Output format:    %d bits align", p_lc3_enc_info->bitalign);
    LOG_I("Target bitrate:   %d", p_lc3_enc_info->bitrate);

#if AOB_CALCULATE_CODEC_MIPS
    memset(&aob_encode_time_info, 0, sizeof(aob_codec_time_info_t));
#endif

#if LC3_AUDIO_UPLOAD_DUMP
    uint32_t dump_frame_len = (uint32_t)(p_lc3_enc_info->sample_rate*p_lc3_enc_info->frame_dms/10000);
    audio_dump_init(dump_frame_len, p_lc3_enc_info->bitalign/8, 1);
#endif

    LOG_I("%s end", __func__);
}

static void gaf_audio_lc3_encoder_deinit(uint8_t instance_handle)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
}

FRAM_TEXT_LOC
static int gaf_audio_lc3_encode(uint8_t instance_handle, void *codec_info,
    uint32_t inputDataLength, void *input, uint16_t frame_size, uint8_t* output)
{
    LC3_Enc_Info *p_lc3_enc_info = &g_lc3_enc_info[instance_handle];
    uint32_t lc3_encoded_frame_len = p_lc3_enc_info->frame_size;
#if LC3_AUDIO_UPLOAD_DUMP
    auto  frame_samples = p_lc3_enc_info->frame_samples;
    char* input_ptr = (char*)input;
    auto sample_len = p_lc3_enc_info->bitalign/8*frame_samples;
    auto chs = p_lc3_enc_info->channels;
    for(int ch = 0; ch < chs; ch++){
        audio_dump_clear_up();
        audio_dump_add_channel_data(0, input_ptr+ch*sample_len, frame_samples);
        audio_dump_run();
    }
#endif
    return aob_stream_lc3_encode_bits(p_lc3_enc_info, input, output, lc3_encoded_frame_len);
}

static void gaf_audio_lc3_encoder_buf_deinit(uint8_t instance_handle)
{
    LOG_I("%s instance_handle:%d", __func__, instance_handle);
    LC3_Enc_Info *p_lc3_enc_info = &g_lc3_enc_info[instance_handle];
    if(p_lc3_enc_info->cb_uninit)
    {
        p_lc3_enc_info->cb_uninit(p_lc3_enc_info);
    }
    p_lc3_enc_info->instance= NULL;
    p_lc3_enc_info->scratch= NULL;
    p_lc3_enc_info->enabled_frame_buff = NULL;
}

static GAF_ENCODER_FUNC_LIST_T gaf_audio_lc3_encoder_func_list =
{
    .encoder_init_buf_func = gaf_audio_lc3_encoder_buf_init,
    .encoder_init_func = gaf_audio_lc3_encoder_init,
    .encoder_deinit_func = gaf_audio_lc3_encoder_deinit,
    .encoder_encode_frame_func = gaf_audio_lc3_encode,
    .encoder_deinit_buf_func = gaf_audio_lc3_encoder_buf_deinit,
    .encoder_set_freq = gaf_audio_lc3_encoder_set_freq,
};

void gaf_audio_lc3_update_encoder_func_list(void *enc_func_list)
{
    GAF_ENCODER_FUNC_LIST_T **encoder_func_list = (GAF_ENCODER_FUNC_LIST_T **)enc_func_list;
    *encoder_func_list = &gaf_audio_lc3_encoder_func_list;
}

int gaf_audio_lc3_encoder_get_max_frame_size(void)
{
    return LC3_MAX_FRAME_SIZE;
}
