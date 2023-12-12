/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#if defined(DUAL_MIC_RECORDING)
#include "ble_ai_voice.h"
#include "gatt_service.h"

#define REC_VOICE_MAX_LEN (509)

#define rec_service_uuid_128_le 0x00,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65
#define rec_cmd_tx_char_uuid_128_le 0x01,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65
#define rec_cmd_rx_char_uuid_128_le 0x02,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65
#define rec_data_tx_char_uuid_128_le 0x03,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65
#define rec_data_rx_char_uuid_128_le 0x04,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65

GATT_DECL_128_LE_PRI_SERVICE(g_ble_ai_rec_service,
    rec_service_uuid_128_le);

GATT_DECL_128_LE_CHAR(g_ble_ai_rec_rx_cmd_character,
    rec_cmd_rx_char_uuid_128_le,
    GATT_WR_REQ|GATT_WR_CMD,
    ATT_WR_ENC);

GATT_DECL_128_LE_CHAR(g_ble_ai_rec_rx_data_character,
    rec_data_rx_char_uuid_128_le,
    GATT_WR_REQ|GATT_WR_CMD,
    ATT_WR_ENC);

GATT_DECL_128_LE_CHAR(g_ble_ai_rec_tx_cmd_character,
    rec_cmd_tx_char_uuid_128_le,
    GATT_NTF_PROP|GATT_IND_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_ble_ai_rec_tx_cmd_cccd,
    ATT_WR_ENC);

GATT_DECL_128_LE_CHAR(g_ble_ai_rec_tx_data_character,
    rec_data_tx_char_uuid_128_le,
    GATT_NTF_PROP|GATT_IND_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_ble_ai_rec_tx_data_cccd,
    ATT_WR_ENC);

static const gatt_attribute_t g_ble_ai_rec_attr_list[] = {
    /* Service */
    gatt_attribute(g_ble_ai_rec_service),
    /* Characteristics */
    gatt_attribute(g_ble_ai_rec_rx_cmd_character),
    /* Characteristics */
    gatt_attribute(g_ble_ai_rec_rx_data_character),
    /* Characteristics */
    gatt_attribute(g_ble_ai_rec_tx_cmd_character),
        gatt_attribute(g_ble_ai_rec_tx_cmd_cccd),
    /* Characteristics */
    gatt_attribute(g_ble_ai_rec_tx_data_character),
        gatt_attribute(g_ble_ai_rec_tx_data_cccd),
};

void ble_ai_recording_send_data(app_ble_ai_data_send_param_t *param)
{
    enum gatt_evt_type evt_type = param->gatt_event_type;
    const uint8_t *character = (param->data_type == BLE_AI_DATA_TYPE) ? g_ble_ai_rec_tx_data_character : g_ble_ai_rec_tx_cmd_character;
    if (evt_type == GATT_NOTIFY)
    {
        gatts_send_notification(gap_conn_bf(param->conidx), character, param->data, (uint16_t)param->data_len);
    }
    else
    {
        gatts_send_indication(gap_conn_bf(param->conidx), character, param->data, (uint16_t)param->data_len);
    }
}

static int ble_ai_rec_server_callback(gatt_svc_t *svc, gatt_server_event_t event, gatt_server_callback_param_t param)
{
    switch (event)
    {
        case GATT_SERV_EVENT_CHAR_WRITE:
        {
            gatt_server_char_write_t *p = param.char_write;
            uint8_t data_type = (p->character == g_ble_ai_rec_rx_data_character) ? BLE_AI_DATA_TYPE : BLE_AI_CMD_TYPE;
            if (p->value_offset != 0 || p->value_len == 0 || p->value == NULL)
            {
                return false;
            }
            ble_ai_report_data_received(APP_BLE_AI_SPEC_RECORDING, (svc->connhdl << 16)|p->con_idx, data_type, p);
            return true;
        }
        case GATT_SERV_EVENT_DESC_WRITE:
        {
            gatt_server_desc_write_t *p = param.desc_write;
            uint16_t config = CO_COMBINE_UINT16_LE(p->value);
            uint8_t data_type = (p->descriptor == g_ble_ai_rec_tx_data_cccd) ? BLE_AI_DATA_TYPE : BLE_AI_CMD_TYPE;
            ble_ai_report_cccd_config_changed(APP_BLE_AI_SPEC_RECORDING, (svc->connhdl << 16)|p->con_idx, data_type, (uint8_t)config);
            return true;
        }
        case GATT_SERV_EVENT_NTF_TX_DONE:
        case GATT_SERV_EVENT_INDICATE_CFM:
        {
            gatt_server_indicate_cfm_t *p = param.confirm;
            uint8_t data_type = (p->character == g_ble_ai_rec_tx_data_character) ? BLE_AI_DATA_TYPE : BLE_AI_CMD_TYPE;
            ble_ai_report_data_tx_done(APP_BLE_AI_SPEC_RECORDING, svc->con_idx, svc->connhdl, data_type);
            break;
        }
        case GATT_SERV_EVENT_MTU_CHANGED:
        {
            gatt_server_mtu_changed_t *p = param.mtu_changed;
            ble_ai_mtu_exchanged(svc->con_idx, svc->connhdl, p->mtu);
            break;
        }
        case GATT_SERV_EVENT_CONN_CLOSED:
        {
            ble_ai_report_disconnected(APP_BLE_AI_SPEC_RECORDING, svc->con_idx, svc->connhdl);
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

void ble_ai_recording_init(void)
{
    gatts_register_service(g_ble_ai_rec_attr_list, ARRAY_SIZE(g_ble_ai_rec_attr_list), ble_ai_rec_server_callback, NULL);
}

#endif /* DUAL_MIC_RECORDING */