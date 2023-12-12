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
#include <string.h>
#include "app_trace_rx.h"
#include "app_bt_cmd.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "bluetooth.h"
#include "crc16_c.h"
#include "heap_api.h"
#include "hci_api.h"
#include "me_api.h"
#include "spp_api.h"
#include "l2cap_api.h"
#include "conmgr_api.h"
#include "bt_if.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "app_bt.h"
#include "btapp.h"
#include "app_factory_bt.h"
#include "apps.h"
#include "app_a2dp.h"
#include "nvrecord_extension.h"
#include "nvrecord_bt.h"
#include "app_bt_func.h"
#include "app_testmode.h"
#include "besbt_cfg.h"
#include "besaud_api.h"
#include "ddbif.h"

#ifdef APP_CHIP_BRIDGE_MODULE
#include "app_chip_bridge.h"
#endif

#if defined(BT_HID_DEVICE)
#include "app_bt_hid.h"
#endif

#if defined(BT_SOURCE)
#include "bt_source.h"
#include "app_a2dp_source.h"
#include "bts_a2dp_source.h"
#endif

#if defined(BT_MAP_SUPPORT)
#include "app_map.h"
#endif

#if defined(BT_PBAP_SUPPORT)
#include "app_pbap.h"
#endif

#if defined(BT_OPP_SUPPORT)
#include "app_opp.h"
#endif

#if defined(BT_PAN_SUPPORT)
#include "app_pan.h"
#endif

#ifdef AUDIO_LINEIN
#include "app_bt_media_manager.h"
#endif

#ifdef IBRT
#include "app_ibrt_internal.h"
#include "earbud_profiles_api.h"
#include "earbud_ux_api.h"
#include "earbud_ux_duplicate_api.h"
#endif

static bt_bdaddr_t g_app_bt_pts_addr = {{
#if 1
    0xed, 0xb6, 0xf4, 0xdc, 0x1b, 0x00
#elif 0
    0x81, 0x33, 0x33, 0x22, 0x11, 0x11
#elif 0
    0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
    0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
}};

bt_bdaddr_t *app_bt_get_pts_address(void)
{
    return &g_app_bt_pts_addr;
}

#define APP_BT_CMD_MAX_TEST_TABLES (100)
static app_bt_cmd_handle_with_parm_table_t g_bt_cmd_with_parm_tables[APP_BT_CMD_MAX_TEST_TABLES];
static app_bt_cmd_handle_table_t g_bt_cmd_tables[APP_BT_CMD_MAX_TEST_TABLES];
extern void change_a2dp_audio_channel(const char* cmd, uint32_t cmd_len);

