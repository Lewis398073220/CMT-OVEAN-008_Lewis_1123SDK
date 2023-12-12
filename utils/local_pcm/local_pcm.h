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
#ifndef __LOCAL_PCM_H__
#define __LOCAL_PCM_H__

#ifdef __cplusplus
extern "C" {
#endif

int32_t local_pcm_init(const uint8_t *buf, uint32_t size);

int32_t local_pcm_get_data(uint8_t *buf, uint32_t size);
int32_t local_pcm_loop_get_data(uint8_t *buf, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif