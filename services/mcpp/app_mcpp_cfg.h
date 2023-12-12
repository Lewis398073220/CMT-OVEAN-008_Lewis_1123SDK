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
#ifndef __APP_MCPP_CFG_H__
#define __APP_MCPP_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

// #define _APP_MCPP_DEBUG

#if defined(_APP_MCPP_DEBUG)
#define MCPP_LOG_D(str, ...)          TRACE(0, "[MCPP_DEBUG] " str, ##__VA_ARGS__)
#else
#define MCPP_LOG_D(str, ...)
#endif
#define MCPP_LOG_I(str, ...)          TRACE(0, "[MCPP] " str, ##__VA_ARGS__)

#ifdef APP_MCPP_BTH_M55
#define APP_MCPP_BTH_M55_ENABLE
#endif

#ifdef APP_MCPP_BTH_HIFI
#define APP_MCPP_BTH_HIFI_ENABLE
#ifdef APP_MCPP_BTH_HIFI_NO_RTOS
#define APP_MCPP_BTH_HIFI_NO_RTOS_ENABLE
#endif
#endif

#ifdef APP_MCPP_BTH_SENS
#define APP_MCPP_BTH_SENS_ENABLE
#endif

#ifdef APP_MCPP_BTH_CP_SUBSYS
#define APP_MCPP_BTH_CP_SUBSYS_ENABLE
#ifdef APP_MCPP_BTH_CP_SUBSYS_NO_RTOS
#define APP_MCPP_BTH_CP_SUBSYS_NO_RTOS_ENABLE
#else
#error "If you want to use CP_SUBSYS as the dsp core, you must enable APP_MCPP_BTH_CP_SUBSYS_NO_RTOS!"
#endif
#endif

#ifdef APP_MCPP_M55_HIFI
#define APP_MCPP_M55_HIFI_ENABLE
#endif

#ifdef __cplusplus
}
#endif

#endif