void app_bt_cmd_add_test_table(const app_bt_cmd_handle_t *start_cmd, uint16_t cmd_count)
{
    int i = 0;
    app_bt_cmd_handle_table_t *table = NULL;
    app_bt_cmd_handle_table_t *found = NULL;

    if (start_cmd == NULL || cmd_count == 0)
    {
        TRACE(3, "%s invalid params %p %d", __func__, start_cmd, cmd_count);
        return;
    }

    for (; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
    {
        table = g_bt_cmd_tables + i;
        if (!table->inuse)
        {
            found = table;
            break;
        }
    }

    if (!found)
    {
        TRACE(1, "%s cannot alloc free table", __func__);
        return;
    }

    found->inuse = true;
    found->start_cmd = start_cmd;
    found->cmd_count = cmd_count;
}

void app_bt_cmd_add_test_table_with_param(const app_bt_cmd_handle_with_parm_t *start_cmd, uint16_t cmd_count)
{
    int i = 0;
    app_bt_cmd_handle_with_parm_table_t *table = NULL;
    app_bt_cmd_handle_with_parm_table_t *found = NULL;

    if (start_cmd == NULL || cmd_count == 0)
    {
        TRACE(3, "%s invalid params %p %d", __func__, start_cmd, cmd_count);
        return;
    }

    for (; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
    {
        table = g_bt_cmd_with_parm_tables + i;
        if (!table->inuse)
        {
            found = table;
            break;
        }
    }

    if (!found)
    {
        TRACE(1, "%s cannot alloc free table", __func__);
        return;
    }

    found->inuse = true;
    found->start_cmd = start_cmd;
    found->cmd_count = cmd_count;
}

static const app_bt_cmd_handle_with_parm_t app_factory_bt_tota_hande_table_with_param[]=
{
    {"LE_TX_TEST",                      app_factory_enter_le_tx_test},
    {"LE_RX_TEST",                      app_factory_enter_le_rx_test},
    {"LE_CONT_TX_TEST",                 app_factory_enter_le_continueous_tx_test},
    {"LE_CONT_RX_TEST",                 app_factory_enter_le_continueous_rx_test},
    {"CHANGE_A2DP_CHANNEL_TEST",        change_a2dp_audio_channel},
};

static const app_bt_cmd_handle_t app_factory_bt_tota_hande_table[]=
{
    {"LE_TEST_RESULT",                  app_factory_remote_fetch_le_teset_result},
    {"LE_TEST_END",                     app_factory_terminate_le_test},
    {"BT_DUT_MODE",                     app_enter_signalingtest_mode},
    {"NON_SIGNALING_TEST",              app_enter_non_signalingtest_mode},
    {"0091",                            app_factory_enter_bt_38chanl_dh5_prbs9_tx_test},
    {"0094",                            app_factory_enter_bt_38chanl_dh1_prbs9_rx_test},
    {"BT_RX_TEST_RESULT",               app_factory_remote_fetch_bt_nonsig_test_result},
};

void app_bt_add_string_test_table(void)
{
    app_bt_cmd_add_test_table(app_factory_bt_tota_hande_table, ARRAY_SIZE(app_factory_bt_tota_hande_table));
    app_bt_cmd_add_test_table_with_param(app_factory_bt_tota_hande_table_with_param,
            ARRAY_SIZE(app_factory_bt_tota_hande_table_with_param));
}

bool app_bt_cmd_table_execute(const app_bt_cmd_handle_t *start_cmd, uint16_t count, char *cmd, unsigned int cmd_length)
{
    const app_bt_cmd_handle_t *command = NULL;

    for (int j = 0; j < count; j += 1)
    {
        command = start_cmd + j;
        if (strncmp((char *)cmd, command->string, cmd_length) == 0 || strstr(command->string, cmd))
        {
            command->function();
            return true;
        }
    }

    return false;
}

bool app_bt_cmd_table_with_param_execute(const app_bt_cmd_handle_with_parm_t *start_cmd, uint16_t count,
        char *cmd_prefix, unsigned int cmd_prefix_length, char *cmd_param, unsigned int param_len)
{
    const app_bt_cmd_handle_with_parm_t *command_with_parm = NULL;

    for (int j = 0; j < count; j += 1)
    {
        command_with_parm = start_cmd + j;
        if (strncmp((char *)cmd_prefix, command_with_parm->string, cmd_prefix_length) == 0 || strstr(command_with_parm->string, cmd_prefix))
        {
            command_with_parm->function(cmd_param, param_len);
            return true;
        }
    }

    return false;
}

void app_bt_cmd_line_handler(char *cmd, unsigned int cmd_length)
{
    const app_bt_cmd_handle_table_t *table = NULL;
    const app_bt_cmd_handle_with_parm_table_t *table_with_parm = NULL;
    int i = 0;
    int param_len = 0;
    char* cmd_param = NULL;
    char* cmd_end = cmd + cmd_length;

    cmd_param = strstr((char*)cmd, (char*)"|");

    if (cmd_param)
    {
        *cmd_param = '\0';
        cmd_length = cmd_param - cmd;
        cmd_param += 1;


        param_len = cmd_end - cmd_param;

        for (i = 0; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
        {
            table_with_parm = g_bt_cmd_with_parm_tables + i;
            if (table_with_parm->inuse)
            {
                if (app_bt_cmd_table_with_param_execute(table_with_parm->start_cmd, table_with_parm->cmd_count, cmd, cmd_length, cmd_param, param_len))
                {
                    return;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
        {
            table = g_bt_cmd_tables + i;
            if (table->inuse)
            {
                if (app_bt_cmd_table_execute(table->start_cmd, table->cmd_count, cmd, cmd_length))
                {
                    return;
                }
            }
        }
    }

    TRACE(2, "%s cmd not found %s", __func__, cmd);
}

#ifdef IBRT
#ifdef IBRT_UI
#include "app_tws_ibrt.h"
#include "app_tws_besaud.h"
#include "app_vendor_cmd_evt.h"
#include "tws_role_switch.h"
#include "app_ibrt_internal.h"
#include "app_ibrt_keyboard.h"
#include "app_tws_ibrt_cmd_handler.h"

#include "app_tws_ibrt_ui_test.h"
#endif

osThreadId app_bt_cmd_tid;
static void app_bt_cmd_thread(void const *argument);
osThreadDef(app_bt_cmd_thread, osPriorityNormal, 1, 2048, "app_bt_cmd_thread");

#define APP_BT_CMD_MAILBOX_MAX (20)
osMailQDef (app_bt_cmd_mailbox, APP_BT_CMD_MAILBOX_MAX, APP_BT_CMD_MSG_BLOCK);
static osMailQId app_bt_cmd_mailbox = NULL;
static uint8_t app_bt_cmd_mailbox_cnt = 0;

static int app_bt_cmd_mailbox_init(void)
{
    app_bt_cmd_mailbox = osMailCreate(osMailQ(app_bt_cmd_mailbox), NULL);
    if (app_bt_cmd_mailbox == NULL)  {
        TRACE(0,"Failed to Create app_bt_cmd_mailbox\n");
        return -1;
    }
    app_bt_cmd_mailbox_cnt = 0;
    return 0;
}

int app_bt_cmd_mailbox_put(APP_BT_CMD_MSG_BLOCK* msg_src)
{
    if(!msg_src){
        TRACE(0,"msg_src is a null pointer in app_bt_cmd_mailbox_put!");
        return -1;
    }
    osStatus status;
    APP_BT_CMD_MSG_BLOCK *msg_p = NULL;
    msg_p = (APP_BT_CMD_MSG_BLOCK*)osMailAlloc(app_bt_cmd_mailbox, 0);
    ASSERT(msg_p, "osMailAlloc error");
    *msg_p = *msg_src;
    status = osMailPut(app_bt_cmd_mailbox, msg_p);
    if (osOK == status)
        app_bt_cmd_mailbox_cnt++;
    return (int)status;
}

int app_bt_cmd_mailbox_free(APP_BT_CMD_MSG_BLOCK* msg_p)
{
    if(!msg_p){
        TRACE(0,"msg_p is a null pointer in app_bt_cmd_mailbox_free!");
        return -1;
    }
    osStatus status;

    status = osMailFree(app_bt_cmd_mailbox, msg_p);
    if (osOK == status)
        app_bt_cmd_mailbox_cnt--;

    return (int)status;
}

int app_bt_cmd_mailbox_get(APP_BT_CMD_MSG_BLOCK** msg_p)
{
    if(!msg_p){
        TRACE(0,"msg_p is a null pointer in app_bt_cmd_mailbox_get!");
        return -1;
    }

    osEvent evt;
    evt = osMailGet(app_bt_cmd_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (APP_BT_CMD_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

extern "C" void app_bt_cmd_perform_test(const char* bt_cmd, uint32_t cmd_len)
{
    APP_BT_CMD_MSG_BLOCK msg = {{0}};
    uint8_t *cmd_buffer = ((uint8_t *)&msg) + sizeof(msg.msg_body.message_id);
    uint32_t msg_max_extra_length = sizeof(APP_BT_CMD_MSG_BLOCK) - sizeof(msg.msg_body.message_id);

    if (cmd_len == 0)
    {
        return;
    }

    if (cmd_len > msg_max_extra_length)
    {
        cmd_len = msg_max_extra_length;
    }

    msg.msg_body.message_id = 0xff;

    /**
     * Max 19-byte cmd match string, the match can be (1) prefix match
     * or (2) substr match.
     *
     * If the destination command function string is "test_hfp_call_answer":
     *
     * (1) firstly perform prefix match, found the 1st one which matched
     *
     *      "test_hfp" "test_hfp_call" "test_hfp_call_an" (max 16-byte) can
     *      all be matched if previous commands dont have the same string prefix.
     *
     * (2) do substr match, also found the 1st one with matched
     *
     *      "hfp_call", "call_answer", "hfp_call_answer" can all be
     *      matched if previous commands dont have the same substr.
     */

    memcpy(cmd_buffer, bt_cmd, cmd_len);

    app_bt_cmd_mailbox_put(&msg);
}


#ifdef IBRT
typedef void (*app_ibrt_peripheral_cb0)(void);
typedef void (*app_ibrt_peripheral_cb1)(void *);
typedef void (*app_ibrt_peripheral_cb2)(void *, void *);
#if defined(BES_AUTOMATE_TEST) || defined(HAL_TRACE_RX_ENABLE)
#define APP_IBRT_PERIPHERAL_BUF_SIZE 2048
static heap_handle_t app_ibrt_peripheral_heap;
uint8_t app_ibrt_peripheral_buf[APP_IBRT_PERIPHERAL_BUF_SIZE]__attribute__((aligned(4)));
bool app_ibrt_auto_test_started = false;
void app_ibrt_peripheral_heap_init(void)
{
    app_ibrt_auto_test_started = true;
    app_ibrt_peripheral_heap = heap_register(app_ibrt_peripheral_buf, APP_IBRT_PERIPHERAL_BUF_SIZE);
}

void *app_ibrt_peripheral_heap_malloc(uint32_t size)
{
    void *ptr = heap_malloc(app_ibrt_peripheral_heap,size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    return ptr;
}

void *app_ibrt_peripheral_heap_cmalloc(uint32_t size)
{
    void *ptr = heap_malloc(app_ibrt_peripheral_heap,size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    memset(ptr, 0, size);
    return ptr;
}

void *app_ibrt_peripheral_heap_realloc(void *rmem, uint32_t newsize)
{
    void *ptr = heap_realloc(app_ibrt_peripheral_heap, rmem, newsize);
    ASSERT(ptr, "%s rmem:%p size:%d", __func__, rmem,newsize);
    return ptr;
}

void app_ibrt_peripheral_heap_free(void *rmem)
{
    ASSERT(rmem, "%s rmem:%p", __func__, rmem);
    heap_free(app_ibrt_peripheral_heap, rmem);
}
#endif

void app_ibrt_peripheral_auto_test_stop(void)
{
#ifdef BES_AUTOMATE_TEST
    app_ibrt_auto_test_started = false;
#endif
}

void app_ibrt_peripheral_automate_test_handler(uint8_t* cmd_buf, uint32_t cmd_len)
{
#ifdef BES_AUTOMATE_TEST
    AUTO_TEST_CMD_T *test_cmd = (AUTO_TEST_CMD_T *)cmd_buf;
    static uint8_t last_group_code = 0xFF;
    static uint8_t last_operation_code = 0xFF;

    //TRACE(4, "%s group 0x%x op 0x%x times %d len %d", __func__,
                //test_cmd->group_code, test_cmd->opera_code, test_cmd->test_times, test_cmd->param_len);
    //TRACE(2, "last group 0x%x last op 0x%x", last_group_code, last_operation_code);
    if (last_group_code != test_cmd->group_code || last_operation_code != test_cmd->opera_code)
    {
        for (uint8_t i=0; i<test_cmd->test_times; i++)
        {
            last_group_code = test_cmd->group_code;
            last_operation_code = test_cmd->opera_code;
            app_ibrt_ui_automate_test_cmd_handler(test_cmd->group_code, test_cmd->opera_code, test_cmd->param, test_cmd->param_len);
        }
    }
    app_ibrt_peripheral_heap_free(cmd_buf);
#endif
}

extern "C" void app_ibrt_peripheral_automate_test(const char* ibrt_cmd, uint32_t cmd_len)
{
#ifdef BES_AUTOMATE_TEST
    uint16_t crc16_rec = 0;
    uint16_t crc16_result = 0;
    uint8_t *cmd_buf = NULL;
    uint32_t _cmd_data_len = 0;
    uint32_t _cmd_data_min_len = sizeof(AUTO_TEST_CMD_T)+AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
    APP_BT_CMD_MSG_BLOCK msg;

    if (ibrt_cmd && cmd_len>=_cmd_data_min_len && cmd_len<=(_cmd_data_min_len+AUTOMATE_TEST_CMD_PARAM_MAX_LEN))
    {
        _cmd_data_len = cmd_len-AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
        crc16_rec = *(uint16_t *)(&ibrt_cmd[_cmd_data_len]);
        crc16_result = _crc16(crc16_result, (const unsigned char *)ibrt_cmd, _cmd_data_len);
        //DUMP8("0x%x ", ibrt_cmd, cmd_len);
        //TRACE(4, "%s crc16 rec 0x%x result 0x%x buf_len %d", __func__, crc16_rec, crc16_result, cmd_len);
        if (crc16_rec == crc16_result && app_ibrt_auto_test_started)
        {
            app_ibrt_auto_test_inform_cmd_received(ibrt_cmd[0], ibrt_cmd[1]);
            cmd_buf = (uint8_t *)app_ibrt_peripheral_heap_cmalloc(_cmd_data_len);
            memcpy(cmd_buf, ibrt_cmd, _cmd_data_len);
            msg.msg_body.message_id = 0xfe;
            msg.msg_body.message_Param0 = (uint32_t)cmd_buf;
            msg.msg_body.message_Param1 = _cmd_data_len;
            app_bt_cmd_mailbox_put(&msg);
        }
        return;
    }
#endif
}

extern "C" void app_ibrt_peripheral_perform_test(const char* ibrt_cmd, uint32_t cmd_len)
{
    uint8_t* p;
    TRACE(1,"Auto test current receive command: %s\n",ibrt_cmd);
    p = (uint8_t* )strstr((char*)ibrt_cmd,(char*)"=");
    if(p)
    {
#ifdef HAL_TRACE_RX_ENABLE
        uint8_t *cmd_name = NULL;
        uint8_t *cmd_param = NULL;
        uint8_t *cmd_param_buf = NULL;
        uint32_t cmd_len;
        uint32_t cmd_param_len;
        cmd_param = (p+1);
        cmd_param_len = strlen((char*)cmd_param);
        cmd_len = strlen(ibrt_cmd) - cmd_param_len - 1;
        cmd_name = (uint8_t *)app_ibrt_peripheral_heap_cmalloc(cmd_len);
        if(NULL == cmd_name){
            TRACE(0,"alloc memory fail\n");
            return;
        }
        memcpy(cmd_name, ibrt_cmd, cmd_len);
        //TRACE(2,"%s auto test command: %s\n",__func__,cmd_name);
        //TRACE(3,"%s rec parameter: %s, parameter len: %d\n",__func__,cmd_param,cmd_param_len);
        cmd_param_buf = (uint8_t *)app_ibrt_peripheral_heap_malloc(cmd_param_len);
        if(NULL == cmd_param_buf){
            TRACE(0,"alloc memory fail\n");
            return;
        }
        memcpy(cmd_param_buf, cmd_param, cmd_param_len);
        //TRACE(1,"auto test command memory copy parameter: %s\n",cmd_param_buf);
        APP_BT_CMD_MSG_BLOCK msg;
        msg.msg_body.message_id = 0xfd;
        msg.msg_body.message_Param0 = (uint32_t)cmd_name;
        msg.msg_body.message_Param1 = (uint32_t)cmd_param_buf;
        msg.msg_body.message_Param2 = (uint32_t)cmd_param_len;
        app_bt_cmd_mailbox_put(&msg);
#endif
    }
    else
    {
        app_bt_cmd_perform_test(ibrt_cmd, cmd_len);
    }
}

void app_ibrt_peripheral_run0(uint32_t ptr)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 0;
    msg.msg_body.message_ptr = ptr;
    app_bt_cmd_mailbox_put(&msg);
}

void app_ibrt_peripheral_run1(uint32_t ptr, uint32_t param0)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 1;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    app_bt_cmd_mailbox_put(&msg);
}

void app_ibrt_peripheral_run2(uint32_t ptr, uint32_t param0, uint32_t param1)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 2;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    app_bt_cmd_mailbox_put(&msg);
}
#endif // IBRT


void app_bt_cmd_thread(void const *argument)
{
    while (1)
    {
        APP_BT_CMD_MSG_BLOCK *msg_p = NULL;
        if ((!app_bt_cmd_mailbox_get(&msg_p)) && (!argument))
        {
            switch (msg_p->msg_body.message_id)
            {
#ifdef IBRT
                case 0:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb0)(msg_p->msg_body.message_ptr))();
                    }
                    break;
                case 1:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb1)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0);
                    }
                    break;
                case 2:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb2)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0,
                                                                                 (void *)msg_p->msg_body.message_Param1);
                    }
                    break;
#if defined(IBRT_UI) && defined(HAL_TRACE_RX_ENABLE)
                case 0xfd:
                    app_ibrt_raw_ui_test_cmd_handler_with_param((unsigned char*)msg_p->msg_body.message_Param0,
                                                                 (unsigned char*)msg_p->msg_body.message_Param1,
                                                                 (uint32_t)msg_p->msg_body.message_Param2);
                    app_ibrt_peripheral_heap_free((uint8_t *)msg_p->msg_body.message_Param0);
                    app_ibrt_peripheral_heap_free((uint8_t *)msg_p->msg_body.message_Param1);
                    break;
#endif
                case 0xfe:
                    app_ibrt_peripheral_automate_test_handler((uint8_t*)msg_p->msg_body.message_Param0,
                                                              (uint32_t)msg_p->msg_body.message_Param1);
                    break;
#endif
                case 0xff: // bt common test
                    {
                        char best_cmd[sizeof(APP_BT_CMD_MSG_BLOCK)+1] = {0}; // cmd prefix match string or substr match string
                        uint32_t cmd_max_length = sizeof(APP_BT_CMD_MSG_BLOCK) - sizeof(msg_p->msg_body.message_id);
                        unsigned int i = 0;
                        memcpy(best_cmd, ((char *)msg_p) + sizeof(msg_p->msg_body.message_id), cmd_max_length);
                        for (; i < sizeof(best_cmd); i += 1)
                        {
                            if (best_cmd[i] == '\r' || best_cmd[i] == '\n')
                            {
                                best_cmd[i] = '\0';
                            }
                        }
                        TRACE(2, "%s process: %s\n", __func__, best_cmd);
                        app_bt_cmd_line_handler(best_cmd, strlen(best_cmd));
                    }
                    break;
                default:
                    break;
            }
            app_bt_cmd_mailbox_free(msg_p);
        }
    }
}

