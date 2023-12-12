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
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "app_dsp_m55.h"
#include "dsp_m55.h"
#include "cqueue.h"
#include "heap_api.h"
#include "hal_timer.h"

#define SEND_TRYCNT 5
#define BTH_M55_TRACE(s,...)
// TRACE(s, ##__VA_ARGS__)

//#define DSP_M55_CORE_BRIDGE_RAW_LOG_DUMP_EN

extern uint32_t __m55_core_bridge_task_cmd_table_start[];
extern uint32_t __m55_core_bridge_task_cmd_table_end[];

extern uint32_t __m55_core_bridge_instant_cmd_table_start[];
extern uint32_t __m55_core_bridge_instant_cmd_table_end[];

extern "C" uint32_t hal_sys_timer_get(void);

#define M55_CORE_BRIDGE_TASK_CMD_PTR_FROM_ENTRY_INDEX(entryIndex)   \
    ((app_dsp_m55_bridge_task_cmd_instance_t *)                    \
    ((uint32_t)__m55_core_bridge_task_cmd_table_start +             \
    (entryIndex)*sizeof(app_dsp_m55_bridge_task_cmd_instance_t)))

#define M55_CORE_BRIDGE_INSTANT_CMD_PTR_FROM_ENTRY_INDEX(entryIndex)    \
    ((app_dsp_m55_bridge_instant_cmd_instance_t *)                     \
    ((uint32_t)__m55_core_bridge_instant_cmd_table_start +              \
    (entryIndex)*sizeof(app_dsp_m55_bridge_instant_cmd_instance_t)))

typedef struct
{
    uint16_t  cmdcode;
    uint16_t cmdseq;
    uint8_t   content[APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE];
} __attribute__((packed)) app_dsp_m55_bridge_data_packet_t;

static bool app_dsp_m55_bridge_inited = false;

static CQueue app_dsp_m55_bridge_receive_queue;
static uint8_t app_dsp_m55_bridge_rx_buff[APP_DSP_M55_BRIDGE_RX_BUFF_SIZE];
static uint16_t g_dsp_m55_cmdseq = 0;

osSemaphoreDef(app_dsp_m55_bridge_wait_tx_done);
osSemaphoreId app_dsp_m55_bridge_wait_tx_done_id = NULL;

osSemaphoreDef(app_dsp_m55_bridge_wait_cmd_rsp);
osSemaphoreId app_dsp_m55_bridge_wait_cmd_rsp_id = NULL;

static app_dsp_m55_bridge_task_cmd_instance_t* app_dsp_m55_bridge_get_task_cmd_entry(uint16_t cmdcode);
static int32_t app_dsp_m55_bridge_queue_push(CQueue* ptrQueue, const void* ptrData, uint32_t length);
static uint16_t app_dsp_m55_bridge_queue_get_next_entry_length(CQueue* ptrQueue);
static void app_dsp_m55_bridge_queue_pop(CQueue* ptrQueue, uint8_t *buff, uint32_t len);
static int32_t app_dsp_m55_bridge_get_queue_length(CQueue *ptrQueue);
static app_dsp_m55_bridge_instant_cmd_instance_t* app_dsp_m55_bridge_get_instant_cmd_entry(uint16_t cmdcode);
static bool app_dsp_m55_enabled = false;

static bool app_dsp_m55_is_ignore_cmdcode(uint16_t cmdCode)
{
#ifdef A2DP_DECODER_CROSS_CORE_USE_M55
    if (cmdCode >= CROSS_CORE_TASK_CMD_A2DP_FEED_ENCODED_DATA_PACKET ||
        cmdCode <= CROSS_CORE_TASK_CMD_A2DP_ACK_FED_PCM_DATA_PACKET) {
        return true;
    }
#endif
    // Add command codes that do not need to print log here
    return false;
}

static void app_dsp_m55_bridge_print_cmdcode(uint16_t cmdCode, uint16_t seq,uint8_t *p_buff)
{
    if (app_dsp_m55_is_ignore_cmdcode(cmdCode)) {
        return;
    }

    POSSIBLY_UNUSED app_dsp_m55_bridge_task_cmd_instance_t* pInstance =
        app_dsp_m55_bridge_get_task_cmd_entry(cmdCode);

    if (M55_CORE_BRIDGE_CMD_CODE(M55_CORE_BRIDGE_CMD_GROUP_TASK, MCU_DSP_M55_TASK_CMD_RSP)
        != cmdCode)
    {
        BTH_M55_TRACE(1,"core bridge:---TRS: cmdcode=%s seq = %d --->", \
              pInstance->log_cmd_code_str,seq);
    }
    else
    {
        POSSIBLY_UNUSED uint16_t responsedCmd = ((p_buff[1] << 8) | p_buff[0]);
        BTH_M55_TRACE(1,"core bridge:transmit RSP to cmdcode=0x%x seq = %d", responsedCmd,seq);
    }
}

static int core_bridge_send(const uint8_t *data, uint32_t len, int trycnt)
{
    int dly_list[] = {2,5,9,17,33};
    int i, ret;
    uint32_t m55_pc = 0;

    for (i = 0; i < trycnt; i++) {
        ret = dsp_m55_send(data, len);
        if (!ret) {
            return 0;
        }
        TRACE(1, "[%s] WARNING: send failed [%d/%d], ret=%d", __func__, i, trycnt, ret);
        osDelay(dly_list[i%5]);
    }

    *(volatile uint32_t*)0x50000054 = 0x103;
    m55_pc = *((volatile uint32_t*)0x500000f4);

    ASSERT(i < trycnt, "[%s] ERORR, PC:0x%x data=0x%x, len=%d",
        __func__, m55_pc, (int)data, len);
    return -1;
}

