/**

 * @copyright Copyright (c) 2015-2023 BES Technic.
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
#ifdef USB_HID_COMMAND_ENABLE
#include "stdio.h"
#include "string.h"
#include "usb_audio.h"
#include "usb_hid_app.h"
#include "usb_audio_app.h"
#include "hal_cmu.h"
#include "hal_norflash.h"
#include "hwtimer_list.h"
#include "pmu.h"
#ifdef CFU_USB_OTA_ENABLE
#include "app_cfu_handler.h"
extern "C" void pmu_reboot(void);
#define HID_REPORT_TYPE_INPUT               0x01
#define HID_REPORT_TYPE_OUTPUT              0x02
#define HID_REPORT_TYPE_FEATURE             0x03
#define CFU_USB_TRACE(n, str, ...)              //TRACE(n, str, ##__VA_ARGS__)
#define CFU_USB_DUMP8(str, ptr, ptr_len)      //DUMP8(str, ptr, ptr_len)
#endif

#define DEVICE_PARAM_SEC_SIZE               0x1000
#define DEVICE_PARAM_HWTIMER_DELAY_FLUSH    1000
#define DEVICE_PARAM_SEC_START_ADDR         (FLASH_BASE + FLASH_SIZE - DEVICE_PARAM_SEC_SIZE)

static bool app_usb_hid_cmd_get_report(struct EP0_TRANSFER *transfer);
static bool app_usb_hid_cmd_set_report(struct EP0_TRANSFER *transfer);
static bool app_usb_hid_cmd_received(struct EP0_TRANSFER *transfer);

static USB_HID_EPINT_OUT_CFG_T usb_hid_epint_out_recv_calbacks = {
    .set_report_setup_callback = app_usb_hid_cmd_set_report,
    .set_report_data_callback = app_usb_hid_cmd_received,
    .get_report_setup_callback = app_usb_hid_cmd_get_report
};
static APP_USB_HID_DATA_RSP_T payload;
static uint16_t payload_len;
static APP_USB_HID_CMD_DEVICE_PARAM_T global_device_param;
static HWTIMER_ID g_hwtimer_id;
#ifdef USB_AUDIO_CUSTOM_ASP
static APP_USB_HID_ASP_CMD_PAYLOAD_T asp_payload;
static uint16_t asp_payload_len;
static APP_USB_HID_ASP_PARAM_T global_device_asp_param;
static void _usb_cmd_asp_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen);
#endif
#define USB_HID_TRACE(n, str, ...)       TRACE(n, str, ##__VA_ARGS__)

#ifdef CFU_USB_OTA_ENABLE
static bool update_success_to_reset = false;
static GET_FWVERSION_RESPONSE payload_version;
static uint16_t payload_version_len;

static FWUPDATE_OFFER_RESPONSE payload_offer;
static uint16_t payload_offer_len;

static FWUPDATE_CONTENT_RESPONSE payload_content;
static uint16_t payload_content_len;
#endif

static APP_USB_HID_CMD_INSTANCE_T* app_usb_hid_cmd_handler_get_entry_pointer_from_cmd_code(APP_USB_HID_CMD_CODE_E cmdCode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__usb_hid_handler_table_end-(uint32_t)__usb_hid_handler_table_start)/sizeof(APP_USB_HID_CMD_INSTANCE_T);index++)
    {
        if (USB_HID_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
        {
            return USB_HID_COMMAND_PTR_FROM_ENTRY_INDEX(index);
        }
    }
    return NULL;
}

static bool app_usb_hid_cmd_get_report(struct EP0_TRANSFER *transfer)
{
    uint8_t __unused report_type;
    uint8_t __unused report_id;
    USB_HID_TRACE(1,"%s enter", __func__);
    report_type = transfer->setup_pkt.wValue >> 8;
    report_id = transfer->setup_pkt.wValue & 0xFF;
    if (report_id == HID_REPORT_ID_RSP)
    {
        transfer->data = (uint8_t *)&payload;
        transfer->length = payload_len;
    }
#ifdef USB_AUDIO_CUSTOM_ASP
    else if(report_id == HID_REPORT_ID_ASP){
        transfer->data = (uint8_t *)&asp_payload;
        transfer->length = asp_payload_len;
    }
#endif
#ifdef CFU_USB_OTA_ENABLE
    else if(report_id == REPORT_ID_VERSIONS_FEATURE) // GET_FWVERSION_RESPONSE
    {
        payload_version_len = HID_CFU_VERSION_LENGTH_BYTES;
        cfu_get_version(&payload_version);
        transfer->data = (uint8_t *)&payload_version;
        transfer->length = payload_version_len;
        CFU_USB_TRACE(0,"payload_version_len %d", payload_version_len);
        CFU_USB_DUMP8("%02x ", transfer->data, payload_version_len);
    }else if(report_id == REPORT_ID_PAYLOAD_INPUT) // FWUPDATE_CONTENT_RESPONSE
    {
        transfer->data = (uint8_t *)&payload_content;
        transfer->length = payload_content_len;
    }else if(report_id == REPORT_ID_OFFER_INPUT) // FWUPDATE_OFFER_RESPONSE
    {
        transfer->data = (uint8_t *)&payload_offer;
        transfer->length = payload_offer_len;
    }
#endif
    transfer->stage = DATA_IN_STAGE;
    return true;
}

static bool app_usb_hid_cmd_set_report(struct EP0_TRANSFER *transfer)
{
    uint8_t __unused report_type;
    uint8_t __unused report_id;
    USB_HID_TRACE(1,"%s enter", __func__);
    report_type = transfer->setup_pkt.wValue >> 8;
    report_id = transfer->setup_pkt.wValue & 0xFF;
    if (report_id == HID_REPORT_ID_REQ)
    {
        transfer->length = HID_REPORT_RSP_PACKET_SIZE;
    }
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
    else if(report_id == HID_REPORT_ID_HID_LED){
        transfer->length = HID_REPORT_KEY_PACKET_SIZE;
    }
#endif
#ifdef USB_AUDIO_CUSTOM_ASP
    else if(report_id == HID_REPORT_ID_ASP){
        transfer->length = HID_REPORT_ASP_PACKET_SIZE;
    }
#endif
#ifdef CFU_USB_OTA_ENABLE
    else if(report_id == REPORT_ID_VERSIONS_FEATURE && report_type == HID_REPORT_TYPE_FEATURE)//GET_FWVERSION_COMMAND
    {
        // GET_FWVERSION_COMMAND does not have any arguments.
    }else if(report_id == REPORT_ID_OFFER_OUTPUT)// FWUPDATE_OFFER_COMMAND
    {
        transfer->length = HID_CFU_OFFER_COMMAND_LENGTH_BYTES;
    }else if(report_id == REPORT_ID_PAYLOAD_OUTPUT && report_type == HID_REPORT_TYPE_OUTPUT)//FWUPDATE_CONTENT_COMMAND
    {
        transfer->length = HID_CFU_CONTENT_COMMAND_LENGTH_BYTES;
    }
#endif
    transfer->stage = DATA_OUT_STAGE;
    return true;
}

static bool app_usb_hid_cmd_received(struct EP0_TRANSFER *transfer)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    uint8_t __unused report_type;
    uint8_t __unused report_id;

    report_type = transfer->setup_pkt.wValue >> 8;
    report_id = transfer->setup_pkt.wValue & 0xFF;
    USB_HID_TRACE(0, "HID_REQ_SET_REPORT: report_id %x !!!",report_id);
    if (report_id == HID_REPORT_ID_REQ)
    {
        APP_USB_HID_DATA_RSP_T* pPayload = (APP_USB_HID_DATA_RSP_T *)transfer->data;
        APP_USB_HID_CMD_PAYLOAD_T* pPayload_data = (APP_USB_HID_CMD_PAYLOAD_T *)(pPayload->param);
        APP_USB_HID_CMD_INSTANCE_T* pInstance = app_usb_hid_cmd_handler_get_entry_pointer_from_cmd_code((APP_USB_HID_CMD_CODE_E)(pPayload_data->cmdCode));
        // execute the command handler
        if (pInstance)
        {
            pInstance->cmdHandler((APP_USB_HID_CMD_CODE_E)(pPayload_data->cmdCode), pPayload_data->param, pPayload_data->paramLen);
        } else {
            USB_HID_TRACE(0,"%s: worry command id!", __func__);
        }
    }
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
    else if(report_id == HID_REPORT_ID_HID_LED){
        USB_AUDIO_HID_KEY_T *pPayload_data = (USB_AUDIO_HID_KEY_T *)transfer->data;
        usb_audio_app_hid_key_set_report_data_parse(pPayload_data);
    }
#endif
#ifdef USB_AUDIO_CUSTOM_ASP
    else if(report_id == HID_REPORT_ID_ASP){
        APP_USB_HID_ASP_CMD_PAYLOAD_T* pPayload = (APP_USB_HID_ASP_CMD_PAYLOAD_T *)transfer->data;
        _usb_cmd_asp_handle(pPayload->message_data, pPayload->primary_payload_data, (((uint16_t)pPayload->primary_payload_len_high_byte << 8) | (uint16_t)pPayload->primary_payload_len_low_byte));
    }
#endif
#ifdef CFU_USB_OTA_ENABLE
    else if(report_id == REPORT_ID_VERSIONS_FEATURE  && report_type == HID_REPORT_TYPE_FEATURE)  //does not have any parameters
    {
        payload_version_len = HID_CFU_VERSION_LENGTH_BYTES;
        cfu_get_version(&payload_version);
        app_usb_hid_send_rsp((uint8_t*)&payload_version, payload_version_len);
    }else if(report_id == REPORT_ID_OFFER_OUTPUT)
    {
        FWUPDATE_OFFER_COMMAND* pCommand = (FWUPDATE_OFFER_COMMAND*)transfer->data;
        payload_offer_len = HID_CFU_OFFER_RESPONSE_LENGTH_BYTES;
        CFU_USB_TRACE(0,"cfu offer:");
        CFU_USB_DUMP8("%02x ", transfer->data, sizeof(FWUPDATE_OFFER_COMMAND));
        if (pCommand->componentInfo.force_reset)
            update_success_to_reset = true;
        cfu_firmware_offer(pCommand, &payload_offer);
        app_usb_hid_send_rsp((uint8_t*)&payload_offer, payload_offer_len);
    }else if(report_id == REPORT_ID_PAYLOAD_OUTPUT && report_type == HID_REPORT_TYPE_OUTPUT)
    {
        FWUPDATE_CONTENT_COMMAND* pCommand = (FWUPDATE_CONTENT_COMMAND *)transfer->data;
        CFU_USB_TRACE(0,"cfu content:");
        CFU_USB_DUMP8("%02x ", transfer->data, sizeof(FWUPDATE_CONTENT_COMMAND));
        payload_content_len = HID_CFU_CONTENT_RESPONSE_LENGTH_BYTES;
        cfu_firmware_content(pCommand, &payload_content);
        CFU_USB_TRACE(0,"cfu content SN 0x%x status 0x%x", payload_content.sequence_number, payload_content.status_code);
        app_usb_hid_send_rsp((uint8_t*)&payload_content, payload_content_len);
        cfu_usb_rece_timer_start();
        if (payload_content.status_code != FIRMWARE_UPDATE_SUCCESS)
        {
            ClearUpdateInfo();
        }
        if(pCommand->flags == FIRMWARE_UPDATE_FLAG_LAST_BLOCK)
        {
            cfu_usb_rece_timer_stop();
        }
        if (update_success_to_reset && pCommand->flags == FIRMWARE_UPDATE_FLAG_LAST_BLOCK
                                    && payload_content.status_code == FIRMWARE_UPDATE_SUCCESS)
            pmu_reboot();
    }
#endif
    else {
        USB_HID_TRACE(1,"%s: worry repord id!", __func__);
    }
    return true;
}

static void usb_hid_app_device_param_flush (APP_USB_HID_CMD_DEVICE_PARAM_T* wr_info_ptr)
{
    if (wr_info_ptr) {
        APP_USB_HID_CMD_DEVICE_PARAM_T rd_info;
        uint8_t i = 3;
        do {
            hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)DEVICE_PARAM_SEC_START_ADDR, DEVICE_PARAM_SEC_SIZE);
            hal_norflash_write(HAL_FLASH_ID_0, (uint32_t)DEVICE_PARAM_SEC_START_ADDR, (uint8_t*)wr_info_ptr, sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T));
            hal_norflash_read(HAL_FLASH_ID_0, (uint32_t)DEVICE_PARAM_SEC_START_ADDR, (uint8_t*)(&rd_info), sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T));

            if (!memcmp(wr_info_ptr, &rd_info, sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T))) {
                break;
            }
            i--;
        } while (i);

        if (!i) {
            USB_HID_TRACE(1,"%s fail", __func__);
        } else {
            USB_HID_TRACE(2,"%s OK[%d]", __func__, i);
        }
    }
}

static void usb_hid_app_device_param_update(void){
    APP_USB_HID_CMD_DEVICE_PARAM_T wr_info_ptr;
    memcpy(&wr_info_ptr, &global_device_param, sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T));
    usb_hid_app_device_param_flush(&wr_info_ptr);
}

uint8_t usb_hid_app_kid_mode_get(void){
    return global_device_param.kid_mode;
}

uint8_t usb_hid_app_volume_max_get(void){
    uint8_t vol = 100;
    if(usb_hid_app_kid_mode_get()){
        vol = global_device_param.volume_max;
    }
    return vol;
}

uint8_t usb_hid_app_tones_status_get(void){
    return global_device_param.tones_status;
}

uint8_t usb_hid_app_tones_volume_get(void){
    return global_device_param.tones_volume;
}

uint8_t usb_hid_app_get_ptt_mode(void){
    return global_device_param.ptt_mode;
}

void usb_hid_app_set_component_status(uint8_t teams, uint8_t hook, uint8_t mute, uint8_t vol_up, uint8_t vol_down)
{
    global_device_param.component_status.component_type = 0x01;
    if(teams != 0xff)global_device_param.component_status.button_status.teams_button = teams;
    if(teams != 0xff)global_device_param.component_status.button_status.hook_switch_button = hook;
    if(teams != 0xff)global_device_param.component_status.button_status.mute_button = mute;
    if(teams != 0xff)global_device_param.component_status.button_status.volume_up_button = vol_up;
    if(teams != 0xff)global_device_param.component_status.button_status.volume_down_button = vol_down;
}

void usb_hid_app_param_init(void){
    USB_HID_TRACE(1,"%s enter", __func__);
    APP_USB_HID_CMD_DEVICE_PARAM_T *device_param = (APP_USB_HID_CMD_DEVICE_PARAM_T *)DEVICE_PARAM_SEC_START_ADDR;
    memcpy(&global_device_param, device_param, sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T));
    USB_HID_TRACE(1, "==> global_device_param.flash_init = %d", global_device_param.flash_init);
    if(global_device_param.flash_init == 0x01){
        USB_HID_TRACE(0, "==> flash do not init !!!");
        return;
    }
    memset(&global_device_param, 0, sizeof(APP_USB_HID_CMD_DEVICE_PARAM_T));
    global_device_param.flash_init = 0x01;
    global_device_param.kid_mode = HID_DEFAULT_KID_MODE;
    global_device_param.volume_max = HID_DEFAULT_KID_MODE_MAX_VOLUME;
    global_device_param.tones_status = HID_DEFAULT_TONE_STATUS;
    global_device_param.tones_volume = HID_DEFAULT_TONE_VOLUME;
    memset(global_device_param.container_id, 0, sizeof(global_device_param.container_id));
    memcpy(global_device_param.color_name, HID_DEFAULT_COLOR_NAME, sizeof(HID_DEFAULT_COLOR_NAME));
    global_device_param.serial_number_len = sizeof(HID_DEFAULT_SERIAL_NUMBER);
    memcpy(global_device_param.serial_number, HID_DEFAULT_SERIAL_NUMBER, sizeof(HID_DEFAULT_SERIAL_NUMBER));
    global_device_param.eq_index = 0;
    global_device_param.mte_mode.mode = HID_DEFAULT_MTE_MODE_ENABLE;
    // device capability
    global_device_param.device_capability.scheme_code = HID_DEFAULT_SCHEME_CODE;
    global_device_param.device_capability.product_brand = HID_DEFAULT_PRODUCT_BRAND;
    global_device_param.device_capability.out_factory_date_year = HID_DEFAULT_DATE_YEAR;
    global_device_param.device_capability.out_factory_date_month = HID_DEFAULT_DATE_MONTH;
    global_device_param.device_capability.out_factory_date_day = HID_DEFAULT_DATE_DAY;
    global_device_param.device_capability.hw_version.hw_minor_version = HID_DEFAULT_HW_MINOR_VERSION;
    global_device_param.device_capability.hw_version.major_version.hw_major_version = HID_DEFAULT_HW_MAJOR_VERSION;
    global_device_param.device_capability.hw_version.major_version.build_stages = HID_DEFAULT_BUILD_STAGES;
    global_device_param.mic_eq_enable_flag = 0;
    global_device_param.spk_eq_enable_flag = 0;
    global_device_param.ptt_mode = HID_DEFAULT_PTT_MODE;
    usb_hid_app_set_component_status(0, 0, 0, 0, 0);
    usb_hid_app_device_param_update();
}

#ifdef USB_AUDIO_CUSTOM_ASP
#include "tgt_hardware.h"
#include "aud_section.h"
extern const struct_anc_cfg * anc_coef_list_50p7k[ANC_COEF_LIST_NUM];

static HWTIMER_ID g_asp_hwtimer_id;
static void usb_hid_asp_timer_cb(void *param){
    if(usb_hid_asp_device_conn_status_get()){
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
        usb_audio_hid_set_event(USB_AUDIO_HID_KEY_MS_TEAM, HID_REPORT_ID_KEY_TEAMS, HID_REPORT_KEY_PACKET_SIZE, 1);
        usb_audio_hid_set_event(USB_AUDIO_HID_KEY_MS_TEAM, HID_REPORT_ID_KEY_TEAMS, HID_REPORT_KEY_PACKET_SIZE, 0);
#endif
    }
}

void usb_hid_asp_report_start(uint32_t ms){
    hwtimer_start(g_asp_hwtimer_id, ms*16);
}

void usb_hid_asp_report_button_press_info(uint8_t hook_onoff, uint8_t mute_onoff){
    global_device_asp_param.telemetry_event.button_press_info[0] = 0x20;
    if(hook_onoff != 0xff){
        global_device_asp_param.telemetry_event.button_press_info[1] = hook_onoff;
    }
    global_device_asp_param.telemetry_event.button_press_info[2] = 0x2F;
    if(mute_onoff != 0xff){
        global_device_asp_param.telemetry_event.button_press_info[3] = mute_onoff;
    }
    global_device_asp_param.telemetry_event.button_press_info_report_len = 4;
}

static void usb_hid_asp_init(void){
    g_asp_hwtimer_id = hwtimer_alloc(usb_hid_asp_timer_cb, NULL);
    global_device_asp_param.context.report_primary_event = CT_MAX;
    global_device_asp_param.notification_event = NT_MAX;

    sprintf((char *)global_device_asp_param.telemetry_event.cur_firmware_endpoint_buf, "V%d.%d", HID_ASP_VERSION_POSITIVE, HID_ASP_VERSION_NEGATIVE);
    sprintf((char *)global_device_asp_param.telemetry_event.cur_firmware_dongle_buf, "V%d.%d", HID_ASP_VERSION_POSITIVE, HID_ASP_VERSION_NEGATIVE);
    global_device_asp_param.telemetry_event.no_answersetting = 0x00;
    global_device_asp_param.telemetry_event.audio_codec_used = 0x02;
    global_device_asp_param.telemetry_event.dsp_effect_enable = 0x00000040;
    global_device_asp_param.telemetry_event.hardmute = 0x00;
    global_device_asp_param.telemetry_event.headset_worn = 0xFF;
    global_device_asp_param.telemetry_event.battery_level = 0xFF;
    global_device_asp_param.telemetry_event.device_ready = 0x00;
    global_device_asp_param.telemetry_event.radio_link_quality = 0xFF;
    global_device_asp_param.telemetry_event.wirless_device_conn_status = 0xFF;
    global_device_asp_param.telemetry_event.people_count = 0xFF;
    global_device_asp_param.telemetry_event.local_conference_count = 0xFFFFFFFF;
    global_device_asp_param.telemetry_event.voice_activity = 0xFF;
    memcpy(global_device_asp_param.telemetry_event.endpoint_device_model_id, "Microsoft Modern USB Headset Mono", sizeof("Microsoft Modern USB Headset Mono"));
    usb_hid_asp_report_button_press_info(0x00, 0x00);

    // TODO : approximate calculation
    struct_anc_cfg *anc_cfg = (struct_anc_cfg *)anc_coef_list_50p7k[0];
    int32_t sidetone_level_d = (anc_cfg->anc_cfg_ff_l.total_gain * 5) / 1024;
    int32_t sidetone_level_f = (((anc_cfg->anc_cfg_ff_l.total_gain * 5) % 1024) * 100) / 1024;
    USB_HID_TRACE(4,"%s ===============> [%d  %d] ==> %d.%d !!!", __func__, sidetone_level_d, sidetone_level_f, sidetone_level_d, sidetone_level_f);
    sprintf((char *)global_device_asp_param.telemetry_event.side_tone_level, "%d.%d", sidetone_level_d, sidetone_level_f);

    memcpy(global_device_asp_param.telemetry_event.microphone_device_status, "normal", sizeof("normal"));
    memcpy(global_device_asp_param.telemetry_event.spk_device_status, "normal", sizeof("normal"));
    memcpy(global_device_asp_param.telemetry_event.audio_stream_status, "normal", sizeof("normal"));
}

static void usb_hid_asp_deinit(void){
    hwtimer_free(g_asp_hwtimer_id);
    g_asp_hwtimer_id = NULL;
}
#endif

static void usb_hid_app_timer_cb(void *param){
    usb_hid_app_device_param_update();
}

void usb_hid_app_flash_flush_start(uint32_t ms){
    hwtimer_start(g_hwtimer_id, ms*16);
}

int usb_hid_app_init_onoff(bool onoff){
    static bool isRun =  false;

    USB_HID_TRACE(2,"%s onoff[%d] enter !!!", __func__, onoff);
    if (isRun == onoff)
        return 0;

    if(onoff){
        usb_hid_app_param_init();
        usb_hid_epint_out_calbacks_register(&usb_hid_epint_out_recv_calbacks);

#ifdef USB_AUDIO_CUSTOM_ASP
        usb_hid_asp_init();
#endif
        g_hwtimer_id = hwtimer_alloc(usb_hid_app_timer_cb, NULL);
    }else{
        usb_hid_epint_out_calbacks_register(NULL);
#ifdef USB_AUDIO_CUSTOM_ASP
        usb_hid_asp_deinit();
#endif
        hwtimer_free(g_hwtimer_id);
        g_hwtimer_id = NULL;
    }

    isRun = onoff;
    USB_HID_TRACE(1,"%s end !!!", __func__);
    return 0;
}

void app_usb_hid_send_rsp(uint8_t* cmdParam, uint16_t cmdParam_len)
{
    return hid_epint_in_send_report(cmdParam, cmdParam_len);
}

static void usb_hid_get_device_capability_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetDeviceCapability;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = HID_REPORT_DEVICE_CAPABILITY_LEN;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, (uint8_t *)&global_device_param.device_capability, HID_REPORT_DEVICE_CAPABILITY_LEN);

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_serial_number_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetSerialNumber;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = global_device_param.serial_number_len;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, global_device_param.serial_number, global_device_param.serial_number_len);

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_container_id_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetContainerId;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = HID_REPORT_CONTAINER_ID_LEN;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, global_device_param.container_id, sizeof(global_device_param.container_id));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_color_name_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetColorName;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = HID_REPORT_COLOR_NAME_LEN;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, global_device_param.color_name, sizeof(global_device_param.color_name));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_reset_user_configuration_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    global_device_param.ptt_mode = HID_DEFAULT_PTT_MODE;
    global_device_param.kid_mode = HID_DEFAULT_KID_MODE;
    global_device_param.volume_max = HID_DEFAULT_KID_MODE_MAX_VOLUME;
    global_device_param.tones_status = HID_DEFAULT_TONE_STATUS;
    global_device_param.tones_volume = HID_DEFAULT_TONE_VOLUME;
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_ResetUserConfiguration;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_firmware_version_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    uint8_t hid_firmware_version[6];
    uint16_t dataLen = 6;
    uint8_t p_numb = HID_ASP_VERSION_POSITIVE;      // positive number
    uint16_t n_numb = HID_ASP_VERSION_NEGATIVE;     // negative number

    hid_firmware_version[1] = n_numb&0xff;
    hid_firmware_version[2] = (n_numb >> 8) & 0xff;
    hid_firmware_version[3] = p_numb;
    
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetFirmwareVersion;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = dataLen;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, hid_firmware_version, dataLen);

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_current_eq_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    global_device_param.eq_index = ptrParam[0];
    USB_HID_TRACE(1, "eq_index = %d ", global_device_param.eq_index);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetCurrentEQ;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_current_eq_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetCurrentEQ;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x01;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    pResponse_data->rspData[0] = global_device_param.eq_index;

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_tones_status_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    global_device_param.tones_status = ptrParam[0];
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetTonesStatus;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_tones_status_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetTonesStatus;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x01;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    pResponse_data->rspData[0] = global_device_param.tones_status;

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_tones_volume_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(paramLen == 0x01){
        if(global_device_param.tones_volume != ptrParam[0]){
            global_device_param.tones_volume = ptrParam[0];
            USB_HID_TRACE(1,"tones_volume = %d", global_device_param.tones_volume);
        }
    }else{
        USB_HID_TRACE(1,"paramLen = %d error", paramLen);
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetTonesVolume;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_tones_volume_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetTonesVolume;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x01;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    pResponse_data->rspData[0] = global_device_param.tones_volume;

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_kid_mode_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    global_device_param.kid_mode = ptrParam[0];
    global_device_param.volume_max = ptrParam[1];
    if(global_device_param.kid_mode == 0x00){
        // TODO:disabled kid mode
    }else{
        // TODO:enabled kid mode
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    // set volume max
    usb_audio_enqueue_cmd(AUDIO_CMD_SET_VOLUME);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetKidMode;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_kid_mode_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetKidMode;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x02;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    pResponse_data->rspData[0] = global_device_param.kid_mode;
    pResponse_data->rspData[1] = global_device_param.volume_max;

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_enable_mute_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    global_device_param.ptt_mode = ptrParam[0];
    if(global_device_param.ptt_mode == 0x00){
        // TODO:disabled mute
    }else{
        // TODO:enabled mute
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetEnableMute;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_enable_mute_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetEnableMute;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x01;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    pResponse_data->rspData[0] = global_device_param.ptt_mode;

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_test_mode_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    global_device_param.mte_mode.mode = ((ptrParam[0] >> 3) & 0X01);
    USB_HID_TRACE(1,"mode = %d", global_device_param.mte_mode.mode);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetTestMode;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_test_mode_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetTestMode;
    if(global_device_param.mte_mode.mode){
        pResponse_data->cmdRetStatus = STATUS_OK;
    }else{
        pResponse_data->cmdRetStatus = STATUS_FAIL;
    }
    pResponse_data->rspDataLen = 0x04;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, (uint8_t *)&global_device_param.mte_mode, pResponse_data->rspDataLen);

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_device_capability_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    uint8_t cpy_len = HID_REPORT_DEVICE_CAPABILITY_LEN > paramLen ? paramLen : HID_REPORT_DEVICE_CAPABILITY_LEN;
    USB_HID_TRACE(3, "cpy_len = %d  hardware version = [%d.%d]", cpy_len, ptrParam[6], ptrParam[5]);
    APP_USB_HID_CMD_DEVICE_CAPABILITY_T *d_ca = (APP_USB_HID_CMD_DEVICE_CAPABILITY_T *)ptrParam;
    memcpy(&global_device_param.device_capability, d_ca, cpy_len);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetDeviceCapability;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_serial_number_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    uint8_t cpy_len = HID_REPORT_SERIAL_NUMBER_LEN > paramLen ? paramLen : HID_REPORT_SERIAL_NUMBER_LEN;
    USB_HID_TRACE(0, "cpy_len = %d", cpy_len);
    memcpy(global_device_param.serial_number, ptrParam, cpy_len);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetSerialNumber;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_led_status_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(global_device_param.mte_mode.mode == 0)return;
    if(paramLen == 0x02){
        APP_USB_HID_CMD_KEY_LEN_STATUS *led_ptr = (APP_USB_HID_CMD_KEY_LEN_STATUS *)ptrParam;
        USB_HID_TRACE(3, "teams = %d hook = %d mute = %d", led_ptr->teams_led_status, led_ptr->hook_switch_led_status, led_ptr->mute_led_status);
        // set led status
        if(led_ptr->teams_led_status){
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_solid_onoff(true);
#endif
        }else{
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_solid_onoff(false);
#endif
        }

        if(led_ptr->hook_switch_led_status){
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_hook_led_solid_onoff(true);
#endif
        }else{
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_hook_led_solid_onoff(false);
#endif
        }

        if(led_ptr->mute_led_status){
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_mute_led_solid_onoff(true);
#endif
        }else{
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_mute_led_solid_onoff(false);
#endif
        }
    }
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetLedStatus;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_container_id_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    uint8_t cpy_len = HID_REPORT_CONTAINER_ID_LEN > paramLen ? paramLen : HID_REPORT_CONTAINER_ID_LEN;
    USB_HID_TRACE(0, "cpy_len = %d", cpy_len);
    memcpy(global_device_param.container_id, ptrParam, cpy_len);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetContainerID;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_device_reboot_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_DeviceReboot;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);

    // reboot
    pmu_reboot();
}

static void usb_hid_factory_reset_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    global_device_param.mte_mode.mode = HID_DEFAULT_MTE_MODE_ENABLE;
    global_device_param.ptt_mode = HID_DEFAULT_PTT_MODE;
    global_device_param.kid_mode = HID_DEFAULT_KID_MODE;
    global_device_param.volume_max = HID_DEFAULT_KID_MODE_MAX_VOLUME;
    global_device_param.tones_status = HID_DEFAULT_TONE_STATUS;
    global_device_param.tones_volume = HID_DEFAULT_TONE_VOLUME;
    global_device_param.eq_index = 0;
    global_device_param.mic_eq_enable_flag = 0;
    global_device_param.spk_eq_enable_flag = 0;
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);

    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_FactoryReset;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_set_color_name_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(1,"%s enter", __func__);
    if(global_device_param.mte_mode.mode == 0)return;
    uint8_t cpy_len = HID_REPORT_COLOR_NAME_LEN > paramLen ? paramLen : HID_REPORT_COLOR_NAME_LEN;
    USB_HID_TRACE(0, "cpy_len = %d", cpy_len);
    memcpy(global_device_param.color_name, ptrParam, cpy_len);
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_SetColorName;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_enable_mic_eq_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(global_device_param.mte_mode.mode == 0)return;
    global_device_param.mic_eq_enable_flag = ptrParam[0];
    USB_HID_TRACE(1,"mic_eq_enable_flag = %d", global_device_param.mic_eq_enable_flag);
    if(global_device_param.mic_eq_enable_flag == 0x00){
        // TODO : disable mic eq
    }else{
        // TODO : enable mic eq
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_EnableMicEQ;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_enable_speaker_eq_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(global_device_param.mte_mode.mode == 0)return;
    global_device_param.spk_eq_enable_flag = ptrParam[0];
    USB_HID_TRACE(1,"mic_eq_enable_flag = %d", global_device_param.spk_eq_enable_flag);
    if(global_device_param.spk_eq_enable_flag == 0x00){
        // TODO : disable mic eq
    }else{
        // TODO : enable mic eq
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_EnableSpeakerEQ;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_enable_mic_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(global_device_param.mte_mode.mode == 0)return;
    global_device_param.spk_eq_enable_flag = ptrParam[0];
    USB_HID_TRACE(1,"mic mute = %d", ptrParam[0]);
    if(global_device_param.mte_mode.mode){
#ifdef USB_AUDIO_CUSTOM_USB_HID_KEY
        if(ptrParam[0] == 0x00){
            mic_mute_state_set(0);
        }else{
            mic_mute_state_set(1);
        }
#endif
    }
    usb_hid_app_flash_flush_start(DEVICE_PARAM_HWTIMER_DELAY_FLUSH);
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_EnableMic;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x00;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));

    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

static void usb_hid_get_component_status_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s paramLen = %d", __func__, paramLen);
    if(global_device_param.mte_mode.mode == 0)return;
    payload.report_id = HID_REPORT_ID_RSP;
    payload_len = HID_REPORT_RSP_PACKET_SIZE;
    APP_USB_HID_CMD_RSP_T* pResponse_data = (APP_USB_HID_CMD_RSP_T *)(payload.param);
    pResponse_data->cmdCodeToRsp = OP_GetComponentStatus;
    pResponse_data->cmdRetStatus = STATUS_OK;
    pResponse_data->rspDataLen   = 0x05;
    memset(pResponse_data->rspData, 0, sizeof(pResponse_data->rspData));
    memcpy(pResponse_data->rspData, (uint8_t *)&global_device_param.component_status, pResponse_data->rspDataLen);
    app_usb_hid_send_rsp((uint8_t*)&payload, payload_len);
}

#ifdef USB_AUDIO_CUSTOM_ASP
static void usb_hid_asp_set_cur_telemetry_event(APP_USB_HID_ASP_TELEMETRY_EVENT_E event){
    global_device_asp_param.cur_telemetry_event = event;
}

uint8_t usb_hid_asp_device_conn_status_get(void){
    return global_device_asp_param.telemetry_event.device_ready;
}

static void usb_hid_asp_device_ready(void){
    uint8_t ready_buf[4] = {0x00, 0x01, TM_DeviceReady, 0X01};
    usb_hid_asp_message_context_report(CT_Telemetry, ready_buf, 4, true);
}

static void usb_hid_asp_report_init_param(void){
    uint8_t ready_buf[7] = {0x00, 0x05, TM_DspEffectsEnableByHeadset, 0x00, 0x00, 0x00, 0x00};
    ready_buf[3] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 24) & 0xff;
    ready_buf[4] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 16) & 0xff;
    ready_buf[5] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 8) & 0xff;
    ready_buf[6] = global_device_asp_param.telemetry_event.dsp_effect_enable & 0xff;
    usb_hid_asp_message_context_report(CT_Telemetry, ready_buf, 7, true);
}

void usb_hid_asp_notification_event_set(APP_USB_ASP_CMD_NOTIFICATION_EVENT_E e){
    global_device_asp_param.notification_event = e;
}

APP_USB_ASP_CMD_NOTIFICATION_EVENT_E usb_hid_asp_notification_event_get(void){
    return global_device_asp_param.notification_event;
}

static void usb_hid_asp_message_version_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);

    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = 0x01;
}

static void usb_hid_asp_message_enum_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);

    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = 0x27;

    // device ready
    global_device_asp_param.telemetry_event.device_ready = 0x01;
    usb_hid_asp_device_ready();
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
    usb_audio_app_teams_led_flash_onoff(false, 0, 0);
    usb_audio_app_teams_led_solid_onoff(true);
#endif
}

static void usb_hid_asp_message_locale_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);
    uint16_t cpt_len = HID_REPORT_ASP_LOCALE_SIZE > paramLen ? paramLen : HID_REPORT_ASP_LOCALE_SIZE;
    memset(global_device_asp_param.locale.locale_str, 0, sizeof(global_device_asp_param.locale.locale_str));
    memcpy(global_device_asp_param.locale.locale_str, ptrParam, cpt_len);
    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = 0x01;
}

static void usb_hid_asp_message_notification_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);
    uint8_t event = ptrParam[0];
    switch(event){
        case NT_NoCurrentScheduled:
            USB_HID_TRACE(1, "====> NT_NoCurrentScheduled");
            usb_hid_asp_notification_event_set(NT_NoCurrentScheduled);
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_pulsing_onoff(false);
            usb_audio_app_teams_led_solid_onoff(true);
#endif
            break;
        case NT_MissIncomingCall:
            USB_HID_TRACE(1, "====> NT_MissIncomingCall");
            usb_hid_asp_notification_event_set(NT_MissIncomingCall);
            usb_hid_asp_message_context_report(CT_TeamsButtonCallTab, NULL, 0, false);
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_pulsing_onoff(true);
#endif
            break;
        case NT_UpcomingScheduledMeeting:
            USB_HID_TRACE(1, "====> NT_UpcomingScheduledMeeting");
            usb_hid_asp_notification_event_set(NT_UpcomingScheduledMeeting);
            usb_hid_asp_message_context_report(CT_TeamsButtonScheduledMeeting, NULL, 0, false);
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_pulsing_onoff(true);
#endif
            break;
        case NT_UncheckedVoiceMail:
            USB_HID_TRACE(1, "====> NT_UncheckedVoiceMail");
            usb_hid_asp_notification_event_set(NT_UncheckedVoiceMail);
            usb_hid_asp_message_context_report(CT_TeamsButtonVoiceMail, NULL, 0, false);
#ifdef USB_AUDIO_CUSTOM_USB_HID_LED
            usb_audio_app_teams_led_pulsing_onoff(true);
#endif
            break;
        case NT_Telemetry:
            USB_HID_TRACE(1, "====> NT_Telemetry");
            usb_hid_asp_notification_event_set(NT_Telemetry);
            break;
        default:break;
    }
    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = 0x01; // 0x01:success;0x00:fail/reject
}

void usb_hid_asp_message_context_report(APP_USB_ASP_CMD_CONTEXT_EVENT_E event, uint8_t *telemetry_event_buf, uint8_t telemetry_event_buf_len, bool report_onoff){
    global_device_asp_param.context.report_primary_event = event;
    global_device_asp_param.context.telemetry_event_buf_len = telemetry_event_buf_len;
    memset(global_device_asp_param.context.telemetry_event_buf, 0, sizeof(global_device_asp_param.context.telemetry_event_buf));
    if((global_device_asp_param.context.telemetry_event_buf_len > 0) && telemetry_event_buf){
        memcpy(global_device_asp_param.context.telemetry_event_buf, telemetry_event_buf, telemetry_event_buf_len);
    }

    if(event == CT_Telemetry){
        usb_hid_asp_set_cur_telemetry_event((APP_USB_HID_ASP_TELEMETRY_EVENT_E)global_device_asp_param.context.telemetry_event_buf[2]);
    }
    if(global_device_asp_param.telemetry_event.device_ready && report_onoff){
        usb_hid_asp_report_start(10);
    }
}

static void usb_hid_asp_message_context_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);

    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    if(global_device_asp_param.context.telemetry_event_buf_len > 0){
        asp_payload.message_data.metadata_flags = 0x01;
    }else{
        asp_payload.message_data.metadata_flags = 0x00;
    }
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = global_device_asp_param.context.report_primary_event;
    memcpy(asp_payload.primary_payload_data+1, global_device_asp_param.context.telemetry_event_buf, global_device_asp_param.context.telemetry_event_buf_len);

    if(global_device_asp_param.cur_telemetry_event == TM_DeviceReady){
        usb_hid_asp_report_init_param();
    }else{
        usb_hid_asp_set_cur_telemetry_event(TM_MAX);
    }
}

static void usb_hid_asp_message_call_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);

    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    asp_payload.primary_payload_len_high_byte = 0x00;
    asp_payload.primary_payload_len_low_byte = 0x01;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    asp_payload.primary_payload_data[0] = 0x01;
}

static void usb_hid_asp_message_telemetry_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen){
    uint16_t __unused report_len = 0;
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);

    asp_payload.report_id = HID_REPORT_ID_ASP;
    asp_payload_len = HID_REPORT_ASP_PACKET_SIZE;
    asp_payload.message_data.message_id = flag.message_id;
    asp_payload.message_data.metadata_flags = 0x00;
    memset(asp_payload.primary_payload_data, 0, sizeof(asp_payload.primary_payload_data));
    for(int i=0; i<paramLen;i++){
        uint8_t cur_type = ptrParam[i];
        USB_HID_TRACE(1, "cur telemetry type [%d]", cur_type);
        asp_payload.primary_payload_data[report_len++] = cur_type;
        switch(cur_type){
            case TM_CurrentFirmwareEndpoint:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.cur_firmware_endpoint_buf, strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_endpoint_buf));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_endpoint_buf) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_endpoint_buf);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_CurrentFirmwareDongle:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.cur_firmware_dongle_buf, strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_dongle_buf));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_dongle_buf) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.cur_firmware_dongle_buf);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DonToAnswerSetting:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.no_answersetting;
                break;
            case TM_EndpointDeviceModelId:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.endpoint_device_model_id, strlen((char *)global_device_asp_param.telemetry_event.endpoint_device_model_id));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.endpoint_device_model_id) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.endpoint_device_model_id);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceSerialNumberEndpoint:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_param.serial_number, strlen((char *)global_device_param.serial_number));
                    report_len += strlen((char *)global_device_param.serial_number) == 0 ? 1:strlen((char *)global_device_param.serial_number);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceSerialNumberDongle:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_param.serial_number, strlen((char *)global_device_param.serial_number));
                    report_len += strlen((char *)global_device_param.serial_number) == 0 ? 1:strlen((char *)global_device_param.serial_number);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_UserModifiedSideToneLevel:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.side_tone_level, strlen((char *)global_device_asp_param.telemetry_event.side_tone_level));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.side_tone_level) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.side_tone_level);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_AudioCodecUsed:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.audio_codec_used;
                break;
            case TM_DspEffectsEnableByHeadset:
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 24) & 0xff;
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 16) & 0xff;
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.dsp_effect_enable >> 8) & 0xff;
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.dsp_effect_enable & 0xff;
                break;
            case TM_Hardmute:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.hardmute;
                break;
            case TM_HeadsetWorn:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.headset_worn;
                break;
            case TM_BatteryLevel:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.battery_level;
                break;
            case TM_DeviceReady:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.device_ready;
                break;
            case TM_RadioLinkQuality:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.radio_link_quality;
                break;
            case TM_ErrorMessage:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.error_message, strlen((char *)global_device_asp_param.telemetry_event.error_message));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.error_message) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.error_message);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_ButtonPressInfo:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.button_press_info, global_device_asp_param.telemetry_event.button_press_info_report_len);
                    report_len += global_device_asp_param.telemetry_event.button_press_info_report_len == 0 ? 1:global_device_asp_param.telemetry_event.button_press_info_report_len;
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_ConnectedWirelessDeviceChange:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.wirless_device_conn_status;
                break;
            case TM_PeopleCount:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.people_count;
                break;
            case TM_LocalConferenceCount:
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.local_conference_count >> 24) & 0xff;
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.local_conference_count >> 16) & 0xff;
                asp_payload.primary_payload_data[report_len++] = (global_device_asp_param.telemetry_event.local_conference_count >> 8) & 0xff;
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.local_conference_count & 0xff;
                break;
            case TM_DeviceMicrophoneStatus:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.microphone_device_status, strlen((char *)global_device_asp_param.telemetry_event.microphone_device_status));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.microphone_device_status) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.microphone_device_status);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceSpeakerStatus:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.spk_device_status, strlen((char *)global_device_asp_param.telemetry_event.spk_device_status));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.spk_device_status) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.spk_device_status);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceAudioStreamsStatus:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.audio_stream_status, strlen((char *)global_device_asp_param.telemetry_event.audio_stream_status));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.audio_stream_status) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.audio_stream_status);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceVideoAndMetadataStreamsStatus:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.video_device_metadata_stream_status, strlen((char *)global_device_asp_param.telemetry_event.video_device_metadata_stream_status));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.video_device_metadata_stream_status) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.video_device_metadata_stream_status);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_AiModelNameAndVersion:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.ai_model_name_version, strlen((char *)global_device_asp_param.telemetry_event.ai_model_name_version));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.ai_model_name_version) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.ai_model_name_version);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_DeviceResourceUsage:
                if(paramLen == 1){
                    memcpy(&asp_payload.primary_payload_data[report_len], global_device_asp_param.telemetry_event.device_resource_usage, strlen((char *)global_device_asp_param.telemetry_event.device_resource_usage));
                    report_len += strlen((char *)global_device_asp_param.telemetry_event.device_resource_usage) == 0 ? 1:strlen((char *)global_device_asp_param.telemetry_event.device_resource_usage);
                }else{
                    asp_payload.primary_payload_data[report_len++] = 0x00;
                }
                break;
            case TM_VoiceActivity:
                asp_payload.primary_payload_data[report_len++] = global_device_asp_param.telemetry_event.voice_activity;
                break;
            default:
                USB_HID_TRACE(1, "error telemetry type [%d]", cur_type);
                break;
        }
    }
    asp_payload.primary_payload_len_high_byte = (report_len >> 8) & 0xff;
    asp_payload.primary_payload_len_low_byte = report_len & 0xff;
}

static void _usb_cmd_metadata_handle(uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2, "%s ==> len = %d", __func__, paramLen);
    DUMP8("%x ", ptrParam, paramLen);
}

static void _usb_cmd_asp_handle(APP_USB_HID_MESSAGE_PAYLOAD_T flag, uint8_t* ptrParam, uint16_t paramLen)
{
    uint8_t message_id = flag.message_id;
    uint8_t metadata_flags = flag.metadata_flags;
    USB_HID_TRACE(3, "metadata_flags[%d] message_id[%d] primary_payload_len[%d]", metadata_flags, message_id, paramLen);

    if(metadata_flags == 0x01){
        uint8_t *metadata_ptr = ptrParam + paramLen + 2;
        uint16_t metadata_len = (metadata_ptr[0] << 8) | metadata_ptr[1];
        _usb_cmd_metadata_handle(metadata_ptr+2, metadata_len);
    }else{
        USB_HID_TRACE(1, "without metadata !!!");
    }

    switch(message_id){
        case ASP_MESSAGE_VERSION:
            usb_hid_asp_message_version_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_ENUM:
            usb_hid_asp_message_enum_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_LOCALE:
            usb_hid_asp_message_locale_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_NOTIFICATION:
            usb_hid_asp_message_notification_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_CONTEXT:
            usb_hid_asp_message_context_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_CALL_EVENT:
            usb_hid_asp_message_call_handle(flag, ptrParam, paramLen);
            break;
        case ASP_MESSAGE_TELEMETRY:
            usb_hid_asp_message_telemetry_handle(flag, ptrParam, paramLen);
            break;
        default:break;
    }
}
#endif

static void _usb_cmd_handle(APP_USB_HID_CMD_CODE_E funcCode, uint8_t* ptrParam, uint16_t paramLen)
{
    USB_HID_TRACE(2,"%s enter: %x", __func__,funcCode);
    switch (funcCode)
    {
        case OP_GetDeviceCapability:
            usb_hid_get_device_capability_handler(ptrParam, paramLen);
            break;
        case OP_GetSerialNumber:
            usb_hid_get_serial_number_handler(ptrParam, paramLen);
            break;
        case OP_GetContainerId:
            usb_hid_get_container_id_handler(ptrParam, paramLen);
            break;
        case OP_GetColorName:
            usb_hid_get_color_name_handler(ptrParam, paramLen);
            break;
        case OP_ResetUserConfiguration:
            usb_hid_reset_user_configuration_handler(ptrParam, paramLen);
            break;
        case OP_GetFirmwareVersion:
            usb_hid_get_firmware_version_handler(ptrParam, paramLen);
            break;
        case OP_SetCurrentEQ:
            usb_hid_set_current_eq_handler(ptrParam, paramLen);
            break;
        case OP_GetCurrentEQ:
            usb_hid_get_current_eq_handler(ptrParam, paramLen);
            break;
        case OP_SetTonesStatus:
            usb_hid_set_tones_status_handler(ptrParam, paramLen);
            break;
        case OP_GetTonesStatus:
            usb_hid_get_tones_status_handler(ptrParam, paramLen);
            break;
        case OP_SetTonesVolume:
            usb_hid_set_tones_volume_handler(ptrParam, paramLen);
            break;
        case OP_GetTonesVolume:
            usb_hid_get_tones_volume_handler(ptrParam, paramLen);
            break;
        case OP_SetKidMode:
            usb_hid_set_kid_mode_handler(ptrParam, paramLen);
            break;
        case OP_GetKidMode:
            usb_hid_get_kid_mode_handler(ptrParam, paramLen);
            break;
        case OP_SetEnableMute:
            usb_hid_set_enable_mute_handler(ptrParam, paramLen);
            break;
        case OP_GetEnableMute:
            usb_hid_get_enable_mute_handler(ptrParam, paramLen);
            break;
        case OP_SetTestMode:
            usb_hid_set_test_mode_handler(ptrParam, paramLen);
            break;
        case OP_GetTestMode:
            usb_hid_get_test_mode_handler(ptrParam, paramLen);
            break;
        case OP_SetDeviceCapability:
            usb_hid_set_device_capability_handler(ptrParam, paramLen);
            break;
        case OP_SetSerialNumber:
            usb_hid_set_serial_number_handler(ptrParam, paramLen);
            break;
        case OP_SetLedStatus:
            usb_hid_set_led_status_handler(ptrParam, paramLen);
            break;
        case OP_SetContainerID:
            usb_hid_set_container_id_handler(ptrParam, paramLen);
            break;
        case OP_DeviceReboot:
            usb_hid_device_reboot_handler(ptrParam, paramLen);
            break;
        case OP_FactoryReset:
            usb_hid_factory_reset_handler(ptrParam, paramLen);
            break;
        case OP_SetColorName:
            usb_hid_set_color_name_handler(ptrParam, paramLen);
            break;
        case OP_EnableMicEQ:
            usb_hid_enable_mic_eq_handler(ptrParam, paramLen);
            break;
        case OP_EnableSpeakerEQ:
            usb_hid_enable_speaker_eq_handler(ptrParam, paramLen);
            break;
        case OP_EnableMic:
            usb_hid_enable_mic_handler(ptrParam, paramLen);
            break;
        case OP_GetComponentStatus:
            usb_hid_get_component_status_handler(ptrParam, paramLen);
            break;
        // TO ADD: new cmd
        default:
            break;
            ;
    }
}

USB_HID_COMMAND_TO_ADD(OP_GetDeviceCapability, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetSerialNumber, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetContainerId, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetColorName, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_ResetUserConfiguration, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetFirmwareVersion, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetCurrentEQ, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetCurrentEQ, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetTonesStatus, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetTonesStatus, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetTonesVolume, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetTonesVolume, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetKidMode, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetKidMode, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetEnableMute, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetEnableMute, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetTestMode, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetTestMode, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetDeviceCapability, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetSerialNumber, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetLedStatus, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetContainerID, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_DeviceReboot, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_FactoryReset, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_SetColorName, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_EnableMicEQ, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_EnableSpeakerEQ, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_EnableMic, _usb_cmd_handle);
USB_HID_COMMAND_TO_ADD(OP_GetComponentStatus, _usb_cmd_handle);
// TO ADD: new cmd
#endif
