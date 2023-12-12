/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef CHIP_SUBSYS_SENS

#include "plat_types.h"
#include "codec_int.h"
#include "hal_codec.h"
#include "hal_trace.h"
#include "hal_sleep.h"
#include "stdbool.h"
#include "string.h"
#include "analog.h"
#include "tgt_hardware.h"

#define CODEC_TRACE(n, s, ...)          TRACE(n, s, ##__VA_ARGS__)

#define CODEC_INT_INST                  HAL_CODEC_ID_0

#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV                CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif

#ifdef __CODEC_ASYNC_CLOSE__
#include "cmsis_os.h"

#define CODEC_ASYNC_CLOSE_DELAY (5000)

static void codec_timehandler(void const *param);

osTimerDef (CODEC_TIMER, codec_timehandler);
static osTimerId codec_timer;
static CODEC_CLOSE_HANDLER close_hdlr;

enum CODEC_HW_STATE_T {
    CODEC_HW_STATE_CLOSED,
    CODEC_HW_STATE_CLOSE_PENDING,
    CODEC_HW_STATE_OPENED,
};

enum CODEC_HW_STATE_T codec_hw_state = CODEC_HW_STATE_CLOSED;
#endif

enum CODEC_USER_T {
    CODEC_USER_STREAM   = (1 << 0), //playback+capture
    CODEC_USER_ANC      = (1 << 1),
    CODEC_USER_VAD      = (1 << 2),
    CODEC_USER_STREAM2  = (1 << 3), //playback2+capture2
    CODEC_USER_STREAM3  = (1 << 4), //playback3
};

#define AUD_STREAM_PLAYBACK2 (AUD_STREAM_NUM)
#define AUD_STREAM_CAPTURE2  (AUD_STREAM_NUM+1)
#define AUD_STREAM_PLAYBACK3 (AUD_STREAM_NUM+2)
#define AUD_STREAM_CFG_NUM   (AUD_STREAM_NUM+3)

enum CODEC_INT_STREAM_T {
    CODEC1_INT_STREAM,
    CODEC2_INT_STREAM,
    CODEC3_INT_STREAM,

    CODEC_INT_STREAM_QTY,
};

struct CODEC_CONFIG_T {
    enum CODEC_USER_T user_map;
    bool resample_en;
    bool mute_state[AUD_STREAM_CFG_NUM];
    bool chan_vol_set[AUD_STREAM_CFG_NUM];
    struct STREAM_CONFIG_T {
        bool opened;
        bool started;
        struct HAL_CODEC_CONFIG_T codec_cfg;
    } stream_cfg[AUD_STREAM_CFG_NUM];
};

static struct CODEC_CONFIG_T codec_int_cfg = {
    .user_map = 0,
    .resample_en = false,
    .mute_state = { false, false, },
    .chan_vol_set = { false, false, },
    //playback
    .stream_cfg[AUD_STREAM_PLAYBACK] = {
        .opened = false,
        .started = false,
        .codec_cfg = {
            .sample_rate = AUD_SAMPRATE_NULL,
        }
    },
    //capture
    .stream_cfg[AUD_STREAM_CAPTURE] = {
        .opened = false,
        .started = false,
        .codec_cfg = {
            .sample_rate = AUD_SAMPRATE_NULL,
        }
    },
    //playback2
    .stream_cfg[AUD_STREAM_PLAYBACK2] = {
        .opened = false,
        .started = false,
        .codec_cfg = {
            .sample_rate = AUD_SAMPRATE_NULL,
        }
    },
    //capture2
    .stream_cfg[AUD_STREAM_CAPTURE2] = {
        .opened = false,
        .started = false,
        .codec_cfg = {
            .sample_rate = AUD_SAMPRATE_NULL,
        }
    },
    //playback3
    .stream_cfg[AUD_STREAM_PLAYBACK3] = {
        .opened = false,
        .started = false,
        .codec_cfg = {
            .sample_rate = AUD_SAMPRATE_NULL,
        }
    }
};

static enum ANC_TYPE_T anc_type_opened;
static enum AUD_SAMPRATE_T codec_anc_samp_rate;
static CODEC_ANC_HANDLER codec_anc_handler;

//#define CODEC_HW_REMAP
//#define CODEC_HW_SWAP

#ifndef CODEC_HW_REMAP
#define codec_remap(codec) (codec)
#else
enum CODEC_INT_STREAM_T codec_remap(enum CODEC_INT_STREAM_T codec)
{
    enum CODEC_INT_STREAM_T remap_list[] = {
#ifdef CODEC_HW_SWAP
        CODEC2_INT_STREAM, CODEC1_INT_STREAM, CODEC3_INT_STREAM,
#else
        CODEC1_INT_STREAM, CODEC2_INT_STREAM, CODEC3_INT_STREAM,
#endif
    };
    ASSERT((int)codec<ARRAY_SIZE(remap_list),"%s: invalid codec %d", __func__, codec);
    return remap_list[(int)codec];
}
#endif