static void app_dsp_m55_bridge_transmit_data(
    app_dsp_m55_bridge_task_cmd_instance_t* pCmdInstance,
    uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    if (!app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: not inited", __func__);
        return;
    }
    if (!app_dsp_m55_enabled)
    {
        TRACE(1, "[%s]: WARNING: not enabled", __func__);
        return;
    }

    app_dsp_m55_bridge_data_packet_t dataPacket;
    dataPacket.cmdcode = cmdcode;
    dataPacket.cmdseq = g_dsp_m55_cmdseq++;

    ASSERT(length <= APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE,
        "core bridge tx size %d > max %d", length,
        APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE);

    if (length > 0)
    {
        ASSERT(p_buff!=NULL, "%s,%d: null ptr", __func__, __LINE__);
        memcpy(dataPacket.content, p_buff, length);
    }

    core_bridge_send((const uint8_t *)&dataPacket, length+sizeof(dataPacket.cmdcode) + sizeof(dataPacket.cmdseq), SEND_TRYCNT);

    app_dsp_m55_bridge_print_cmdcode(cmdcode, dataPacket.cmdseq,p_buff);
#ifdef DSP_M55_CORE_BRIDGE_RAW_LOG_DUMP_EN
    DUMP8("%02x ", (uint8_t*)(&dataPacket), length + sizeof(dataPacket.cmdcode) + sizeof(dataPacket.cmdseq));
#endif
    //uint32_t currentMs = hal_sys_timer_get();

    osSemaphoreWait(app_dsp_m55_bridge_wait_tx_done_id, osWaitForever);

    //BTH_M55_TRACE(0, "tx cost %d", hal_sys_timer_get()-currentMs);

    if (pCmdInstance->app_dsp_m55_bridge_transmisson_done_handler)
    {
        pCmdInstance->app_dsp_m55_bridge_transmisson_done_handler(
            cmdcode, p_buff, length);
    }
}

// Important: This function cannot be used directly, it needs to be used in the IPC command table
void app_dsp_m55_bridge_send_instant_cmd_data(uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    if (!app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: not inited", __func__);
        return;
    }

    app_dsp_m55_bridge_data_packet_t dataPacket;
    dataPacket.cmdcode = cmdcode;
    dataPacket.cmdseq = g_dsp_m55_cmdseq++;

    ASSERT(length <= APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE,
        "%s: core bridge tx size %d > max %d", __func__, length,
        APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE);

    if (length > 0)
    {
        ASSERT(p_buff!=NULL, "%s,%d: null ptr", __func__, __LINE__);
        memcpy(dataPacket.content, p_buff, length);
    }

    core_bridge_send((const uint8_t *)&dataPacket, length+sizeof(dataPacket.cmdcode) + sizeof(dataPacket.cmdseq), SEND_TRYCNT);
    osSemaphoreWait(app_dsp_m55_bridge_wait_tx_done_id, osWaitForever);
}

// Important: This function cannot be used directly, it needs to be used in the IPC command table
void app_dsp_m55_bridge_send_data_without_waiting_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    app_dsp_m55_bridge_task_cmd_instance_t* pInstance =
            app_dsp_m55_bridge_get_task_cmd_entry(cmdcode);

    ASSERT(0 == pInstance->wait_rsp_timeout_ms, \
           "%s: cmdCode:0x%x wait rsp timeout:%d should be 0", __func__, \
           pInstance->wait_rsp_timeout_ms, \
           cmdcode);

    app_dsp_m55_bridge_transmit_data(pInstance, cmdcode, p_buff, length);
}

// Important: This function cannot be used directly, it needs to be used in the IPC command table
void app_dsp_m55_bridge_send_data_with_waiting_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length)
{
    app_dsp_m55_bridge_task_cmd_instance_t* pInstance =
            app_dsp_m55_bridge_get_task_cmd_entry(cmdcode);

    ASSERT(pInstance->wait_rsp_timeout_ms > 0, \
           "%s: cmdCode:0x%x wait rsp timeout:%d should be > 0", __func__, \
           pInstance->wait_rsp_timeout_ms, \
           cmdcode);

    app_dsp_m55_bridge_transmit_data(pInstance, cmdcode, p_buff, length);

    uint32_t stime = 0, etime = 0;

    stime = hal_sys_timer_get();

    int32_t returnValue = 0;
    returnValue = osSemaphoreWait(app_dsp_m55_bridge_wait_cmd_rsp_id,
        pInstance->wait_rsp_timeout_ms);

    etime = hal_sys_timer_get();

    if ((0 == returnValue) || (-1 == returnValue))
    {
        TRACE(2,"%s err = %d",__func__,returnValue);
        TRACE(2,"core bridge:wait rsp to cmdcode=%s timeout(%d)", \
              pInstance->log_cmd_code_str, \
              (etime-stime)/16000);

        if (pInstance->app_dsp_m55_bridge_wait_rsp_timeout_handle)
        {
            pInstance->app_dsp_m55_bridge_wait_rsp_timeout_handle(
                p_buff,length);
        }
    }
}

unsigned int app_dsp_m55_bridge_data_received(const void* data, unsigned int len)
{
    if (!app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: not inited", __func__);
        return len;
    }

    app_dsp_m55_bridge_data_packet_t* pDataPacket = (app_dsp_m55_bridge_data_packet_t *)data;

    if (M55_CORE_BRIDGE_CMD_GROUP_INSTANT == M55_CORE_BRIDGE_CMD_GROUP(pDataPacket->cmdcode))
    {
        app_dsp_m55_bridge_instant_cmd_instance_t* pInstance =
            app_dsp_m55_bridge_get_instant_cmd_entry(pDataPacket->cmdcode);

        if (pInstance->cmdhandler)
        {
            pInstance->cmdhandler(pDataPacket->content, len-sizeof(pDataPacket->cmdcode) - sizeof((pDataPacket->cmdseq)));
        }
        else
        {
            TRACE(1,"core bridge:%s cmd not handled",__func__);
        }
    }
    else
    {
        app_dsp_m55_bridge_queue_push(&app_dsp_m55_bridge_receive_queue, data, len);
    }
    return len;
}

void app_dsp_m55_bridge_data_tx_done(const void* data, unsigned int len)
{
    if (app_dsp_m55_bridge_wait_tx_done_id)
    {
        osSemaphoreRelease(app_dsp_m55_bridge_wait_tx_done_id);
    }
}