void app_bt_shutdown_test(void)
{
    app_shutdown();
}

void app_bt_flush_nv_test(void)
{
    nv_record_flash_flush();
}

void app_bt_show_device_linkkey_test(void)
{
    TRACE(1, "%s", __func__);
    nv_record_all_ddbrec_print();
}

static bool app_bt_scan_bdaddr_from_string(const char* param, uint32_t len, bt_bdaddr_t *out_addr)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return false;
    }

    TRACE(0,  "%s '%s'", __func__,param);


    if (6 != sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5))
    {
        TRACE(0, "%s parse address failed %s sscanf=%d", __func__, param,sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5));
        return false;
    }

    bt_bdaddr_t addr = {{
        (uint8_t)(bytes[0]&0xff),
        (uint8_t)(bytes[1]&0xff),
        (uint8_t)(bytes[2]&0xff),
        (uint8_t)(bytes[3]&0xff),
        (uint8_t)(bytes[4]&0xff),
        (uint8_t)(bytes[5]&0xff)}};

    *out_addr = addr;

    return true;
}

void app_bt_delete_device_linkkey_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    nv_record_ddbrec_delete(&addr);

    TRACE(0, "nv devices after delete:");
    nv_record_all_ddbrec_print();
}

bool app_bt_is_pts_address(void *remote)
{
    return memcmp(remote, g_app_bt_pts_addr.address, sizeof(g_app_bt_pts_addr)) == 0;
}

void app_bt_pts_set_address(const char* param, uint32 len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    g_app_bt_pts_addr = addr;

#ifndef BLE_ONLY_ENABLED
    btif_register_is_pts_address_check_callback(app_bt_is_pts_address);
#endif
}

#ifndef BLE_ONLY_ENABLED
void app_bt_set_access_mode_test(const char* param, uint32 len)
{
    int mode = 0;

    if (!param || !len)
    {
        return;
    }

    if (1 != sscanf(param, "%d", &mode))
    {
        TRACE(2, "%s invalid param %s", __func__, param);
        return;
    }

    app_bt_set_access_mode((btif_accessible_mode_t)mode);
}

void app_bt_access_mode_set_test(const char* param, uint32 len)
{
    int mode = 0;

    if (!param || !len)
    {
        return;
    }

    if (1 != sscanf(param, "%d", &mode))
    {
        TRACE(2, "%s invalid param %s", __func__, param);
        return;
    }

    app_bt_set_access_mode((btif_accessible_mode_t)mode);
}

void app_bt_start_both_scan_test(void)
{
    app_bt_set_access_mode(BTIF_BAM_GENERAL_ACCESSIBLE);
}

void app_bt_stop_both_scan_test(void)
{
    app_bt_set_access_mode(BTIF_BAM_NOT_ACCESSIBLE);
}

void app_bt_pts_create_hf_channel(void)
{
    app_bt_reconnect_hfp_profile(app_bt_get_pts_address());
}

void app_bt_pts_create_av_channel(void)
{
    btif_pts_av_create_channel(app_bt_get_pts_address());
}

void app_bt_pts_create_ar_channel(void)
{
    app_pts_ar_connect(app_bt_get_pts_address());
}

void app_bt_enable_tone_intrrupt_a2dp(void)
{
    app_bt_manager.config.a2dp_prompt_play_only_when_avrcp_play_received = false;
}

void app_bt_disable_tone_intrrupt_a2dp(void)
{
    app_bt_manager.config.a2dp_prompt_play_only_when_avrcp_play_received = true;
}

void app_bt_enable_a2dp_delay_prompt(void)
{
    app_bt_manager.config.a2dp_delay_prompt_play = true;
}

void app_bt_disable_a2dp_delay_prompt(void)
{
    app_bt_manager.config.a2dp_delay_prompt_play = false;
}

void app_bt_disable_a2dp_aac_codec_test(void)
{
    app_bt_a2dp_disable_aac_codec(true);
}

void app_bt_disable_a2dp_vendor_codec_test(void)
{
    app_bt_a2dp_disable_vendor_codec(true);
}
#endif /* BLE_ONLY_ENABLED */

#if defined(BT_SOURCE)
void app_bt_a2dp_source_pts_enable(bool en)
{
    besbt_cfg.a2dp_source_pts_test = en;
}

bool app_bt_a2dp_source_pts_is_enabled(void)
{
    return besbt_cfg.a2dp_source_pts_test;
}

void app_bt_source_start_search_test(void)
{
    app_bt_stop_inquiry();
    app_bt_source_search_device();
}

void app_bt_source_stop_search_test(void)
{
    app_bt_stop_inquiry();
}

void app_bts_start_search_test(void)
{
    app_bt_stop_inquiry();
    bts_a2dp_source_search();
}

void app_bts_stop_search_test(void)
{
    //sbts_a2dp_source_stop_search();
}

void app_bts_slow_search_test(void)
{
    besbt_cfg.force_normal_search = false;
    besbt_cfg.watch_is_sending_spp = true;
    app_bts_start_search_test();
    besbt_cfg.watch_is_sending_spp = false;
}

void app_bts_force_normal_search_test(void)
{
    besbt_cfg.watch_is_sending_spp = false;
    besbt_cfg.force_normal_search = true;
    app_bts_start_search_test();
    besbt_cfg.force_normal_search = false;
}

extern "C" void bts_boost_check(uint16_t len);

void app_bts_boost_up_test(void)
{
    bts_boost_check(900);
}

void app_bt_set_curr_nv_source(void)
{
    uint8_t device_id = app_bt_find_connected_device();
    struct BT_DEVICE_T *curr_device = NULL;
    btif_device_record_t record;

    if (device_id != BT_DEVICE_INVALID_ID)
    {
        curr_device = app_bt_get_device(device_id);
        if (ddbif_find_record(&curr_device->remote, &record) == BT_STS_SUCCESS)
        {
            ddbif_delete_record(&record.bdAddr);
            record.for_bt_source = true;
            ddbif_add_record(&record);
            nv_record_flash_flush();
        }
        else
        {
            TRACE(1, "%s no record", __func__);
        }
    }
    else
    {
        TRACE(1, "%s device not found", __func__);
    }
}

void app_bt_clear_curr_nv_source(void)
{
    uint8_t device_id = app_bt_find_connected_device();
    struct BT_DEVICE_T *curr_device = NULL;
    btif_device_record_t record;

    if (device_id != BT_DEVICE_INVALID_ID)
    {
        curr_device = app_bt_get_device(device_id);
        if (ddbif_find_record(&curr_device->remote, &record) == BT_STS_SUCCESS)
        {
            ddbif_delete_record(&record.bdAddr);
            record.for_bt_source = false;
            ddbif_add_record(&record);
            nv_record_flash_flush();
        }
        else
        {
            TRACE(1, "%s no record", __func__);
        }
    }
    else
    {
        TRACE(1, "%s device not found", __func__);
    }
}

static bool a2dp_source_inited = false;
typedef int wal_status_t;
extern "C" void bts_a2dp_source_test_start_cmd(char* param);
extern "C" wal_status_t mock_music_init(void);
extern "C" wal_status_t mock_music_earphone_start(void);
extern "C" wal_status_t mock_music_start(void);

void app_bt_start_a2dp_source_test(void)
{
    if (!a2dp_source_inited)
    {
        //sbts_a2dp_source_test_start_cmd(NULL);
        //mock_music_init();
        //a2dp_source_inited = true;
    }
}



