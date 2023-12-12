/***************************************************************************
 * Copyright 2022-2023 BES.
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
 ***************************************************************************/
#ifdef CAPSENSOR_SPP_SERVER
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

#include "app_spp_uart_debug.h"
#include "string.h"
#include "capsensor_debug_server.h"
#include "hal_trace.h"
#include "tgt_hardware_capsensor.h"
#ifdef CHIP_SUBSYS_SENS
#include "app_sensor_hub.h"
#endif
#include "capsensor_algorithm.h"

#if defined(CHIP_SUBSYS_SENS)
void app_send_spp_capsensor_test_data(uint8_t * ptr, uint16_t len)
{
    //TRACE(2, "%s  send capsensor test %d", __func__, len);
    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_SPP_CAPSENSOR_TEST, ptr, len);
}

static void app_sensor_hub_spp_capsensor_test_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    //TRACE(2, "%s receive sensor hub data len %d", __func__, len);
    capsensor_check_mem_data_flag_set(ptr);
}

static void app_sensor_hub_spp_transmit_capsensor_test_cmd_handler(uint8_t* ptr, uint16_t len)
{
	//TRACE(2, "%s len %d", __func__, len);
	app_core_bridge_send_data_without_waiting_rsp(MCU_SENSOR_HUB_TASK_CMD_SPP_CAPSENSOR_TEST, ptr, len);
}

static void app_sensor_hub_spp_capsensor_test_tx_done_handler(uint16_t cmdCode, uint8_t* ptr, uint16_t len)
{
	//TRACE(2, "%s cmdCode 0x%x", __func__, cmdCode);
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_SPP_CAPSENSOR_TEST,
                                "spp receive no rsp req from sensor",
                                app_sensor_hub_spp_transmit_capsensor_test_cmd_handler,
                                app_sensor_hub_spp_capsensor_test_cmd_received_handler,
                                0,
                                NULL,
                                NULL,
                                app_sensor_hub_spp_capsensor_test_tx_done_handler);
#endif

void spp_server_recv_data_callback(uint8_t *data, uint16_t data_len);
static app_spp_server_callback_t app_spp_server_cb = {
    .spp_server_connection_state_cb = NULL,
    .spp_server_recv_data_cb = spp_server_recv_data_callback,
    .spp_server_tx_done = NULL,
};

static uint32_t capsensor_dump_flag = false;

void capsensor_check_mem_data_flag_set(uint8_t* data)
{
    TRACE(1, "receive capsensor set dump flag %d", *data);
    capsensor_dump_flag = *data;
}

void spp_server_recv_data_callback(uint8_t *data, uint16_t data_len)
{
    // TRACE(0, "spp_server_recv_data_callback");
    if(data_len < 16)
    {
        return;
    }
    if(memcmp(data, "CAPSENSOR_DEBUG", 15) == 0)
    {
        if(data[15] == 0x01)
        {
            //open debug
            capsensor_dump_flag = 1;
        }
        else
        {
            //close debug
            capsensor_dump_flag = 0;
        }
    }
}

char cap_send_data[128];
void capsensor_spp_send_data(uint8_t *data, uint16_t data_len)
{
    //report format: "CAPSENSOR" + len(2bytes) + data
    memcpy(cap_send_data, "CAPSENSOR", 9);
    cap_send_data[9] = data_len & 0xFF;
    cap_send_data[10] = (data_len>>8) & 0xFF;
    memcpy(&cap_send_data[11], data, data_len);
#ifndef CHIP_SUBSYS_SENS
    app_spp_server_send_data((uint8_t*)cap_send_data, 11+data_len);
#else
    app_send_spp_capsensor_test_data((uint8_t*)cap_send_data, 11+data_len);
#endif
}

void capsensor_check_mem_data(struct capsensor_sample_data * data, int len, uint8_t m_touch_event)
{
    uint8_t dataL = len;
    uint8_t i = 0;
    uint8_t j = 0;
    uint32_t adcin[CAP_CHNUM*CAP_REPNUM];
    uint32_t chan_num[CAP_CHNUM];
    uint32_t chan_data_sum[CAP_CHNUM];
    uint32_t chan_and_data[CAP_CHNUM*2+6];

#if defined(CAPSENSOR_WEAR)
    int app_para[5] = {0};

    capsensor_get_app_para(&app_para[0], &app_para[1], &app_para[2], &app_para[3], &app_para[4]);
#endif

    for (i = 0; i < dataL; i++)
    {
        adcin[i] = data[i].sdm;
    }

    for (i = 0; i < CAP_CHNUM; i++)
    {
#if (CHIP_CAPSENSOR_VER < 1)
        chan_num[i] = data[2*i].ch;
#else
        chan_num[i] = data[i].ch;
#endif
    }

    for (i = 0; i < CAP_CHNUM; i++) { //cap_chNum
        chan_data_sum[i] = 0;   // clear channel i data sum.

        for (j = 0; j < CAP_REPNUM; j++) { //cap_repNum
            chan_data_sum[i] = chan_data_sum[i] + adcin[i*CAP_REPNUM + j];
        }
    }

    for(i = 0; i < CAP_CHNUM; i++)
    {
        chan_and_data[2*i] = chan_num[i];
        chan_and_data[2*i+1] = chan_data_sum[i];
    }

    for(i = 0; i < 5; i++)
    {
#if defined(CAPSENSOR_WEAR)
        chan_and_data[i+CAP_CHNUM*2] = app_para[i];
#else
        chan_and_data[i+CAP_CHNUM*2] = 0;
#endif
    }

    chan_and_data[CAP_CHNUM*2 + 5] = m_touch_event;

    // for(i = 0; i < CAP_CHNUM*2+6; i++)
    // {
    //     TRACE(0, "chan_and_data[%d]=%d", i, chan_and_data[i]);
    // }

    if(capsensor_dump_flag)
    {
        // TRACE(0, "capsensor_dump_flag =1, app_tota_send_data");
        capsensor_spp_send_data((uint8_t *)chan_and_data, CAP_CHNUM*8 + 24);
    }
}

void app_spp_capsensor_server(void)
{
    app_spp_server_init(&app_spp_server_cb);
    TRACE(0, "app_spp_server_init success");
}

#endif