static void app_dsp_m55_bridge_rx_handler(uint8_t* p_data_buff, uint16_t length)
{
    app_dsp_m55_bridge_data_packet_t* pDataPacket = (app_dsp_m55_bridge_data_packet_t *)p_data_buff;

    if (M55_CORE_BRIDGE_CMD_GROUP_TASK == M55_CORE_BRIDGE_CMD_GROUP(pDataPacket->cmdcode))
    {
        volatile app_dsp_m55_bridge_task_cmd_instance_t* pInstance =
            app_dsp_m55_bridge_get_task_cmd_entry(pDataPacket->cmdcode);

        if (!app_dsp_m55_is_ignore_cmdcode(pDataPacket->cmdcode)) {
            if (MCU_DSP_M55_TASK_CMD_RSP != pDataPacket->cmdcode)
            {
                BTH_M55_TRACE(0, "core bridge:<---RCV:cmdcode=%s seq = %d len=%d", \
                      pInstance->log_cmd_code_str, pDataPacket->cmdseq ,length);
            } else{
                BTH_M55_TRACE(2, "core bridge:<---RCV_RSP: cmdcode=%s seq = %d len %d", \
                     pInstance->log_cmd_code_str,pDataPacket->cmdseq,length);
            }
        }

#ifdef DSP_M55_CORE_BRIDGE_RAW_LOG_DUMP_EN
        DUMP8("%02x ", p_data_buff, length);
#endif

        if (pInstance->cmdhandler)
        {
            pInstance->cmdhandler(pDataPacket->content, length-sizeof(pDataPacket->cmdcode) - sizeof(pDataPacket->cmdseq));
        }
        else
        {
            TRACE(1,"core bridge:%s cmd not handled",__func__);
        }
    }
    else
    {
        if (MCU_DSP_M55_DUMPPY_INFO == pDataPacket->cmdcode) {
            TRACE(1, "core bridge:%s ignore dummy info", __func__);
            return;
        }
        ASSERT(false, "%s: invalid cmdcode %x", __func__, pDataPacket->cmdcode);
    }
}

static osThreadId app_dsp_m55_bridge_rx_thread_id = NULL;
static void app_dsp_m55_bridge_rx_thread(const void *arg);
osThreadDef(app_dsp_m55_bridge_rx_thread, osPriorityHigh, 1,
    (APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE), "core_bridge_rx_thread");
#define APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_DATA_RECEIVED  0x01
#define APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_TURN_OFF_M55   0x02

static void app_dsp_m55_bridge_rx_thread(const void *arg)
{

    while(1)
    {
        osEvent evt;
        // wait any signal
        evt = osSignalWait(0x0, osWaitForever);

        // get role from signal value
        if (osEventSignal == evt.status)
        {
            if (evt.value.signals & APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_DATA_RECEIVED)
            {
                while (app_dsp_m55_bridge_get_queue_length(&app_dsp_m55_bridge_receive_queue) > 0)
                {
                    uint8_t rcv_tmp_buffer[APP_DSP_M55_BRIDGE_RX_THREAD_TMP_BUF_SIZE];

                    uint16_t rcv_length =
                        app_dsp_m55_bridge_queue_get_next_entry_length(
                            &app_dsp_m55_bridge_receive_queue);

                    ASSERT(rcv_length <= sizeof(rcv_tmp_buffer),
                        "received data size %d is bigger than tmp rx buf size!", rcv_length);

                    if (rcv_length > 0)
                    {
                        app_dsp_m55_bridge_queue_pop(
                            &app_dsp_m55_bridge_receive_queue,
                            rcv_tmp_buffer, rcv_length);
                        app_dsp_m55_bridge_rx_handler(rcv_tmp_buffer, rcv_length);
                    }
                }
            }
            else if (evt.value.signals & APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_TURN_OFF_M55)
            {
                dsp_m55_core_deinit();
            }
        }
    }
}

static osThreadId app_dsp_m55_bridge_tx_thread_id = NULL;
static void app_dsp_m55_bridge_tx_thread(const void *arg);
osThreadDef(app_dsp_m55_bridge_tx_thread, osPriorityHigh, 1,
    (APP_DSP_M55_BRIDGE_TX_THREAD_STACK_SIZE), "core_bridge_tx_thread");

typedef struct
{
    uint16_t cmdCode;
    uint16_t cmd_len;
    uint8_t *cmd_buffer;
} APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T;

osMailQDef (app_dsp_m55_bridge_tx_mailbox, APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX,
            APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T);

static osMutexId app_dsp_m55_bridge_tx_mutex_id = NULL;
osMutexDef(app_dsp_m55_bridge_tx_mutex);

static osMailQId app_dsp_m55_bridge_tx_mailbox_id = NULL;

static heap_handle_t app_dsp_m55_bridge_tx_mailbox_heap;
static uint8_t app_dsp_m55_bridge_tx_mailbox_heap_pool[APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE*(APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX/2)];

static void app_dsp_m55_dump_register_info(void)
{
    uint32_t m55_pc = 0;
    uint32_t m55_lr = 0;
    uint32_t m55_sp = 0;
    uint32_t m55_pc1 = 0;
    uint32_t m55_lr1 = 0;
    uint32_t m55_pc2 = 0;
    uint32_t m55_lr2 = 0;

    app_dsp_m55_bridge_data_packet_t dummy_info;

    *(volatile uint32_t*)0x50000054 = 0x103;
    m55_pc = *((volatile uint32_t*)0x500000f4);

    *(volatile uint32_t*)0x50000054 = 0x903;
    m55_lr = *((volatile uint32_t*)0x500000f4);

    *(volatile uint32_t*)0x50000054 = 0x1103;
    m55_sp = *((volatile uint32_t*)0x500000f4);

    hal_sys_timer_delay(MS_TO_TICKS(2));

    *(volatile uint32_t*)0x50000054 = 0x103;
    m55_pc1 = *((volatile uint32_t*)0x500000f4);

    *(volatile uint32_t*)0x50000054 = 0x903;
    m55_lr1 = *((volatile uint32_t*)0x500000f4);

    dummy_info.cmdcode = MCU_DSP_M55_DUMPPY_INFO;

    core_bridge_send((const uint8_t *)&dummy_info, sizeof(dummy_info.cmdcode) + sizeof(dummy_info.cmdseq), SEND_TRYCNT);

    *(volatile uint32_t*)0x50000054 = 0x103;
    m55_pc2 = *((volatile uint32_t*)0x500000f4);

    *(volatile uint32_t*)0x50000054 = 0x903;
    m55_lr2 = *((volatile uint32_t*)0x500000f4);

    ASSERT(0, "M55 Info,PC:0x%x-0x%x-0x%x, LR:0x%x-0x%x-0x%x, SP:0x%x",
            m55_pc, m55_pc1, m55_pc2, m55_lr, m55_lr1, m55_lr2, m55_sp);
}