void app_bt_source_music_test(void)
{
    app_bt_start_a2dp_source_test();

    mock_music_earphone_start();
}

void app_bt_local_music_test(void)
{
    app_bt_start_a2dp_source_test();

    mock_music_start();
}

void app_bt_connect_earbud_link_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_start_a2dp_source_test();

    bt_source_perform_profile_reconnect(&addr);
}

void app_bt_connect_earbud_a2dp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_start_a2dp_source_test();

    bt_source_reconnect_a2dp_profile(&addr);
}

void app_bt_connect_earbud_avrcp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    bt_source_reconnect_avrcp_profile(&addr);
}

void app_bt_connect_earbud_hfp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    bt_source_reconnect_hfp_profile(&addr);
}

void app_bt_connect_mobile_link_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_reconnect_a2dp_profile(&addr);

    app_bt_reconnect_hfp_profile(&addr);
}

void app_bt_connect_mobile_a2dp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_reconnect_a2dp_profile(&addr);
}

void app_bt_connect_mobile_avrcp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_reconnect_avrcp_profile(&addr);
}

void app_bt_connect_mobile_hfp_test(const char* param, uint32_t len)
{
    bt_bdaddr_t addr;

    if (!app_bt_scan_bdaddr_from_string(param, len, &addr))
    {
        return;
    }

    TRACE(2, "%s %02x:%02x:%02x:%02x:%02x:%02x", __func__,
            addr.address[0], addr.address[1], addr.address[2],
            addr.address[3], addr.address[4], addr.address[5]);

    app_bt_reconnect_hfp_profile(&addr);
}

#if defined(mHDT_SUPPORT)
static bool app_bt_scan_tx_rx_rate_from_string(const char* param, uint32_t len, uint8_t *tx_rate, uint8_t *rx_rate)
{
    //"tx:4rx:4"; at least 8 tyte
    if (len < 8)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return false;
    }

    int tx_rate_temp;
    int rx_rate_temp;

    TRACE(0,  "%s '%s'", __func__,param);

    if (2 != sscanf(param, "tx:%d rx:%x",&tx_rate_temp, &rx_rate_temp))
    {
        TRACE(0, "%s parse rate failed %s", __func__, param);
        return false;
    }
    *tx_rate = (uint8_t)tx_rate_temp;
    *rx_rate = (uint8_t)rx_rate_temp;

    return true;
}

void app_bt_source_enter_mhdt_mode_test(const char* param, uint32_t len)
{
    uint8 tx_rates;
    uint8 rx_rates;
    if (!app_bt_scan_tx_rx_rate_from_string(param, len, &tx_rates, &rx_rates))
    {
        return;
    }
    app_a2dp_source_enter_mhdt_mode(tx_rates, rx_rates);
}

void app_bt_source_exit_mhdt_mode_test(const char* param, uint32_t len)
{
    TRACE(0,  "%s '%s'", __func__,param);
    app_a2dp_source_exit_mhdt_mode();
}

#endif

void app_bt_source_disconnect_link(void)
{
    app_bt_source_disconnect_all_connections(true);
}

extern void a2dp_source_pts_send_sbc_packet(void);
void app_bt_pts_source_send_sbc_packet(void)
{
    a2dp_source_pts_send_sbc_packet();
}

void app_bt_source_reconfig_codec_to_sbc(void)
{
    struct BT_SOURCE_DEVICE_T *source_device = app_bt_source_get_device(BT_SOURCE_DEVICE_ID_1);
    app_bt_a2dp_reconfig_to_sbc(source_device->base_device->a2dp_connected_stream);
}

void app_bt_pts_source_set_get_all_cap_flag(void)
{
    app_bt_source_set_source_pts_get_all_cap_flag(true);
}

void app_bt_pts_source_set_suspend_err_flag(void)
{
    app_bt_source_set_source_pts_suspend_err_flag(true);
}

void app_bt_pts_source_set_unknown_cmd_flag(void)
{
    app_bt_source_set_source_pts_unknown_cmd_flag(true);
}
#endif

#if defined(BT_HFP_AG_ROLE)
void app_bt_pts_hf_ag_set_connectable_state(void)
{
    app_bt_source_set_connectable_state(true);
}

