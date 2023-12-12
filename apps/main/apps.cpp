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
#include "stdio.h"
#include "cmsis_os.h"
#include "list.h"
#include "string.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_bootmode.h"
#include "hal_sleep.h"
#include "pmu.h"
#include "audioflinger.h"
#include "apps.h"
#include "app_thread.h"
#include "app_key.h"
#include "bluetooth_bt_api.h"
#include "app_bt_media_manager.h"
#include "app_pwl.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_battery.h"
#include "app_trace_rx.h"
#include "app_utils.h"
#include "app_status_ind.h"
#include "bt_drv_interface.h"
#include "besbt.h"
#include "norflash_api.h"
#include "nvrecord_appmode.h"
#include "nvrecord_bt.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "crash_dump_section.h"
#include "log_section.h"
#include "factory_section.h"
#include "a2dp_api.h"
#include "me_api.h"
#include "btapp.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "bt_if.h"
#ifdef APP_CHIP_BRIDGE_MODULE
#include "app_chip_bridge.h"
#endif
#ifdef CAPSENSOR_ENABLE
#include "app_capsensor.h"
#ifdef CAPSENSOR_SPP_SERVER
#include "capsensor_debug_server.h"
#endif
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "bluetooth_ble_api.h"
#endif

#ifdef BLE_BIS_TRANSPORT
#include "app_bis_transport.h"
#endif

#include "app_bt_func.h"
#include "app_bt_cmd.h"
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#endif
#include "audio_process.h"

#if defined(BT_WATCH_MASTER) || defined(BT_WATCH_SLAVE)
#include "app_bt_watch.h"
#endif

#ifdef __PC_CMD_UART__
#include "app_cmd.h"
#endif

#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#include "app_factory_bt.h"
#endif

#ifdef __INTERCONNECTION__
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_ble.h"
#endif

#ifdef __INTERACTION__
#include "app_interaction.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom_reset.h"
#include "nvrecord_gsound.h"
#include "gsound_custom_actions.h"
#include "gsound_custom_ota.h"
#endif

#ifdef PROMPT_IN_FLASH
#include "nvrecord_prompt.h"
#endif

#ifdef OTA_ENABLE
#include "ota_basic.h"
#endif

#ifdef BES_OTA
#include "ota_bes.h"
#include "ota_control.h"
#include "ota_config.h"
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#include "bluetooth_bt_api.h"
#include "app_media_player.h"
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif

#ifdef TILE_DATAPATH
#include "tile_target_ble.h"
#endif

#if defined(IBRT)
#include "app_ibrt_internal.h"
#include "app_ibrt_customif_ui.h"
#include "app_ibrt_voice_report.h"

#if defined(IBRT_UI)
#include "app_tws_ibrt_ui_test.h"
#include "app_ibrt_tws_ext_cmd.h"
#include "app_tws_ibrt_conn_api.h"
#include "app_custom_api.h"
#include "app_ibrt_auto_test.h"
#include "app_tws_ctrl_thread.h"
#endif
#include "earbud_ux_api.h"
#endif

#ifdef ANC_APP
#include "app_anc.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef AI_OTA
#include "nvrecord_ota.h"
#include "ota_common.h"
#endif

#include "watchdog/watchdog.h"
#ifdef VOICE_DETECTOR_EN
#include "app_voice_detector.h"
#endif

#ifdef IBRT_UI
#include "app_ui_api.h"
#endif

#ifdef APP_SOUND_ENABLE
#include "SoundApi.h"
#endif
#ifdef SENSOR_HUB
#include "mcu_sensor_hub_app.h"
#endif

#ifdef DSP_M55
#include "mcu_dsp_m55_app.h"
#endif

#ifdef DSP_HIFI4
#include "mcu_dsp_hifi4_app.h"
#endif

#if defined(ESHELL_ON)
#include "eshell.h"
#endif

#if defined(ANC_ASSIST_ENABLED)
#include "app_anc_assist.h"
#include "app_voice_assist_ai_voice.h"
#include "app_voice_assist_wd.h"
#include "app_voice_assist_pilot_anc.h"
#include "app_voice_assist_ultrasound.h"
#include "app_voice_assist_custom_leak_detect.h"
#include "app_voice_assist_prompt_leak_detect.h"
#include "app_voice_assist_noise_adapt_anc.h"
#include "app_voice_assist_anc.h"
#if defined(VOICE_ASSIST_FF_FIR_LMS)
#include "app_voice_assist_fir_lms.h"
#include "app_voice_assist_fir_anc_open_leak.h"
#endif
#include "app_voice_assist_noise_classify_adapt_anc.h"
#include "app_voice_assist_wsd.h"
#if defined(VOICE_ASSIST_ONESHOT_ADAPTIVE_ANC)
#include "app_voice_assist_oneshot_adapt_anc.h"
#endif
#if defined(AUDIO_ADAPTIVE_EQ)
#include "app_voice_assist_adaptive_eq.h"
#endif
#if defined(AUDIO_ADJ_EQ)
#include "app_voice_assist_adj_eq.h"
#endif
#include "app_voice_assist_pnc_adapt_anc.h"
#include "app_voice_assist_optimal_tf.h"
#endif

#if defined(AUDIO_OUTPUT_DC_AUTO_CALIB) || defined(AUDIO_ADC_DC_AUTO_CALIB)
#include "codec_calib.h"
#endif

#ifdef GFPS_ENABLED
#include "gfps.h"
#endif

#ifdef DMA_AUDIO_APP
#include "dma_audio_cli.h"
#endif

#ifdef AUDIO_DEBUG
extern "C" int speech_tuning_init(void);
#endif

#ifdef AUDIO_DEBUG_CMD
extern "C" int32_t audio_test_cmd_init(void);
#endif

#ifdef AUDIO_HEARING_COMPSATN
extern "C" int32_t hearing_det_cmd_init(void);
#endif

#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
extern "C" bool app_usbaudio_mode_on(void);
#endif

#ifdef VOICE_DEV
extern "C" int32_t voice_dev_init(void);
#endif

#ifdef APP_MCPP_CLI
extern "C" void app_mcpp_init(void);
#endif

#ifdef APP_MCPP_SRV
extern "C" void mcpp_srv_open(void);
#endif

#ifdef APP_UART_MODULE
#include "app_uart_dma_thread.h"
#endif

#ifdef MSD_MODE
#include "sdmmc_msd.h"
#endif
#ifdef BLE_WALKIE_TALKIE
#include "walkie_talkie_test.h"
#include "app_walkie_talkie.h"
#endif
#if BLE_AUDIO_ENABLED
#include "gaf_media_stream.h"
#endif
#ifdef BLE_WIFI_DUAL_CHIP_MODE
#include "app_ble_wifi_communication.h"
#endif

#ifdef WIRELESS_MIC
#include "app_wireless_mic.h"
#endif

#ifdef __LVGL_EN__
extern "C" int ui_os_init(void);
#endif

#include "audio_player_adapter.h"

#define APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD (1)
#define POWERON_PRESSMAXTIME_THRESHOLD_MS  (5000)

#ifdef FPGA
uint32_t __ota_upgrade_log_start[100];
#endif

enum APP_POWERON_CASE_T {
    APP_POWERON_CASE_NORMAL = 0,
    APP_POWERON_CASE_DITHERING,
    APP_POWERON_CASE_REBOOT,
    APP_POWERON_CASE_ALARM,
    APP_POWERON_CASE_CALIB,
    APP_POWERON_CASE_BOTHSCAN,
    APP_POWERON_CASE_CHARGING,
    APP_POWERON_CASE_FACTORY,
    APP_POWERON_CASE_TEST,
    APP_POWERON_CASE_INVALID,

    APP_POWERON_CASE_NUM
};

#ifdef RB_CODEC
extern int rb_ctl_init();
extern bool rb_ctl_is_init_done(void);
extern void app_rbplay_audio_reset_pause_status(void);
#endif

uint8_t  app_poweroff_flag = 0;
static enum APP_POWERON_CASE_T g_pwron_case = APP_POWERON_CASE_INVALID;

#ifndef APP_TEST_MODE
static uint8_t app_status_indication_init(void)
{
    struct APP_PWL_CFG_T cfg;
    memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
    app_pwl_open();
    app_pwl_setup(APP_PWL_ID_0, &cfg);
    app_pwl_setup(APP_PWL_ID_1, &cfg);
    return 0;
}
#endif

#if defined(__BTIF_EARPHONE__)

typedef void (*APP_10_SECOND_TIMER_CB_T)(void);

void app_pair_timerout(void);
void app_poweroff_timerout(void);
void CloseEarphone(void);

typedef struct
{
    uint8_t timer_id;
    uint8_t timer_en;
    uint8_t timer_count;
    uint8_t timer_period;
    APP_10_SECOND_TIMER_CB_T cb;
}APP_10_SECOND_TIMER_STRUCT;

#define INIT_APP_TIMER(_id, _en, _count, _period, _cb) \
    { \
        .timer_id = _id, \
        .timer_en = _en, \
        .timer_count = _count, \
        .timer_period = _period, \
        .cb = _cb, \
    }

void app_enter_fastpairing_mode(void)
{
#ifdef GFPS_ENABLED
    gfps_enter_fastpairing_mode();
    app_start_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);
#endif
}

void app_exit_fastpairing_mode(void)
{
#ifdef GFPS_ENABLED
    if (gfps_is_in_fastpairing_mode())
    {
        TRACE(0,"[FP]exit fast pair mode");
        app_stop_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);
        gfps_exit_fastpairing_mode();
    }
#endif
}

void app_fast_pairing_timeout_timehandler(void)
{
    app_exit_fastpairing_mode();
}

