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
#ifndef __SPA_CTRL_H__
#define __SPA_CTRL_H__

#include "spa_common.h"

#ifndef SPA_DATA_RELAY_TIME_INTERVAL_MS
#define SPA_DATA_RELAY_TIME_INTERVAL_MS (10)
#endif

typedef struct{
    /* counter */
    int call_cnt_proc;
    int call_cnt_snd_aud;
    int call_cnt_snd_relay;
    int call_cnt_rcv_sens;
    int call_cnt_rcv_relay;

    /* timestamp */
    unsigned int call_tm_proc;
    unsigned int call_tm_snd_aud;
    unsigned int call_tm_snd_relay;
    unsigned int call_tm_rcv_sens;
    unsigned int call_tm_rcv_relay;
}spa_ctrl_debugging_statistic_info_t;

typedef struct spa_ctrl_state_s{
    /* general info */
    data_info_t data_info;
    data_sens_t data_sens;
    data_aud_t data_aud;

    spa_algo_op_role_e  algo_op_role;

    bool  inited;

    /* debugging info */
    spa_ctrl_debugging_statistic_info_t debug_info;

    /* internal func handler */
    spa_algo_op_handler_t algo_op_handler;

    /* internal data buffer */
    spa_buffer_t buffer;

}spa_ctrl_state_t;

int spa_ctrl_create(data_info_body_t * info, ext_alloc_func_t  alloc, const spa_algo_op_handler_t* algo_func,spa_algo_op_role_e role);
int spa_ctrl_proc(unsigned char * data_ptr, int data_size, data_info_t *data_info_ptr, spa_algo_op_role_e role);
int spa_ctrl_update_sens_data(unsigned char * ptr, int data_src);
void spa_ctrl_update_cfg_op_role(spa_algo_op_role_e role);
int spa_ctrl_update_cfg_data_info(unsigned char * ptr, int data_src);
#endif
