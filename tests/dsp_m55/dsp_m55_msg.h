/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef __DSP_M55_MSG_H__
#define __DSP_M55_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

enum DSP_M55_MSG_ID_T {
    DSP_M55_MSG_ID_TEST,
};

struct DSP_M55_MSG_HDR_T {
    uint16_t id     : 15;
    uint16_t reply  : 1;
};

#ifdef __cplusplus
}
#endif

#endif

