/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifndef __RX_DSP_TRC_H__
#define __RX_DSP_TRC_H__

#ifdef __cplusplus
extern "C" {
#endif

unsigned int dsp_hifi4_trace_rx_handler(const void *data, unsigned int len);

void dsp_hifi4_trace_server_open(void);

void dsp_hifi4_trace_server_close(void);

#ifdef __cplusplus
}
#endif

#endif