APP_10_SECOND_TIMER_STRUCT app_10_second_array[] =
{
    INIT_APP_TIMER(APP_PAIR_TIMER_ID, 0, 0, 6, bes_bt_me_transfer_pairing_to_connectable),
    INIT_APP_TIMER(APP_POWEROFF_TIMER_ID, 0, 0, 90, CloseEarphone),
#ifdef GFPS_ENABLED
    INIT_APP_TIMER(APP_FASTPAIR_LASTING_TIMER_ID, 0, 0, APP_FAST_PAIRING_TIMEOUT_IN_SECOND/10,
        app_fast_pairing_timeout_timehandler),
#endif
};

void app_stop_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = 0;
    timer->timer_count = 0;
}

void app_start_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = 1;
    timer->timer_count = 0;
}

void app_set_10_second_timer(uint8_t timer_id, uint8_t enable, uint8_t period)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = enable;
    timer->timer_count = period;
}

void app_10_second_timer_check(void)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(app_10_second_array); i++) {
        if (timer->timer_en) {
            timer->timer_count++;
            if (timer->timer_count >= timer->timer_period) {
                timer->timer_en = 0;
                if (timer->cb)
                    timer->cb();
            }
        }
        timer++;
    }
}

void CloseEarphone(void)
{
#ifdef SLIM_BTC_ONLY

#else

#ifdef ANC_APP
    if(app_anc_work_status()) {
        app_set_10_second_timer(APP_POWEROFF_TIMER_ID, 1, 30);
        return;
    }
#endif /* ANC_APP */

#ifndef BLE_ONLY_ENABLED
    int activeCons = 0;
    int activeSourceCons = 0;

    activeCons = app_bt_get_active_cons();
    activeSourceCons = btif_me_get_source_activeCons();

    if(activeCons == 0 && activeSourceCons == 0) {
        TRACE(0,"!!!CloseEarphone\n");
        app_shutdown();
    }
#endif /* BLE_ONLY_ENABLED */
#endif
}
#endif /* #if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__) */

extern "C" int signal_send_to_main_thread(uint32_t signals);
static uint8_t stack_ready_flag = 0;
void app_notify_stack_ready(uint8_t ready_flag)
{
    TRACE(2,"app_notify_stack_ready %d %d", stack_ready_flag, ready_flag);

    stack_ready_flag |= ready_flag;

#ifdef __IAG_BLE_INCLUDE__
    if(stack_ready_flag == (STACK_READY_BT|STACK_READY_BLE))
#endif
    {
        signal_send_to_main_thread(STACK_READY_BLE | STACK_READY_BT);
    }
}

bool app_is_stack_ready(void)
{
    bool ret = false;

    if (stack_ready_flag == (STACK_READY_BT
#ifdef __IAG_BLE_INCLUDE__
                             | STACK_READY_BLE
#endif
                             ))
    {
        ret = true;
    }

    return ret;
}

__attribute__((weak)) void ble_uart_cmd_init(void)
{

}

extern "C" WEAK void app_debug_tool_printf(const char *fmt, ...)
{

}

static void app_stack_ready_cb(void)
{
    TRACE(0,"stack init done");
}

#ifdef MEDIA_PLAYER_SUPPORT

void app_status_set_num(const char* p)
{
    media_Set_IncomingNumber(p);
}

int app_voice_stop(APP_STATUS_INDICATION_T status, uint8_t device_id)
{
    AUD_ID_ENUM id = MAX_RECORD_NUM;

    TRACE(2,"%s %d", __func__, status);

    if (status == APP_STATUS_INDICATION_FIND_MY_BUDS)
        id = AUDIO_ID_FIND_MY_BUDS;

    if (id != MAX_RECORD_NUM)
        trigger_media_stop(id, device_id);

    return 0;
}
#endif

static void app_poweron_normal(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    g_pwron_case = APP_POWERON_CASE_NORMAL;

    signal_send_to_main_thread(0x2);
}

#if !defined(BLE_ONLY_ENABLED)
static void app_poweron_scan(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    g_pwron_case = APP_POWERON_CASE_BOTHSCAN;

    signal_send_to_main_thread(0x2);
}
#endif

#ifdef __ENGINEER_MODE_SUPPORT__
#if !defined(BLE_ONLY_ENABLED)
static void app_poweron_factorymode(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_factorymode_enter();
}
#endif
#endif

void app_enter_signalingtest_mode(void)
{
    TRACE(0, "APP: enter signaling test");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE|HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
    app_reset();
}

void app_enter_non_signalingtest_mode(void)
{
    TRACE(0, "APP: enter non-signaling test");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE|HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
    app_reset();
}

static bool g_pwron_finished = false;
static void app_poweron_finished(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    g_pwron_finished = true;
    signal_send_to_main_thread(0x2);
}

void app_poweron_wait_finished(void)
{
    if (!g_pwron_finished){
        osSignalWait(0x2, osWaitForever);
    }
}

#if  defined(__POWERKEY_CTRL_ONOFF_ONLY__)
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},           "power on: shutdown"     , app_bt_key_shutdown, NULL},
};
#elif defined(__ENGINEER_MODE_SUPPORT__)
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
#if !defined(BLE_ONLY_ENABLED)
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGLONGPRESS},"power on: factory mode", app_poweron_factorymode  , NULL},
#endif
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#else
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#endif

#ifndef APP_TEST_MODE
static void app_poweron_key_init(void)
{
    uint8_t i = 0;
    TRACE(1,"%s",__func__);

    for (i = 0; i< ARRAY_SIZE(pwron_key_handle_cfg); i++){
        app_key_handle_registration(&pwron_key_handle_cfg[i]);
    }
}

static uint8_t app_poweron_wait_case(void)
{
    uint32_t stime = 0, etime = 0;

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    g_pwron_case = APP_POWERON_CASE_NORMAL;
#else
    TRACE(1,"poweron_wait_case enter:%d", g_pwron_case);
    if (g_pwron_case == APP_POWERON_CASE_INVALID){
        stime = hal_sys_timer_get();
        osSignalWait(0x2, POWERON_PRESSMAXTIME_THRESHOLD_MS);
        etime = hal_sys_timer_get();
    }
    TRACE(2,"powon raw case:%d time:%d", g_pwron_case, TICKS_TO_MS(etime - stime));
#endif
    return g_pwron_case;

}
#endif

static void POSSIBLY_UNUSED app_wait_stack_ready(void)
{
    uint32_t stime, etime;
    stime = hal_sys_timer_get();
    osSignalWait(0x3, osWaitForever);
    etime = hal_sys_timer_get();
    TRACE(1,"app_wait_stack_ready: wait:%d ms", TICKS_TO_MS(etime - stime));
    app_stack_ready_cb();
}

extern "C" int system_shutdown(void);
extern "C" int system_reset(void);

int app_shutdown(void)
{
    system_shutdown();
    return 0;
}

int app_reset(void)
{
    TRACE(0, "Retention bit when reset happens is 0x%x", hal_sw_bootmode_get());
    system_reset();
    return 0;
}

static void app_postponed_reset_timer_handler(void const *param);
osTimerDef(APP_POSTPONED_RESET_TIMER, app_postponed_reset_timer_handler);
static osTimerId app_postponed_reset_timer = NULL;
#define APP_RESET_PONTPONED_TIME_IN_MS  2000
static void app_postponed_reset_timer_handler(void const *param)
{
    app_reset();
}

void app_start_postponed_reset(void)
{
    if (NULL == app_postponed_reset_timer)
    {
        app_postponed_reset_timer = osTimerCreate(osTimer(APP_POSTPONED_RESET_TIMER), osTimerOnce, NULL);
    }

#ifndef FLASH_REMAP
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
#endif
    osTimerStart(app_postponed_reset_timer, APP_RESET_PONTPONED_TIME_IN_MS);
}
#ifdef PROMPT_IN_FLASH
void app_start_ota_language_reset(void)
{
    if (NULL == app_postponed_reset_timer)
    {
        app_postponed_reset_timer = osTimerCreate(osTimer(APP_POSTPONED_RESET_TIMER), osTimerOnce, NULL);
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);

    osTimerStart(app_postponed_reset_timer, APP_RESET_PONTPONED_TIME_IN_MS);
}
#endif

void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_reset();
#else
    app_shutdown();
#endif
}

void app_bt_key_enter_testmode(APP_KEY_STATUS *status, void *param)
{
    TRACE(1,"%s\n",__FUNCTION__);

    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__
        app_factorymode_bt_signalingtest(status, param);
#endif
    }
}

void app_bt_key_enter_nosignal_mode(APP_KEY_STATUS *status, void *param)
{
    TRACE(1,"%s\n",__FUNCTION__);
    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__
        app_factorymode_bt_nosignalingtest(status, param);
#endif
    }
}

#define PRESS_KEY_TO_ENTER_OTA_INTERVEL    (15000)          // press key 15s enter to ota
#define PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT    ((PRESS_KEY_TO_ENTER_OTA_INTERVEL - 2000) / 500)
void app_otaMode_enter(APP_KEY_STATUS *status, void *param)
{
    TRACE(1,"%s",__func__);

    hal_norflash_disable_protection(HAL_FLASH_ID_0);

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
#ifdef __KMATE106__
    app_status_indication_set(APP_STATUS_INDICATION_OTA);
    audio_player_play_prompt(AUD_ID_BT_WARNING, 0);
    osDelay(1200);
#endif
    pmu_reboot();
}

extern "C" void sys_otaMode_enter()
{
    app_otaMode_enter(NULL,NULL);
}

