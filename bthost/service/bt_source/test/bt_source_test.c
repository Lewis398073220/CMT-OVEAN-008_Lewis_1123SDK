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
#if defined(BT_SOURCE) && defined(APP_BT_SOURCE_TEST) && defined(ESHELL_ON)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_a2dp_source.h"
#include "app_avrcp_target.h"
#include "app_hfp_ag.h"
#include "btapp.h"
#include "besbt_cfg.h"
#include "nvrecord_env.h"
#include "hal_trace.h"
#include "app_bt_func.h"
#include "app_bt.h"
#include "app_bt_media_manager.h"

#include "bt_source.h"
#include "eshell.h"

static int bt_source_event_callback(bt_source_event_t event, bt_source_event_param_t *param)
{
    bt_bdaddr_t *addr = NULL;
    switch (event) {
        case BT_SOURCE_EVENT_SEARCH_RESULT:
            addr = param->p.search_result.result->addr;
            eshell_putstring_nl("bt_source_search : found device, addr=0x%x 0x%x 0x%x 0x%x 0x%x 0x%x,name=%s",
                addr->address[0], addr->address[1], addr->address[2], addr->address[3], addr->address[4], addr->address[5],
                param->p.search_result.result->name);
        break;
        case BT_SOURCE_EVENT_SEARCH_COMPLETE:
            eshell_putstring_nl("bt_source_search : complete");
        break;
        case BT_SOURCE_EVENT_A2DP_SOURCE_CONNECT_FAIL:
            eshell_putstring_nl("bt_source: connect fail");
        break;
        case BT_SOURCE_EVENT_A2DP_SOURCE_STREAM_OPEN:
            eshell_putstring_nl("bt_source: stream open");
        break;
        case BT_SOURCE_EVENT_A2DP_SOURCE_STREAM_CLOSE:
            eshell_putstring_nl("bt_source: stream close");
        break;
    }
    return 0;
}

// bt_source_test_start
static void bt_source_test_start_cmd(char* param)
{
    eshell_putstring_nl("bt_source_test_start...");
    bt_source_register_callback(bt_source_event_callback);
}
ESHELL_DEF_COMMAND("bt_source_test_start", "start bt source test", bt_source_test_start_cmd);

// search device
static void bt_source_search_cmd(char* param)
{
    eshell_putstring_nl("bt_source_search...");
    app_bt_source_search_device();
}
ESHELL_DEF_COMMAND("bt_source_search", "search bt device", bt_source_search_cmd);

#endif /* BT_SOURCE */