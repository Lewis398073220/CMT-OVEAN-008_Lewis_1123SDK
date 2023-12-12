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
#ifndef __A2DP_DECODER_CC_OFF_BTH_H__
#define __A2DP_DECODER_CC_OFF_BTH_H__

#ifdef __cplusplus
extern "C" {
#endif

uint8_t a2dp_decoder_cc_get_channel(void);

void a2dp_decoder_cc_off_bth_init(void);

#ifdef A2DP_LDAC_ON
uint32_t a2dp_decoder_cc_get_sample_rate(void);

uint32_t a2dp_decoder_cc_get_channnel_mode(void);
#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_LHDCV5_ON)

uint32_t a2dp_decoder_cc_get_sample_rate(void);
uint8_t a2dp_decoder_cc_get_bits_depth(void);
uint8_t a2dp_decoder_cc_get_lhdc_ext_flag(void);
uint8_t *a2dp_decoder_cc_get_lhdc_data(void);
uint8_t a2dp_decoder_cc_get_channel_sel(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