#ifdef __USB_COMM__
void app_usb_cdc_comm_key_handler(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d", __func__, status->code, status->event);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_CDC_COMM);
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#endif

void app_dfu_key_handler(APP_KEY_STATUS *status, void *param)
{
    TRACE(1,"%s ",__func__);
    hal_sw_bootmode_clear(0xffffffff);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD | HAL_SW_BOOTMODE_SKIP_FLASH_BOOT);
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}

void app_ota_key_handler(APP_KEY_STATUS *status, void *param)
{
    static uint32_t time = hal_sys_timer_get();
    static uint16_t cnt = 0;

    TRACE(3,"%s %d,%d",__func__, status->code, status->event);

    if (TICKS_TO_MS(hal_sys_timer_get() - time) > 600) // 600 = (repeat key intervel)500 + (margin)100
        cnt = 0;
    else
        cnt++;

    if (cnt == PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT) {
        app_otaMode_enter(NULL, NULL);
    }

    time = hal_sys_timer_get();
}
extern "C" void app_bt_key(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
#define DEBUG_CODE_USE 0
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            TRACE(0,"first blood!");
#if DEBUG_CODE_USE
            if (status->code == APP_KEY_CODE_PWR)
            {
#ifdef __INTERCONNECTION__
                // add favorite music
                // app_interconnection_handle_favorite_music_through_ccmp(1);

                // ask for ota update
                ota_update_request();
                return;
#else
                static int m = 0;
                if (m == 0) {
                    m = 1;
                    hal_iomux_set_analog_i2c();
                }
                else {
                    m = 0;
                    hal_iomux_set_uart0();
                }
#endif
            }
#endif
            break;
        case APP_KEY_EVENT_DOUBLECLICK:
            TRACE(0,"double kill");
#if DEBUG_CODE_USE
            if (status->code == APP_KEY_CODE_PWR)
            {
#ifdef __INTERCONNECTION__
                // play favorite music
                app_interconnection_handle_favorite_music_through_ccmp(2);
#else
                app_otaMode_enter(NULL, NULL);
#endif
                return;
            }
#endif
			break;
        case APP_KEY_EVENT_TRIPLECLICK:
            TRACE(0,"triple kill");
            if (status->code == APP_KEY_CODE_PWR)
            {
				if(bes_bt_me_count_mobile_link() < BT_DEVICE_NUM){
                    bes_bt_me_write_access_mode(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR,1);
#ifdef __INTERCONNECTION__
	                app_interceonnection_start_discoverable_adv(INTERCONNECTION_BLE_FAST_ADVERTISING_INTERVAL,
	                                                            APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
	                return;
#endif
#ifdef GFPS_ENABLED
	                app_enter_fastpairing_mode();
#endif
#ifdef MEDIA_PLAYER_SUPPORT
                    audio_player_play_prompt(AUD_ID_BT_PAIR_ENABLE, 0);
#endif
                }
                return;
            }
            break;
        case APP_KEY_EVENT_ULTRACLICK:
            TRACE(0,"ultra kill");
            break;
        case APP_KEY_EVENT_RAMPAGECLICK:
            TRACE(0,"rampage kill!you are crazy!");
            break;

        case APP_KEY_EVENT_UP:
            break;
    }
#ifdef __FACTORY_MODE_SUPPORT__
    if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN && (status->event == APP_KEY_EVENT_DOUBLECLICK)){
        app_factorymode_languageswitch_proc();
    }else
#endif
    {
        bt_key_send(status);
    }
}

#ifdef RB_CODEC
extern bool app_rbcodec_check_hfp_active(void );
void app_switch_player_key(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);

    if(!rb_ctl_is_init_done()) {
        TRACE(0,"rb ctl not init done");
        return ;
    }

    if( app_rbcodec_check_hfp_active() ) {
        app_bt_key(status,param);
        return;
    }

    app_rbplay_audio_reset_pause_status();

    if(app_rbplay_mode_switch()) {
        media_PlayAudio(AUD_ID_POWER_ON, 0);
        app_rbcodec_ctr_play_onoff(true);
    } else {
        app_rbcodec_ctr_play_onoff(false);
        media_PlayAudio(AUD_ID_POWER_OFF, 0);
    }
    return ;

}
#endif

void app_voice_assistant_key(APP_KEY_STATUS *status, void *param)
{
    TRACE(2,"%s event %d", __func__, status->event);
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
    if (app_ai_manager_is_in_multi_ai_mode())
    {
        if (app_ai_manager_spec_get_status_is_in_invalid()) {
            TRACE(0,"AI feature has been diabled");
            return;
        }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
        if (app_ai_manager_get_spec_update_flag()) {
            TRACE(0,"device reboot is ongoing...");
            return;
        }
#endif

        if (app_ai_manager_is_need_reboot())
        {
            TRACE(1, "%s ai need to reboot", __func__);
            return;
        }

        if(app_ai_manager_voicekey_is_enable()) {
            if (AI_SPEC_GSOUND == app_ai_manager_get_current_spec()) {
#ifdef BISTO_ENABLED
	            gsound_custom_actions_handle_key(status, param);
#endif
            } else if(AI_SPEC_INIT != app_ai_manager_get_current_spec()) {
                app_ai_key_event_handle(status, 0);
            }
        }
    }
    else
    {
	    app_ai_key_event_handle(status, 0);
#ifdef BISTO_ENABLED
	    gsound_custom_actions_handle_key(status, param);
#endif
    }
#endif
}

#ifdef IS_MULTI_AI_ENABLED
void app_voice_gva_onoff_key(APP_KEY_STATUS *status, void *param)
{
    uint8_t current_ai_spec = app_ai_manager_get_current_spec();

    TRACE(2,"%s current_ai_spec %d", __func__, current_ai_spec);
    if (current_ai_spec == AI_SPEC_INIT)
    {
        app_ai_manager_enable(true, AI_SPEC_GSOUND);
    }
    else if(current_ai_spec == AI_SPEC_GSOUND)
    {
        app_ai_manager_enable(false, AI_SPEC_GSOUND);
    }
    else if(current_ai_spec == AI_SPEC_AMA)
    {
        app_ai_manager_switch_spec(AI_SPEC_GSOUND);
    }
    bes_ble_gap_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_voice_ama_onoff_key(APP_KEY_STATUS *status, void *param)
{
    uint8_t current_ai_spec = app_ai_manager_get_current_spec();

    TRACE(2,"%s current_ai_spec %d", __func__, current_ai_spec);
    if (current_ai_spec == AI_SPEC_INIT)
    {
        app_ai_manager_enable(true, AI_SPEC_AMA);
    }
    else if(current_ai_spec == AI_SPEC_AMA)
    {
        app_ai_manager_enable(false, AI_SPEC_AMA);
    }
    else if(current_ai_spec == AI_SPEC_GSOUND)
    {
        app_ai_manager_switch_spec(AI_SPEC_AMA);
    }
    bes_ble_gap_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}
#endif

#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
extern "C" void test_btusb_switch(void);
void app_btusb_audio_dual_mode_test(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"test_btusb_switch");
    test_btusb_switch();
}
#endif

extern void switch_dualmic_status(void);

void app_switch_dualmic_key(APP_KEY_STATUS *status, void *param)
{
    switch_dualmic_status();
}

#if defined(ANC_APP)
void app_anc_key(APP_KEY_STATUS *status, void *param)
{
    app_anc_loop_switch();
}
#endif

#ifdef POWERKEY_I2C_SWITCH
extern void app_factorymode_i2c_switch(APP_KEY_STATUS *status, void *param);
#endif

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#if defined(__APP_KEY_FN_STYLE_A__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#endif
#else
#if defined(__APP_KEY_FN_STYLE_A__)
//--
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
#ifdef RB_CODEC
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_switch_player_key, NULL},
#else
    //{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"btusb mode switch key.",app_btusb_audio_dual_mode_test, NULL},
#endif
#endif
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
#if RAMPAGECLICK_TEST_MODE
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},
#endif
#ifdef POWERKEY_I2C_SWITCH
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt i2c key",app_factorymode_i2c_switch, NULL},
#endif
    //{{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
#if defined(BT_SOURCE)
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt mode src snk key",app_bt_key, NULL},
#endif
    //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN15,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},

#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
#if defined( __BT_ANC_KEY__)&&defined(ANC_APP)
	{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt anc key",app_anc_key, NULL},
#endif
#if defined(VOICE_DATAPATH) || defined(__AI_VOICE__)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN}, "google assistant key", app_voice_assistant_key, NULL},
#if defined(IS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED) || defined(PUSH_AND_HOLD_ENABLED) || defined(__TENCENT_VOICE__)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP}, "google assistant key", app_voice_assistant_key, NULL},
#endif
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP_AFTER_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK}, "google assistant key", app_voice_assistant_key, NULL},
#endif
#ifdef IS_MULTI_AI_ENABLED
    {{APP_KEY_CODE_FN13, APP_KEY_EVENT_CLICK}, "gva on-off key", app_voice_gva_onoff_key, NULL},
    {{APP_KEY_CODE_FN14, APP_KEY_EVENT_CLICK}, "ama on-off key", app_voice_ama_onoff_key, NULL},
#endif
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
    {{APP_KEY_CODE_FN15, APP_KEY_EVENT_CLICK}, "btusb mode switch key.", app_btusb_audio_dual_mode_test, NULL},
#endif
};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN}, "google assistant key", app_voice_assistant_key, NULL},
#if defined(IS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED) || defined(PUSH_AND_HOLD_ENABLED)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP}, "google assistant key", app_voice_assistant_key, NULL},
#endif
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP_AFTER_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK}, "google assistant key", app_voice_assistant_key, NULL},
};
#endif
#endif