void app_dsp_m55_bridge_tx_mailbox_heap_init(void)
{
    app_dsp_m55_bridge_tx_mailbox_heap =
        heap_register(app_dsp_m55_bridge_tx_mailbox_heap_pool,
        sizeof(app_dsp_m55_bridge_tx_mailbox_heap_pool));
}

void *app_dsp_m55_bridge_tx_mailbox_heap_malloc(uint32_t size)
{
    void *ptr = NULL;
    if (size){
        ptr = heap_malloc(app_dsp_m55_bridge_tx_mailbox_heap, size);
        if (!ptr) {
            TRACE(0, "tx_mailbox_heap_malloc failed,size:%d", size);
            app_dsp_m55_dump_register_info();
        }
    }
    return ptr;
}

void app_dsp_m55_bridge_tx_mailbox_heap_free(void *rmem)
{
    if (rmem){
        heap_free(app_dsp_m55_bridge_tx_mailbox_heap, rmem);
    }
}

static int32_t app_dsp_m55_bridge_tx_mailbox_get(APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T** msg_p)
{
    osEvent evt;

    evt = osMailGet(app_dsp_m55_bridge_tx_mailbox_id, osWaitForever);
    if (evt.status == osEventMail)
    {
        *msg_p = (APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *)evt.value.p;
        return 0;
    }
    return -1;
}

static int32_t app_dsp_m55_bridge_tx_mailbox_raw(APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T** msg_p)
{
    osEvent evt;
    evt = osMailGet(app_dsp_m55_bridge_tx_mailbox_id, 0);
    if (evt.status == osEventMail)
    {
        *msg_p = (APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *)evt.value.p;
        return 0;
    }
    return -1;
}

static int32_t app_dsp_m55_bridge_tx_mailbox_free(APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T* msg_p)
{
    osStatus status;
    app_dsp_m55_bridge_tx_mailbox_heap_free((void *)msg_p->cmd_buffer);
    status = osMailFree(app_dsp_m55_bridge_tx_mailbox_id, msg_p);
    return (int32_t)status;
}

static int32_t app_dsp_m55_bridge_tx_mailbox_put(APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T* msg_src)
{
    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *msg_p = NULL;
    osStatus status;

    msg_p = (APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T*)osMailAlloc(app_dsp_m55_bridge_tx_mailbox_id, 0);
    if (!msg_p){
        TRACE_IMM(0, "core bridge mailbox alloc error dump");

        for (uint8_t i=0; i<APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX; i++){
            if (!app_dsp_m55_bridge_tx_mailbox_raw(&msg_p)) {
                app_dsp_m55_bridge_task_cmd_instance_t* cmdEntry;
                cmdEntry = app_dsp_m55_bridge_get_task_cmd_entry(msg_p->cmdCode);

                TRACE(3, "ctrl_mailbox:DUMP: cmdcode=%s", \
                      cmdEntry->log_cmd_code_str);
            }
        }
        TRACE_IMM(0,"core bridge mailbox Alloc error dump end");
        TRACE_IMM(0,"core bridge mailbox reAlloc New");
        msg_p = (APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T*)osMailAlloc(app_dsp_m55_bridge_tx_mailbox_id, 0);
    }

    if (!msg_p) {
        TRACE(0, "core bridge mailbox Alloc error");
        app_dsp_m55_dump_register_info();
    }

    msg_p->cmdCode = msg_src->cmdCode;
    msg_p->cmd_len = msg_src->cmd_len ;
    msg_p->cmd_buffer = msg_src->cmd_buffer;

    status = osMailPut(app_dsp_m55_bridge_tx_mailbox_id, msg_p);

    return (int32_t)status;
}

static int32_t app_dsp_m55_bridge_tx_mailbox_init(void)
{
    app_dsp_m55_bridge_tx_mailbox_id = osMailCreate(osMailQ(app_dsp_m55_bridge_tx_mailbox), NULL);
    if (NULL == app_dsp_m55_bridge_tx_mailbox_id)
    {
        // should increase OS_DYNAMIC_MEM_SIZE
        ASSERT(0, "Failed to Create core bridge mailbox\n");
        return -1;
    }
    return 0;
}

static void app_dsp_m55_bridge_tx_thread_init(void)
{
    if (app_dsp_m55_bridge_tx_mutex_id == NULL) {
        app_dsp_m55_bridge_tx_mutex_id = osMutexCreate(osMutex(app_dsp_m55_bridge_tx_mutex));
    }

    app_dsp_m55_bridge_tx_mailbox_heap_init();
    app_dsp_m55_bridge_tx_mailbox_init();
    if (app_dsp_m55_bridge_tx_thread_id == NULL) {
        app_dsp_m55_bridge_tx_thread_id =
            osThreadCreate(osThread(app_dsp_m55_bridge_tx_thread), NULL);
    }
}

app_dsp_m55_bridge_task_cmd_instance_t* app_dsp_m55_bridge_find_task_cmd_entry(uint16_t cmdcode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__m55_core_bridge_task_cmd_table_end-
        (uint32_t)__m55_core_bridge_task_cmd_table_start)/sizeof(app_dsp_m55_bridge_task_cmd_instance_t);
        index++) {
        if (M55_CORE_BRIDGE_TASK_CMD_PTR_FROM_ENTRY_INDEX(index)->cmdcode == cmdcode) {
            return M55_CORE_BRIDGE_TASK_CMD_PTR_FROM_ENTRY_INDEX(index);
        }
    }
    return NULL;
}

static app_dsp_m55_bridge_task_cmd_instance_t* app_dsp_m55_bridge_get_task_cmd_entry(uint16_t cmdcode)
{
    app_dsp_m55_bridge_task_cmd_instance_t* inst = NULL;

    inst = app_dsp_m55_bridge_find_task_cmd_entry(cmdcode);
    ASSERT(inst!=NULL, "[%s] invalid cmdcode=%x", __func__, cmdcode);
    return inst;
}