static enum AUD_STREAM_T map_to_codec_int_stream(enum CODEC_INT_STREAM_T codec, enum AUD_STREAM_T stream)
{
    static int8_t codec_int_stream_map[3][2] = {
        {AUD_STREAM_PLAYBACK, AUD_STREAM_CAPTURE},
        {AUD_STREAM_PLAYBACK2, AUD_STREAM_CAPTURE2},
        {AUD_STREAM_PLAYBACK3, -1},
    };
    int8_t  map_stream;

    ASSERT((codec==CODEC1_INT_STREAM)
            ||(codec==CODEC2_INT_STREAM)
            ||(codec==CODEC3_INT_STREAM),
        "%s: invalid codec=%d", __func__, codec);

    map_stream = (codec_int_stream_map[codec][stream]);
    ASSERT(map_stream >= 0, "%s: bad codec[%d] stream %d", __func__, codec, stream);

    return (enum AUD_STREAM_T)map_stream;
}

static void codec_analog_adc_enable(enum AUD_STREAM_T stream, bool enable)
{
    if (stream == AUD_STREAM_CAPTURE) {
        analog_aud_codec_adc_enable(
            codec_int_cfg.stream_cfg[stream].codec_cfg.io_path,
            codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map, enable);
    } else if (stream == AUD_STREAM_CAPTURE2) {
        analog_aud_codec_adc2_enable(
            codec_int_cfg.stream_cfg[stream].codec_cfg.io_path,
            codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map, enable);
    }
}

static void codec_hw_start(enum AUD_STREAM_T stream)
{
    CODEC_TRACE(2,"%s: stream=%d", __func__, stream);

    if (stream == AUD_STREAM_PLAYBACK) {
        // Enable DAC before starting stream (specifically before enabling PA)
        analog_aud_codec_dac_enable(true);
    }

    hal_codec_start_stream(CODEC_INT_INST, stream);
}

static void codec_hw_stop(enum AUD_STREAM_T stream)
{
    CODEC_TRACE(2,"%s: stream=%d", __func__, stream);

    hal_codec_stop_stream(CODEC_INT_INST, stream);

    if (stream == AUD_STREAM_PLAYBACK) {
        // Disable DAC after stopping stream (specifically after disabling PA)
        analog_aud_codec_dac_enable(false);
    }
}

static void codec_hw_open(enum CODEC_USER_T user)
{
    enum CODEC_USER_T old_map;
#ifdef __AUDIO_RESAMPLE__
    bool resample_en = !!hal_cmu_get_audio_resample_status();
#endif

    old_map = codec_int_cfg.user_map;
    codec_int_cfg.user_map |= user;

    if (old_map) {
#ifdef __AUDIO_RESAMPLE__
        ASSERT(codec_int_cfg.resample_en == resample_en,
            "%s: Bad resamp status %d for user 0x%X (old_map=0x%X)", __func__, resample_en, user, old_map);
#endif
        return;
    }

#ifdef __AUDIO_RESAMPLE__
    codec_int_cfg.resample_en = resample_en;
#endif

    CODEC_TRACE(1,"%s", __func__);

#ifdef __CODEC_ASYNC_CLOSE__
    if (codec_timer == NULL) {
        codec_timer = osTimerCreate (osTimer(CODEC_TIMER), osTimerOnce, NULL);
    }
    osTimerStop(codec_timer);
#endif

    // Audio resample: Might have different clock source, so must be reconfigured here
    hal_codec_open(CODEC_INT_INST);

#ifdef __CODEC_ASYNC_CLOSE__
    CODEC_TRACE(2,"%s: codec_hw_state=%d", __func__, codec_hw_state);

    if (codec_hw_state == CODEC_HW_STATE_CLOSED) {
        codec_hw_state = CODEC_HW_STATE_OPENED;
        // Continue to open the codec hardware
    } else if (codec_hw_state == CODEC_HW_STATE_CLOSE_PENDING) {
        // Still opened
        codec_hw_state = CODEC_HW_STATE_OPENED;
        return;
    } else {
        // Already opened
        return;
    }
#endif

    analog_aud_codec_open();
}