void app_key_init(void)
{
#if defined(IBRT) && defined(IBRT_UI)
		app_tws_ibrt_raw_ui_test_key_init();
#else
    uint8_t i = 0;
    TRACE(1,"%s",__func__);

    app_key_handle_clear();
    for (i = 0; i < ARRAY_SIZE(app_key_handle_cfg); i++) {
        app_key_handle_registration(&app_key_handle_cfg[i]);
    }
#endif
}
void app_key_init_on_charging(void)
{
#ifdef APP_KEY_ENABLE
    uint8_t i = 0;
    static const APP_KEY_HANDLE  key_cfg[] = {
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_REPEAT},"ota function key",app_ota_key_handler, NULL},
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_dfu_key_handler, NULL},
#ifdef __USB_COMM__
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"usb cdc key",app_usb_cdc_comm_key_handler, NULL},
#endif
    };

    TRACE(1,"%s",__func__);
    for (i = 0; i < ARRAY_SIZE(key_cfg); i++) {
        app_key_handle_registration(&key_cfg[i]);
    }
#endif
}

bool app_is_power_off_in_progress(void)
{
    return app_poweroff_flag?TRUE:FALSE;
}

#if GFPS_ENABLED
#define APP_GFPS_BATTERY_TIMEROUT_VALUE             (10000)
static void app_gfps_battery_show_timeout_timer_cb(void const *n);
osTimerDef (GFPS_BATTERY_SHOW_TIMEOUT_TIMER, app_gfps_battery_show_timeout_timer_cb);
static osTimerId app_gfps_battery_show_timer_id = NULL;
static void app_gfps_battery_show_timeout_timer_cb(void const *n)
{
    TRACE(1,"%s", __func__);
    gfps_set_battery_datatype(HIDE_UI_INDICATION);
}

void app_gfps_battery_show_timer_start()
{
    if (app_gfps_battery_show_timer_id == NULL)
        app_gfps_battery_show_timer_id = osTimerCreate(osTimer(GFPS_BATTERY_SHOW_TIMEOUT_TIMER), osTimerOnce, NULL);
    osTimerStart(app_gfps_battery_show_timer_id, APP_GFPS_BATTERY_TIMEROUT_VALUE);
}

void app_gfps_battery_show_timer_stop()
{
    if (app_gfps_battery_show_timer_id)
        osTimerStop(app_gfps_battery_show_timer_id);
}


enum GFPS_FIND_BUDS_STATUS {
    GFPS_MUSIC_OFF_FIND_OFF,
    GFPS_MUSIC_OFF_FIND_ON,
    GFPS_MUSIC_ON_FIND_OFF,
    GFPS_MUSIC_ON_FIND_ON,
};
static void app_gfps_find_mybuds_timer_cb(void const *n);
osTimerDef (GFPS_FIND_MYBUDS_TIMER, app_gfps_find_mybuds_timer_cb);
static osTimerId app_gfps_find_mybuds_timer_id = NULL;
static uint8_t current_gfps_find_state = GFPS_MUSIC_OFF_FIND_OFF;
extern struct BT_DEVICE_T  app_bt_device;
uint8_t gfps_ring_mode = GFPS_RING_MODE_BOTH_OFF;

static void app_gfps_delay_to_start_find_mybuds()
{
    if (app_gfps_find_mybuds_timer_id == NULL)
        app_gfps_find_mybuds_timer_id = osTimerCreate(osTimer(GFPS_FIND_MYBUDS_TIMER), osTimerOnce, NULL);
    osTimerStart(app_gfps_find_mybuds_timer_id, 1000);
}

static void app_gfps_find_mybuds_timer_cb(void const *n)
{
    app_gfps_find_sm(1);
}

void app_gfps_ring_mode_set(uint8_t ring_mode)
{
    TRACE(2, "%s ring_mode:%d", __func__,ring_mode);

    gfps_ring_mode = ring_mode;
}

uint8_t app_gfps_ring_mode_get()
{
    return gfps_ring_mode;
}

void app_voice_start_gfps_find(void)
{
#ifdef MEDIA_PLAYER_SUPPORT
    media_PlayAudio(AUDIO_ID_FIND_TILE, 0);
#endif
}

void app_voice_stop_gfps_find(void)
{
    app_voice_stop(APP_STATUS_INDICATION_TILE_FIND, 0);
}

void app_gfps_find_sm(bool find_on_off)
{
    bool music_on_off = 0;
    osTimerStop(app_gfps_find_mybuds_timer_id);
    TRACE(3, "%s %d %d", __func__, find_on_off, current_gfps_find_state);
    switch(current_gfps_find_state)
    {
        case GFPS_MUSIC_OFF_FIND_OFF:
            if(find_on_off)
            {
                app_voice_start_gfps_find();
                if(music_on_off)
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_ON;
                else
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_ON;
            }
            else
            {
                if(music_on_off)
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_OFF;
				app_voice_stop_gfps_find();
            }
            break;

        case GFPS_MUSIC_OFF_FIND_ON:
            if(find_on_off)
            {
                if(music_on_off)
                {
                    app_voice_stop_gfps_find();
                    app_gfps_delay_to_start_find_mybuds();
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_OFF;//DELAY TO SATRT SM,TO OPEN FIND
                }
                else
                {
                    app_voice_start_gfps_find();
				}
                //app_voice_start_gfps_find();
            }
            else
            {
                app_voice_stop_gfps_find();
                if(music_on_off)
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_OFF;
                else
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_OFF;
            }
            break;

        case GFPS_MUSIC_ON_FIND_OFF:
            if(find_on_off)
            {
                if(music_on_off)
                {
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_ON;
                }
                else
                {
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_ON;
                }
                app_voice_start_gfps_find();
            }
            else
            {
                if(!music_on_off)
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_OFF;
				app_voice_stop_gfps_find();
            }
            break;

        case GFPS_MUSIC_ON_FIND_ON:
            if(find_on_off)
            {
                if(!music_on_off)
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_ON;
                app_voice_start_gfps_find();
            }
            else
            {
                app_voice_stop_gfps_find();
                if(music_on_off)
                {
                    current_gfps_find_state=GFPS_MUSIC_ON_FIND_OFF;
                }
                else
                {
                    current_gfps_find_state=GFPS_MUSIC_OFF_FIND_OFF;
                }
            }
            break;
    }
}

static bool _find_my_buds_peer_status = 0;
void app_set_find_my_buds_peer_status(bool onoff)
{
    //TRACE(3,"get find my buds status %d", find_my_buds_status);

    _find_my_buds_peer_status = onoff;
}
#endif

int app_deinit(int deinit_case)
{
    int nRet = 0;
    TRACE(2,"%s case:%d",__func__, deinit_case);

#if defined(SENSOR_HUB) && !defined(__NuttX__) && !defined(SPEECH_ALGO_DSP_SENS)
    app_sensor_hub_deinit();
#endif

#ifdef __PC_CMD_UART__
    app_cmd_close();
#endif
#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
    if(app_usbaudio_mode_on())
    {
        return 0;
    }
#endif
    if (!deinit_case){
#ifdef FREEMAN_ENABLED_STERO
        app_ibrt_if_event_entry(APP_UI_EV_CASE_CLOSE);
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        app_prompt_flush_pending_prompts();
#endif
#if defined(ANC_APP)
        app_anc_deinit();
#endif
#if defined(ANC_ASSIST_ENABLED)
        app_anc_assist_deinit();
#endif
        app_poweroff_flag = 1;
#if defined(APP_LINEIN_A2DP_SOURCE)
        app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
#if defined(APP_I2S_A2DP_SOURCE)
        app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
#if defined(APP_USB_A2DP_SOURCE)
        app_audio_sendrequest(APP_A2DP_SOURCE_USB_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif

        app_status_indication_filter_set(APP_STATUS_INDICATION_BOTHSCAN);
        app_audio_sendrequest(APP_BT_STREAM_INVALID, (uint8_t)APP_BT_SETTING_CLOSEALL, 0);
#if BLE_AUDIO_ENABLED
        gaf_audio_playback_closeall();
#endif
#if defined(QUICK_POWER_OFF_ENABLED)
        bes_bt_me_acl_disc_all_bt_links(true);
        osDelay(200);
#else
        osDelay(500);
        bes_bt_me_acl_disc_all_bt_links(true);
        osDelay(500);
        app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio_standalone_locally(AUD_ID_POWER_OFF, 0);
#endif
        osDelay(1000);
#endif
        af_close();
#if defined(__THIRDPARTY) && defined(__AI_VOICE__)
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1, THIRDPARTY_DEINIT, AI_SPEC_INIT);
#endif
#if FPGA==0
        nv_record_flash_flush();
        norflash_api_flush_all();
#if defined(DUMP_LOG_ENABLE)
        log_dump_flush_all();
#endif
#endif
    }

    return nRet;
}

#ifdef APP_TEST_MODE
extern void app_test_init(void);
#ifdef CURRENT_TEST
int app_init(void)
{
    int nRet = 0;
    //uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE(1,"%s",__func__);
    app_poweroff_flag = 0;

#ifdef APP_TRACE_RX_ENABLE
    app_trace_rx_open();
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_52M);
    list_init();
    af_open();
    app_os_init();
    app_pwl_open();
    bes_bt_app_init();
    btdrv_start_bt();
    bes_bt_thread_init();
    app_wait_stack_ready();
    bt_drv_config_after_hci_reset();
    app_bt_sleep_init();
    app_bt_start_custom_function_in_bt_thread((uint32_t)0,
        0, (uint32_t)bes_bt_init_global_handle);
    osDelay(500);
    bes_bt_me_write_access_mode(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR, 1);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_32K);
    return nRet;
}
#else
int app_init(void)
{
    int nRet = 0;
    //uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE(1,"%s",__func__);
    app_poweroff_flag = 0;

#ifdef APP_TRACE_RX_ENABLE
    app_trace_rx_open();
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_52M);
    list_init();
    af_open();
    app_os_init();
    app_pwl_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();
    if (app_key_open(true))
    {
        nRet = -1;
        goto exit;
    }

    app_test_init();
