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
#ifndef __SPA_EXT_M55_HANDLER_H__
#define __SPA_EXT_M55_HANDLER_H__

void app_spa_audio_aud_data_snd(unsigned char * pmsg, int32_t opt);
void app_spa_audio_codec_config_snd(spa_audio_codec_cfg_t* config, uint16_t length);
#endif