app_dsp_m55_bridge_instant_cmd_instance_t* app_dsp_m55_bridge_find_instant_cmd_entry(uint16_t cmdcode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__m55_core_bridge_instant_cmd_table_end-
        (uint32_t)__m55_core_bridge_instant_cmd_table_start)/sizeof(app_dsp_m55_bridge_instant_cmd_instance_t);
        index++) {
        if (M55_CORE_BRIDGE_INSTANT_CMD_PTR_FROM_ENTRY_INDEX(index)->cmdcode == cmdcode) {
            return M55_CORE_BRIDGE_INSTANT_CMD_PTR_FROM_ENTRY_INDEX(index);
        }
    }
    return NULL;
}

static app_dsp_m55_bridge_instant_cmd_instance_t* app_dsp_m55_bridge_get_instant_cmd_entry(uint16_t cmdcode)
{
    app_dsp_m55_bridge_instant_cmd_instance_t* inst = NULL;

    inst = app_dsp_m55_bridge_find_instant_cmd_entry(cmdcode);
    ASSERT(inst != NULL, "[%s] invalid cmdcode=%x", __func__, cmdcode);
    return inst;
}

void app_dsp_m55_bridge_tx_thread(const void *arg)
{
    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *msg_p = NULL;

    while(1)
    {
        if (!app_dsp_m55_enabled) {
            osSignalWait(SIGNAL_M55_BOOT_DONE, osWaitForever);
        }

        int32_t ret = app_dsp_m55_bridge_tx_mailbox_get(&msg_p);

        //BES intentional code. msg_p is not NULL
        if ((0 == ret) && (M55_CORE_BRIDGE_CMD_GROUP_TASK == M55_CORE_BRIDGE_CMD_GROUP(msg_p->cmdCode)))
        {
            app_dsp_m55_bridge_task_cmd_instance_t* pTaskCmdInstance =
                app_dsp_m55_bridge_get_task_cmd_entry(msg_p->cmdCode);

            pTaskCmdInstance->core_bridge_cmd_transmit_handler(
                msg_p->cmd_buffer, msg_p->cmd_len);
        }
        else if ((0 == ret) && (M55_CORE_BRIDGE_CMD_GROUP_INSTANT == M55_CORE_BRIDGE_CMD_GROUP(msg_p->cmdCode)))
        {
            app_dsp_m55_bridge_instant_cmd_instance_t* pInstantCmdInstance =
                app_dsp_m55_bridge_get_instant_cmd_entry(msg_p->cmdCode);

            pInstantCmdInstance->core_bridge_cmd_transmit_handler(
                msg_p->cmd_buffer, msg_p->cmd_len);
        }
        else
        {
            ASSERT(false, "Wrong core bridge cmd code 0x%x", msg_p->cmdCode);
        }

        app_dsp_m55_bridge_tx_mailbox_free(msg_p);
    }
}

int32_t app_dsp_m55_bridge_send_cmd(uint16_t cmd_code, uint8_t *p_buff, uint16_t length)
{
    if (!app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: not inited", __func__);
        return 0;
    }

    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T msg = {0,};
    int nRet = 0;
    ASSERT(length <= APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE,
        "%s p_buff overflow %d", __func__, length);

    msg.cmdCode = cmd_code;
    msg.cmd_len = length;

    if (length > 0)
    {
        msg.cmd_buffer = (uint8_t *)app_dsp_m55_bridge_tx_mailbox_heap_malloc(msg.cmd_len);
        memcpy(msg.cmd_buffer, p_buff, msg.cmd_len);
    }
    else
    {
        msg.cmd_buffer = NULL;
    }

    osMutexWait(app_dsp_m55_bridge_tx_mutex_id, osWaitForever);
    nRet = app_dsp_m55_bridge_tx_mailbox_put(&msg);
    osMutexRelease(app_dsp_m55_bridge_tx_mutex_id);

    return nRet;
}

int32_t app_dsp_m55_bridge_send_rsp(uint16_t rsp_code, uint8_t *p_buff, uint16_t length)
{
    int nRet = 0;
    if (!app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: not inited", __func__);
        return 0;
    }

    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T msg = {0,};

    ASSERT(length + sizeof(rsp_code) <= APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE, \
           "%s p_buff overflow: %u", \
           __func__, \
           length);

    msg.cmd_len = length + sizeof(rsp_code);
    msg.cmd_buffer = (uint8_t *)app_dsp_m55_bridge_tx_mailbox_heap_malloc(msg.cmd_len);
    *(uint16_t *)msg.cmd_buffer = rsp_code;
    memcpy(msg.cmd_buffer + sizeof(rsp_code), p_buff, length);

    msg.cmdCode = MCU_DSP_M55_TASK_CMD_RSP;

    osMutexWait(app_dsp_m55_bridge_tx_mutex_id, osWaitForever);
    nRet = app_dsp_m55_bridge_tx_mailbox_put(&msg);
    osMutexRelease(app_dsp_m55_bridge_tx_mutex_id);

    return nRet;
}

static void app_dsp_m55_bridge_send_cmd_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    POSSIBLY_UNUSED app_dsp_m55_bridge_task_cmd_instance_t* cmdEntry;
    app_dsp_m55_bridge_data_packet_t cmddata;

    ASSERT(length <= APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE,
        "%s: length(%d) out of range(%d)", __func__,
        length, APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE);

    cmddata.cmdcode = MCU_DSP_M55_TASK_CMD_RSP;
    cmddata.cmdseq = g_dsp_m55_cmdseq++;
    memcpy(cmddata.content, p_buff, length);

    core_bridge_send((const uint8_t *)&cmddata, length+sizeof(cmddata.cmdcode) + sizeof(cmddata.cmdseq), SEND_TRYCNT);

    cmdEntry = app_dsp_m55_bridge_get_task_cmd_entry(*(uint16_t *)p_buff);

    BTH_M55_TRACE(2,"core bridge:-------TRS RSP to cmdcode=%s seq = %d------->", \
          cmdEntry->log_cmd_code_str,cmddata.cmdseq);
#ifdef DSP_M55_CORE_BRIDGE_RAW_LOG_DUMP_EN
    DUMP8("%02x ", (uint8_t*)&cmddata, length + sizeof(cmddata.cmdseq) + sizeof(cmddata.cmdcode));
#endif
    osSemaphoreWait(app_dsp_m55_bridge_wait_tx_done_id, osWaitForever);
}