exit:
    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_32K);
    return nRet;
}
#endif
#else /* !defined(APP_TEST_MODE) */

int app_bt_connect2tester_init(void)
{
    btif_device_record_t rec;
    bt_bdaddr_t tester_addr;
    uint8_t i;
    bool find_tester = false;
    struct nvrecord_env_t *nvrecord_env;
    btdevice_profile *btdevice_plf_p;
    nv_record_env_get(&nvrecord_env);

    if (nvrecord_env->factory_tester_status.status != NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT)
        return 0;

    if (!nvrec_dev_get_dongleaddr(&tester_addr)){
        nv_record_open(section_usrdata_ddbrecord);
        for (i = 0; nv_record_enum_dev_records(i, &rec) == BT_STS_SUCCESS; i++) {
            if (!memcmp(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE)){
                find_tester = true;
            }
        }
        if(i==0 && !find_tester){
            memset(&rec, 0, sizeof(btif_device_record_t));
            memcpy(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE);
            nv_record_add(section_usrdata_ddbrecord, &rec);
            btdevice_plf_p = (btdevice_profile *)bes_bt_me_profile_active_store_ptr_get(rec.bdAddr.address);
            nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_plf_p, true);
            nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_plf_p, true);
        }
        if (find_tester && i>2){
            nv_record_ddbrec_delete(&tester_addr);
            nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
            nv_record_env_set(nvrecord_env);
        }
    }

    return 0;
}

int app_nvrecord_rebuild(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    nv_record_sector_clear();
    nv_record_env_init();
    nv_record_update_factory_tester_status(NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS);
    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();

    return 0;
}

#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
#include "app_audio.h"
#include "usb_audio_frm_defs.h"
#include "usb_audio_app.h"

static bool app_usbaudio_mode = false;

extern "C" void btusbaudio_entry(void);
void app_usbaudio_entry(void)
{
    btusbaudio_entry();
    app_usbaudio_mode = true ;
}

bool app_usbaudio_mode_on(void)
{
    return app_usbaudio_mode;
}

void app_usb_key(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);

}

const APP_KEY_HANDLE  app_usb_handle_cfg[] = {
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"USB HID FN1 UP key",app_usb_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"USB HID FN2 UP key",app_usb_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"USB HID PWR UP key",app_usb_key, NULL},
};

void app_usb_key_init(void)
{
    uint8_t i = 0;
    TRACE(1,"%s",__func__);
    for (i = 0; i < ARRAY_SIZE(app_usb_handle_cfg); i++) {
        app_key_handle_registration(&app_usb_handle_cfg[i]);
    }
}
#endif /* (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE)) */

#ifdef OS_THREAD_TIMING_STATISTICS_ENABLE
#ifdef DEBUG
static uint8_t isSwitchToUart = false;
#endif
extern "C" void rtx_show_all_threads_usage(void);
static void cpu_usage_timer_handler(void const *param);
osTimerDef(cpu_usage_timer, cpu_usage_timer_handler);
static osTimerId cpu_usage_timer_id = NULL;
static void cpu_usage_timer_handler(void const *param)
{
#ifdef DEBUG
    if (isSwitchToUart)
    {
        hal_cmu_jtag_clock_disable();
#if (DEBUG_PORT == 1)
        hal_iomux_set_uart0();
#elif (DEBUG_PORT == 2)
        hal_iomux_set_uart1();
#endif
        isSwitchToUart = false;
    }
#endif
    rtx_show_all_threads_usage();
}
#endif

int btdrv_tportopen(void);

void app_ibrt_start_power_on_tws_pairing(void);
void app_ibrt_start_power_on_freeman_pairing(void);

WEAK void app_ibrt_handler_before_starting_ibrt_functionality(void)
{

}

void app_ibrt_init(void)
{
    if (app_bt_sink_is_enabled())
    {
        bes_bt_init_global_handle();
#if defined(IBRT)
        ibrt_config_t config;
        app_tws_ibrt_init();
        app_tws_ibrt_init_default_tws_bandwidth_config(
            IBRT_TWS_LINK_DEFAULT_DURATION,
            IBRT_UI_DEFAULT_POLL_INTERVAL,
            IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO);
        app_tws_ibrt_request_modify_tws_bandwidth(TWS_TIMING_CONTROL_USER_DEFAULT, true);

//todo: adapte in ibrt_ui_v2
#ifdef IBRT_SEARCH_UI
        app_tws_ibrt_start(&config, true);
        app_ibrt_search_ui_init(false,IBRT_NONE_EVENT);
#else
        app_tws_ibrt_start(&config, false);
#endif

#if defined(IBRT_UI)
#ifdef MEDIA_PLAYER_SUPPORT
        tws_ctrl_register_send_status_callback(audio_prompt_req_send_failed_cb);
#endif
#ifdef A2DP_DECODER_CROSS_CORE
        app_ibrt_if_get_sbm_status_callback();
#endif
        app_ibrt_ui_v2_test_config_load(&config);
        hal_trace_global_tag_register(app_ibrt_middleware_fill_debug_info);
        app_ibrt_internal_register_ibrt_cbs();
        app_ibrt_tws_ext_cmd_init();
        app_ibrt_if_init_newly_paired_mobile_callback();
        app_ibrt_if_init_newly_paired_tws_callback();
        app_custom_ui_bt_ctx_init();
        app_ibrt_conn_init();
        app_ibrt_conn_check_allow_role_switch_handler_cb_register(app_ibrt_middleware_checker_allow_role_switch_handler);
        app_ibrt_auto_test_init();
#ifdef CODEC_ERROR_HANDLING
        af_register_enter_abnormal_state_callback(app_ibrt_conn_codec_error_handler);
#endif
#ifdef SEARCH_UI_COMPATIBLE_UI_V2
        app_tws_ibrt_start(&config,true);
#else
        app_tws_ibrt_start(&config,false);
#endif

#endif

#if defined(IBRT_UI)
        app_ui_init();
        app_ibrt_customif_ui_start();
#ifdef SEARCH_UI_COMPATIBLE_UI_V2
        app_ibrt_search_ui_init(false,APP_UI_EV_NONE);
#endif
#endif
        app_ibrt_handler_before_starting_ibrt_functionality();

        __attribute__((unused)) bool isAllowFollowingHandler = true;

        isAllowFollowingHandler = app_ibrt_internal_post_custom_reboot_handler();

        if (isAllowFollowingHandler)
        {
#ifdef POWER_ON_OPEN_BOX_ENABLED
            app_ibrt_if_event_entry(APP_UI_EV_CASE_OPEN);
#else
    #ifdef POWER_ON_ENTER_TWS_PAIRING_ENABLED
    #if BLE_AUDIO_ENABLED
        if (bes_ble_audio_is_ux_mobile())
        {

        }
        else
    #endif
        {
        #if defined(IBRT_UI)
            app_ibrt_start_power_on_tws_pairing();
        #endif
        }
    #elif defined(POWER_ON_ENTER_FREEMAN_PAIRING_ENABLED)
            app_ibrt_start_power_on_freeman_pairing();
    #elif defined(POWER_ON_ENTER_BOTH_SCAN_MODE)
            // enter both scan mode
            bes_bt_me_write_access_mode(BTIF_BAM_GENERAL_ACCESSIBLE, 0);
    #endif
#endif
        }

        app_ibrt_if_register_retrigger_prompt_id(AUDIO_ID_BT_MUTE);
#endif
    }
#ifdef IBRT
        app_ibrt_internal_stack_is_ready();
#endif
}

void app_walkie_talkie_mode_init()
{
#ifdef __IAG_BLE_INCLUDE__
#ifdef BLE_WALKIE_TALKIE
    app_walkie_talkie_init();
#endif
#endif //__IAG_BLE_INCLUDE__
}

POSSIBLY_UNUSED static void app_mobile_mode_init()
{
#if BLE_AUDIO_ENABLED
    app_custom_ui_lea_ctx_init();
#if defined(AOB_MOBILE_ENABLED) && (defined(BLE_USB_AUDIO_SUPPORT) || defined(WIRELESS_MIC))
    if (bes_ble_audio_is_ux_mobile())
    {
        bes_ble_mobile_start_connect();
    }
#endif
#endif
}

static void app_earbud_mode_init()
{
#ifdef __IAG_BLE_INCLUDE__
#ifdef IBRT
    bes_ble_gap_force_switch_adv(BLE_SWITCH_USER_IBRT, true);
#endif //IBRT
#if !(BLE_AUDIO_ENABLED)
    bes_ble_gap_stub_user_init();
#endif
#ifndef BLE_STACK_NEW_DESIGN
    bes_ble_gap_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
#endif
#endif // __IAG_BLE_INCLUDE__

#ifdef APP_SOUND_ENABLE
    soundInit(true);
#else
    app_ibrt_init();
#endif

#ifdef IBRT
    app_ibrt_internal_stack_is_ready();
#endif
}

static void app_default_mode_init(nvrec_appmode_e mode)
{
    ble_uart_cmd_init();
#ifdef BLE_HOST_PTS_TEST_ENABLED
    // Noting to do
#else
#ifdef __IAG_BLE_INCLUDE__
    bes_ble_app_init(mode);
#endif //__IAG_BLE_INCLUDE__
#endif //BLE_HOST_PTS_TEST_ENABLED

#if BLE_AUDIO_ENABLED
    if (bes_ble_audio_is_ux_mobile())
    {
        app_mobile_mode_init();
#ifdef WIRELESS_MIC
        app_wireless_mic_init();
#endif
    }
    else
#endif
    {
        app_earbud_mode_init();
    }

#ifdef BLE_BIS_TRANSPORT
    app_bis_tran_cmd_init();
#endif
}

