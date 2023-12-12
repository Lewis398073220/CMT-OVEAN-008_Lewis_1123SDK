/**
 * @copyright Copyright (c) 2015-20223 BES Technic.
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
 */
#ifndef APP_BIS_TRANSPOND_LINEIN_INPUT_H_
#define APP_BIS_TRANSPOND_LINEIN_INPUT_H_

/*****************************header include********************************/


/*********************external function declaration*************************/


/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declaration*************************/

/************************private variable defination************************/
void app_bis_tran_linein_stream_start();

void app_bis_tran_linein_stream_stop();

void app_bis_tran_linein_src_init(uint8_t mode);

void app_bis_tran_linein_src_deinit();
#endif //APP_BIS_TRANSPOND_LINEIN_INPUT_H_