static void codec_hw_close(enum CODEC_CLOSE_TYPE_T type, enum CODEC_USER_T user)
{
    codec_int_cfg.user_map &= ~user;

    if (type == CODEC_CLOSE_NORMAL) {
        if (codec_int_cfg.user_map) {
            return;
        }
    }

#ifdef __CODEC_ASYNC_CLOSE__
    CODEC_TRACE(3,"%s: type=%d codec_hw_state=%d", __func__, type, codec_hw_state);
#else
    CODEC_TRACE(2,"%s: type=%d", __func__, type);
#endif

    if (type == CODEC_CLOSE_NORMAL) {
        // Audio resample: Might have different clock source, so close now and reconfigure when open
        hal_codec_close(CODEC_INT_INST);
#ifdef CODEC_POWER_DOWN
        memset(&codec_int_cfg, 0, sizeof(codec_int_cfg));
#endif
#ifdef __CODEC_ASYNC_CLOSE__
        ASSERT(codec_hw_state == CODEC_HW_STATE_OPENED, "%s: (type=%d) Bad codec_hw_state=%d", __func__, type, codec_hw_state);
        // Start a timer to close the codec hardware
        codec_hw_state = CODEC_HW_STATE_CLOSE_PENDING;
        osTimerStop(codec_timer);
        osTimerStart(codec_timer, CODEC_ASYNC_CLOSE_DELAY);
        return;
    } else if (type == CODEC_CLOSE_ASYNC_REAL) {
        if (codec_hw_state != CODEC_HW_STATE_CLOSE_PENDING) {
            // Already closed or reopened
            return;
        }
        codec_hw_state = CODEC_HW_STATE_CLOSED;
#endif
    } else if (type == CODEC_CLOSE_FORCED) {
        hal_codec_crash_mute();
    }

    analog_aud_codec_close();
}

#ifdef __CODEC_ASYNC_CLOSE__
void codec_int_set_close_handler(CODEC_CLOSE_HANDLER hdlr)
{
    close_hdlr = hdlr;
}

static void codec_timehandler(void const *param)
{
    if (close_hdlr) {
        close_hdlr();
    }
}
#endif

int codec_anc_open(enum ANC_TYPE_T type, enum AUD_SAMPRATE_T dac_rate, enum AUD_SAMPRATE_T adc_rate, CODEC_ANC_HANDLER hdlr)
{
    bool anc_running = false;
    enum AUD_SAMPRATE_T cfg_rate, opened_rate;
    int i;

    CODEC_TRACE(4,"%s: type=%d dac_rate=%d adc_rate=%d", __func__, type, dac_rate, adc_rate);

    dac_rate = hal_codec_anc_convert_rate(dac_rate);
    adc_rate = hal_codec_anc_convert_rate(adc_rate);
    ASSERT(dac_rate == adc_rate, "%s: Unmatched rates: dac_rate=%u adc_rate=%u", __func__, dac_rate, adc_rate);

    opened_rate = AUD_SAMPRATE_NULL;
    for (i = 0; i < AUD_STREAM_CFG_NUM; i++) {
        if (codec_int_cfg.stream_cfg[i].opened) {
            cfg_rate = hal_codec_anc_convert_rate(codec_int_cfg.stream_cfg[i].codec_cfg.sample_rate);
            if (opened_rate == AUD_SAMPRATE_NULL) {
                opened_rate = cfg_rate;
            } else {
                ASSERT(opened_rate == cfg_rate, "%s: [%d] adc/dac rate unmatched: opened=%u, cur=%u",
                    __func__, i, opened_rate, cfg_rate);
            }
        }
    }
    if (opened_rate == AUD_SAMPRATE_NULL) {
        cfg_rate = dac_rate;
    }
    if ((hdlr != NULL) && (dac_rate != cfg_rate)) {
        hdlr(AUD_STREAM_PLAYBACK, cfg_rate, NULL, NULL);
        TRACE(3,"%s: ANC sample rate changes from %u to %u", __func__, dac_rate, cfg_rate);
        dac_rate = cfg_rate;
    }

    codec_anc_samp_rate = dac_rate;
    codec_anc_handler = hdlr;

    if (anc_type_opened != ANC_NOTYPE) {
        anc_running = true;
    }

    anc_type_opened |= type;

    if (!anc_running) {
        hal_subsys_wake_lock(HAL_SUBSYS_WAKE_LOCK_USER_ANC);

        hal_cmu_anc_enable(HAL_CMU_ANC_CLK_USER_ANC);

        enum AUD_STREAM_T stream;
        enum AUD_CHANNEL_NUM_T play_chan_num;

        codec_hw_open(CODEC_USER_ANC);

        if (CODEC_OUTPUT_DEV == (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
            play_chan_num = AUD_CHANNEL_NUM_2;
        } else {
            play_chan_num = AUD_CHANNEL_NUM_1;
        }

        for (stream = AUD_STREAM_PLAYBACK; stream <= AUD_STREAM_CAPTURE; stream++) {
            if (codec_int_cfg.stream_cfg[stream].opened) {
                if (stream == AUD_STREAM_PLAYBACK) {
                /*    ASSERT(codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num == play_chan_num,
                        "Invalid existing ANC channel num %d != %d for stream %d",
                        codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num,
                        play_chan_num,
                        stream);*/
                }
            } else {
                codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag = 0;
                codec_int_cfg.stream_cfg[stream].codec_cfg.sample_rate = codec_anc_samp_rate;
                codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_SAMPLE_RATE;
                if (stream == AUD_STREAM_PLAYBACK) {
                    codec_int_cfg.stream_cfg[stream].codec_cfg.vol = TGT_VOLUME_LEVEL_10;
                } else {
                    codec_int_cfg.stream_cfg[stream].codec_cfg.vol = CODEC_SADC_VOL;
                }
                codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_VOL;
                if (stream == AUD_STREAM_PLAYBACK) {
                    codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num = play_chan_num;
                    codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map = CODEC_OUTPUT_DEV;
                } else {
                    codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num = 0;
                    codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map = 0;
                }
                codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_CHANNEL_NUM;

                hal_codec_setup_stream(CODEC_INT_INST, stream, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
            }
        }

        // Start play first, then start capture last
        for (stream = AUD_STREAM_PLAYBACK; stream <= AUD_STREAM_CAPTURE; stream++) {
            if (!codec_int_cfg.stream_cfg[stream].started) {
                // If DAC2/DAC3 stream started, skip to start HW for DAC1 stream
                if (stream == AUD_STREAM_PLAYBACK) {
                    if (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started
                        || codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started) {
                        continue;
                    }
                }
                // If ADC2 stream started, skip to start HW for ADC1 stream
                if (stream == AUD_STREAM_CAPTURE) {
                    if (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE2].started) {
                        continue;
                    }
                }
                codec_hw_start(stream);
            }
        }
    }

    analog_aud_codec_anc_enable(type, true);

    hal_codec_anc_adc_enable(type);

    if (!anc_running) {
        // Enable pa if dac muted before
        if (codec_int_cfg.mute_state[AUD_STREAM_PLAYBACK]) {
            analog_aud_codec_nomute();
        }

#ifdef CODEC_ANC_BOOST
        analog_aud_codec_anc_boost(true);
#endif
    }

    return 0;
}

int codec_anc_close(enum ANC_TYPE_T type)
{
    CODEC_TRACE(2,"%s: type=%d", __func__, type);

    if (anc_type_opened == ANC_NOTYPE) {
        return 0;
    }

    anc_type_opened &= ~type;

    hal_codec_anc_adc_disable(type);

    analog_aud_codec_anc_enable(type, false);

    if (anc_type_opened != ANC_NOTYPE) {
        return 0;
    }

#ifdef CODEC_ANC_BOOST
    analog_aud_codec_anc_boost(false);
#endif

    enum AUD_STREAM_T stream;

    // Stop capture first, then stop play last
    for (stream = AUD_STREAM_CAPTURE; stream >= AUD_STREAM_PLAYBACK && stream <= AUD_STREAM_CAPTURE; stream--) {
        if (!codec_int_cfg.stream_cfg[stream].started) {
            // if DAC2 is start, not stop HW
            if (stream == AUD_STREAM_PLAYBACK) {
                if (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started
                    || codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started) {
                    continue;
                }
            }
            if (stream == AUD_STREAM_CAPTURE) {
                if (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE2].started) {
                    continue;
                }
            }
            codec_hw_stop(stream);
        }
    }

    codec_hw_close(CODEC_CLOSE_NORMAL, CODEC_USER_ANC);

    hal_cmu_anc_disable(HAL_CMU_ANC_CLK_USER_ANC);

    hal_subsys_wake_unlock(HAL_SUBSYS_WAKE_LOCK_USER_ANC);

    // Disable pa if dac muted before
    if (codec_int_cfg.mute_state[AUD_STREAM_PLAYBACK]) {
        analog_aud_codec_mute();
    }

    codec_anc_handler = NULL;

    return 0;
}