int app_bluetooth_application_init()
{
    nvrec_appmode_e mode = nv_record_appmode_get();

    TRACE(0, "%s: mode %d", __func__, mode);

    switch (mode)
    {
        case NV_APP_EARBUDS_MONO:
        case NV_APP_EARBUDS_STEREO_ONE_CIS:
        case NV_APP_EARBUDS_STEREO_TWO_CIS:
        case NV_APP_HEADSET_STEREO_ONE_CIS:
        case NV_APP_HEADSET_STEREO_TWO_CIS:
            app_default_mode_init(mode);
            break;
        case NV_APP_WALKIE_TALKIE:
            app_walkie_talkie_mode_init();
            break;
        default:
            break;
    }

#ifdef BLE_STACK_NEW_DESIGN
#ifdef BLE_HOST_SUPPORT
    bes_ble_gap_ready_and_init_done(mode);
#endif
#endif

    return 0;
}

int app_switch_mode(int mode, bool reboot)
{
    if (nv_record_appmode_get() == mode)
    {
        TRACE(0, "Already in mode=%d", mode);
        return -1;
    }
    nv_record_appmode_set((nvrec_appmode_e)mode);
    if (reboot)
    {
        app_reset();
    }
    return 0;
}

#ifdef GFPS_ENABLED
static void app_tell_battery_info_handler(uint8_t *batteryValueCount,
                                          uint8_t *batteryValue)
{
    GFPS_BATTERY_STATUS_E status;
    if (app_battery_is_charging())
    {
        status = BATTERY_CHARGING;
    }
    else
    {
        status = BATTERY_NOT_CHARGING;
    }

    // TODO: add the charger case's battery level
#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
    if (app_tws_ibrt_tws_link_connected())
    {
        *batteryValueCount = 2;
    }
    else
    {
        *batteryValueCount = 1;
    }
#else
    *batteryValueCount = 1;
#endif

    TRACE(2,"%s,*batteryValueCount is %d",__func__,*batteryValueCount);
    if (1 == *batteryValueCount)
    {
        batteryValue[0] = ((app_battery_current_level()+1) * 10) | (status << 7);
    }
    else
    {
        batteryValue[0] = ((app_battery_current_level()+1) * 10) | (status << 7);
        batteryValue[1] = ((app_battery_current_level()+1) * 10) | (status << 7);

    }
}
#endif
extern uint32_t __coredump_section_start[];
extern uint32_t __ota_upgrade_log_start[];
extern uint32_t __log_dump_start[];
extern uint32_t __crash_dump_start[];
extern uint32_t __custom_parameter_start[];
extern uint32_t __lhdc_license_start[];
extern uint32_t __aud_start[];
extern uint32_t __userdata_start[];
extern uint32_t __factory_start[];


int app_init(void)
{
    int nRet = 0;
    struct nvrecord_env_t *nvrecord_env;
#if defined(IGNORE_POWER_ON_KEY_DURING_BOOT_UP)
    bool need_check_key = false;
#else
    bool need_check_key = true;
#endif
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
#ifdef BT_USB_AUDIO_DUAL_MODE
    uint8_t usb_plugin = 0;
#endif
#ifdef IBRT_SEARCH_UI
    bool is_charging_poweron=false;
#endif
    TRACE(0,"please check all sections sizes and heads is correct ........");
    TRACE(2,"__coredump_section_start: %p length: 0x%x", __coredump_section_start, CORE_DUMP_SECTION_SIZE);
    TRACE(2,"__ota_upgrade_log_start: %p length: 0x%x", __ota_upgrade_log_start, OTA_UPGRADE_LOG_SIZE);
    TRACE(2,"__log_dump_start: %p length: 0x%x", __log_dump_start, LOG_DUMP_SECTION_SIZE);
    TRACE(2,"__crash_dump_start: %p length: 0x%x", __crash_dump_start, CRASH_DUMP_SECTION_SIZE);
    TRACE(2,"__custom_parameter_start: %p length: 0x%x", __custom_parameter_start, CUSTOM_PARAMETER_SECTION_SIZE);
    TRACE(2,"__lhdc_license_start: %p length: 0x%x", __lhdc_license_start, LHDC_LICENSE_SECTION_SIZE);
    TRACE(2,"__userdata_start: %p length: 0x%x", __userdata_start, USERDATA_SECTION_SIZE*2);
    TRACE(2,"__aud_start: %p length: 0x%x", __aud_start, AUD_SECTION_SIZE);
    TRACE(2,"__factory_start: %p length: 0x%x", __factory_start, FACTORY_SECTION_SIZE);

    TRACE(0,"app_init\n");

#ifdef APP_TRACE_RX_ENABLE
    app_trace_rx_open();
    app_bt_cmd_init();
#endif

    app_bt_add_string_test_table();

#ifdef __RPC_ENABLE__
extern int rpc_service_setup(void);
    rpc_service_setup();
#endif

#if defined(ESHELL_ON)
    eshell_open();
#endif

#ifdef VOICE_DEV
    voice_dev_init();
#endif

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
    // init tws interface
    app_ibrt_middleware_init();
#endif // #ifdef IBRT

#ifdef VOICE_DETECTOR_EN
    app_voice_detector_init();
#endif // #ifdef VOICE_DETECTOR_EN

    nv_record_init();
    factory_section_init();
#ifdef PROMPT_IN_FLASH
    nv_record_prompt_rec_init();
#if defined(MEDIA_PLAYER_SUPPORT)
    app_play_audio_set_lang(nv_record_prompt_get_prompt_language(1));
#endif
#endif

#ifdef DMA_AUDIO_APP
    dma_audio_cli_open();
#endif

#ifdef AUDIO_OUTPUT_DC_AUTO_CALIB
    /* DAC DC and Dig gain calibration
     * This subroutine will try to load DC and gain calibration
     * parameters from user data section located at NV record;
     * If failed to loading parameters, It will open AF and start
     * to calibration DC and gain;
     * After accomplished new calibrtion parameters, it will try
     * to save these data into user data section at NV record if
     * CODEC_DAC_DC_NV_DATA defined;
     */
    codec_dac_dc_auto_load(true, false, false);
#endif

#ifdef AUDIO_ADC_DC_AUTO_CALIB
    codec_adc_dc_auto_load(true, false, false);
#endif

    af_open();

#if defined(DMA_AUDIO_APP) && defined(DMA_AUDIO_APP_INIT_ON)
    dma_audio_cli_key_on(DAUD_CLI_KEY_NORMAL);
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_104M);
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    TRACE(1,"sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));
#endif

    list_init();
    nRet = app_os_init();
    if (nRet) {
        goto exit;
    }
#ifdef OS_THREAD_TIMING_STATISTICS_ENABLE
    cpu_usage_timer_id = osTimerCreate(osTimer(cpu_usage_timer), osTimerPeriodic, NULL);
    if (cpu_usage_timer_id != NULL) {
        osTimerStart(cpu_usage_timer_id, OS_THREAD_TIMING_STATISTICS_PEROID_MS);
    }
#endif

#if defined(DSP_M55)
    app_dsp_m55_first_init();
#endif

#if defined(DSP_HIFI4)
    app_dsp_hifi4_first_init();
#endif

    app_status_indication_init();

#ifdef FORCE_SIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
#elif defined FORCE_NOSIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
#endif

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT_FROM_CRASH){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT_FROM_CRASH);
        TRACE(0,"Crash happened!!!");
    #ifdef BISTO_ENABLED
        gsound_dump_set_flag(true);
    #endif
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pwron_case = APP_POWERON_CASE_REBOOT;
        need_check_key = false;
        TRACE(0,"Initiative REBOOT happens!!!");
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
        pwron_case = APP_POWERON_CASE_TEST;
        need_check_key = false;
        TRACE(0,"To enter test mode!!!");
    }

#if !defined(IS_CUSTOM_UART_APPLICATION_ENABLED)
#ifdef APP_UART_MODULE
    app_uart_init();
#endif
#endif

#ifdef APP_CHIP_BRIDGE_MODULE
    app_chip_bridge_init();
#endif

#ifdef BLE_WIFI_DUAL_CHIP_MODE
    app_ble_wifi_dual_mode_init();
