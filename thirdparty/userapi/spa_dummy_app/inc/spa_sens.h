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
#ifndef __SPA_SENS_H__
#define __SPA_SENS_H__

#include "spa_common.h"

typedef struct{
    /* counter */
    int call_cnt_proc;
    int call_cnt_snd_sens;

    /* timestamp */
    int call_tm_proc;
    int call_tm_snd_sens;
}spa_sens_debugging_statistic_info_t;

typedef struct spa_sens_state_s{
    /* general info */
    data_info_t data_info;
    data_sens_t data_sens;

    bool  inited;

    /* debugging info */
    spa_sens_debugging_statistic_info_t debug_info;

    /* internal func handler */
    spa_algo_op_handler_t algo_op_handler;

}spa_sens_state_t;

int spa_sens_update_sens_data(short * ptr, int data_src);
int spa_sens_create(data_info_t * info, ext_alloc_func_t  alloc, const spa_algo_op_handler_t* algo_func);
int spa_sens_proc(void);

#endif
