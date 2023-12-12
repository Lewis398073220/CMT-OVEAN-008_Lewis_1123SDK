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
#ifndef __APP_IBRT_IF_CUSTOM_CMD__
#define __APP_IBRT_IF_CUSTOM_CMD__

#include "app_ibrt_custom_cmd.h"

#define app_ibrt_custom_cmd_rsp_timeout_handler_null   (0)
#define app_ibrt_custom_cmd_rsp_handler_null           (0)
#define app_ibrt_custom_cmd_rx_handler_null            (0)

typedef enum
{
    APP_TWS_CMD_SHARE_FASTPAIR_INFO = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x01,
#ifdef __GMA_VOICE__
    APP_TWS_CMD_GMA_SECRET_KEY      = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x02,
#endif

#ifdef DUAL_MIC_RECORDING
    APP_IBRT_CUSTOM_CMD_DMA_AUDIO     = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x04,
    APP_IBRT_CUSTOM_CMD_UPDATE_BITRATE= APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x05,
    APP_IBRT_CUSTOM_CMD_REPORT_BUF_LVL= APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x06,
#endif

#if defined(ANC_APP)
    APP_TWS_CMD_SYNC_ANC_STATUS         = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x07,
#endif

#if defined(PSAP_APP)
    APP_TWS_CMD_SYNC_PSAP_STATUS        = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x08,
#endif

#if defined(ANC_ASSIST_ENABLED)
    APP_TWS_CMD_SYNC_ANC_ASSIST_STATUS  = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x09,
#endif

    APP_TWS_CMD_SYNC_AUDIO_PROCESS      = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0A,
    APP_TWS_CMD_CONTROL_SBM             = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0B,

#if defined(__BIXBY_VOICE__)
    APP_TWS_CMD_SYNC_BIXBY_STATE        = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0C,
#endif

#if !defined(IBRT_UI) && defined(__REPORT_EVENT_TO_CUSTOMIZED_UX__)
    APP_TWS_CMD_SHARE_LINK_INFO         = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0D,
#endif
    APP_TWS_CMD_ROLE_SWITCH_MONITOR     = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0E,
#if defined (SPA_AUDIO_ENABLE)
    APP_TWS_CMD_SPA_SENS_DATA_SYNC = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x20,
#endif
#ifdef CUSTOM_BITRATE
    APP_IBRT_CUSTOM_CMD_SYNC_CODEC_INFO  = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x11,
#endif
    //new customer cmd add here
} app_ibrt_custom_cmd_code_e;

#ifdef CUSTOM_BITRATE
typedef struct
{
    uint32_t aac_bitrate;
    uint32_t sbc_bitpool;
    uint32_t audio_latentcy;
} ibrt_custome_codec_t; 

#ifdef FREEMAN_ENABLED_STERO
//stereo 150ms audio latency by default
#define USER_CONFIG_DEFAULT_AUDIO_LATENCY (150)
#else
//tws 280ms audio latency by default
#define USER_CONFIG_DEFAULT_AUDIO_LATENCY (280)
#endif

#define USER_CONFIG_SBC_AUDIO_LATENCY_ERROR (0)
#define USER_CONFIG_AAC_AUDIO_LATENCY_ERROR (0)

#define USER_CONFIG_MIN_AAC_BITRATE (197*1024)
#define USER_CONFIG_MAX_AAC_BITRATE (256*1024)
#define USER_CONFIG_DEFAULT_AAC_BITRATE USER_CONFIG_MAX_AAC_BITRATE

#define USER_CONFIG_MIN_SBC_BITPOOL (41)
#define USER_CONFIG_MAX_SBC_BITPOOL (53)
#define USER_CONFIG_DEFAULT_SBC_BITPOOL USER_CONFIG_MAX_SBC_BITPOOL

#endif

#endif