static uint32_t codec_internal_stream_setup(enum CODEC_INT_STREAM_T codec,
    enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg)
{
    enum AUD_CHANNEL_MAP_T ch_map;
    enum AUD_STREAM_T codec_stream = stream;

    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_setup] codec=%d stream=%d", codec, stream);

    if (codec_int_cfg.stream_cfg[stream].codec_cfg.sample_rate == AUD_SAMPRATE_NULL) {
        // Codec uninitialized -- all config items should be set
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag = HAL_CODEC_CONFIG_ALL;
    } else {
        // Codec initialized before -- only different config items need to be set
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag = HAL_CODEC_CONFIG_NULL;
    }

    // Always config sample rate, for the pll setting might have been changed by the other stream
    CODEC_TRACE(2,"[sample_rate]old=%d new=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.sample_rate, cfg->sample_rate);

    if (codec_int_cfg.user_map & CODEC_USER_ANC) {
        // Check ANC sample rate
        if (codec_anc_handler) {
            enum AUD_SAMPRATE_T cfg_rate;
            enum AUD_SAMPRATE_T old_rate;

            cfg_rate = hal_codec_anc_convert_rate(cfg->sample_rate);
            if (cfg_rate != codec_anc_samp_rate) {
                old_rate = codec_anc_samp_rate;

                codec_anc_handler(stream, cfg_rate, NULL, NULL);

                codec_anc_samp_rate = cfg_rate;
                TRACE(5,"%s: ANC sample rate changes from %u to %u due to stream=%d samp_rate=%u",
                    __func__, old_rate, codec_anc_samp_rate, stream, cfg->sample_rate);
            }
        }
    }
    codec_int_cfg.stream_cfg[stream].codec_cfg.sample_rate = cfg->sample_rate;
    codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_SAMPLE_RATE;

    if(codec_int_cfg.stream_cfg[stream].codec_cfg.bits != cfg->bits)
    {
        CODEC_TRACE(2,"[bits]old=%d new=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.bits, cfg->bits);
        codec_int_cfg.stream_cfg[stream].codec_cfg.bits = cfg->bits;
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_BITS;
    }

    if(codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num != cfg->channel_num)
    {
        CODEC_TRACE(2,"[channel_num]old=%d new=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num, cfg->channel_num);
        codec_int_cfg.stream_cfg[stream].codec_cfg.channel_num = cfg->channel_num;
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_CHANNEL_NUM;
    }

    ch_map = cfg->channel_map;
    if (ch_map == 0) {
        if (codec_stream == AUD_STREAM_PLAYBACK) {
            ch_map = (enum AUD_CHANNEL_MAP_T)CODEC_OUTPUT_DEV;
        } else {
            ch_map = (enum AUD_CHANNEL_MAP_T)hal_codec_get_input_path_cfg(cfg->io_path);
        }
        ch_map &= AUD_CHANNEL_MAP_ALL;
    }
    if(codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map != ch_map)
    {
        CODEC_TRACE(2,"[channel_map]old=0x%x new=0x%x", codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map, ch_map);
        codec_int_cfg.stream_cfg[stream].codec_cfg.channel_map = ch_map;
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_VOL | HAL_CODEC_CONFIG_BITS;
    }

    if(codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma != cfg->use_dma)
    {
        CODEC_TRACE(2,"[use_dma]old=%d new=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma, cfg->use_dma);
        codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma = cfg->use_dma;
    }

    if(codec_int_cfg.stream_cfg[stream].codec_cfg.vol != cfg->vol)
    {
        CODEC_TRACE(3,"[vol]old=%d new=%d chan_vol_set=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.vol, cfg->vol, codec_int_cfg.chan_vol_set[stream]);
        codec_int_cfg.stream_cfg[stream].codec_cfg.vol = cfg->vol;
        if (!codec_int_cfg.chan_vol_set[stream]) {
            codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_VOL;
        }
    }

    if(codec_int_cfg.stream_cfg[stream].codec_cfg.io_path != cfg->io_path)
    {
        CODEC_TRACE(2,"[io_path]old=%d new=%d", codec_int_cfg.stream_cfg[stream].codec_cfg.io_path, cfg->io_path);
        codec_int_cfg.stream_cfg[stream].codec_cfg.io_path = cfg->io_path;
    }

    CODEC_TRACE(3,"[int_stream_setup] codec=%d, stream=%d set_flag=0x%x",
        codec, stream, codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag);

    switch (codec) {
    case CODEC1_INT_STREAM:
        hal_codec_setup_stream(CODEC_INT_INST, stream, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
        break;
    case CODEC2_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK2) {
            hal_codec_dac2_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
        } else {
            hal_codec_adc2_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
        }
        break;
    case CODEC3_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK3) {
            hal_codec_dac3_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
        }
        break;
    default:
        break;
    }
    return 0;
}

static void codec_internal_stream_mute(enum CODEC_INT_STREAM_T codec, enum AUD_STREAM_T stream, bool mute)
{
    bool anc_on;

    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(3,"[int_stream_mute] stream=%d mute=%d", stream, mute);

    if (mute == codec_int_cfg.mute_state[stream]) {
        CODEC_TRACE(2,"[int_stream_mute] Codec already in mute status: %d", mute);
        return;
    }

    anc_on = !!(codec_int_cfg.user_map & CODEC_USER_ANC);

    if ((stream == AUD_STREAM_PLAYBACK)
        || (stream == AUD_STREAM_PLAYBACK2)
        || (stream == AUD_STREAM_PLAYBACK3)) {
        if (mute) {
            if (!anc_on) {
                analog_aud_codec_mute();
            }
            switch (codec) {
            case CODEC1_INT_STREAM:
                hal_codec_dac_mute(true);
                break;
            case CODEC2_INT_STREAM:
                hal_codec_dac2_mute(true);
                break;
            case CODEC3_INT_STREAM:
                hal_codec_dac3_mute(true);
                break;
            default:
                break;
            }
        } else {
            switch (codec) {
            case CODEC1_INT_STREAM:
                hal_codec_dac_mute(false);
                break;
            case CODEC2_INT_STREAM:
                hal_codec_dac2_mute(false);
                break;
            case CODEC3_INT_STREAM:
                hal_codec_dac3_mute(false);
                break;
            default:
                break;
            }
            if (!anc_on) {
                analog_aud_codec_nomute();
            }
        }
    } else if ((stream == AUD_STREAM_CAPTURE)
                || (stream == AUD_STREAM_CAPTURE2)){
        switch (codec) {
        case CODEC1_INT_STREAM:
            hal_codec_adc_mute(mute);
            break;
        case CODEC2_INT_STREAM:
            hal_codec_adc2_mute(mute);
            break;
        default:
            break;
        }
    }

    codec_int_cfg.mute_state[stream] = mute;
}

static void codec_internal_stream_set_chan_vol(enum CODEC_INT_STREAM_T codec,
    enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(4,"[int_stream_set_chan_vol] codec=%d stream=%d ch_map=0x%X vol=%u", codec, stream, ch_map, vol);

    codec_int_cfg.chan_vol_set[stream] = true;

    switch (codec) {
    case CODEC1_INT_STREAM:
        hal_codec_set_chan_vol(stream, ch_map, vol);
        break;
    case CODEC2_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK2) {
            hal_codec_dac2_set_chan_vol(ch_map, vol);
        } else {
            hal_codec_adc2_set_chan_vol(ch_map, vol);
        }
        break;
    case CODEC3_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK3) {
            hal_codec_dac3_set_chan_vol(ch_map, vol);
        }
        break;
    default:
        break;
    }
}

static void codec_internal_stream_restore_chan_vol(enum CODEC_INT_STREAM_T codec, enum AUD_STREAM_T stream)
{
    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_restore_chan_vol] codec=%d stream=%d", codec, stream);

    if (codec_int_cfg.chan_vol_set[stream]) {
        codec_int_cfg.chan_vol_set[stream] = false;
        // Restore normal volume
        codec_int_cfg.stream_cfg[stream].codec_cfg.set_flag = HAL_CODEC_CONFIG_VOL;

        switch (codec) {
        case CODEC1_INT_STREAM:
            hal_codec_setup_stream(CODEC_INT_INST, stream, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
            break;
        case CODEC2_INT_STREAM:
            if (stream == AUD_STREAM_PLAYBACK2) {
                hal_codec_dac2_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
            } else {
                hal_codec_adc2_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
            }
            break;
        case CODEC3_INT_STREAM:
            if (stream == AUD_STREAM_PLAYBACK3) {
                hal_codec_dac3_setup_stream(CODEC_INT_INST, &(codec_int_cfg.stream_cfg[stream].codec_cfg));
            }
            break;
        default:
            break;
        }
    }
}

static uint32_t codec_internal_stream_start(enum CODEC_INT_STREAM_T codec, enum AUD_STREAM_T stream)
{
    int on = 0;
    enum AUD_STREAM_T codec_stream = stream;

    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_start] stream=%d", stream);

    // set stream started flag
    codec_int_cfg.stream_cfg[stream].started = true;

    // enable analog ADC for capture1/capture2
    if ((stream == AUD_STREAM_CAPTURE) || (stream == AUD_STREAM_CAPTURE2)) {
        codec_analog_adc_enable(stream, true);
    }

    // start stream interface
    switch (codec) {
    case CODEC1_INT_STREAM:
        hal_codec_start_interface(CODEC_INT_INST, stream, codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma);
        break;
    case CODEC2_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK2) {
            hal_codec_dac2_start_interface(CODEC_INT_INST, codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma);
        } else {
            hal_codec_adc2_start_interface(CODEC_INT_INST, codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma);
        }
        break;
    case CODEC3_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK3) {
            hal_codec_dac3_start_interface(CODEC_INT_INST, codec_int_cfg.stream_cfg[stream].codec_cfg.use_dma);
        }
        break;
    default:
        break;
    }

    // check other modules working state
    switch ((int)stream) {
    case AUD_STREAM_PLAYBACK:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started)) {
            on++;
        }
        break;
    case AUD_STREAM_PLAYBACK2:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started)) {
            on++;
        }
        break;
    case AUD_STREAM_PLAYBACK3:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started)) {
            on++;
        }
        break;
    case AUD_STREAM_CAPTURE:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE2].started)) {
            on++;
        }
        break;
    case AUD_STREAM_CAPTURE2:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE].started)) {
            on++;
        }
        break;
    default:
        break;
    }
    // if no any other modules are working, start codec hardware
    if (!on) {
        codec_hw_start(codec_stream);
    }
    return 0;
}

