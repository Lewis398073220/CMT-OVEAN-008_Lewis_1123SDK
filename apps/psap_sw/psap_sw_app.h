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
#ifndef __PSAP_SW_APP_H__
#define __PSAP_SW_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

int32_t psap_sw_app_open(void);

int32_t psap_sw_app_close(void);

bool psap_sw_app_is_opened(void);

#ifdef __cplusplus
}
#endif

#endif