#endif

    nRet = app_battery_open();
    TRACE(1,"BATTERY %d",nRet);
    if (pwron_case != APP_POWERON_CASE_TEST){
        switch (nRet) {
            case APP_BATTERY_OPEN_MODE_NORMAL:
                nRet = 0;
                break;
            case APP_BATTERY_OPEN_MODE_CHARGING:
                app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
                TRACE(0,"CHARGING!");
                app_battery_start();

                app_key_open(false);
                app_key_init_on_charging();
                nRet = 0;
#if defined(BT_USB_AUDIO_DUAL_MODE)
                usb_plugin = 1;
#elif defined(BTUSB_AUDIO_MODE)
                goto exit;
#endif
                break;
            case APP_BATTERY_OPEN_MODE_CHARGING_PWRON:
                TRACE(0,"CHARGING PWRON!");
#ifdef IBRT_SEARCH_UI
                is_charging_poweron=true;
#endif
#if defined(BT_USB_AUDIO_DUAL_MODE)
                usb_plugin = 1;
#endif
                need_check_key = false;
                nRet = 0;
#ifdef MSD_MODE
                pwron_case = APP_BATTERY_OPEN_MODE_CHARGING_PWRON;
                goto exit;
#endif
                break;
            case APP_BATTERY_OPEN_MODE_INVALID:
            default:
                nRet = -1;
                goto exit;
                break;
        }
    }

    if (app_key_open(need_check_key)){
        TRACE(0,"PWR KEY DITHER!");
        nRet = -1;
        goto exit;
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
    app_poweron_key_init();
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("Power on.");
#endif
    bes_bt_app_init();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();
    app_overlay_subsys_open();
    nv_record_env_init();
    nvrec_dev_data_open();
//    app_bt_connect2tester_init();
    nv_record_env_get(&nvrecord_env);

#ifdef CAPSENSOR_ENABLE
    app_mcu_core_capsensor_init();
#endif

#ifdef BISTO_ENABLED
    nv_record_gsound_rec_init();
#endif

    audio_process_init();
#ifdef __PC_CMD_UART__
    app_cmd_open();
#endif

#ifdef AUDIO_DEBUG
    speech_tuning_init();
#endif

#ifdef AUDIO_DEBUG_CMD
    audio_test_cmd_init();
#endif

#ifdef AUDIO_HEARING_COMPSATN
    hearing_det_cmd_init();
#endif

#if defined(ANC_ASSIST_ENABLED)
    app_anc_assist_init();
    app_voice_assist_wd_init();
    app_voice_assist_noise_adapt_anc_init();
#if defined(VOICE_ASSIST_FF_FIR_LMS)
    app_voice_assist_fir_anc_open_leak_init();
    app_voice_assist_fir_lms_init();
#endif
    app_voice_assist_noise_classify_adapt_anc_init();
    app_voice_assist_wsd_init();
#if defined(VOICE_ASSIST_PILOT_ANC_ENABLED)
    app_voice_assist_pilot_anc_init();
#endif
#if defined(VOICE_ASSIST_ONESHOT_ADAPTIVE_ANC)
    app_voice_assist_oneshot_adapt_anc_init();
#endif
#ifdef ANC_ASSIST_ALGO_ON_DSP
    app_voice_assist_anc_dsp_init();
#endif
    app_voice_assist_ai_voice_init();
#if defined(VOICE_ASSIST_WD_ENABLED)
	app_voice_assist_ultrasound_init();
#endif
#if defined(AUDIO_ADAPTIVE_EQ)
    app_voice_assist_adaptive_eq_init();
#endif
#if defined(AUDIO_ADJ_EQ)
    app_voice_assist_adj_eq_init();
#endif
    app_voice_assist_custom_leak_detect_init();
    app_voice_assist_prompt_leak_detect_init();
    app_voice_assist_pnc_adapt_anc_init();
    app_voice_assist_optimal_tf_anc_init();
#endif

#ifdef ANC_APP
    app_anc_init();
#endif

#if defined(MEDIA_PLAYER_SUPPORT) && !defined(PROMPT_IN_FLASH)
    app_play_audio_set_lang(nvrecord_env->media_language.language);
#endif
    app_bt_stream_volume_ptr_update(NULL);

#ifdef __THIRDPARTY
    app_thirdparty_init();
#if defined(__AI_VOICE__)
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_INIT, AI_SPEC_INIT);
#endif
#endif

    // TODO: freddie->unify all of the OTA modules
#if defined(BES_OTA)
    bes_ota_init_handler();
#endif

#ifdef AI_OTA
    /// init OTA common module
    ota_common_init_handler();
#endif // AI_OTA

    btdrv_start_bt();

#ifdef APP_MCPP_CLI
    app_mcpp_init();
#endif

#ifdef APP_MCPP_SRV
    mcpp_srv_open();
#endif

#if defined (__GMA_VOICE__) && defined(IBRT_SEARCH_UI)
    app_ibrt_reconfig_btAddr_from_nv();
#endif

#if defined(BT_WATCH_MASTER) || defined(BT_WATCH_SLAVE)
    app_bt_watch_open();
#endif

#ifdef SPECIFIC_FREQ_POWER_CONSUMPTION_MEASUREMENT_ENABLE
    app_start_power_consumption_thread();
#endif

#ifdef __CONNECTIVITY_LOG_REPORT__
    app_ibrt_if_report_connectivity_log_init();
#endif

#ifdef __LVGL_EN__
    ui_os_init();
#endif

    if (pwron_case != APP_POWERON_CASE_TEST) {
        bes_bt_thread_init();
        app_wait_stack_ready();

#if defined(SENSOR_HUB) && !defined(__NuttX__) && !defined(SPEECH_ALGO_DSP_SENS)
        app_sensor_hub_init();
#endif

        bt_drv_config_after_hci_reset();
        bt_generate_ecdh_key_pair();

        app_bt_start_custom_function_in_bt_thread((uint32_t)0,
            0, (uint32_t)app_bluetooth_application_init);

#ifdef CAPSENSOR_ENABLE
#ifdef CAPSENSOR_SPP_SERVER
        app_spp_capsensor_server();
#endif
#endif
    }

#if defined(IBRT_UI)
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
    app_ui_get_config()->ibrt_with_ai=true;
#endif
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_52M);
    TRACE(1,"\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);

    app_application_ready_to_start_callback();
    if (pwron_case == APP_POWERON_CASE_REBOOT){

        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_POWER_ON, 0);
#endif
        app_bt_sleep_init();

#if defined(OTA_ENABLE)
        ota_basic_env_init();
#endif


#if defined(BES_OTA)
#if !defined(OTA_OVER_TOTA_ENABLED)
        bes_ota_init();
#endif
#endif

#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
        if(is_charging_poweron==false)
        {
            if(IBRT_UNKNOW == nvrecord_env->ibrt_mode.mode)
            {
                TRACE(0,"power on unknow mode");
                app_ibrt_enter_limited_mode();
            }
            else
            {
                TRACE(1,"power on %d fetch out", nvrecord_env->ibrt_mode.mode);
#ifdef SEARCH_UI_COMPATIBLE_UI_V2
                app_ibrt_if_event_entry(APP_UI_EV_UNDOCK);
#else
                app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
#endif
            }
        }
#endif
#else
        bes_bt_me_write_access_mode(BTIF_BAM_NOT_ACCESSIBLE, 1);
#endif

        app_key_init();
        app_battery_start();
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
        app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif

#ifdef __THIRDPARTY
#if defined(__AI_VOICE__)
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_INIT, AI_SPEC_INIT);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START, AI_SPEC_INIT);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_BT_CONNECTABLE, AI_SPEC_INIT);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,THIRDPARTY_START, AI_SPEC_INIT);
#endif
#endif
#if defined( __BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__)
#if defined(FREEMAN_ENABLED_STERO)
        app_bt_profile_connect_manager_opening_reconnect();
#endif
#endif
    }
#ifdef __ENGINEER_MODE_SUPPORT__
    else if(pwron_case == APP_POWERON_CASE_TEST){
        app_factorymode_set(true);
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_POWER_ON, 0);
#endif
#ifdef __WATCHER_DOG_RESET__
        app_wdt_close();
#endif
        TRACE(0,"!!!!!ENGINEER_MODE!!!!!\n");
        nRet = 0;
        app_nvrecord_rebuild();
        app_factorymode_key_init();
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_signalingtest(NULL, NULL);
        }
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_nosignalingtest(NULL, NULL);
        }
    }
#endif
    else{
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_POWER_ON, 0);
#endif
        if (need_check_key){
            pwron_case = app_poweron_wait_case();
        }
        else
        {
            pwron_case = APP_POWERON_CASE_NORMAL;
        }
        if (pwron_case != APP_POWERON_CASE_INVALID && pwron_case != APP_POWERON_CASE_DITHERING){
            TRACE(1,"power on case:%d\n", pwron_case);
            nRet = 0;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
            app_status_indication_set(APP_STATUS_INDICATION_INITIAL);
#endif
            app_bt_sleep_init();

#if defined(OTA_ENABLE)
            ota_basic_env_init();
#endif

#ifdef BES_OTA
#if !defined(OTA_OVER_TOTA_ENABLED)
            bes_ota_init();
#endif
#endif

#ifdef __INTERCONNECTION__
            app_interconnection_init();
#endif

#ifdef __INTERACTION__
            app_interaction_init();
#endif

#ifdef GFPS_ENABLED
            gfps_reg_battery_handler(app_tell_battery_info_handler);
            gfps_set_battery_datatype(SHOW_UI_INDICATION);
#endif
            switch (pwron_case) {
                case APP_POWERON_CASE_CALIB:
                    break;
                case APP_POWERON_CASE_BOTHSCAN:
                    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
#ifdef MEDIA_PLAYER_SUPPORT
                    media_PlayAudio(AUD_ID_BT_PAIR_ENABLE, 0);
#endif
#if defined( __BTIF_EARPHONE__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
                    if(false==is_charging_poweron)
                        app_ibrt_enter_limited_mode();
#endif
#else
                    bes_bt_me_write_access_mode(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR, 1);
#endif
#ifdef GFPS_ENABLED
                    app_enter_fastpairing_mode();
#endif
#if defined(__BTIF_AUTOPOWEROFF__)
                    app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#endif
#ifdef __THIRDPARTY
#if defined(__AI_VOICE__)
                    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_BT_DISCOVERABLE, AI_SPEC_INIT);
#endif
#endif
                    break;
                case APP_POWERON_CASE_NORMAL:
#if defined( __BTIF_EARPHONE__ ) && !defined(__EARPHONE_STAY_BOTH_SCAN__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
                    if(is_charging_poweron==false)
                    {
                        if(IBRT_UNKNOW == nvrecord_env->ibrt_mode.mode)
                        {
                            TRACE(0,"power on unknow mode");
                            app_ibrt_enter_limited_mode();
                        }
                        else
                        {
                            TRACE(1,"power on %d fetch out", nvrecord_env->ibrt_mode.mode);
#ifdef SEARCH_UI_COMPATIBLE_UI_V2
                            app_ibrt_if_event_entry(APP_UI_EV_UNDOCK);
#else
                            app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
#endif


                        }
                    }
#elif defined(IS_MULTI_AI_ENABLED)
                    //when ama and bisto switch, earphone need reconnect with peer, master need reconnect with phone
                    //app_ibrt_ui_event_entry(IBRT_OPEN_BOX_EVENT);
                    //TRACE(1,"ibrt_ui_log:power on %d fetch out", nvrecord_env->ibrt_mode.mode);
                    //app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
#endif
#else
                    bes_bt_me_write_access_mode(BTIF_BAM_NOT_ACCESSIBLE, 1);
#endif
#endif
                    /* FALL THROUGH*/
                case APP_POWERON_CASE_REBOOT:
                case APP_POWERON_CASE_ALARM:
                default:
                    app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
#if defined( __BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__) && defined(FREEMAN_ENABLED_STERO)
                    app_bt_profile_connect_manager_opening_reconnect();
#endif
#ifdef __THIRDPARTY
#if defined(__AI_VOICE__)
                    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_BT_CONNECTABLE, AI_SPEC_INIT);