static uint32_t codec_internal_stream_stop(enum CODEC_INT_STREAM_T codec, enum AUD_STREAM_T stream)
{
    int on = 0;
    enum AUD_STREAM_T codec_stream = stream;

    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_stop] stream=%d", stream);

    // start stream interface
    switch (codec) {
    case CODEC1_INT_STREAM:
           hal_codec_stop_interface(CODEC_INT_INST, stream);
        break;
    case CODEC2_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK2) {
            hal_codec_dac2_stop_interface(CODEC_INT_INST);
        } else {
            hal_codec_adc2_stop_interface(CODEC_INT_INST);
        }
        break;
    case CODEC3_INT_STREAM:
        if (stream == AUD_STREAM_PLAYBACK3) {
            hal_codec_dac3_stop_interface(CODEC_INT_INST);
        }
        break;
    default:
        break;
    }

    // check other modules working state
    switch ((int)stream) {
    case AUD_STREAM_PLAYBACK:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started)) {
            on++;
        }
        break;
    case AUD_STREAM_PLAYBACK2:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].started)) {
            on++;
        }
        break;
    case AUD_STREAM_PLAYBACK3:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK].started)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].started)) {
            on++;
        }
        break;
    case AUD_STREAM_CAPTURE:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE2].started)) {
            on++;
        }
        break;
    case AUD_STREAM_CAPTURE2:
        if ((codec_int_cfg.user_map & CODEC_USER_ANC)
            || (codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE].started)) {
            on++;
        }
        break;
    default:
        break;
    }
    // if no any other modules are working, stop codec hardware
    if (!on) {
        codec_hw_stop(codec_stream);
    }

    // disable analog ADC for capture1/capture2
    if ((stream == AUD_STREAM_CAPTURE2) || (stream == AUD_STREAM_CAPTURE)) {
        codec_analog_adc_enable(stream, false);
    }

    // clear started flag
    codec_int_cfg.stream_cfg[stream].started = false;
    return 0;
}

