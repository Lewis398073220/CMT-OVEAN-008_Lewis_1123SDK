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

#ifndef __APP_TOTA_CUSTOM_H__
#define __APP_TOTA_CUSTOM_H__

#include "app_tota_cmd_code.h"

void app_tota_custom_init();
void _custom_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen);

#endif