static void app_dsp_m55_bridge_cmd_rsp_handler(uint8_t *p_buff, uint16_t length)
{
    //real cmd code responsed
    uint16_t rspcode = ((p_buff[1] << 8) | p_buff[0]);
    app_dsp_m55_bridge_task_cmd_instance_t* pInstance =
                app_dsp_m55_bridge_get_task_cmd_entry(rspcode);

     BTH_M55_TRACE(2,"core bridge:<-----------RCV_RSP: cmdcode=%s-------", \
          pInstance->log_cmd_code_str);

    uint32_t timeout_ms = pInstance->wait_rsp_timeout_ms;
    if (pInstance->app_dsp_m55_bridge_rsp_handle)
    {
        pInstance->app_dsp_m55_bridge_rsp_handle(&p_buff[2],
            length - 2);

        if (timeout_ms > 0)
        {
            osSemaphoreRelease(app_dsp_m55_bridge_wait_cmd_rsp_id);
        }
    }
    else
    {
        TRACE(0,"core bridge:rsp no handler");
    }
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_RSP,
                                "core bridge task cmd rsp",
                                app_dsp_m55_bridge_send_cmd_rsp_handler,
                                app_dsp_m55_bridge_cmd_rsp_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);

#if defined(UTILS_ESHELL_EN)
M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_BTH_M55_TASK_CMD_ESHELL_TRANSMIT_DATA,
                            "eshell rpc data comm",
                            app_dsp_m55_bridge_send_cmd_rsp_handler,
                            app_dsp_m55_bridge_cmd_rsp_handler,
                            0,
                            NULL,
                            NULL,
                            NULL);
#endif

#define APP_DSP_M55_BRIDGE_QUEUE_DATA_LEN_BYTES    2

static void app_dsp_m55_bridge_queue_init(CQueue* ptrQueue, uint8_t* ptrBuf, uint32_t bufLen)
{
    uint32_t lock = int_lock();
    memset(ptrBuf, 0, bufLen);
    InitCQueue(ptrQueue, bufLen, (CQItemType *)ptrBuf);
    int_unlock(lock);
}

void app_dsp_m55_bridge_trigger_turn_off(void)
{
    osSignalSet(app_dsp_m55_bridge_rx_thread_id, APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_TURN_OFF_M55);
}

static int32_t app_dsp_m55_bridge_queue_push(CQueue* ptrQueue, const void* ptrData, uint32_t length)
{
    int32_t nRet = -1;
    uint32_t lock = int_lock();
    if (length > 0)
    {
        uint16_t dataLen = (uint16_t)length;
        int queueAvailableLen = AvailableOfCQueue(ptrQueue);
        if ((dataLen+APP_DSP_M55_BRIDGE_QUEUE_DATA_LEN_BYTES) <= queueAvailableLen)
        {
            EnCQueue(ptrQueue, (CQItemType *)&dataLen, APP_DSP_M55_BRIDGE_QUEUE_DATA_LEN_BYTES);
            EnCQueue(ptrQueue, (CQItemType *)ptrData, length);
            nRet = 0;
        }
    }
    int_unlock(lock);
    osSignalSet(app_dsp_m55_bridge_rx_thread_id, APP_DSP_M55_BRIDGE_RX_THREAD_SIGNAL_DATA_RECEIVED);

    return nRet;
}

static uint16_t app_dsp_m55_bridge_queue_get_next_entry_length(CQueue* ptrQueue)
{
    uint8_t *e1 = NULL, *e2 = NULL;
    uint32_t len1 = 0, len2 = 0;
    uint16_t length = 0;
    uint8_t* ptr;

    uint32_t lock = int_lock();

    ptr = (uint8_t *)&length;
    // get the length of the fake message
    PeekCQueue(ptrQueue, APP_DSP_M55_BRIDGE_QUEUE_DATA_LEN_BYTES, &e1, &len1, &e2, &len2);

    memcpy(ptr,e1,len1);
    memcpy(ptr+len1,e2,len2);

    int_unlock(lock);

    return length;
}

static void app_dsp_m55_bridge_queue_pop(CQueue* ptrQueue, uint8_t *buff, uint32_t len)
{
    uint8_t *e1 = NULL, *e2 = NULL;
    uint32_t len1 = 0, len2 = 0;

    uint32_t lock = int_lock();
    // overcome the two bytes of msg length
    DeCQueue(ptrQueue, 0, APP_DSP_M55_BRIDGE_QUEUE_DATA_LEN_BYTES);

    PeekCQueue(ptrQueue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(ptrQueue, 0, len);

        // reset the poped data to ZERO
        memset(e1, 0, len1);
        memset(e2, 0, len2);
    }else{
        memset(buff, 0x00, len);
    }
    int_unlock(lock);
}

static int32_t app_dsp_m55_bridge_get_queue_length(CQueue *ptrQueue)
{
    int32_t nRet = 0;

    uint32_t lock = int_lock();
    nRet = LengthOfCQueue(ptrQueue);
    int_unlock(lock);

    return nRet;
}

void app_dsp_m55_bridge_free_tx_mailbox(void)
{
    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *msg_p = NULL;

    while (1)
    {
        osEvent evt;

        evt = osMailGet(app_dsp_m55_bridge_tx_mailbox_id, 5);
        if (evt.status == osEventMail)
        {
            msg_p = (APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *)evt.value.p;
            app_dsp_m55_bridge_tx_mailbox_free(msg_p);
        }
        else
        {
            break;
        }
    }
}

uint32_t app_dsp_m55_bridge_get_tx_mailbox_cnt(void)
{
    uint32_t result = 0;
    result = osMailGetCount(app_dsp_m55_bridge_tx_mailbox_id);
    return result;
}

uint32_t app_dsp_m55_bridge_get_tx_mailbox_free_cnt(void)
{
    return APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX - app_dsp_m55_bridge_get_tx_mailbox_cnt();
}

