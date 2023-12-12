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
#include "app_tota_cmd_code.h"
#include "bluetooth_bt_api.h"
#include "app_bt_stream.h"
#include "app_audio.h"
#include "nvrecord_bt.h"
#include "nvrecord_env.h"
#include "app_tota.h"
#include "app_tota_cmd_handler.h"
#include "apps.h"

#include "app_tota_multi_chip_updata.h"

/**/

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/

void _custom_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG(3,"[%s]: opCode:0x%x, paramLen:%d", __func__, funcCode, paramLen);

    if(funcCode != OP_TOTA_OTA)
    {
        app_tota_send_rsp(funcCode,TOTA_NO_ERROR,NULL,0);
    }
    switch (funcCode)
    {
        case OP_TOTA_FACTORY_RESET:
            TRACE(0,"##### custom: factory reset");
            app_reset();
            break;
        case OP_TOTA_CLEAR_PAIRING_INFO:
            TRACE(0,"##### custom: clear pairing info");
            //app_ibrt_nvrecord_delete_all_mobile_record();
            break;
        case OP_TOTA_SHUTDOWM:
            TRACE(0,"##### custom: shutdown");
            app_shutdown();
            break;
        case OP_TOTA_REBOOT:
            TRACE(0,"##### custom: reboot");
            // TODO:
            break;
#if defined(OTA_OVER_TOTA_ENABLED)
        case OP_TOTA_OTA:
            TRACE(0,"##### custom: ota");
            TOTA_LOG_DBG(2,"[OTA]: ota_opCode:0x%x, paramLen:%d", *ptrParam, paramLen);
#ifdef TOTA_CROSS_CHIP_OTA
            app_handle_multi_chip_tota_data(ptrParam,paramLen);
#else
            app_ota_over_tota_receive_data(ptrParam, paramLen);
#endif
            break;
#endif
        default:
            ;
    }
}

TOTA_COMMAND_TO_ADD(OP_TOTA_FACTORY_RESET, _custom_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_CLEAR_PAIRING_INFO, _custom_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_SHUTDOWM, _custom_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_REBOOT, _custom_cmd_handle, false, 0, NULL );
#if defined(OTA_OVER_TOTA_ENABLED)
TOTA_COMMAND_TO_ADD(OP_TOTA_OTA, _custom_cmd_handle, false, 0, NULL );
#endif