void app_bt_pts_create_hf_ag_channel(void)
{
    //app_bt_source_set_connectable_state(true);
    bt_source_reconnect_hfp_profile(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_create_audio_link(void)
{
    bt_source_create_audio_link(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_disc_audio_link(void)
{
    bt_source_disc_audio_link(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_send_mobile_signal_level(void)
{
    btif_ag_send_mobile_signal_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_send_mobile_signal_level_0(void)
{
    btif_ag_send_mobile_signal_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_service_status(void)
{
    btif_ag_send_service_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_send_service_status_0(void)
{
    btif_ag_send_service_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_call_active_status(void)
{
    btif_ag_send_call_active_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_hangup_call(void)
{
    btif_ag_send_call_active_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_callsetup_status(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,1);
}

void app_bt_pts_hf_ag_send_callsetup_status_0(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_callsetup_status_2(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,2);
}

void app_bt_pts_hf_ag_send_callsetup_status_3(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_enable_roam(void)
{
    btif_ag_send_mobile_roam_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_disable_roam(void)
{
    btif_ag_send_mobile_roam_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_mobile_battery_level(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_send_full_battery_level(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,5);
}

void app_bt_pts_hf_ag_send_battery_level_0(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_calling_ring(void)
{
    //const char* number = NULL;
    char number[] = "1234567";
    btif_ag_send_calling_ring(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,number);
}

void app_bt_pts_hf_ag_enable_inband_ring_tone(void)
{
    btif_ag_set_inband_ring_tone(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_place_a_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status();
    app_bt_pts_hf_ag_send_calling_ring();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_create_audio_link();
}

void app_bt_pts_hf_ag_ongoing_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_2();
    app_bt_pts_hf_ag_send_callsetup_status_3();
}

void app_bt_pts_hf_ag_ongoing_call_setup(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_2();
    app_bt_pts_hf_ag_create_audio_link();
    osDelay(100);
    app_bt_pts_hf_ag_send_callsetup_status_3();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
}

void app_bt_pts_hf_ag_answer_incoming_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_create_audio_link();
}

void app_bt_pts_hf_ag_clear_last_dial_number(void)
{
    btif_ag_set_last_dial_number(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_call_waiting_notification(void)
{
    char number[] = "7654321";
    btif_ag_send_call_waiting_notification(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,number);
}

void app_bt_pts_hf_ag_send_callheld_status(void)
{
    btif_ag_send_callheld_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,1);
}

void app_bt_pts_hf_ag_send_callheld_status_0(void)
{
    btif_ag_send_callheld_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_status_3_0_4_1(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_send_callheld_status();
}

void app_bt_pts_hf_ag_set_pts_enable(void)
{
    app_bt_source_set_hfp_ag_pts_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecs_01(void)
{
    app_bt_source_set_hfp_ag_pts_esc_01_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecs_02(void)
{
    app_bt_source_set_hfp_ag_pts_esc_02_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecc(void)
{
    app_bt_source_set_hfp_ag_pts_ecc_enable(true);
}
#endif

#ifdef IBRT_V2_MULTIPOINT
void app_ibrt_set_access_mode_test(void)
{
    app_bt_set_access_mode(IBRT_BAM_GENERAL_ACCESSIBLE);
}

void app_ibrt_get_pnp_info_test(void)
{
#if defined(BT_DIP_SUPPORT)
    ibrt_if_pnp_info* pnp_info = NULL;
    struct BT_DEVICE_T *curr_device = NULL;

    curr_device = app_bt_get_device(BT_DEVICE_ID_1);

    if (curr_device)
    {
        pnp_info = app_ibrt_if_get_pnp_info(&curr_device->remote);
    }

    if (pnp_info)
    {
        TRACE(4, "%s vendor_id %04x product_id %04x product_version %04x",
                __func__, pnp_info->vend_id, pnp_info->prod_id, pnp_info->prod_ver);
    }
    else
    {
        TRACE(1, "%s N/A", __func__);
    }
#endif
}

void app_ibrt_send_pause_test(void)
{
    app_ibrt_if_a2dp_send_pause(BT_DEVICE_ID_1);
}

void app_ibrt_send_play_test(void)
{
    app_ibrt_if_a2dp_send_play(BT_DEVICE_ID_1);
}

void app_ibrt_send_forward_test(void)
{
    app_ibrt_if_a2dp_send_forward(BT_DEVICE_ID_1);
}

void app_ibrt_send_backward_test(void)
{
    app_ibrt_if_a2dp_send_backward(BT_DEVICE_ID_1);
}

void app_ibrt_send_volume_up_test(void)
{
    app_ibrt_if_a2dp_send_volume_up(BT_DEVICE_ID_1);
}

void app_ibrt_send_volume_down_test(void)
{
    app_ibrt_if_a2dp_send_volume_down(BT_DEVICE_ID_1);
}

void app_ibrt_send_set_abs_volume_test(void)
{
    static uint8_t abs_vol = 80;
    app_ibrt_if_a2dp_send_set_abs_volume(BT_DEVICE_ID_1, abs_vol++);
}

void app_ibrt_adjust_local_volume_up_test(void)
{
    app_ibrt_if_set_local_volume_up();
}

void app_ibrt_adjust_local_volume_down_test(void)
{
    app_ibrt_if_set_local_volume_down();
}

void app_ibrt_hf_call_redial_test(void)
{
    app_ibrt_if_hf_call_redial(BT_DEVICE_ID_1);
}

void app_ibrt_hf_call_answer_test(void)
{
    app_ibrt_if_hf_call_answer(BT_DEVICE_ID_1);
}

void app_ibrt_hf_call_hangup_test(void)
{
    app_ibrt_if_hf_call_hangup(BT_DEVICE_ID_1);
}

void app_ibrt_a2dp_profile_conn_test(void)
{
    app_ibrt_if_connect_a2dp_profile(BT_DEVICE_ID_1);
}

void app_ibrt_a2dp_profile_disc_test(void)
{
    app_ibrt_if_disconnect_a2dp_profile(BT_DEVICE_ID_1);
}

void app_ibrt_send_tota_data_test(void)
{
#ifdef TOTA_v2
#ifdef IS_TOTA_LOG_PRINT_ENABLED
    app_ibrt_if_tota_printf("TOTA TEST");
#endif
#endif
}

void app_ibrt_connect_tws_test(void)
{
    app_ibrt_conn_tws_connect_request(false, 0);
}

void app_ibrt_connect_mobile_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
        (uint8_t)(bytes[0]&0xff),
        (uint8_t)(bytes[1]&0xff),
        (uint8_t)(bytes[2]&0xff),
        (uint8_t)(bytes[3]&0xff),
        (uint8_t)(bytes[4]&0xff),
        (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_remote_dev_connect_request(&addr, OUTGOING_CONNECTION_REQ, true, 0);
}

void app_ibrt_connect_ibrt_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
            (uint8_t)(bytes[0]&0xff),
            (uint8_t)(bytes[1]&0xff),
            (uint8_t)(bytes[2]&0xff),
            (uint8_t)(bytes[3]&0xff),
            (uint8_t)(bytes[4]&0xff),
            (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_connect_ibrt(&addr);
}

void app_ibrt_role_switch_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
            (uint8_t)(bytes[0]&0xff),
            (uint8_t)(bytes[1]&0xff),
            (uint8_t)(bytes[2]&0xff),
            (uint8_t)(bytes[3]&0xff),
            (uint8_t)(bytes[4]&0xff),
            (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_tws_role_switch(&addr);
}

#endif /* IBRT_UI_V2 */

#ifdef AUDIO_LINEIN
void app_bt_test_linein_start(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                              BT_STREAM_LINEIN,
                              BT_DEVICE_ID_1,
                              0);
}

void app_bt_test_linein_stop(void)
{
    Audio_device_t audio_device;
    audio_device.audio_device.device_id = BT_DEVICE_ID_1;
    audio_device.audio_device.device_type = AUDIO_TYPE_BT;
    audio_device.aud_id = MAX_RECORD_NUM;
    audio_player_playback_stop(BT_STREAM_LINEIN, &audio_device);
}
#endif

#if defined(BT_PAN_SUPPORT)
void app_bt_pts_pan_connect_mobile(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
        (uint8_t)(bytes[0]&0xff),
        (uint8_t)(bytes[1]&0xff),
        (uint8_t)(bytes[2]&0xff),
        (uint8_t)(bytes[3]&0xff),
        (uint8_t)(bytes[4]&0xff),
        (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, sizeof(bt_bdaddr_t));

    app_bt_connect_pan_profile(&addr);
}

void app_bt_pts_pan_create_channel(void)
{
    app_bt_connect_pan_profile(app_bt_get_pts_address());
}

void app_bt_pts_pan_disconnect_channel(void)
{
    app_bt_disconnect_pan_profile(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_dhcp_discover(void)
{
    app_bt_pan_test_send_DHCP_protocol_discover(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_dhcp_request(void)
{
    app_bt_pan_test_send_DHCP_protocol_request(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_arp_probe(void)
{
    app_bt_pan_test_send_ARP_probe(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_arp_request(void)
{
    app_bt_pan_test_send_ARP_protocol_request(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_dns_request(void)
{
    const char domain_name[] = "baidu.com";
    app_bt_pan_test_send_DNS_protocol_request(BT_DEVICE_ID_1, domain_name, sizeof(domain_name)-1);
}

void app_bt_pts_pan_send_icmp_echo_request(void)
{
    app_bt_pan_test_send_ICMP_echo_request(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_ndp_request(void)
{
    app_bt_pan_test_send_NDP_request(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_http_request(void)
{
    app_bt_pan_test_send_HTTP_request(BT_DEVICE_ID_1);
}

void app_bt_pts_pan_send_http_ipv6_request(void)
{
    app_bt_pan_test_send_HTTP_ipv6_request(BT_DEVICE_ID_1);
}

void app_bt_pan_test_enable_web_test(void);

void app_bt_pan_test_disable_web_test(void);

#endif

#ifdef BT_USE_COHEAP_ALLOC
void app_bt_coheap_statistics_dump(void)
{
    btif_coheap_dump_statistics();
}
void app_bt_coheap_enable_debug(void)
{
    btif_coheap_enable_debug(true);
}
void app_bt_coheap_disable_debug(void)
{
    btif_coheap_enable_debug(false);
}
#endif

#if defined(IBRT_V2_MULTIPOINT) && defined(TOTA_v2)
void tota_v2_send_data(void)
{
    if (bt_defer_curr_func_0(tota_v2_send_data))
    {
        TRACE(0, "tota_v2_send_data: queue to bt thread");
        return;
    }
    struct BT_DEVICE_T *curr_device = NULL;
    curr_device = app_bt_get_connected_device_byaddr(app_bt_get_pts_address());
    if (curr_device == NULL)
    {
        TRACE(0, "tota_send_data: device not connected");
        return;
    }
    uint8_t *data = (uint8_t *)curr_device;
    TRACE(0, "tota_v2_send_data: %p", data);
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
}
void tota_v2_more_data(void)
{
    if (bt_defer_curr_func_0(tota_v2_more_data))
    {
        TRACE(0, "tota_v2_more_data: queue to bt thread");
        return;
    }
    struct BT_DEVICE_T *curr_device = NULL;
    curr_device = app_bt_get_connected_device_byaddr(app_bt_get_pts_address());
    if (curr_device == NULL)
    {
        TRACE(0, "tota_v2_more_data: device not connected");
        return;
    }
    uint8_t *data = (uint8_t *)curr_device;
    TRACE(0, "tota_v2_more_data: 1st packet");
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
    TRACE(0, "tota_v2_more_data: 2nd packet");
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
    TRACE(0, "tota_v2_more_data: 3rd packet");
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
    TRACE(0, "tota_v2_more_data: 4th packet");
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
    TRACE(0, "tota_v2_more_data: 5th packet");
    app_ibrt_if_start_user_action_v2(curr_device->device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)data, 128);
}
#endif

extern void app_otaMode_enter(APP_KEY_STATUS *status, void *param);

static void app_ota_mode_enter_test(void)
{
    app_otaMode_enter(NULL, NULL);
}

#ifdef APP_CHIP_BRIDGE_MODULE
void app_test_uart_relay(void)
{
    char test_str[] = "pong";
    //app_uart_send_data((uint8_t *)test_str, strlen(test_str));
    app_chip_bridge_send_cmd(SET_TEST_MODE_CMD, (uint8_t *)test_str, strlen(test_str));
}
#endif

static void app_enable_hci_cmd_evt_debug(void)
{
    btif_hci_enable_cmd_evt_debug(true);
}

static void app_disable_hci_cmd_evt_debug(void)
{
    btif_hci_enable_cmd_evt_debug(false);
}

static void app_enable_hci_tx_flow_debug(void)
{
    btif_hci_enable_tx_flow_debug(true);
}

static void app_disable_hci_tx_flow_debug(void)
{
    btif_hci_enable_tx_flow_debug(false);
}

static const app_bt_cmd_handle_t app_bt_common_test_handle[]=
{
#ifdef APP_CHIP_BRIDGE_MODULE
    {"app_test_uart_relay", app_test_uart_relay},
#endif
    {"bt_shutdown",             app_bt_shutdown_test},
    {"bt_flush_nv",             app_bt_flush_nv_test},
    {"bt_show_link_key",        app_bt_show_device_linkkey_test},
    {"hci_check_state",         btif_hci_print_statistic},
    {"hci_en_cmd_evt_debug",    app_enable_hci_cmd_evt_debug},
    {"hci_de_cmd_evt_debug",    app_disable_hci_cmd_evt_debug},
    {"hci_en_tx_flow_debug",    app_enable_hci_tx_flow_debug},
    {"hci_de_tx_flow_debug",    app_disable_hci_tx_flow_debug},
    {"ota_mode_enter",          app_ota_mode_enter_test},
#ifndef BLE_ONLY_ENABLED
    {"bt_key_click",            bt_key_handle_func_click},
    {"bt_key_double_click",     bt_key_handle_func_doubleclick},
    {"bt_key_long_click",       bt_key_handle_func_longpress},
    {"bt_start_both_scan",      app_bt_start_both_scan_test},
    {"bt_stop_both_scan",       app_bt_stop_both_scan_test},
    {"bt_disc_acl_link",        app_bt_disconnect_acl_link},
    {"rfc_register",            btif_pts_rfc_register_channel},
    {"rfc_close",               btif_pts_rfc_close},
#endif
#if defined(BT_HFP_TEST_SUPPORT)
    {"hf_create_service_link",  app_bt_pts_create_hf_channel},
    {"hf_disc_service_link",    app_pts_hf_disc_service_link},
    {"hf_create_audio_link",    app_pts_hf_create_audio_link},
    {"hf_disc_audio_link",      app_pts_hf_disc_audio_link},
    {"hf_send_key_pressed",     app_pts_hf_send_key_pressed},
    {"hf_answer_call",          app_pts_hf_answer_call},
    {"hf_hangup_call",          app_pts_hf_hangup_call},
    {"hf_dial_number",          app_pts_hf_dial_number},
    {"hf_dial_number_memory_index",app_pts_hf_dial_number_memory_index},
    {"hf_dial_number_invalid_index",app_pts_hf_dial_number_invalid_memory_index},
    {"hf_redial_call",          app_pts_hf_redial_call},
    {"hf_release_call",         app_pts_hf_release_active_call},
    {"hf_release_active_call_2",app_pts_hf_release_active_call_2},
    {"hf_hold_call",            app_pts_hf_hold_active_call},
    {"hf_hold_active_call_2",   app_pts_hf_hold_active_call_2},
    {"hf_hold_transfer",        app_pts_hf_hold_call_transfer},
    {"hf_vr_enable",            app_pts_hf_vr_enable},
    {"hf_vr_disable",           app_pts_hf_vr_disable},
    {"hf_list_current_calls",   app_pts_hf_list_current_calls},
    {"hf_report_mic_volume",    app_pts_hf_report_mic_volume},
    {"hf_attach_voice_tag",     app_pts_hf_attach_voice_tag},
    {"hf_update_ind_value",     app_pts_hf_update_ind_value},
    {"hf_ind_activation",       app_pts_hf_ind_activation},
    {"hf_acs_bv_09_i_enable",   app_pts_hf_acs_bv_09_i_set_enable},
    {"hf_acs_bv_09_i_disable",  app_pts_hf_acs_bv_09_i_set_disable},
    {"hf_acs_bi_13_i_enable",   app_pts_hf_acs_bi_13_i_set_enable},
    {"hf_acs_bi_13_i_disable",  app_pts_hf_acs_bi_13_i_set_disable},
#endif /* BT_HFP_TEST_SUPPORT */
#if defined(BT_A2DP_TEST_SUPPORT)
    {"av_create_channel",       app_bt_pts_create_av_channel},
    {"av_disc_channel",         app_pts_av_disc_channel},
    {"av_en_tone_interrupt",    app_bt_enable_tone_intrrupt_a2dp},
    {"av_de_tone_interrupt",    app_bt_disable_tone_intrrupt_a2dp},
    {"av_en_delay_prompt",      app_bt_enable_a2dp_delay_prompt},
    {"av_de_delay_prompt",      app_bt_disable_a2dp_delay_prompt},
    {"av_de_aac_codec",         app_bt_disable_a2dp_aac_codec_test},
    {"av_de_vnd_codec",         app_bt_disable_a2dp_vendor_codec_test},
    {"av_create_media_channel", btif_pts_av_create_media_channel},
    {"av_close_channel",        app_pts_av_close_channel},
    {"av_send_getconf",         btif_pts_av_send_getconf},
    {"av_send_reconf",          btif_pts_av_send_reconf},
    {"av_send_open",            btif_pts_av_send_open},
#endif /* BT_A2DP_TEST_SUPPORT */
#if defined(BT_AVRCP_TEST_SUPPORT)
    {"ar_connect",              app_bt_pts_create_ar_channel},
    {"ar_disconnect",           app_pts_ar_disconnect},
    {"ar_panel_play",           app_pts_ar_panel_play},
    {"ar_panel_pause",          app_pts_ar_panel_pause},
    {"ar_panel_stop",           app_pts_ar_panel_stop},
    {"ar_panel_forward",        app_pts_ar_panel_forward},
    {"ar_panel_backward",       app_pts_ar_panel_backward},
    {"ar_volume_up",            app_pts_ar_volume_up},
    {"ar_volume_down",          app_pts_ar_volume_down},
    {"ar_volume_notify",        app_pts_ar_volume_notify},
    {"ar_volume_change",        app_pts_ar_volume_change},
    {"ar_set_absolute_volume",  app_pts_ar_set_absolute_volume},
#endif /* BT_AVRCP_TEST_SUPPORT */

#if defined(BT_USE_COHEAP_ALLOC)
    {"coheap_dump",             app_bt_coheap_statistics_dump},
    {"coheap_enable_debug",     app_bt_coheap_enable_debug},
    {"coheap_disable_debug",    app_bt_coheap_disable_debug},
#endif

#if defined(NORMAL_TEST_MODE_SWITCH)
    {"enter_signal_testmode",   app_enter_signal_testmode},
    {"exit_signal_testmode",    app_exit_signal_testmode},
    {"enter_nosignal_tx_testmode",app_enter_nosignal_tx_testmode},
    {"enter_nosignal_rx_testmode",app_enter_nosignal_rx_testmode},
    {"exit_nosignal_trx_testmode",app_exit_nosignal_trx_testmode},
    {"enter_ble_tx_v1",         app_enter_ble_tx_v1_testmode},
    {"enter_ble_rx_v1",         app_enter_ble_rx_v1_testmode},
    {"enter_ble_tx_v2",         app_enter_ble_tx_v2_testmode},
    {"enter_ble_rx_v2",         app_enter_ble_rx_v2_testmode},
    {"enter_ble_tx_v3",         app_enter_ble_tx_v3_testmode},
    {"enter_ble_rx_v3",         app_enter_ble_rx_v3_testmode},
    {"enter_ble_tx_v4",         app_enter_ble_tx_v4_testmode},
    {"exit_ble_trx",            app_exit_ble_trx_testmode},
    {"enter_normal_mode",       app_enter_normal_mode},
#endif
#if defined(BT_HID_DEVICE)
    {"hid_enter",               app_bt_hid_enter_shutter_mode},
    {"hid_exit",                app_bt_hid_exit_shutter_mode},
    {"hid_capture",             app_bt_hid_send_capture},
#endif
#if defined(BT_SOURCE)
/*
    {"bt_start_search",         app_bt_source_start_search_test},
    {"bt_stop_search",          app_bt_source_stop_search_test},
    {"bts_start_search",        app_bts_start_search_test},
    {"bts_stop_search",         app_bts_stop_search_test},
    {"bts_slow_search",         app_bts_slow_search_test},
    {"bts_normal_search",       app_bts_force_normal_search_test},
    {"bts_boost_up_test",       app_bts_boost_up_test},
    {"bt_set_nv_source",        app_bt_set_curr_nv_source},
    {"bt_clear_nv_source",      app_bt_clear_curr_nv_source},
    {"bt_disc_earbud_link",     app_bt_disconnect_earbud_link},
    {"bt_disc_earbud_a2dp",     app_bt_disconnect_earbud_a2dp},
    {"bt_disc_earbud_avrcp",    app_bt_disconnect_earbud_avrcp},
    {"bt_disc_earbud_hfp",      app_bt_disconnect_earbud_hfp},
    {"bt_disc_mobile_link",     app_bt_disconnect_mobile_link},
    {"bt_disc_mobile_a2dp",     app_bt_disconnect_mobile_a2dp},
    {"bt_disc_mobile_avrcp",    app_bt_disconnect_mobile_avrcp},
    {"bt_disc_mobile_hfp",      app_bt_disconnect_mobile_hfp},
    {"bt_start_stream",         app_bt_source_start_stream},
    {"bt_suspend_stream",       app_bt_source_suspend_stream},
    {"bt_toggle_stream",        app_bt_source_toggle_stream},
    {"bt_source_music",         app_bt_source_music_test},
    {"bt_local_music",          app_bt_local_music_test},
    {"av_source_connect",       app_bt_pts_create_av_source_channel},
    {"avrcp_tg_test_start",     app_bt_pts_avrcp_tg_start_test},
    {"avrcp_tg_set_no_track",   app_bt_pts_avrcp_tg_set_no_track},
    {"avrcp_tg_set_large_rsp",  app_bt_pts_avrcp_tg_set_larget_rsp},
    {"avrcp_tg_track_change",   app_bt_pts_avrcp_tg_track_change_test},
*/
    {"source_disc_link",        app_bt_source_disconnect_link},
    {"reconfig_to_sbc",         app_bt_source_reconfig_codec_to_sbc},
    {"source_send_sbc_pkt",     app_bt_pts_source_send_sbc_packet},
    {"source_create_media_chnl",btif_pts_source_cretae_media_channel},
    {"source_send_close_cmd",   btif_pts_source_send_close_cmd},
    {"source_send_get_config_cmd",btif_pts_source_send_get_configuration_cmd},
    {"source_send_reconfigure_cmd",btif_pts_source_send_reconfigure_cmd},
    {"source_send_abort_cmd",   btif_pts_source_send_abort_cmd},
    {"source_send_suspend_cmd", btif_pts_source_send_suspend_cmd},
    {"source_set_get_all_cap_flag",app_bt_pts_source_set_get_all_cap_flag},
    {"source_set_suspend_err_flag",app_bt_pts_source_set_suspend_err_flag},
    {"source_set_unknown_cmd_flag",app_bt_pts_source_set_unknown_cmd_flag},
    {"source_send_start_cmd",   btif_pts_source_send_start_cmd},
#endif
#if defined(BT_HFP_TEST_SUPPORT) && defined(BT_HFP_AG_ROLE)
    {"ag_set_connect_state",    app_bt_pts_hf_ag_set_connectable_state},
    {"ag_create_service_link",  app_bt_pts_create_hf_ag_channel},
    {"ag_create_audio_link",    app_bt_pts_hf_ag_create_audio_link},
    {"ag_disc_audio_link",      app_bt_pts_hf_ag_disc_audio_link},
    {"ag_send_mobile_signal",   app_bt_pts_hf_ag_send_mobile_signal_level},
    {"ag_send_mobile_signal_0", app_bt_pts_hf_ag_send_mobile_signal_level_0},
    {"ag_send_service_status",  app_bt_pts_hf_ag_send_service_status},
    {"ag_send_service_status_0",app_bt_pts_hf_ag_send_service_status_0},
    {"ag_send_callsetup_status",app_bt_pts_hf_ag_send_callsetup_status},
    {"ag_send_callsetup_status_0",app_bt_pts_hf_ag_send_callsetup_status_0},
    {"ag_send_callactive_status",app_bt_pts_hf_ag_send_call_active_status},
    {"ag_send_hangup_call",     app_bt_pts_hf_ag_hangup_call},
    {"ag_send_enable_roam",     app_bt_pts_hf_ag_enable_roam},
    {"ag_send_disable_roam",    app_bt_pts_hf_ag_disable_roam},
    {"ag_send_batt_level",      app_bt_pts_hf_ag_send_mobile_battery_level},
    {"ag_send_full_batt_level", app_bt_pts_hf_ag_send_full_battery_level},
    {"ag_send_batt_level_0",    app_bt_pts_hf_ag_send_battery_level_0},
    {"ag_send_calling_ring",    app_bt_pts_hf_ag_send_calling_ring},
    {"ag_enable_inband_ring",   app_bt_pts_hf_ag_enable_inband_ring_tone},
    {"ag_place_a_call",         app_bt_pts_hf_ag_place_a_call},
    {"ag_ongoing_call",         app_bt_pts_hf_ag_ongoing_call},
    {"ag_ongoing_call_setup",   app_bt_pts_hf_ag_ongoing_call_setup},
    {"ag_clear_dial_num",       app_bt_pts_hf_ag_clear_last_dial_number},
    {"ag_send_ccwa",            app_bt_pts_hf_ag_send_call_waiting_notification},
    {"ag_send_callheld_status", app_bt_pts_hf_ag_send_callheld_status},
    {"ag_send_callheld_status_0",app_bt_pts_hf_ag_send_callheld_status_0},
    {"ag_send_status_3_0_4_1",  app_bt_pts_hf_ag_send_status_3_0_4_1},
    {"ag_set_pts_enable",       app_bt_pts_hf_ag_set_pts_enable},
    {"ag_answer_incoming_call", app_bt_pts_hf_ag_answer_incoming_call},
    {"ag_set_pts_ecs_01",       app_bt_pts_hf_ag_set_pts_ecs_01},
    {"ag_set_pts_ecs_02",       app_bt_pts_hf_ag_set_pts_ecs_02},
    {"ag_set_pts_ecc",          app_bt_pts_hf_ag_set_pts_ecc},
#endif
#if defined(BT_MAP_TEST_SUPPORT)
    {"map_connect",             app_bt_pts_map_connect},
    {"map_disconnect",          app_bt_pts_map_disconnect},
    {"map_set_over_rfcomm",     bt_map_set_obex_over_rfcomm},
    {"map_clear_over_rfcomm",   bt_map_clear_obex_over_rfcomm},
    {"map_open_all_mas",        app_bt_pts_map_open_all_mas_channel},
    {"map_mns_obex_disc",       app_bt_pts_map_mns_obex_disc_req},
    {"map_close_mns",           app_bt_pts_map_close_mns_channel},
    {"map_client_test",         app_bt_pts_map_client_test},
    {"map_send_sms",            app_bt_pts_map_send_sms},
#endif
#if defined(BT_PBAP_TEST_SUPPORT)
    {"pb_connect",              app_bt_pts_pbap_create_channel},
    {"pb_disconnect",           app_bt_pts_pbap_disconnect_channel},
    {"pb_obex_disc",            app_bt_pts_pbap_send_obex_disc_req},
    {"pb_get_req",              app_bt_pts_pbap_send_obex_get_req},
    {"pb_auth_req",             app_bt_pts_pbap_send_auth_conn_req},
    {"pb_abort",                app_bt_pts_pbap_send_abort},
    {"pb_client_test",          app_bt_pbap_client_test},
    {"pb_pull_pb",              app_bt_pts_pbap_pull_phonebook_pb},
    {"pb_pull_size",            app_bt_pts_pbap_get_phonebook_size},
    {"pb_to_root",              app_bt_pts_pbap_set_path_to_root},
    {"pb_to_parent",            app_bt_pts_pbap_set_path_to_parent},
    {"pb_to_telecom",           app_bt_pts_pbap_enter_path_to_telecom},
    {"pb_list_pb",              app_bt_pts_pbap_list_phonebook_pb},
    {"pb_list_size",            app_bt_pts_pbap_list_phonebook_pb_size},
    {"pb_entry_n_tel",          app_bt_pts_pbap_pull_pb_entry_n_tel},
    {"pb_entry_uid",            app_bt_pts_pbap_pull_uid_entry},
#endif
#if defined(BT_OPP_SUPPORT)
    {"opp_connect",             app_bt_pts_opp_create_channel},
    {"op_disconnect",           app_bt_pts_opp_disconnect_channel},
    {"op_pull_vcard",           app_bt_pts_opp_pull_vcard_object},
    {"op_pull_srm_wait_vcard",  app_bt_pts_opp_srm_wait_pull_vcard_object},
    {"op_push_vcard",           app_bt_pts_opp_push_vcard_object},
    {"op_push_vcard2",           app_bt_pts_opp_push_vcard_object2},
    {"op_push_2vcard_1put",     app_bt_pts_opp_push_two_vcard_on_single_put_operation},
    {"op_push_1kfile",          app_bt_pts_opp_push_1kfile_start_object},
    {"op_push_vcal",            app_bt_pts_opp_push_vcal_object},
    {"op_push_vcal2",           app_bt_pts_opp_push_vcal_object2},
    {"op_push_2vcal_1put",      app_bt_pts_opp_push_two_vcal_on_single_put_operation},
    {"op_push_vmsg",            app_bt_pts_opp_push_vmsg_object},
    {"op_push_vmsg2",           app_bt_pts_opp_push_vmsg_object2},
    {"op_push_2vmsg_1put",      app_bt_pts_opp_push_two_vmsg_on_single_put_operation},
    {"op_push_vnt",             app_bt_pts_opp_push_vnt_object},
    {"op_push_vnt2",            app_bt_pts_opp_push_vnt_object2},
    {"op_push_2vnt_1put",       app_bt_pts_opp_push_two_vnt_on_single_put_operation},
    {"op_push_unsupport_object",app_bt_pts_opp_push_unsupport_object},
    {"op_obex_abort",           app_bt_pts_opp_send_obex_abort_req},
    {"op_obex_disc",            app_bt_pts_opp_send_obex_disc_req},
    {"op_push_both_vcard_1put", app_bt_pts_opp_push_both_vcard_on_once_put},
    {"op_push_both_vcal",       app_bt_pts_opp_push_both_vcal_on_once_object},
    {"op_push_both_vmsg",       app_bt_pts_opp_push_both_vmsg_on_once_object},
    {"op_push_both_vnt",        app_bt_pts_opp_push_both_vnt_on_once_object},
    {"op_exchange_vcard",       app_bt_pts_opp_exchange_vcard_object},
    {"op_pull_reject",          app_bt_pts_opp_send_pull_reject_rsp},
#endif
#if defined(BT_PAN_SUPPORT)
    {"pa_connect",              app_bt_pts_pan_create_channel},
    {"pa_disconnect",           app_bt_pts_pan_disconnect_channel},
    {"pa_dhcp_discover",        app_bt_pts_pan_send_dhcp_discover},
    {"pa_dhcp_request",         app_bt_pts_pan_send_dhcp_request},
    {"pa_arp_probe",            app_bt_pts_pan_send_arp_probe},
    {"pa_arp_request",          app_bt_pts_pan_send_arp_request},
    {"pa_dns_request",          app_bt_pts_pan_send_dns_request},
    {"pa_icmp_echo_req",        app_bt_pts_pan_send_icmp_echo_request},
    {"pa_ndp_request",          app_bt_pts_pan_send_ndp_request},
    {"pa_http_request",         app_bt_pts_pan_send_http_request},
    {"pa_http_ipv6_request",    app_bt_pts_pan_send_http_ipv6_request},
    {"pa_receive_data",         app_bt_pan_test_start_receive_enternet_data},
    {"pa_enable_web_test",      app_bt_pan_test_enable_web_test},
    {"pa_disable_web_test",     app_bt_pan_test_disable_web_test},
    {"pa_access_web",           app_bt_pan_test_web_site_access},
#endif
#ifdef IBRT_V2_MULTIPOINT
    {"ib_set_access_mode",      app_ibrt_set_access_mode_test},
    {"ib_enable_bluetooth",     app_ibrt_if_enable_bluetooth},
    {"ib_disable_bluetooth",    app_ibrt_if_disable_bluetooth},
    {"ib_get_pnp_info",         app_ibrt_get_pnp_info_test},
    {"ib_send_pause",           app_ibrt_send_pause_test},
    {"ib_send_play",            app_ibrt_send_play_test},
    {"ib_send_forward",         app_ibrt_send_forward_test},
    {"ib_send_backward",        app_ibrt_send_backward_test},
    {"ib_send_volumeup",        app_ibrt_send_volume_up_test},
    {"ib_send_volumedn",        app_ibrt_send_volume_down_test},
    {"ib_send_setabsvol",       app_ibrt_send_set_abs_volume_test},
    {"ib_adj_local_volup",      app_ibrt_adjust_local_volume_up_test},
    {"ib_adj_local_voldn",      app_ibrt_adjust_local_volume_down_test},
    {"ib_call_redial",          app_ibrt_hf_call_redial_test},
    {"ib_call_answer",          app_ibrt_hf_call_answer_test},
    {"ib_call_hangup",          app_ibrt_hf_call_hangup_test},
    {"ib_switch_sco",           app_ibrt_if_switch_streaming_sco},
    {"ib_switch_a2dp",          app_ibrt_if_switch_streaming_a2dp},
    {"ib_conn_a2dp",            app_ibrt_a2dp_profile_conn_test},
    {"ib_disc_a2dp",            app_ibrt_a2dp_profile_disc_test},
    {"ib_tota_send",            app_ibrt_send_tota_data_test},
    {"ib_conn_tws",             app_ibrt_connect_tws_test},
   // {"ib_a2dp_recheck",         app_bt_audio_recheck_a2dp_streaming},
#endif
#ifdef AUDIO_LINEIN
    {"linein_start",            app_bt_test_linein_start},
    {"linein_stop",             app_bt_test_linein_stop},
#endif
#if defined(IBRT_V2_MULTIPOINT) && defined(TOTA_v2)
    {"tota_v2_send_data",       tota_v2_send_data},
    {"tota_v2_more_data",       tota_v2_more_data},
#endif
};

static const app_bt_cmd_handle_with_parm_t app_bt_common_test_with_param_handle[]=
{
#ifndef BLE_ONLY_ENABLED
    {"bt_set_access_mode",      app_bt_set_access_mode_test},       // bt_set_access_mode|2
    {"bt_access_mode_set",      app_bt_access_mode_set_test},
#endif
    {"bt_set_pts_address",      app_bt_pts_set_address},            // bt_set_pts_address|ed:b6:f4:dc:1b:00
    {"bt_delete_link_key",      app_bt_delete_device_linkkey_test}, // bt_delete_link_key|ed:b6:f4:dc:1b:00
#if defined(BT_PBAP_TEST_SUPPORT)
    {"pb_to",                   app_bt_pts_pbap_set_path_to},
    {"pb_list",                 app_bt_pts_pbap_list_phonebook},
    {"pb_pull",                 app_bt_pts_pbap_pull_phonebook},
    {"pb_reset",                app_bt_pts_pbap_pull_reset_missed_call},
    {"pb_vcsel",                app_bt_pts_pbap_pull_vcard_select},
    {"pb_list_reset",           app_bt_pts_pbap_list_reset_missed_call},
    {"pb_list_vcsel",           app_bt_pts_pbap_list_vcard_select},
    {"pb_list_vcsch",           app_bt_pts_pbap_list_vcard_search},
#endif
#if defined(BT_MAP_TEST_SUPPORT)
    {"map_obex_disc",           app_bt_pts_map_obex_disc_req},          // map_obex_disc|0:mas_instance_id
    {"map_obex_conn",           app_bt_pts_map_obex_conn_req},          // map_obex_conn|0
    {"map_dont_auto_conn",      app_bt_pts_map_dont_auto_conn_req},     // map_dont_auto_conn|0
    {"map_connect_mas",         app_bt_pts_map_connect_mas},            // map_connect_mas|0
    {"map_open_mas",            app_bt_pts_map_open_mas_channel},       // map_open_mas|0
    {"map_close_mas",           app_bt_pts_map_close_mas_channel},      // map_close_mas|0
    {"map_to_root",             app_bt_pts_map_enter_to_root_folder},   // map_to_root|0
    {"map_to_parent",           app_bt_pts_map_enter_to_parent_folder}, // map_to_parent|0
    {"map_to_msg_folder",       app_bt_pts_map_enter_to_msg_folder},    // map_to_msg_foler|0
    {"map_to_child",            app_bt_pts_map_enter_to_child_folder},  // map_to_child|0|telecom:folder_name
    {"map_send_gsm_sms",        app_bt_pts_map_send_gsm_sms},           // map_send_gsm_sms|0
    {"map_send_cdma_sms",       app_bt_pts_map_send_cdma_sms},          // map_send_cdma_sms|0
    {"map_send_mms",            app_bt_pts_map_send_mms},               // map_send_mms|0
    {"map_send_im",             app_bt_pts_map_send_im},                // map_send_im|0
    {"map_send_email",          app_bt_pts_map_send_email},             // map_send_email|0
    {"map_replace_email",       app_bt_pts_map_replace_email},          // map_replace_email|0|dd:msg_handle
    {"map_forward_email",       app_bt_pts_map_forward_email},          // map_forward_email|0|dd
    {"map_forward_incl_attach", app_bt_pts_map_forward_including_attachment},
    {"map_push_to_convo",       app_bt_pts_map_push_to_conversation},   // map_push_to_convo:0|cc:conversation_id
    {"map_put_start",           app_bt_pts_map_put_start},              // map_put_start|0|30
    {"map_put_cont",            app_bt_pts_map_put_continue},           // map_put_cont|0
    {"map_put_end",             app_bt_pts_map_put_end},                // map_put_end|0
    {"map_get_inst_info",       app_bt_pts_map_get_instance_info},      // map_get_inst_info|0
    {"map_get_object",          app_bt_pts_map_get_object_test},        // map_get_object|0|put.gif
    {"map_update_inbox",        app_bt_pts_map_update_inbox},           // map_update_inbox|0
    {"map_notify_register",     app_bt_pts_map_notify_register},        // map_notify_register|0|1:on_or_off
    {"map_notify_filter",       app_bt_pts_map_notify_filter},          // map_notify_filter|0|1ff:notify_masks
    {"map_set_read",            app_bt_pts_map_set_msg_read},           // map_set_read|0|dd:msg_handle
    {"map_set_unread",          app_bt_pts_map_set_msg_unread},         // map_set_unread|0|dd
    {"map_set_delete",          app_bt_pts_map_set_msg_delete},         // map_set_delete|0|dd
    {"map_set_undelete",        app_bt_pts_map_set_msg_undelete},       // map_set_undelete|0|dd
    {"map_set_extdata",         app_bt_pts_map_set_msg_extdata},        // map_set_extdata|0|dd
    {"map_get_fl",              app_bt_pts_map_get_folder_listing},     // map_get_fl|0
    {"map_get_fl_size",         app_bt_pts_map_get_folder_listing_size},// map_get_fl_size|0
    {"map_srm_wait",            app_bt_pts_map_set_srm_in_wait},        // map_srm_wait|0|1
    {"map_get_msg",             app_bt_pts_map_get_message},            // map_get_msg|0|dd
    {"map_get_ml",              app_bt_pts_map_get_message_listing},    // map_get_ml|0
    {"map_get_ml_size",         app_bt_pts_map_get_msg_listing_size},   // map_get_ml_size|0
    {"map_get_ml_type",         app_bt_pts_map_get_message_listing_of_type},        // map_get_ml_type|0|84
    {"map_get_ml_handle",       app_bt_pts_map_get_message_listing_of_handle},      // map_get_ml_handle|0|dd
    {"map_get_ml_sz_convoid",   app_bt_pts_map_get_msg_listing_size_of_convoid},    // map_get_ml_sz_convoid|0|cc
    {"map_get_ml_convoid",      app_bt_pts_map_get_message_listing_of_convoid},     // map_get_ml_convoid|0|cc
    {"map_get_ml_readstatus",   app_bt_pts_map_get_message_listing_of_readstatus},  // map_get_ml_readstatus|0|0
    {"map_get_ml_priority",     app_bt_pts_map_get_message_listing_of_priority},    // map_get_ml_priority|0|1
    {"map_get_ml_originator",   app_bt_pts_map_get_message_listing_of_originator},  // map_get_ml_originator|0
    {"map_get_ml_recipient",    app_bt_pts_map_get_message_listing_of_recipient},   // map_get_ml_recipient|0
    {"map_get_ml_period_begin", app_bt_pts_map_get_message_listing_of_period_begin},// map_get_ml_period_begin|0
    {"map_get_ml_period_end",   app_bt_pts_map_get_message_listing_of_period_end},  // map_get_ml_period_end|0
    {"map_get_ml_period_bend",  app_bt_pts_map_get_message_listing_of_period_bend}, // map_get_ml_period_bend|0
    {"map_get_owner",           app_bt_pts_map_get_owner_status},       // map_get_owner|0|cc:conversation_id
    {"map_set_owner",           app_bt_pts_map_set_owner_status},       // map_set_owner|0|cc
    {"map_get_cl",              app_bt_pts_map_get_convo_listing},      // map_get_cl|0|cc
    {"map_get_cl_size",         app_bt_pts_map_get_convo_listing_size}, // map_get_cl_size|0|cc
    {"map_get_cl_readstatus",   app_bt_pts_map_get_convo_listing_by_readstatus},        // map_get_cl_readstatus|0|cc|1
    {"map_get_cl_recipient",    app_bt_pts_map_get_convo_listing_by_recipient},         // map_get_cl_recipient|0|cc
    {"map_get_cl_last_activity",app_bt_pts_map_get_convo_listing_by_last_activity},     // map_get_cl_last_activity|0|cc
#endif
#if defined(BT_SOURCE)
    {"bt_conn_earbud_link",     app_bt_connect_earbud_link_test},   // bt_conn_earbud_link|af:19:b0:bb:22:74
    {"bt_conn_earbud_a2dp",     app_bt_connect_earbud_a2dp_test},   // bt_conn_earbud_a2dp|af:19:b0:bb:22:74
    {"bt_conn_earbud_avrcp",    app_bt_connect_earbud_avrcp_test},
    {"bt_conn_earbud_hfp",      app_bt_connect_earbud_hfp_test},
    {"bt_conn_mobile_link",     app_bt_connect_mobile_link_test},
    {"bt_conn_mobile_a2dp",     app_bt_connect_mobile_a2dp_test},
    {"bt_conn_mobile_avrcp",    app_bt_connect_mobile_avrcp_test},
    {"bt_conn_mobile_hfp",      app_bt_connect_mobile_hfp_test},
#if defined(mHDT_SUPPORT)
    {"enter_mhdt_mode",         app_bt_source_enter_mhdt_mode_test},
    {"exit_mhdt_mode",          app_bt_source_exit_mhdt_mode_test},
#endif
#endif
#if defined(BT_PAN_SUPPORT)
    {"pa_conn_mobile",          app_bt_pts_pan_connect_mobile},
#endif
#ifdef IBRT_V2_MULTIPOINT
    {"ib_c_m",                  app_ibrt_connect_mobile_test},
    {"ib_c_i",                  app_ibrt_connect_ibrt_test},
    {"ib_r_s",                  app_ibrt_role_switch_test},
#endif
};

void app_bt_add_common_test_table(void)
{
    app_bt_cmd_add_test_table(app_bt_common_test_handle, ARRAY_SIZE(app_bt_common_test_handle));
    app_bt_cmd_add_test_table_with_param(app_bt_common_test_with_param_handle, ARRAY_SIZE(app_bt_common_test_with_param_handle));
}

void app_bt_cmd_init(void)
{
    if (app_bt_cmd_mailbox_init())
        return;

    app_bt_cmd_tid = osThreadCreate(osThread(app_bt_cmd_thread), NULL);
    if (app_bt_cmd_tid == NULL)  {
        TRACE(0,"Failed to Create app_bt_cmd_thread\n");
        return;
    }

    memset(g_bt_cmd_tables, 0, sizeof(g_bt_cmd_tables));
    memset(g_bt_cmd_with_parm_tables, 0, sizeof(g_bt_cmd_with_parm_tables));

#ifdef IBRT
#if defined(BES_AUTOMATE_TEST) || defined(HAL_TRACE_RX_ENABLE)
    app_ibrt_peripheral_heap_init();
#endif

#ifdef IBRT_UI
    app_ibrt_ui_v2_add_test_cmd_table();
#endif /*IBRT_UI*/

#ifdef SPA_AUDIO_SEC
    app_tz_add_test_cmd_table();
#endif

#endif /* IBRT */

    app_bt_add_common_test_table();

    return;
}
#endif