void app_dsp_m55_bridge_fetch_tx_mailbox(uint16_t cmd_code, uint8_t *buffer, uint16_t *length, uint16_t threshold)
{
    ASSERT(buffer != NULL, "%s, error parameter, cmd_code:0x%x", __func__, cmd_code);

    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *msg_p = NULL;
    osStatus status;
    uint32_t totalCnt = 0;

    osMutexWait(app_dsp_m55_bridge_tx_mutex_id, osWaitForever);
    totalCnt = osMailGetCount(app_dsp_m55_bridge_tx_mailbox_id);
    for (uint32_t i=0; i < totalCnt; i++) {

        if (app_dsp_m55_bridge_tx_mailbox_raw(&msg_p)) {
            continue;
        }
#if 0
        app_dsp_m55_bridge_task_cmd_instance_t* cmdEntry;
        cmdEntry = app_dsp_m55_bridge_get_task_cmd_entry(msg_p->cmdCode);
        TRACE(3, "fetch_tx_mailbox:DUMP: cmdcode=%d %s, len:%d - %d - %d", cmd_code, \
            cmdEntry ? cmdEntry->log_cmd_code_str : "NULL", msg_p->cmd_len, *length, threshold);
#endif

        if ((cmd_code != msg_p->cmdCode) || (msg_p->cmd_len > threshold) ||
            (*length >= threshold)) {
            // push back
            status = osMailPut(app_dsp_m55_bridge_tx_mailbox_id, msg_p);
            if (status) {
                TRACE(3, "fetch_tx_mailbox:push back failed");
            }
        } else if (cmd_code == msg_p->cmdCode) {
            memcpy(buffer, msg_p->cmd_buffer, msg_p->cmd_len);
            buffer += msg_p->cmd_len;
            *length += msg_p->cmd_len;
            app_dsp_m55_bridge_tx_mailbox_free(msg_p);
        }

    }
    osMutexRelease(app_dsp_m55_bridge_tx_mutex_id);
}

void app_dsp_m55_bridge_clear_tx_mailbox(uint16_t cmd_code, uint16_t reserve_cnt)
{
    APP_DSP_M55_BRIDGE_TX_MAILBOX_PARAM_T *msg_p = NULL;
    osStatus status;
    uint32_t totalCnt = 0;
    uint32_t cmd_cnt = 0;

    osMutexWait(app_dsp_m55_bridge_tx_mutex_id, osWaitForever);
    totalCnt = osMailGetCount(app_dsp_m55_bridge_tx_mailbox_id);
    for (uint32_t i=0; i < totalCnt; i++) {
        if (app_dsp_m55_bridge_tx_mailbox_raw(&msg_p)) {
            continue;
        }
        if ((cmd_code != msg_p->cmdCode) || (cmd_cnt < reserve_cnt)) {
            // push back
            status = osMailPut(app_dsp_m55_bridge_tx_mailbox_id, msg_p);
            if (status) {
                TRACE(3, "fetch_tx_mailbox:push back failed");
            }
            if (cmd_code == msg_p->cmdCode) {
                cmd_cnt++;
            }
        }
    }
    osMutexRelease(app_dsp_m55_bridge_tx_mutex_id);
}

