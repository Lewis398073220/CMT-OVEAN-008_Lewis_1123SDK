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
#ifndef __A2DP_DECODER_CC_BTH_H__
#define __A2DP_DECODER_CC_BTH_H__

#ifdef __cplusplus
extern "C" {
#endif
bool a2dp_decoder_bth_feed_encoded_data_into_off_core(
    uint8_t* list_data, uint32_t time_stamp, uint16_t seq_num,
    uint16_t total_sub_seq, uint16_t sub_seq, uint16_t data_len, uint8_t* pData);
void a2dp_decoder_bth_start_stream(
                    A2DP_DECODER_CODEC_TYPE_E codecType,
                    A2DP_DECODER_CHANNEL_SELECT_E chnlSel,
                    A2DP_AUDIO_CC_OUTPUT_CONFIG_T *config,
                    uint16_t maxseqnum);
void a2dp_decoder_bth_stop_stream(void);
void a2dp_decoder_bth_remove_specifc_frame(    A2DP_DECODER_CC_FLUSH_T flushInfo);
int a2dp_decoder_bth_fetch_pcm_data(uint16_t last_seq_num,
    uint16_t last_total_sub_seq, uint16_t last_sub_seq,
    uint8_t *ptrData, uint16_t dataLen);
void a2dp_decoder_bth_a2dp_boost_freq_req(void);
int a2dp_decoder_cc_sema_init(void);
int a2dp_decoder_cc_sema_deinit(void);
#ifdef __cplusplus
}
#endif

#endif

