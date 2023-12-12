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
#ifndef __SPA_BTH_MAIN_H__
#define __SPA_BTH_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

void spatial_audio_sens_process_init_if(uint32_t sample_rate, uint8_t sample_bits, uint8_t channel, uint32_t frame_len,void * alloc_func);
void spatial_audio_sens_process_deinit_if(void);
void spatial_audio_sens_process_run_if(unsigned char * data_ptr, int data_size);

#ifdef __cplusplus
}
#endif

#endif