static uint32_t codec_internal_stream_open(enum CODEC_INT_STREAM_T codec,
    enum AUD_STREAM_T stream)
{
    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_open] codec=%d stream=%d", codec, stream);
    codec_int_cfg.stream_cfg[stream].opened = true;
    return 0;
}

static uint32_t codec_internal_stream_close(enum CODEC_INT_STREAM_T codec,
    enum AUD_STREAM_T stream)
{
    enum AUD_STREAM_T codec_stream = stream;
    stream = map_to_codec_int_stream(codec, stream);

    CODEC_TRACE(2,"[int_stream_close] codec=%d stream=%d", codec, stream);

    // if stream started, stop it
    if (codec_int_cfg.stream_cfg[stream].started) {
        switch(codec) {
        case CODEC1_INT_STREAM:
            codec_int_stream_stop(codec_stream);
            break;
        case CODEC2_INT_STREAM:
            codec2_int_stream_stop(codec_stream);
            break;
        case CODEC3_INT_STREAM:
            codec3_int_stream_stop(codec_stream);
            break;
        default:
            break;
        }
    }
    // restore chan vol
    switch(codec) {
    case CODEC1_INT_STREAM:
        codec_int_stream_restore_chan_vol(codec_stream);
        break;
    case CODEC2_INT_STREAM:
        codec2_int_stream_restore_chan_vol(codec_stream);
        break;
    case CODEC3_INT_STREAM:
        codec3_int_stream_restore_chan_vol(codec_stream);
        break;
    default:
        break;
    }
    codec_int_cfg.stream_cfg[stream].opened = false;
    return 0;
}

