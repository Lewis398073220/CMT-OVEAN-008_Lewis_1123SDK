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
#ifndef __A2DP_ENCODER_CC_BTH_H__
#define __A2DP_ENCODER_CC_BTH_H__

#include "a2dp_encoder_cc_common.h"
#include "app_source_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

bool a2dp_encoder_bth_feed_pcm_data_into_off_core(uint8_t * pcm_buf, uint16_t len);
void a2dp_encoder_bth_start_stream(
                    A2DP_ENCODER_CODEC_TYPE_E codecType,
                    A2DP_AUDIO_CC_OUTPUT_CONFIG_T *config,
                    uint16_t maxseqnum,
                    uint16_t maxosize_pframe);
void a2dp_encoder_bth_stop_stream(void);
int a2dp_encoder_bth_fetch_encoded_data(a2dp_source_packet_t *source_packe);
int a2dp_encoder_cc_sema_init(void);
int a2dp_encoder_cc_sema_deinit(void);
#ifdef __cplusplus
}
#endif

#endif

