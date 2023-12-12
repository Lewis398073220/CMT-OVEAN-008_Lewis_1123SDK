/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
/**
 ****************************************************************************************
 * @addtogroup GAF_NON_CODEC_STREAM
 * @{
 ****************************************************************************************
 */

#ifndef __GAF_NON_CODEC_STREAM_H__
#define __GAF_NON_CODEC_STREAM_H__
#include "app_bap_uc_srv_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*HEAD_TRACKING_DATA_CHANGED_CB_T)(uint8_t *headTrack_data);
void gaf_media_head_track_data_changed_register_cb(HEAD_TRACKING_DATA_CHANGED_CB_T cb);
void gaf_media_head_tracking_data_changed_handler(uint8_t *headTrack_data);
void gaf_non_codec_stream_update_env(void* pStreamEnv);
void gaf_non_codec_upstream_start(void);
void gaf_non_codec_upstream_stop(uint8_t con_lid);
void gaf_non_codec_stream_update_output_data(uint8_t* pBuf, uint32_t bufLen);
void gaf_simulated_sensor_data_update_upstream_start(app_bap_ascs_ase_t * p_bap_ase_info, uint8_t ase_lid);

#ifdef __cplusplus
}
#endif

#endif // __GAF_NON_CODEC_STREAM_H__

/// @} GAF_NON_CODEC_STREAM