static uint32_t codec_internal_open(enum CODEC_INT_STREAM_T codec)
{
    enum CODEC_USER_T user;

    CODEC_TRACE(2,"[codec_int_open] codec=%d, user_map=0x%X", codec, codec_int_cfg.user_map);
    switch (codec) {
    case CODEC1_INT_STREAM:
        user = CODEC_USER_STREAM;
        break;
    case CODEC2_INT_STREAM:
        user = CODEC_USER_STREAM2;
        break;
    case CODEC3_INT_STREAM:
        user = CODEC_USER_STREAM3;
        break;
    default:
        break;
    }
    codec_hw_open(user);
    return 0;
}

static uint32_t codec_internal_close(enum CODEC_INT_STREAM_T codec, enum CODEC_CLOSE_TYPE_T type)
{
    CODEC_TRACE(3,"[codec_int_close]: codec=%d, type=%d user_map=0x%X",
        codec, type, codec_int_cfg.user_map);
    switch (codec) {
    case CODEC1_INT_STREAM:
        if (type == CODEC_CLOSE_NORMAL) {
            if (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK].opened == false &&
                    codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE].opened == false){
                codec_hw_close(type, CODEC_USER_STREAM);
            }
        } else {
            codec_hw_close(type, CODEC_USER_STREAM);
        }
        break;
    case CODEC2_INT_STREAM:
        if (type == CODEC_CLOSE_NORMAL) {
            if (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK2].opened == false &&
                    codec_int_cfg.stream_cfg[AUD_STREAM_CAPTURE2].opened == false){
                codec_hw_close(type, CODEC_USER_STREAM2);
            }
        } else {
            codec_hw_close(type, CODEC_USER_STREAM2);
        }
        break;
    case CODEC3_INT_STREAM:
        if (type == CODEC_CLOSE_NORMAL) {
            if (codec_int_cfg.stream_cfg[AUD_STREAM_PLAYBACK3].opened == false) {
                codec_hw_close(type, CODEC_USER_STREAM3);
            }
        } else {
            codec_hw_close(type, CODEC_USER_STREAM3);
        }
        break;
    default:
        break;
    }
    return 0;
}

/*
 * CODEC1 DRIVER MODULE
 */

uint32_t codec_int_stream_setup(enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg)
{
    return codec_internal_stream_setup(codec_remap(CODEC1_INT_STREAM), stream, cfg);
}

