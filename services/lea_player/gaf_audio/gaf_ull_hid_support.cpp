/**

 * @copyright Copyright (c) 2015-2022 BES Technic.
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
 */
 

#ifdef HID_ULL_ENABLE

#include "app_utils.h"
#include "app_gaf_dbg.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "app_audio.h"
#include "rwble_config.h"
#include "hal_usb.h"
#include "usb_audio.h"
#include "gaf_media_sync.h"
#include "gaf_media_common.h"
#include "app_bap_data_path_itf.h"
#include "gaf_media_stream.h"
#include "app_gaf_custom_api.h"
#include "bluetooth_ble_api.h"
#include "app_bt_func.h"
#include "app_audio.h"
#include "gaf_media_common.h"

static uint8_t sensor_simulate_data = 0;


static void app_sample_code_test_gpio_irq_handler(enum HAL_GPIO_PIN_T pin)
{
    if ( pin == HAL_GPIO_PIN_P2_0)
    {
    sensor_simulate_data++;
    TRACE(0, "GPIO %u IRQ happes!", pin);
    }
    if (sensor_simulate_data == 1) {
        hal_iomux_tportset(0);
    }
    if ( pin == HAL_GPIO_PIN_P2_1) {
    sensor_simulate_data = 0;
    TRACE(0, "GPIO %u IRQ happes!", pin);
    }
}
static void app_sample_code_gpio_wakeup_demo_handler(enum HAL_IOMUX_PIN_T ull_pin)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP test_pin[] =
    {
        {
            .pin = ull_pin,
            .function = HAL_IOMUX_FUNC_AS_GPIO,
            .volt = HAL_IOMUX_PIN_VOLTAGE_VIO,
            .pull_sel = HAL_IOMUX_PIN_NOPULL,
        },
    };

    hal_iomux_init(&test_pin[0], sizeof(test_pin)/sizeof(HAL_IOMUX_PIN_FUNCTION_MAP));

    const struct HAL_GPIO_IRQ_CFG_T test_gpio_cfg =
    {
        .irq_enable = true,
        .irq_debounce = true,
        .irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE,
        .irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING,
        .irq_handler = app_sample_code_test_gpio_irq_handler,
    };
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)ull_pin, HAL_GPIO_DIR_IN, 0);
    hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)ull_pin, &test_gpio_cfg);
}

uint8_t gaf_ull_get_sensor_data(uint8_t con_lid)
{
    app_sample_code_gpio_wakeup_demo_handler(HAL_IOMUX_PIN_P2_0);
    app_sample_code_gpio_wakeup_demo_handler(HAL_IOMUX_PIN_P2_1);
    return sensor_simulate_data;
}

#ifdef AOB_MOBILE_ENABLED
void ull_hid_epint_in_send_report(uint8_t* cmdParam, uint32_t cmdParam_len)
{
    hid_epint_in_send_report(cmdParam, cmdParam_len);
}

static void gaf_ULL_stream_receive_data(uint16_t conhdl, GAF_ISO_PKT_STATUS_E pkt_status)
{
    dp_itf_iso_buffer_t* decoder_frame_info = NULL;
    dp_itf_iso_buffer_t p_sdu_buf;
    uint8_t usb_hid_test[40] = {0};
    while ((decoder_frame_info = (dp_itf_iso_buffer_t *)bes_ble_bap_dp_itf_get_rx_data(conhdl, NULL)))
    {
        p_sdu_buf = *decoder_frame_info;
        LOG_I("gaf_ULL_stream_receive_data:");
        app_bap_dp_itf_rx_data_done(conhdl, p_sdu_buf.sdu_length,
            p_sdu_buf.time_stamp, p_sdu_buf.sdu);
        DUMP8("%02x ", p_sdu_buf.sdu, p_sdu_buf.sdu_length);
        memcpy(usb_hid_test, p_sdu_buf.sdu, p_sdu_buf.sdu_length);
#ifdef IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
        if (usb_hid_test[0] == 1) {
            hal_iomux_tportset(0);
        }
        if (usb_hid_test[0] == 2) {
            hal_iomux_tportset(1);
        }
#endif
        ull_hid_epint_in_send_report(usb_hid_test, (uint32_t)p_sdu_buf.sdu_length);
        gaf_stream_data_free(decoder_frame_info);
#ifdef IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
        hal_iomux_tportclr(0);
        hal_iomux_tportclr(1);
#endif
    }
}

static void gaf_ull_mobile_audio_stream_start(app_bap_ascc_ase_t *p_bap_ase_info)
{
    LOG_I("%s start", __func__);
    bes_ble_bap_dp_itf_data_come_callback_register((void *)gaf_ULL_stream_receive_data);
}

void gaf_mobile_ull_stream_stop_handler(uint8_t ase_lid)
{
    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_32K);
    LOG_I("gaf_ULL_mobile_audio_stream_stop");
}

void gaf_mobile_ull_stream_start_handler(uint8_t ase_lid)
{
    app_bap_ascc_ase_t *p_bap_ase_info = app_bap_uc_cli_get_ase_info(ase_lid);
    uint32_t audioCacheHeapSize = 1024;
    uint8_t* heapBufStartAddr = NULL;

    app_sysfreq_req(APP_SYSFREQ_USER_AOB_PLAYBACK, APP_SYSFREQ_208M);
    app_audio_mempool_get_buff(&heapBufStartAddr, audioCacheHeapSize);
    gaf_stream_heap_init(heapBufStartAddr, audioCacheHeapSize);
    gaf_ull_mobile_audio_stream_start(p_bap_ase_info);
}

extern "C"{
void gaf_mobile_ull_stream_update_and_start_handler(uint8_t ase_lid)
{
    LOG_I("%s", __func__);
    app_bt_start_custom_function_in_bt_thread((uint32_t)ase_lid,
                                              0,
                                              (uint32_t)gaf_mobile_ull_stream_start_handler);
}

void gaf_mobile_ull_stream_update_and_stop_handler(uint8_t ase_lid)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)ase_lid,
                                              0,
                                              (uint32_t)gaf_mobile_ull_stream_stop_handler);
}
}

#endif
#endif