void app_dsp_m55_bridge_init(void)
{
    if (app_dsp_m55_bridge_inited)
    {
        TRACE(1, "[%s]: WARNING: already inited", __func__);
        if (app_dsp_m55_bridge_wait_tx_done_id)
        {
            osSemaphoreWait(app_dsp_m55_bridge_wait_tx_done_id, 0);
        }

        osMutexWait(app_dsp_m55_bridge_tx_mutex_id, osWaitForever);
        if (!app_dsp_m55_enabled) {
            // free all of the tx mailbox
            app_dsp_m55_bridge_free_tx_mailbox();
            app_dsp_m55_bridge_tx_mailbox_heap_init();
        }
        osMutexRelease(app_dsp_m55_bridge_tx_mutex_id);
        return;
    }
    ASSERT(APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE \
        > (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4)/APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX/2,
        "core bridge init: xfer data size %d too small, should larger than %d",
        APP_DSP_M55_BRIDGE_MAX_XFER_DATA_SIZE,
        (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4)/APP_DSP_M55_BRIDGE_TX_MAILBOX_MAX/2);

    ASSERT(APP_DSP_M55_BRIDGE_RX_BUFF_SIZE > (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2,
        "core bridge init: rx buff size %d too small, should large than %d",
        APP_DSP_M55_BRIDGE_RX_BUFF_SIZE, (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2);

    ASSERT(APP_DSP_M55_BRIDGE_TX_THREAD_STACK_SIZE > (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2,
        "core bridge tx thread: stack size %d too small, should larger than %d",
        APP_DSP_M55_BRIDGE_TX_THREAD_STACK_SIZE,(APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2);

    ASSERT(APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE > (APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2,
        "core bridge rx thread: stack size %d too small, should larger than %d",
        APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE,(APP_DSP_M55_BRIDGE_MAX_DATA_PACKET_SIZE+4) * 2);

    ASSERT(APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE > APP_DSP_M55_BRIDGE_RX_THREAD_TMP_BUF_SIZE * 2,
        "core bridge rx thread: stack size %d too small, should larger than tmp size %d",
        APP_DSP_M55_BRIDGE_RX_THREAD_STACK_SIZE, APP_DSP_M55_BRIDGE_RX_THREAD_TMP_BUF_SIZE * 2);

    memset(&app_dsp_m55_bridge_receive_queue, 0, sizeof(app_dsp_m55_bridge_receive_queue));

    app_dsp_m55_bridge_queue_init(&app_dsp_m55_bridge_receive_queue, \
                            app_dsp_m55_bridge_rx_buff, \
                            sizeof(app_dsp_m55_bridge_rx_buff));
    g_dsp_m55_cmdseq = 0;

    app_dsp_m55_bridge_wait_tx_done_id =
        osSemaphoreCreate(osSemaphore(app_dsp_m55_bridge_wait_tx_done), 0);

    app_dsp_m55_bridge_wait_cmd_rsp_id =
        osSemaphoreCreate(osSemaphore(app_dsp_m55_bridge_wait_cmd_rsp), 0);

    app_dsp_m55_bridge_tx_thread_init();

    if (app_dsp_m55_bridge_rx_thread_id == NULL) {
        app_dsp_m55_bridge_rx_thread_id =
            osThreadCreate(osThread(app_dsp_m55_bridge_rx_thread), NULL);
    }

    app_dsp_m55_bridge_inited = true;
}

void app_dsp_m55_bridge_deinit(void)
{
    memset(&app_dsp_m55_bridge_receive_queue, 0, sizeof(app_dsp_m55_bridge_receive_queue));

    app_dsp_m55_bridge_queue_init(&app_dsp_m55_bridge_receive_queue, \
                            app_dsp_m55_bridge_rx_buff, \
                            sizeof(app_dsp_m55_bridge_rx_buff));

    app_dsp_m55_bridge_free_tx_mailbox();

    // release the waiting tx done semphore
    app_dsp_m55_bridge_data_tx_done(NULL, 0);

    // release the wait cmd rsp semphore
    if (app_dsp_m55_bridge_wait_cmd_rsp_id) {
        osSemaphoreRelease(app_dsp_m55_bridge_wait_cmd_rsp_id);
    }
}

void app_dsp_m55_notify_tx_thread_status(bool isEnabled)
{
    uint32_t lock = int_lock();
    app_dsp_m55_enabled = isEnabled;
    int_unlock(lock);
    if (isEnabled) {
        osSignalSet(app_dsp_m55_bridge_tx_thread_id, SIGNAL_M55_BOOT_DONE);
    }
}

/*
 * CORE BRIDGE DEMO MSG
 ********************************************************
 */
#if defined(M55_CORE_BRIDGE_DEMO_CLIENT) || defined(M55_CORE_BRIDGE_DEMO_SERVICE)
#define CORE_BRIDGE_DEMO_REQ_DATA       0x55
#define CORE_BRIDGE_DEMO_REQ_RSP_DATA   0xAA

static void app_dsp_m55_core_transmit_demo_no_rsp_cmd_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_data_without_waiting_rsp(MCU_DSP_M55_TASK_CMD_DEMO_REQ_NO_RSP, ptr, len);
}

static void app_dsp_m55_core_demo_no_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "Get demo no rsp command:");
    DUMP8("%02x ", ptr, len);
}

static void app_dsp_m55_core_demo_no_rsp_cmd_tx_done_handler(uint16_t cmdCode, uint8_t* ptr, uint16_t len)
{
    TRACE(0, "cmdCode 0x%x tx done", cmdCode);
    DUMP8("%02x ", ptr, len);
}

extern "C" void app_dsp_m55_core_send_demo_req_no_rsp(void)
{
    uint8_t req = CORE_BRIDGE_DEMO_REQ_DATA;
    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_TASK_CMD_DEMO_REQ_NO_RSP, &req, sizeof(req));
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_DEMO_REQ_NO_RSP,
                                "demo no rsp req",
                                app_dsp_m55_core_transmit_demo_no_rsp_cmd_handler,
                                app_dsp_m55_core_demo_no_rsp_cmd_received_handler,
                                0,
                                NULL,
                                NULL,
                                app_dsp_m55_core_demo_no_rsp_cmd_tx_done_handler);

static void app_dsp_m55_core_transmit_demo_wait_rsp_cmd_handler(uint8_t* ptr, uint16_t len)
{
#if defined(M55_CORE_BRIDGE_DEMO_CLIENT)
    app_dsp_m55_bridge_send_data_with_waiting_rsp(MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP, ptr, len);
#elif defined(M55_CORE_BRIDGE_DEMO_SERVICE)
    app_dsp_m55_bridge_send_data_without_waiting_rsp(MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP, ptr, len);
#else
    ASSERT(0, "[%s] Need to define M55_CORE_BRIDGE_DEMO_CLIENT or M55_CORE_BRIDGE_DEMO_SERVICE", __func__);
#endif
}

static void app_dsp_m55_core_demo_wait_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    // TRACE(0, "Get demo with rsp command:");
    // DUMP8("%02x ", ptr, len);

    uint8_t rsp = CORE_BRIDGE_DEMO_REQ_RSP_DATA;
    app_dsp_m55_bridge_send_rsp(MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP, &rsp, sizeof(rsp));
}

static void app_dsp_m55_core_demo_wait_rsp_cmd_wait_rsp_timeout(uint8_t* ptr, uint16_t len)
{
    // TRACE(0, "Waiting for rsp timeout.");
    ASSERT(0, "Waiting for rsp timeout.");
}

static void app_dsp_m55_core_demo_wait_rsp_cmd_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    // TRACE(0, "Gets rsp 0x%x:", *ptr);
}

static void app_dsp_m55_core_demo_with_rsp_cmd_tx_done_handler(uint16_t cmdCode, uint8_t* ptr, uint16_t len)
{
    // TRACE(0, "cmdCode 0x%x tx done", cmdCode);
}

extern "C" void app_dsp_m55_core_send_demo_req_with_rsp(void)
{
    uint8_t req = CORE_BRIDGE_DEMO_REQ_DATA;
    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP, &req, sizeof(req));
}

M55_CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_DSP_M55_TASK_CMD_DEMO_REQ_WITH_RSP,
                                "demo with rsp req",
                                app_dsp_m55_core_transmit_demo_wait_rsp_cmd_handler,
                                app_dsp_m55_core_demo_wait_rsp_cmd_received_handler,
                                APP_DSP_M55_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_dsp_m55_core_demo_wait_rsp_cmd_wait_rsp_timeout,
                                app_dsp_m55_core_demo_wait_rsp_cmd_rsp_received_handler,
                                app_dsp_m55_core_demo_with_rsp_cmd_tx_done_handler);

static void app_dsp_m55_core_transmit_demo_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    app_dsp_m55_bridge_send_instant_cmd_data(MCU_DSP_M55_INSTANT_CMD_DEMO_REQ, ptr, len);
}

static void app_dsp_m55_core_demo_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    // for test purpose, we add log print here.
    // but as instant cmd handler will be directly called in intersys irq context,
    // for realistic use, should never do log print
    TRACE(0, "Get demo instant req command:");
    DUMP8("%02x ", ptr, len);
}

extern "C" void app_dsp_m55_core_send_demo_req_instant(void)
{
    uint8_t req = CORE_BRIDGE_DEMO_REQ_DATA;
    app_dsp_m55_bridge_send_cmd(MCU_DSP_M55_INSTANT_CMD_DEMO_REQ, &req, sizeof(req));
}


M55_CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_DSP_M55_INSTANT_CMD_DEMO_REQ,
                                app_dsp_m55_core_transmit_demo_instant_req_handler,
                                app_dsp_m55_core_demo_instant_req_handler);

#endif
