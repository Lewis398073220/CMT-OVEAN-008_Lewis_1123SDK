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
#ifndef __SPA_APP_MAIN_H__
#define __SPA_APP_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

void spatial_audio_app_init_if(unsigned int sample_rate, unsigned char sample_bits, unsigned char channel, unsigned int frame_len,void * alloc_func,unsigned char role, unsigned char chan_id);
void spatial_audio_app_deinit_if(void);
void spatial_audio_app_run_if(unsigned char * data_ptr, int data_size, unsigned char role);
void spatial_audio_app_audio_conf_udpate_if(unsigned char id, unsigned char * param_ptr, unsigned int param_size);

#ifdef __cplusplus
}
#endif

#endif