void codec_int_stream_mute(enum AUD_STREAM_T stream, bool mute)
{
    codec_internal_stream_mute(codec_remap(CODEC1_INT_STREAM), stream, mute);
}

void codec_int_stream_set_chan_vol(enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    codec_internal_stream_set_chan_vol(codec_remap(CODEC1_INT_STREAM), stream, ch_map, vol);
}

void codec_int_stream_restore_chan_vol(enum AUD_STREAM_T stream)
{
    codec_internal_stream_restore_chan_vol(codec_remap(CODEC1_INT_STREAM), stream);
}

uint32_t codec_int_stream_start(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_start(codec_remap(CODEC1_INT_STREAM), stream);
}

uint32_t codec_int_stream_stop(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_stop(codec_remap(CODEC1_INT_STREAM), stream);
}

uint32_t codec_int_stream_open(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_open(codec_remap(CODEC1_INT_STREAM), stream);
}

uint32_t codec_int_stream_close(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_close(codec_remap(CODEC1_INT_STREAM), stream);
}

uint32_t codec_int_stream_reset_state(enum AUD_STREAM_T stream)
{
    ASSERT(!codec_int_cfg.stream_cfg[stream].started, "%s: stream=%d reset after started", __func__, stream);
    codec_int_cfg.stream_cfg[stream].codec_cfg.sample_rate = AUD_SAMPRATE_NULL;
    return 0;
}

uint32_t codec_int_open(void)
{
    return codec_internal_open(codec_remap(CODEC1_INT_STREAM));
}

uint32_t codec_int_close(enum CODEC_CLOSE_TYPE_T type)
{
    return codec_internal_close(codec_remap(CODEC1_INT_STREAM), type);
}

/*
 * CODEC2 DRIVER MODULE
 */

uint32_t codec2_int_stream_setup(enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg)
{
    return codec_internal_stream_setup(codec_remap(CODEC2_INT_STREAM), stream, cfg);
}

void codec2_int_stream_mute(enum AUD_STREAM_T stream, bool mute)
{
    codec_internal_stream_mute(codec_remap(CODEC2_INT_STREAM), stream, mute);
}

void codec2_int_stream_set_chan_vol(enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    codec_internal_stream_set_chan_vol(codec_remap(CODEC2_INT_STREAM), stream, ch_map, vol);
}

void codec2_int_stream_restore_chan_vol(enum AUD_STREAM_T stream)
{
    codec_internal_stream_restore_chan_vol(codec_remap(CODEC2_INT_STREAM), stream);
}

uint32_t codec2_int_stream_start(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_start(codec_remap(CODEC2_INT_STREAM), stream);
}

uint32_t codec2_int_stream_stop(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_stop(codec_remap(CODEC2_INT_STREAM), stream);
}

uint32_t codec2_int_stream_open(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_open(codec_remap(CODEC2_INT_STREAM), stream);
}

uint32_t codec2_int_stream_close(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_close(codec_remap(CODEC2_INT_STREAM), stream);
}

uint32_t codec2_int_open(void)
{
    return codec_internal_open(codec_remap(CODEC2_INT_STREAM));
}

uint32_t codec2_int_close(enum CODEC_CLOSE_TYPE_T type)
{
    return codec_internal_close(codec_remap(CODEC2_INT_STREAM), type);
}

/*
 * CODEC3 DRIVER MODULE
 */

uint32_t codec3_int_stream_setup(enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg)
{
    return codec_internal_stream_setup(codec_remap(CODEC3_INT_STREAM), stream, cfg);
}

void codec3_int_stream_mute(enum AUD_STREAM_T stream, bool mute)
{
    codec_internal_stream_mute(codec_remap(CODEC3_INT_STREAM), stream, mute);
}

void codec3_int_stream_set_chan_vol(enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    codec_internal_stream_set_chan_vol(codec_remap(CODEC3_INT_STREAM), stream, ch_map, vol);
}

void codec3_int_stream_restore_chan_vol(enum AUD_STREAM_T stream)
{
    codec_internal_stream_restore_chan_vol(codec_remap(CODEC3_INT_STREAM), stream);
}

uint32_t codec3_int_stream_start(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_start(codec_remap(CODEC3_INT_STREAM), stream);
}

uint32_t codec3_int_stream_stop(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_stop(codec_remap(CODEC3_INT_STREAM), stream);
}

uint32_t codec3_int_stream_open(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_open(codec_remap(CODEC3_INT_STREAM), stream);
}

uint32_t codec3_int_stream_close(enum AUD_STREAM_T stream)
{
    return codec_internal_stream_close(codec_remap(CODEC3_INT_STREAM), stream);
}

uint32_t codec3_int_open(void)
{
    return codec_internal_open(codec_remap(CODEC3_INT_STREAM));
}

uint32_t codec3_int_close(enum CODEC_CLOSE_TYPE_T type)
{
    return codec_internal_close(codec_remap(CODEC3_INT_STREAM), type);
}

#endif

