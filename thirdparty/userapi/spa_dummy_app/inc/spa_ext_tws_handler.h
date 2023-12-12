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
#ifndef __SPA_EXT_TWS_HANDLER_H__
#define __SPA_EXT_TWS_HANDLER_H__

void app_spa_tws_sens_data_sync_snd(unsigned char *p_buff, unsigned short length);
void app_spa_tws_sens_data_sync_snd_handler(unsigned char *p_buff, unsigned short length);
void app_spa_tws_sens_data_sync_rcv_handler(unsigned short rsp_seq, unsigned char *p_buff, unsigned short length);

#endif