#endif
#endif

                    break;
            }
            if (need_check_key)
            {
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
                app_poweron_wait_finished();
#endif
            }
            app_key_init();
            app_battery_start();
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
            app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
#ifdef __THIRDPARTY
#if defined(__AI_VOICE__)
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_INIT, AI_SPEC_INIT);
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START, AI_SPEC_INIT);
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,THIRDPARTY_START, AI_SPEC_INIT);
#endif
#endif

#ifdef RB_CODEC
            rb_ctl_init();
#endif
        }else{
            af_close();
            app_key_close();
            nRet = -1;
        }
    }
exit:

#ifdef MSD_MODE
    if (pwron_case == APP_BATTERY_OPEN_MODE_CHARGING_PWRON)
    {
        TRACE(1, "usbstorage_init !\n");
        usbstorage_init();
    }
#endif

#ifdef IS_MULTI_AI_ENABLED
    app_ai_tws_clear_reboot_box_state();
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
    btusb_init();
    TRACE(1,"usb_plugin=%d",usb_plugin);
#if defined(APP_USB_A2DP_SOURCE)
    btusb_switch(BTUSB_MODE_USB);
#else
#ifdef BLE_USB_AUDIO_SUPPORT
    if(usb_plugin)
    {
        btusb_start_switch_to(BTUSB_MODE_USB);
    }
    else
    {
        btusb_switch(BTUSB_MODE_BT);
    }
#else
    if (app_battery_is_charging())
    {
        btusb_switch(BTUSB_MODE_USB);
    }
    else
    {
        btusb_switch(BTUSB_MODE_BT);
    }
#endif
#endif
#else //BT_USB_AUDIO_DUAL_MODE
#if defined(BTUSB_AUDIO_MODE)
    if(pwron_case == APP_POWERON_CASE_CHARGING) {
        app_wdt_close();
        af_open();
        app_key_handle_clear();
        app_usb_key_init();
        app_usbaudio_entry();
    }

#endif // BTUSB_AUDIO_MODE
#endif // BT_USB_AUDIO_DUAL_MODE
    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_32K);
#ifdef IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
    hal_iomux_tportopen();
#endif
    return nRet;
}

#ifdef SPECIFIC_FREQ_POWER_CONSUMPTION_MEASUREMENT_ENABLE

// for power consumption test on different cpu frequency
// test configuation
#define POWER_CONSUMPTION_MEASUREMENT_FREQ  APP_SYSFREQ_26M
#define IS_TEST_MCU_AND_CP

#if defined(CP_IN_SAME_EE)&&defined(IS_TEST_MCU_AND_CP)
#include "cp_accel.h"

CP_TEXT_SRAM_LOC
static unsigned int cp_power_consumption_measurement_main(uint8_t event)
{
    return 0;
}

static const struct cp_task_desc task_desc_power_consumption_measurement = {
    CP_ACCEL_STATE_CLOSED,
    cp_power_consumption_measurement_main,
    NULL, NULL, NULL
};

static void power_consumption_measurement_start_cp(void)
{
    norflash_api_flush_disable(NORFLASH_API_USER_CP,(uint32_t)cp_accel_init_done);
    cp_accel_open(CP_TASK_POWER_CONSUMPTION_MEASUREMENT, &task_desc_power_consumption_measurement);
    while(cp_accel_init_done() == false) {
        hal_sys_timer_delay_us(100);
    }
    norflash_api_flush_enable(NORFLASH_API_USER_CP);
}
#endif

static void power_consumption_thread(const void *arg);
osThreadDef(power_consumption_thread, osPriorityHigh, 1, (1024), "power_consumption_thread");
static osThreadId power_consumption_thread_id;

static void power_consumption_thread(const void *arg)
{
    // release all the allocaetd frequencies
    for (uint32_t user = APP_SYSFREQ_USER_APP_INIT;
        user < APP_SYSFREQ_USER_QTY; user++)
    {
        app_sysfreq_req((enum APP_SYSFREQ_USER_T)user, APP_SYSFREQ_32K);
    }

#if defined(CP_IN_SAME_EE)&&defined(IS_TEST_MCU_AND_CP)
    power_consumption_measurement_start_cp();
#endif

    enum APP_SYSFREQ_FREQ_T testFreq = POWER_CONSUMPTION_MEASUREMENT_FREQ;

    if (APP_SYSFREQ_26M == testFreq)
    {
        TRACE(0, "Test power consumption at sys Freq 24Mhz.");
    }
    else if (APP_SYSFREQ_52M == testFreq)
    {
        TRACE(0, "Test power consumption at sys Freq 48Mhz.");
    }
    else if (APP_SYSFREQ_78M == testFreq)
    {
        TRACE(0, "Test power consumption at sys Freq 72Mhz.");
    }
    else if (APP_SYSFREQ_104M == testFreq)
    {
        TRACE(0, "Test power consumption at sys Freq 96Mhz.");
    }
    else if (APP_SYSFREQ_208M == testFreq)
    {
        TRACE(0, "Test power consumption at sys Freq 192Mhz.");
    }

    app_sysfreq_req(APP_SYSFREQ_USER_APP_15,
        (enum APP_SYSFREQ_FREQ_T)POWER_CONSUMPTION_MEASUREMENT_FREQ);
    while(1);
}

void app_start_power_consumption_thread(void)
{
    power_consumption_thread_id =
        osThreadCreate(osThread(power_consumption_thread), NULL);
}
#endif

int app_init_btc(void)
{
    struct nvrecord_env_t *nvrecord_env;

    int nRet = 0;
    POSSIBLY_UNUSED uint8_t pwron_case = APP_POWERON_CASE_INVALID;

    TRACE(0,"please check all sections sizes and heads is correct ........");
    TRACE(2,"__coredump_section_start: %p length: 0x%x", __coredump_section_start, CORE_DUMP_SECTION_SIZE);
    TRACE(2,"__ota_upgrade_log_start: %p length: 0x%x", __ota_upgrade_log_start, OTA_UPGRADE_LOG_SIZE);
    TRACE(2,"__log_dump_start: %p length: 0x%x", __log_dump_start, LOG_DUMP_SECTION_SIZE);
    TRACE(2,"__crash_dump_start: %p length: 0x%x", __crash_dump_start, CRASH_DUMP_SECTION_SIZE);
    TRACE(2,"__custom_parameter_start: %p length: 0x%x", __custom_parameter_start, CUSTOM_PARAMETER_SECTION_SIZE);
    TRACE(2,"__lhdc_license_start: %p length: 0x%x", __lhdc_license_start, LHDC_LICENSE_SECTION_SIZE);
    TRACE(2,"__userdata_start: %p length: 0x%x", __userdata_start, USERDATA_SECTION_SIZE*2);
    TRACE(2,"__aud_start: %p length: 0x%x", __aud_start, AUD_SECTION_SIZE);
    TRACE(2,"__factory_start: %p length: 0x%x", __factory_start, FACTORY_SECTION_SIZE);

    TRACE(0,"app_init\n");
    nv_record_init();
    factory_section_init();

    nv_record_env_init();
    nvrec_dev_data_open();
    factory_section_open();
//    app_bt_connect2tester_init();
    nv_record_env_get(&nvrecord_env);



    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_104M);
    list_init();
    nRet = app_os_init();
    if (nRet) {
        goto exit;
    }



#ifdef FORCE_SIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
#elif defined FORCE_NOSIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
#endif



    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pwron_case = APP_POWERON_CASE_REBOOT;
        TRACE(0,"Initiative REBOOT happens!!!");
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
        pwron_case = APP_POWERON_CASE_TEST;
        TRACE(0,"To enter test mode!!!");
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
 //   app_poweron_key_init();

    btdrv_start_bt();

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_52M);

#ifdef __ENGINEER_MODE_SUPPORT__
    if(pwron_case == APP_POWERON_CASE_TEST){
        app_factorymode_set(true);
#ifdef __WATCHER_DOG_RESET__
    //    app_wdt_close();
#endif
        TRACE(0,"!!!!!ENGINEER_MODE!!!!!\n");
        nRet = 0;

    //    app_factorymode_key_init();
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_signalingtest(NULL, NULL);
        }
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_btc_only_mode(NULL, NULL);
        }
    }
#endif
exit:

    app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_32K);

    return nRet;
}

int app_deinit_btc(int deinit_case)
{
    int nRet = 0;
    TRACE(2,"%s case:%d",__func__, deinit_case);

    if (!deinit_case){

        app_poweroff_flag = 1;
    }

    return nRet;
}

#endif /* APP_TEST_MODE */

WEAK void app_application_ready_to_start_callback(void)
{

}
