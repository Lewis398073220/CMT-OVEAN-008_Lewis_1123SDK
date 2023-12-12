/**
 * @file gaf_stream_dbg.h
 * @author BES CD team
 * @version 0.1
 * @date 2023-05-13
 *
 * @copyright Copyright (c) 2015-2023 BES Technic.
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

#ifndef __GAF_STREAM_DBG_H__
#define __GAF_STREAM_DBG_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "hal_trace.h"

#define MODULE_TRACE_LEVEL  TR_LEVEL_INFO

/******************************macro defination*****************************/
#define LOG_V(str, ...) if (MODULE_TRACE_LEVEL >= TR_LEVEL_VERBOSE) TR_VERBOSE(TR_MOD(AOB_SM), str, ##__VA_ARGS__)
#define LOG_D(str, ...) if (MODULE_TRACE_LEVEL >= TR_LEVEL_DEBUG) TR_DEBUG(TR_MOD(AOB_SM), str, ##__VA_ARGS__)
#define LOG_I(str, ...) if (MODULE_TRACE_LEVEL >= TR_LEVEL_INFO) TR_INFO(TR_MOD(AOB_SM), str, ##__VA_ARGS__)
#define LOG_W(str, ...) if (MODULE_TRACE_LEVEL >= TR_LEVEL_WARN) TR_WARN(TR_MOD(AOB_SM), str, ##__VA_ARGS__)
#define LOG_E(str, ...) if (MODULE_TRACE_LEVEL >= TR_LEVEL_ERROR) TR_ERROR(TR_MOD(AOB_SM), str, ##__VA_ARGS__)


/******************************type defination******************************/

/****************************function declearation**************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __GAF_STREAM_DBG_H__ */
