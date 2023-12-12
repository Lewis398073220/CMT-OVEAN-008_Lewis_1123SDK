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
#undef MOUDLE
#define MOUDLE APP_BLE
#include "app_ble.h" // keep first line for CFG_APP_GFPS define
#ifdef GFPS_ENABLED // google fast pair server (server: fast pair provider, client: fast pair seeker)
#include "ble_gfps.h"
#include "bt_drv_interface.h"
#include "nvrecord_fp_account_key.h"
#include "ble_gfps_common.h"
#include "app_bt.h"
#include "hci_i.h"
#include "apps.h"
#include "hwtimer_list.h"

#ifdef IBRT
#include "app_tws_ibrt.h"
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "app_ibrt_customif_cmd.h"
#include "app_tws_ibrt_cmd_handler.h"
#endif

#ifdef SPOT_ENABLED // google eddystone protocol
#include "stdlib.h"
#include "uECC_vli.h"
#endif

#define GFPS_INITIAL_ADV_RAND_SALT 0xFF

#define GFPS_PREFERRED_MTU 512
#define GFPSP_VAL_MAX_LEN 128
#define GFPSP_SYS_ID_LEN 0x08
#define GFPSP_IEEE_CERTIF_MIN_LEN 0x06
#define GFPSP_PNP_ID_LEN 0x07

#define BLE_GFPS_SERVICE_UUID 0xFE2C
#define BLE_SPOT_SERVICE_UUID 0xFEAA

#define GFPS_USE_128BIT_UUID

#ifdef GFPS_USE_128BIT_UUID
#define gfps_model_id_char_uuid_128_le      0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x33,0x12,0x2C,0xFE
#define gfps_keybased_pairing_uuid_128_le   0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x34,0x12,0x2C,0xFE
#define gfps_passkey_char_uuid_128_le       0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x35,0x12,0x2C,0xFE
#define gfps_account_key_uuid_128_le        0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x36,0x12,0x2C,0xFE
#define gfps_additional_data_uuid_128_le    0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x37,0x12,0x2C,0xFE
#define gfps_beacon_actions_uuid_128_le     0xEA,0x0B,0x10,0x32,0xDE,0x01,0xB0,0x8E,0x14,0x48,0x66,0x83,0x38,0x12,0x2C,0xFE

GATT_DECL_PRI_SERVICE(g_ble_gfps_service,
    BLE_GFPS_SERVICE_UUID);

GATT_DECL_128_LE_CHAR(g_ble_gfps_model_id_character,
    gfps_model_id_char_uuid_128_le,
    GATT_RD_REQ,
    ATT_SEC_NONE);

GATT_DECL_128_LE_CHAR(g_ble_gfps_keybased_pairing_character,
    gfps_keybased_pairing_uuid_128_le,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_keybased_pairing_cccd,
    ATT_SEC_NONE);

GATT_DECL_128_LE_CHAR(g_ble_gfps_passkey_character,
    gfps_passkey_char_uuid_128_le,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_passkey_cccd,
    ATT_SEC_NONE);

GATT_DECL_128_LE_CHAR(g_ble_gfps_account_key_character,
    gfps_account_key_uuid_128_le,
    GATT_WR_REQ,
    ATT_SEC_NONE);

GATT_DECL_128_LE_CHAR(g_ble_gfps_additional_data_character,
    gfps_additional_data_uuid_128_le,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_additional_data_cccd,
    ATT_SEC_NONE);

#ifdef SPOT_ENABLED
GATT_DECL_128_LE_CHAR(g_ble_gfps_beacon_actions_character,
    gfps_beacon_actions_uuid_128_le,
    GATT_RD_REQ|GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_beacon_actions_cccd,
    ATT_SEC_NONE);
#endif

#else /* GFPS_USE_128BIT_UUID */
#define gfps_model_id_char_uuid_16      0x1233
#define gfps_keybased_pairing_uuid_16   0x1234
#define gfps_passkey_char_uuid_16       0x1235
#define gfps_account_key_uuid_16        0x1236
#define gfps_additional_data_uuid_16    0x1237
#define gfps_beacon_actions_uuid_16     0x1238

GATT_DECL_PRI_SERVICE(g_ble_gfps_service,
    BLE_GFPS_SERVICE_UUID);

GATT_DECL_CHAR(g_ble_gfps_model_id_character,
    gfps_model_id_char_uuid_16,
    GATT_RD_REQ,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_ble_gfps_keybased_pairing_character,
    gfps_keybased_pairing_uuid_16,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_keybased_pairing_cccd,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_ble_gfps_passkey_character,
    gfps_passkey_char_uuid_16,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_passkey_cccd,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_ble_gfps_account_key_character,
    gfps_account_key_uuid_16,
    GATT_WR_REQ,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_ble_gfps_additional_data_character,
    gfps_additional_data_uuid_16,
    GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_additional_data_cccd,
    ATT_SEC_NONE);

#ifdef SPOT_ENABLED
GATT_DECL_CHAR(g_ble_gfps_beacon_actions_character,
    gfps_beacon_actions_uuid_16,
    GATT_RD_REQ|GATT_WR_REQ|GATT_NTF_PROP,
    ATT_SEC_NONE);

GATT_DECL_CUDD_DESCRIPTOR(g_ble_gfps_beacon_actions_cccd,
    ATT_SEC_NONE);
#endif

#endif /* GFPS_USE_128BIT_UUID */

static const gatt_attribute_t g_ble_gfps_attr_list[] = {
    /* Service */
    gatt_attribute(g_ble_gfps_service),
    /* Characteristics */
    gatt_attribute(g_ble_gfps_model_id_character),
    /* Characteristics */
    gatt_attribute(g_ble_gfps_keybased_pairing_character),
        gatt_attribute(g_ble_gfps_keybased_pairing_cccd),
    /* Characteristics */
    gatt_attribute(g_ble_gfps_passkey_character),
        gatt_attribute(g_ble_gfps_passkey_cccd),
    /* Characteristics */
    gatt_attribute(g_ble_gfps_account_key_character),
    /* Characteristics */
    gatt_attribute(g_ble_gfps_additional_data_character),
        gatt_attribute(g_ble_gfps_additional_data_cccd),
#ifdef SPOT_ENABLED
    /* Characteristics */
    gatt_attribute(g_ble_gfps_beacon_actions_character),
        gatt_attribute(g_ble_gfps_beacon_actions_cccd),
#endif
};

typedef struct ble_gfps_t {
    gatt_svc_t head;
    uint8_t isInitialPairing;
    bt_bdaddr_t seeker_bt_addr;
    bt_bdaddr_t local_bt_addr;
    ble_bdaddr_t local_le_addr;
    uint32_t l2cap_handle;
    uint8_t keybase_pair_key[16];
    uint8_t aesKeyFromECDH[16];
    uint8_t isPendingForWritingNameReq;
    uint8_t isInFastPairing;
#ifdef SPOT_ENABLED
    uint8_t protocol_version;
    uint8_t spot_ring_timeout_timer_id;
    uint8_t spot_ring_nums;
    uint8_t hashed_ring_key[GFPS_RING_KEY_SIZE+8];
    uint8_t nonce[8];
    uint8_t adv_identifer[20];
    uint8_t beacon_time[4];
    uint16_t remaining_ring_time;
    uint8_t control_flag;
    bool enable_unwanted_tracking_mode;
    uint8_t original_flag;
    uint8_t hashed_flag;
    uint8_t Ecc_private_key[20];
    uint8_t matched_account_key[FP_ACCOUNT_KEY_SIZE];
    bool ring_state;
#endif
} ble_gfps_t;

typedef struct {
    uint8_t initialized;
    uint8_t advRandSalt;
    gfps_get_battery_info_cb get_battery_info_handler;
    fp_event_cb gfps_event_cb;
    char gfps_tx_power;
    FastPairInfo fast_pair_info;
    bool isWaitingForFpToConnect;
    gfps_enter_pairing_mode enter_pairing_mode;
    gfps_bt_io_cap_set bt_set_iocap;
    gfps_bt_io_authrequirements_set bt_set_authrequirements;
    uint8_t bt_iocap;
    uint8_t bt_authrequirements;
    uint8_t bondMode;
#ifdef SPOT_ENABLED
    uint8_t protocol_version;
    bool curr_unwanted_tracking_mode;
    uint8_t curr_original_flag;
    uint8_t curr_hashed_flag;
#endif
} ble_gfps_global_t;

static ble_gfps_global_t g_ble_gfps_global;
static att_error_code_t app_gfps_write_key_based_pairing_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p);
static void app_gfps_write_passkey_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p);
static void app_gfps_write_accountkey_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p);
static att_error_code_t app_gfps_write_name_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p);
#ifdef SPOT_ENABLED
static uint8_t app_gfps_write_beacon_actions_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p);
static void spot_ring_timer_stop(ble_gfps_t *gfps);
static const uint8_t *app_gfps_generate_nonce(ble_gfps_t *gfps);
static const uint8_t *app_gfps_get_nonce(ble_gfps_t *gfps);
static uint8_t app_gfps_get_protocol_version(void);
#endif

extern void AES128_ECB_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output);
extern int rand(void);

ble_gfps_global_t *ble_gfps_global(void)
{
    return &g_ble_gfps_global;
}

void big_little_switch(const uint8_t *in, uint8_t *out, uint8_t len)
{
    if (len < 1)
        return;
    for (int i = 0; i < len; i++)
    {
        out[i] = in[len - i - 1];
    }
    return;
}

ble_gfps_t *ble_get_gfps(uint16_t connhdl)
{
    return (ble_gfps_t *)gatts_get_service(connhdl, g_ble_gfps_service, 0);
}

static uint8_t app_gfps_get_addr_type(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    uint8_t type;
    if (g->bondMode == GFPS_BOND_OVER_BT)
    {
        type = GAPM_GEN_RSLV_ADDR;
    }
    else
    {
        type = GAPM_STATIC_ADDR;
    }
    type = GAPM_GEN_RSLV_ADDR;
    return type;
}

static ble_bdaddr_t app_gfps_get_ble_addr(gap_conn_item_t *conn)
{
    ble_bdaddr_t ble_addr;
    uint8_t conidx = conn ? conn->con_idx : GAP_INVALID_CONIDX;
    uint8_t addr_type = app_gfps_get_addr_type();
    if (addr_type == GAPM_STATIC_ADDR)
    {
        ble_addr = app_ble_get_local_identity_addr(conidx);
    }
    else
    {
        ble_addr.addr_type = BT_ADDR_TYPE_RANDOM;
        memcpy(ble_addr.addr, app_ble_get_local_rpa_addr(conidx), GAP_ADDR_LEN);
    }
    return ble_addr;
}

void app_gfps_read_rpa_when_bt_connect(const bt_bdaddr_t *peer_addr)
{
#if 0
    app_ble_read_local_rpa_addr(BT_ADDR_TYPE_PUBLIC, peer_addr);
#else
    const bt_bdaddr_t *ble_addr  = (const bt_bdaddr_t *)bt_get_ble_local_address();
    app_ble_read_local_rpa_addr(BT_ADDR_TYPE_PUBLIC, ble_addr);
#endif
}

#if BLE_AUDIO_ENABLED

static int ble_gfps_l2cap_callback(uintptr_t connhdl, bt_l2cap_event_t event, bt_l2cap_callback_param_t param)
{
    ble_gfps_global_t *g = ble_gfps_global();
    bt_l2cap_param_t *l2cap = param.param_ptr;
    ble_gfps_t *gfps = NULL;
    GFPS_SRV_EVENT_PARAM_T evtParam;

    if (event == BT_L2CAP_EVENT_ACCEPT)
    {
        return BT_L2CAP_ACCEPT_REQ;
    }
    else if (event == BT_L2CAP_EVENT_OPENED)
    {
        CO_LOG_MAIN_S_1(BT_STS_CHANNEL_STATUS, GFPS, l2cap->error_code);
        if (l2cap->error_code)
        {
            return 0;
        }

        gfps = ble_get_gfps((uint16_t)connhdl);
        if (gfps == NULL)
        {
            CO_LOG_ERR_2(BT_STS_NOT_FOUND, event, connhdl);
            return 0;
        }

        gfps->l2cap_handle = l2cap->l2cap_handle;

        evtParam.event = FP_SRV_EVENT_CONNECTED;
        memcpy(evtParam.p.addr.address, &l2cap->peer_addr, sizeof(bt_bdaddr_t));

        TRACE(2,"%s conidx %d", __func__, l2cap->device_id);
        g->gfps_event_cb(gap_zero_based_conidx(l2cap->device_id), &evtParam);
    }
    else
    {
        gfps = ble_get_gfps((uint16_t)connhdl);
        if (gfps == NULL)
        {
            CO_LOG_ERR_2(BT_STS_NOT_FOUND, event, connhdl);
            return 0;
        }

        if (event == BT_L2CAP_EVENT_CLOSED)
        {
            evtParam.event = FP_SRV_EVENT_DISCONNECTED;

            TRACE(2,"%s conidx %d", __func__, l2cap->device_id);
            g->gfps_event_cb(gap_zero_based_conidx(l2cap->device_id), &evtParam);
        }
        else if (event == BT_L2CAP_EVENT_RX_DATA)
        {
            struct pp_buff *ppb = l2cap->tx_priv_rx_ppb;
            evtParam.event = FP_SRV_EVENT_DATA_IND;
            evtParam.p.data.len = ppb->len;
            evtParam.p.data.pBuf = (uint8_t *)ppb_get_data(ppb);

            TRACE(2, "%s len %d", __func__, ppb->len);
            g->gfps_event_cb(gap_zero_based_conidx(l2cap->device_id), &evtParam);
        }
    }

    return 0;
}

void app_gfps_ble_register_callback(fp_event_cb callback)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->gfps_event_cb = callback;
}

uint8_t app_gfps_l2cap_send(uint8_t conidx, uint8_t *data, uint32_t length)
{
    ble_gfps_t *gfps = NULL;

    gfps = ble_get_gfps(gap_zero_based_ble_conidx_as_hdl(conidx));
    if (gfps == NULL)
    {
        CO_LOG_ERR_1(BT_STS_INVALID_CONN_INDEX, conidx);
        return BT_STS_INVALID_CONN_INDEX;
    }

    if (bt_l2cap_send_packet(gfps->head.connhdl, gfps->l2cap_handle, data, (uint16_t)length, NULL) != BT_STS_SUCCESS)
    {
        return BT_STS_FAILED;
    }

    return BT_STS_SUCCESS;
}

#endif

void app_gfps_l2cap_disconnect(uint8_t conidx)
{
    ble_gfps_t *gfps = NULL;

    gfps = ble_get_gfps(gap_zero_based_ble_conidx_as_hdl(conidx));
    if (gfps == NULL)
    {
        CO_LOG_ERR_1(BT_STS_INVALID_CONN_INDEX, conidx);
        return ;
    }

    bt_l2cap_disconnect(gfps->l2cap_handle, 0);
}

static void app_gfps_connected_evt_handler(ble_gfps_t *gfps, gap_conn_item_t *conn)
{
    ble_bdaddr_t local_le_addr = app_gfps_get_ble_addr(conn);
    const uint8_t *local_bt_addr = gap_hci_bt_address()->address;

    big_little_switch(local_le_addr.addr, gfps->local_le_addr.addr, 6);
    gfps->local_le_addr.addr_type = local_le_addr.addr_type;
    TRACE(0,"gfps local LE addr: ");
    DUMP8("0x%02x ", local_le_addr.addr, 6);

    big_little_switch(local_bt_addr, gfps->local_bt_addr.address, 6);
    TRACE(0,"gfps local bt addr: ");
    DUMP8("0x%02x ", local_bt_addr, 6);
}

static void app_gfps_disconnected_evt_handler(ble_gfps_t *gfps)
{
    ble_gfps_global_t *g = ble_gfps_global();

    if (g->bt_set_iocap != NULL)
    {
        g->bt_set_iocap(gfps_get_bt_iocap());
    }

    if(g->bt_set_authrequirements!=NULL)
    {
        g->bt_set_authrequirements(gfps_get_bt_auth());
    }

#ifdef SPOT_ENABLED
    spot_ring_timer_stop(gfps);
#endif

    gfps->isPendingForWritingNameReq = false;
}

static int ble_gfps_server_callback(gatt_svc_t *svc, gatt_server_event_t event, gatt_server_callback_param_t param)
{
    ble_gfps_t *gfps = (ble_gfps_t *)svc;

    switch (event)
    {
        case GATT_SERV_EVENT_CONN_OPENED:
        {
            gatt_server_conn_opened_t *p = param.opened;
            app_gfps_connected_evt_handler(gfps, p->conn);
            break;
        }
        case GATT_SERV_EVENT_CONN_CLOSED:
        {
            app_gfps_disconnected_evt_handler(gfps);
            break;
        }
        case GATT_SERV_EVENT_CHAR_READ:
        {
            gatt_server_char_read_t *p = param.char_read;
            if (p->character == g_ble_gfps_model_id_character)
            {
#ifndef IS_USE_CUSTOM_FP_INFO
                uint32_t model_id = 0x2B677D;
#else
                uint32_t model_id = app_bt_get_model_id();
#endif
                model_id = co_host_to_uint32_le(model_id);
                gatts_write_read_rsp_data(p->ctx, (uint8_t *)&model_id, sizeof(uint32_t));
                return true;
            }
#ifdef SPOT_ENABLED
            else if (p->character == g_ble_gfps_beacon_actions_character)
            {
                uint8_t nonce[9];
                nonce[0]= app_gfps_get_protocol_version(); // protocol major version number
                memcpy(&nonce[1], app_gfps_generate_nonce(gfps), 8);
                gatts_write_read_rsp_data(p->ctx, nonce, sizeof(nonce));
                return true;
            }
#endif
            break;
        }
        case GATT_SERV_EVENT_CHAR_WRITE:
        {
            gatt_server_char_write_t *p = param.char_write;
            if (p->value_offset != 0 || p->value_len == 0 || p->value == NULL)
            {
                return false;
            }
            if (p->character == g_ble_gfps_keybased_pairing_character)
            {
                p->rsp_error_code = app_gfps_write_key_based_pairing_ind_hander(gfps, p);
                return (p->rsp_error_code == ATT_ERROR_NO_ERROR);
            }
            else if (p->character == g_ble_gfps_passkey_character)
            {
                app_gfps_write_passkey_ind_hander(gfps, p);
                return true;
            }
            else if (p->character == g_ble_gfps_account_key_character)
            {
                app_gfps_write_accountkey_ind_hander(gfps, p);
                return true;
            }
            else if (p->character == g_ble_gfps_additional_data_character)
            {
                p->rsp_error_code = app_gfps_write_name_ind_hander(gfps, p);
                return (p->rsp_error_code == ATT_ERROR_NO_ERROR);
            }
#ifdef SPOT_ENABLED
            else if (p->character == g_ble_gfps_beacon_actions_character)
            {
                p->rsp_error_code = (att_error_code_t)app_gfps_write_beacon_actions_ind_hander(gfps, p);
                return (p->rsp_error_code == ATT_ERROR_NO_ERROR);
            }
#endif
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

static void ble_gfps_send_notification(uint16_t connhdl, const uint8_t* character, const uint8_t *value, uint16_t len)
{
    ble_gfps_t *gfps = (ble_gfps_t *)ble_get_gfps(connhdl);
    if (gfps == NULL)
    {
        return;
    }

    gatts_send_notification(gap_conn_bf(gfps->head.con_idx), character, value, len);
}

uint32_t app_sec_get_P256_key(uint8_t * out_public_key,uint8_t * out_private_key)
{
    return gfps_crypto_make_P256_key(out_public_key,out_private_key);
}

extern uint8_t ble_public_key[64]; //MSB->LSB
extern uint8_t ble_private_key[32]; //MSB->LSB
extern void fast_pair_enter_pairing_mode_handler(void);
extern uint32_t Get_ModelId();
uint8_t app_gfps_generate_accountkey_data(uint8_t *outputData);

void app_gfps_set_bt_access_mode(gfps_enter_pairing_mode cb)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->enter_pairing_mode = cb;
}

void app_gfps_set_io_cap(gfps_bt_io_cap_set cb)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->bt_set_iocap = cb;
}

void app_gfps_set_authrequirements(gfps_bt_io_authrequirements_set cb)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->bt_set_authrequirements = cb;
}

static bool app_gfps_adv_activity_prepare(ble_adv_activity_t *adv)
{
    gap_adv_param_t *adv_param = &adv->adv_param;
#ifdef SPOT_ENABLED
    bool spot_adv_enable = false;
#endif

    if (!ble_adv_is_allowed())
    {
        return false;
    }

#if defined(IBRT)
    if (!app_ble_check_ibrt_allow_adv(USER_GFPS))
    {
        return false;
    }
#endif

#ifdef SPOT_ENABLED
    spot_adv_enable = nv_record_fp_get_spot_adv_enable_value();
#endif

    adv->user = USER_GFPS;
    adv_param->connectable = true;
    adv_param->scannable = true;
    adv_param->use_legacy_pdu = true;
    adv_param->include_tx_power_data = true;
    adv_param->own_addr_use_rpa = true;
    adv_param->fast_advertising = false; // BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL

    if (gfps_is_in_fastpairing_mode())
    {
        adv_param->fast_advertising = true; // BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL
    }

    app_ble_set_adv_tx_power_level(adv, BLE_ADV_TX_POWER_LEVEL_1);

    app_ble_dt_set_flags(adv_param, true);

    if (gfps_is_in_fastpairing_mode())
    {
        uint32_t modelId = 0;
        uint8_t model_id_data[3];
        char gfps_tx_power;

        TRACE(0, "fast pair mode");

#ifndef IS_USE_CUSTOM_FP_INFO
        modelId = FP_DEVICE_MODEL_ID;
#else
        modelId = app_bt_get_model_id();
#endif
        model_id_data[0] = (modelId >> 16) & 0xFF;
        model_id_data[1] = (modelId >> 8) & 0xFF;
        model_id_data[2] = (modelId >> 0) & 0xFF;

        gap_dt_add_service_data(&adv_param->adv_data, BLE_GFPS_SERVICE_UUID, model_id_data, sizeof(model_id_data));

#ifndef IS_USE_CUSTOM_FP_INFO
        gfps_tx_power = 0xf5;
#else
        gfps_tx_power = ble_gfps_global()->gfps_tx_power;
#endif
        gap_dt_add_tx_power(&adv_param->adv_data, gfps_tx_power);
    }
    else
    {
        TRACE(0, "not in fast pair mode");

#if BLE_APP_GFPS_VER == FAST_PAIR_REV_2_0
        uint8_t service_data[32];
        uint16_t data_len = 0;

        data_len = app_gfps_generate_accountkey_data(service_data);

        gap_dt_add_service_data(&adv_param->adv_data, BLE_GFPS_SERVICE_UUID, service_data, data_len);

        gap_dt_add_tx_power(&adv_param->adv_data, 0xf5);
#endif
    }

#ifdef SPOT_ENABLED
    if (spot_adv_enable)
    {
        uint8_t service_data[24];
        uint8_t *data_ptr = service_data;
        ble_gfps_global_t *gfps = ble_gfps_global();

        *data_ptr = gfps->curr_unwanted_tracking_mode ? FP_EID_FRAME_TYPE_WHEN_ENABLE_UTP : FP_EID_FRAME_TYPE_WHEN_DISABLE_UTP;
        data_ptr += 1;

        memcpy(data_ptr, nv_record_fp_get_spot_adv_data(), 20);
        data_ptr += 20;

        if (gfps->curr_original_flag != 0x00)
        {
            *data_ptr = gfps->curr_hashed_flag;
            data_ptr += 1;
        }

        gap_dt_add_service_data(&adv_param->adv_data, BLE_SPOT_SERVICE_UUID, service_data, data_ptr-service_data);
    }
#endif

    app_ble_dt_set_local_name(adv_param, NULL);

    return true;
}

void app_bt_get_fast_pair_info(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->fast_pair_info.model_id = Get_ModelId();
    switch (g->fast_pair_info.model_id)
    {
        case FP_DEVICE_MODEL_ID: //default model id(bes moddel id)
        {
            memcpy (g->fast_pair_info.public_anti_spoofing_key, ble_public_key, sizeof(ble_public_key));
            memcpy (g->fast_pair_info.private_anti_spoofing_key, ble_private_key, sizeof(ble_private_key));
        }
        break;
        default: //customer add customer model id here;
        {
            g->fast_pair_info.model_id = FP_DEVICE_MODEL_ID;
            memcpy (g->fast_pair_info.public_anti_spoofing_key, ble_public_key, sizeof(ble_public_key));
            memcpy (g->fast_pair_info.private_anti_spoofing_key, ble_private_key, sizeof(ble_private_key));
        }
    }
}

uint32_t app_bt_get_model_id(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    return g->fast_pair_info.model_id;
}

void ble_gfps_init(void)
{
    ble_gfps_global_t *g = ble_gfps_global();

    if (g->initialized)
    {
        return;
    }

    g->initialized = true;
    g->advRandSalt = GFPS_INITIAL_ADV_RAND_SALT;
    g->gfps_tx_power = 0xf5; // APP_GFPS_ADV_POWER_UUID
    g->bondMode = GFPS_BOND_OVER_BT;
#ifdef SPOT_ENABLED
    g->protocol_version = GFPS_BEACON_PROTOCOL_VERSION;
#endif

#if BLE_AUDIO_ENABLED
    bt_l2cap_create_port(PSM_SPSM_GFPS, ble_gfps_l2cap_callback);
    bt_l2cap_listen(PSM_SPSM_GFPS);
#endif

    app_gfps_set_bt_access_mode(fast_pair_enter_pairing_mode_handler);
    app_gfps_set_io_cap(( gfps_bt_io_cap_set )btif_sec_set_io_capabilities);
    app_gfps_set_authrequirements((gfps_bt_io_authrequirements_set)btif_sec_set_authrequirements);

    nv_record_fp_account_key_init();

    gfps_crypto_init();

    app_bt_get_fast_pair_info();

    app_ble_register_advertising(BLE_GFPS_ADV_HANDLE, app_gfps_adv_activity_prepare);

#ifndef IS_USE_CUSTOM_FP_INFO
    gfps_crypto_set_p256_key(ble_public_key, ble_private_key);
#else
    gfps_crypto_set_p256_key(app_bt_get_fast_pair_public_key(), app_bt_get_fast_pair_private_key());
#endif

    gatts_cfg_t cfg = {0};
    cfg.svc_size = sizeof(ble_gfps_t);
    cfg.preferred_mtu = GFPS_PREFERRED_MTU;
    cfg.eatt_preferred = false;
    gatts_register_service(g_ble_gfps_attr_list, ARRAY_SIZE(g_ble_gfps_attr_list), ble_gfps_server_callback, &cfg);
}

void app_gfps_send_keybase_pairing_via_notification(ble_gfps_t *gfps, const uint8_t *data, uint32_t length)
{
    if (gfps == NULL || (gfps != (ble_gfps_t *)ble_get_gfps(gfps->head.connhdl)))
    {
        CO_LOG_ERR_1(BT_STS_INVALID_GFPS, gfps);
        return;
    }
    ble_gfps_send_notification(gfps->head.connhdl, g_ble_gfps_keybased_pairing_character, data, (uint16_t)length);
}

void app_gfps_send_passkey_via_notification(ble_gfps_t *gfps, const uint8_t *data, uint32_t length)
{
    ble_gfps_send_notification(gfps->head.connhdl, g_ble_gfps_passkey_character, data, (uint16_t)length);
}

void app_gfps_send_naming_packet_via_notification(ble_gfps_t *gfps, const uint8_t *data, uint32_t length)
{
    ble_gfps_send_notification(gfps->head.connhdl, g_ble_gfps_additional_data_character, data, (uint16_t)length);
}

#ifdef SPOT_ENABLED
void app_gfps_send_beacon_data_via_notification(ble_gfps_t *gfps, const uint8_t *data, uint32_t length)
{
    ble_gfps_send_notification(gfps->head.connhdl, g_ble_gfps_beacon_actions_character, data, (uint16_t)length);
}
#endif

#if 0
void app_gfps_handling_on_mobile_link_disconnection(btif_remote_device_t* pRemDev) // bt link disconnected
{
    bool isDisconnectedWithMobile = false;
#ifdef IBRT
    ibrt_link_type_e link_type = app_tws_ibrt_get_remote_link_type((bt_bdaddr_t *)pRemDev);
    if (MOBILE_LINK == link_type)
    {
        isDisconnectedWithMobile = true;
    }
#else
    isDisconnectedWithMobile = true;
#endif

    if (isDisconnectedWithMobile)
    {
        if (gfps_is_last_response_pending())
        {
            gfps_enter_connectable_mode_req_handler(NULL);
        }
    }
}
#endif

static uint8_t app_gfps_handle_decrypted_keybase_pairing_request(ble_gfps_t *gfps, gfpsp_req_resp *raw_req, uint8_t *out_key)
{
    ble_gfps_global_t *g = ble_gfps_global();
    uint8_t rawData[KEY_BASE_RSP_LEN] = {0};
    uint8_t rawDataLen = 0, offsetOfSalt = 0, saltLen = 0;
    gfpsp_encrypted_resp en_rsp;

    memcpy(gfps->keybase_pair_key, out_key, 16);
    memcpy(gfps->seeker_bt_addr.address, raw_req->rx_tx.key_based_pairing_req.seeker_addr, 6);
    if (raw_req->rx_tx.key_based_pairing_req.flags_discoverability == RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN)
    {
        TRACE(0,"TODO discoverable 10S");
        //TODO start a timer keep discoverable 10S...
        //TODO make sure there is no ble ADV with the MODEL ID data
    }

    if (raw_req->rx_tx.key_based_pairing_req.flags_support_le)
    {
        gfpsp_raw_ext_resp *ext_rsp = (gfpsp_raw_ext_resp *)rawData;
        saltLen = KEY_BASE_EXT_RSP_SALT_LEN;

        ext_rsp->message_type = KEY_BASED_PAIRING_EXT_RSP;// Key-based Pairing Response
        ext_rsp->flags_is_le_only = 0;
        if (!raw_req->rx_tx.key_based_pairing_req.flags_support_le_audio)
        {
            ext_rsp->flags_msg_via_le = 0;
            ext_rsp->flags_ble_bonding = 0;
        }
        else if (g->bondMode == GFPS_BOND_OVER_BT)
        {
            ext_rsp->flags_msg_via_le = 1;
            ext_rsp->flags_ble_bonding = 0;
        }
        else
        {
            ext_rsp->flags_msg_via_le = 0;
            ext_rsp->flags_ble_bonding = 1;
        }

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
        if (raw_req->rx_tx.key_based_pairing_req.flags_support_le_audio)
        {
            ext_rsp->addrNum = 2;
            big_little_switch(nv_record_get_ibrt_peer_addr(), ext_rsp->peerAddrandSalt, sizeof(bt_bdaddr_t));
            saltLen -= sizeof(bt_bdaddr_t);
        }
        else
#endif
        {
            ext_rsp->addrNum = 1;
        }

        memcpy(ext_rsp->local_addr, gfps->local_bt_addr.address, sizeof(bt_bdaddr_t));
        rawDataLen = sizeof(gfpsp_raw_ext_resp);
    }
    else
    {
        gfpsp_raw_resp *raw_rsp = (gfpsp_raw_resp *)rawData;
        saltLen = KEY_BASE_RSP_SALT_LEN;
        raw_rsp->message_type = KEY_BASED_PAIRING_RSP;// Key-based Pairing Response
        memcpy(raw_rsp->provider_addr, gfps->local_bt_addr.address, sizeof(bt_bdaddr_t));

        rawDataLen = sizeof(gfpsp_raw_resp);
    }

    offsetOfSalt = rawDataLen - saltLen;
    for (uint8_t index = 0; index < saltLen; index++)
    {
        rawData[offsetOfSalt + index] = ( uint8_t )rand();
    }

    TRACE(2,"%d %d raw_rsp:", offsetOfSalt, rawDataLen);
    DUMP8("%02x ", rawData, 16);

    gfps_crypto_encrypt(rawData, rawDataLen, gfps->keybase_pair_key, en_rsp.uint128_array);

    TRACE(1,"message type is 0x%x", raw_req->rx_tx.raw_req.message_type);
    TRACE(6,"bit 0: %d, bit 1: %d, bit 2: %d, bit 3: %d, bit 4:%d, bit 5:%d",
        raw_req->rx_tx.key_based_pairing_req.flags_discoverability,
        raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr,
        raw_req->rx_tx.key_based_pairing_req.flags_get_existing_name,
        raw_req->rx_tx.key_based_pairing_req.flags_retroactively_write_account_key,
        raw_req->rx_tx.key_based_pairing_req.flags_support_le,
        raw_req->rx_tx.key_based_pairing_req.flags_support_le_audio);

    bool isReturnName = raw_req->rx_tx.key_based_pairing_req.flags_get_existing_name;

    if(raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr == RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN)
    {
        TRACE(0,"try connect to remote BR/EDR addr");
        // TODO:
        app_gfps_send_keybase_pairing_via_notification(gfps, en_rsp.uint128_array, sizeof(en_rsp));
    }
    else if (raw_req->rx_tx.key_based_pairing_req.flags_retroactively_write_account_key)
    {
        // check whether the seeker's bd address is the same as already connected mobile
        uint8_t swapedBtAddr[6];
        big_little_switch(gfps->seeker_bt_addr.address, swapedBtAddr, sizeof(swapedBtAddr));

        uint8_t isMatchMobileAddr = false;
        for (uint32_t devId = 0; devId < bes_bt_me_count_mobile_link(); devId++)
        {
            uint8_t connectedAddr[6];
            app_bt_get_device_bdaddr(devId, connectedAddr);
            if (!memcmp(connectedAddr, swapedBtAddr, 6))
            {
                isMatchMobileAddr = true;
                break;
            }
        }

        if (isMatchMobileAddr)
        {
            app_gfps_send_keybase_pairing_via_notification(gfps, en_rsp.uint128_array, sizeof(en_rsp));
        }
        else
        {
            // reject the write request
            return ATT_ERROR_WR_NOT_PERMITTED;
        }
    }
    else if (raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr == RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS)
    {
        gfps_last_response_t response = {en_rsp, gfps};
        gfps_enter_connectable_mode_req_handler(&response);
    }
    else
    {
        app_gfps_send_keybase_pairing_via_notification(gfps, en_rsp.uint128_array, sizeof(en_rsp));
    }

    if (isReturnName)
    {
        gfps->isPendingForWritingNameReq = true;
        TRACE(0,"get existing name.");
        uint8_t response[16+FP_MAX_NAME_LEN];
        uint8_t* ptrRawName;
        uint32_t rawNameLen;
        ptrRawName = nv_record_fp_get_name_ptr(&rawNameLen);

        gfps_encrypt_name(gfps->keybase_pair_key,
                          ptrRawName,
                          rawNameLen,
                          &response[16],
                          response,
                          &response[8]);

        app_gfps_send_naming_packet_via_notification(gfps, response, 16 + rawNameLen);
    }
    else
    {
        TRACE(0,"Unusable bit.");
    }

    return ATT_ERROR_NO_ERROR;
}

static void app_gfps_update_local_bt_name(void)
{
#if 0
    uint8_t* ptrRawName;
    uint32_t rawNameLen;
    // name has been updated to fp nv record
    ptrRawName = nv_record_fp_get_name_ptr(&rawNameLen);
    if(rawNameLen > 0)
    {
        bt_set_local_dev_name((const unsigned char*)(ptrRawName),
                                strlen((char *)(ptrRawName)) + 1);
        btif_update_bt_name((const unsigned char*)(ptrRawName),
                                strlen((char *)(ptrRawName)) + 1);
    }
#endif
}

static bool app_gfps_decrypt_keybase_pairing_request(ble_gfps_t *gfps, uint8_t *pairing_req, uint8_t *output)
{
    uint8_t keyCount = nv_record_fp_account_key_count();
    if (0 == keyCount)
    {
        return false;
    }

    gfpsp_req_resp raw_req;
    uint8_t accountKey[FP_ACCOUNT_KEY_SIZE];
    for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
    {
        nv_record_fp_account_key_get_by_index(keyIndex, accountKey);

        AES128_ECB_decrypt(pairing_req, ( const uint8_t * )accountKey, ( uint8_t * )&raw_req);
        TRACE(0,"Decrypted keybase pairing req result:");
        DUMP8("0x%02x ", ( uint8_t * )&raw_req, 16);

        if ((memcmp(raw_req.rx_tx.key_based_pairing_req.provider_addr,
                gfps->local_bt_addr.address, 6) == 0) ||
            (memcmp(raw_req.rx_tx.key_based_pairing_req.provider_addr,
                gfps->local_le_addr.addr , 6) == 0))
        {
            memcpy(output, accountKey, FP_ACCOUNT_KEY_SIZE);
            TRACE(1,"fp message type 0x%02x.", raw_req.rx_tx.raw_req.message_type);
            if (KEY_BASED_PAIRING_REQ == raw_req.rx_tx.raw_req.message_type)
            {
                app_gfps_handle_decrypted_keybase_pairing_request(gfps, &raw_req, accountKey);
                return true;
            }
            else if (ACTION_REQUEST == raw_req.rx_tx.raw_req.message_type)
            {
                memcpy(gfps->keybase_pair_key, accountKey, 16);
                memcpy(gfps->seeker_bt_addr.address, raw_req.rx_tx.key_based_pairing_req.seeker_addr, 6);
                gfpsp_encrypted_resp  en_rsp;
                gfpsp_raw_resp  raw_rsp;

                raw_rsp.message_type = KEY_BASED_PAIRING_RSP;// Key-based Pairing Response
                memcpy(raw_rsp.provider_addr, gfps->local_bt_addr.address, 6);

                TRACE(0,"raw_rsp.provider_addr:");
                DUMP8("%02x ",raw_rsp.provider_addr, BT_ADDR_OUTPUT_PRINT_NUM);

                for (uint8_t index = 0; index < 9; index++)
                {
                    raw_rsp.salt[index] = (uint8_t)rand();
                }

                gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type), sizeof(raw_rsp),
                    gfps->keybase_pair_key, en_rsp.uint128_array);

                app_gfps_send_keybase_pairing_via_notification(gfps, en_rsp.uint128_array, sizeof(en_rsp));

                if (raw_req.rx_tx.action_req.isDeviceAction)
                {
                    // TODO: device action via BLE
                }
                else if (raw_req.rx_tx.action_req.isFollowedByAdditionalDataCh)
                {
                    // write name request will be received
                    TRACE(0,"FP write name request will be received.");
                    gfps->isPendingForWritingNameReq = true;
                }
                return true;
            }
        }
    }

    return false;
}

static att_error_code_t app_gfps_write_key_based_pairing_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p)
{
    uint8_t write_rsp_status = ATT_ERROR_NO_ERROR;
    gfpsp_Key_based_Pairing_req en_req;
    gfpsp_req_resp *ptr_raw_req = NULL;
    uint8_t out_key[16] = {0};
    uint8_t decryptdata[16] = {0};
    uint32_t gfps_state = 0;

    en_req.en_req = ( gfpsp_encrypted_req_uint128 * )&(p->value[0]);
    en_req.pub_key = ( gfpsp_64B_public_key * )&(p->value[16]);

    TRACE(0, "length = %d value = 0x%x  0x%x", p->value_len, p->value[0], p->value[1]);
    DUMP8("%02x ", p->value, 80);

    if (p->value_len == GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY)
    {
        memset(gfps->keybase_pair_key, 0, 16);
        gfps_state = gfps_crypto_get_secret_decrypt((uint8_t *)en_req.en_req, (uint8_t *)en_req.pub_key, out_key, decryptdata);
        if (gfps_state == GFPS_SUCCESS)
        {
            memcpy(gfps->aesKeyFromECDH, out_key, 16);
            gfps->isInitialPairing = true;
            ptr_raw_req = (gfpsp_req_resp *)decryptdata;
            TRACE(0,"raw req provider's addr:");
            DUMP8("%02x ", ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr, BT_ADDR_OUTPUT_PRINT_NUM);
            TRACE(0,"raw req seeker's addr:");
            DUMP8("%02x ", ptr_raw_req->rx_tx.key_based_pairing_req.seeker_addr, BT_ADDR_OUTPUT_PRINT_NUM);
            TRACE(1,"fp message type 0x%02x.", ptr_raw_req->rx_tx.raw_req.message_type);
            if ((KEY_BASED_PAIRING_REQ == ptr_raw_req->rx_tx.raw_req.message_type) &&
                ((memcmp(ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr,
                    gfps->local_bt_addr.address, 6) == 0) ||
                 (memcmp(ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr,
                    gfps->local_le_addr.addr, 6) == 0)))
            {
                write_rsp_status = app_gfps_handle_decrypted_keybase_pairing_request(gfps, ptr_raw_req, out_key);
            }
            else
            {
                TRACE(0,"decrypt false..ingore");
            }
        }
        else
        {
            TRACE(0, "error = %x", gfps_state);
        }
    }
    else if (p->value_len == GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY)
    {
        gfps->isInitialPairing = false;
        gfps_state = app_gfps_decrypt_keybase_pairing_request(gfps, (uint8_t *)en_req.en_req, out_key);
        TRACE(0, "Decrypt keybase pairing req without public key result: %d", gfps_state);
    }
    else
    {
        TRACE(0, "who you are?? %d", p->value_len);
    }

    return (att_error_code_t)write_rsp_status;
}

static void app_gfps_write_passkey_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p)
{
    gfpsp_raw_pass_key_resp raw_rsp;
    gfpsp_encrypted_resp en_rsp;
    uint8_t decryptdata[16] = {0};
    TRACE(1,"length = %d value = 0x", p->value_len);
    DUMP8("%02X, ", p->value, 16);
    gfps_crypto_decrypt(p->value, 16, gfps->keybase_pair_key, decryptdata);
    TRACE(0,"decrypt data =0x");
    TRACE(0,"===============================");
    DUMP8("%02X", decryptdata, 16);
    TRACE(0,"===============================");

    TRACE(0,"pass key = 1-3 bytes");

    raw_rsp.message_type = 0x03;  //Provider's passkey
    raw_rsp.passkey[0]   = decryptdata[1];
    raw_rsp.passkey[1]   = decryptdata[2];
    raw_rsp.passkey[2]   = decryptdata[3];
    raw_rsp.reserved[0]  = 0x38;  //my magic num  temp test
    raw_rsp.reserved[1]  = 0x30;
    raw_rsp.reserved[2]  = 0x23;
    raw_rsp.reserved[3]  = 0x30;
    raw_rsp.reserved[4]  = 0x06;
    raw_rsp.reserved[5]  = 0x10;
    raw_rsp.reserved[6]  = 0x05;
    raw_rsp.reserved[7]  = 0x13;
    raw_rsp.reserved[8]  = 0x06;
    raw_rsp.reserved[9]  = 0x12;
    raw_rsp.reserved[10] = 0x12;
    raw_rsp.reserved[11] = 0x01;
    gfps_crypto_encrypt((uint8_t *)(&raw_rsp.message_type), sizeof(raw_rsp), gfps->keybase_pair_key, en_rsp.uint128_array);
    app_gfps_send_passkey_via_notification(gfps, en_rsp.uint128_array, sizeof(en_rsp));
}

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
void app_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SHARE_FASTPAIR_INFO, p_buff, length);
}

static void app_tws_send_fastpair_info_to_slave(void)
{
    TRACE(0,"Send fastpair info to secondary device.");
    NV_FP_ACCOUNT_KEY_RECORD_T *pFpData = nv_record_get_fp_data_structure_info();
    app_ibrt_share_fastpair_info(( uint8_t * )pFpData, sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
}

void app_ibrt_shared_fastpair_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    NV_FP_ACCOUNT_KEY_RECORD_T *pFpData = ( NV_FP_ACCOUNT_KEY_RECORD_T * )p_buff;
    nv_record_update_fp_data_structure(pFpData);
}
#endif

static att_error_code_t app_gfps_write_name_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p)
{
    bool succ = false;
    att_error_code_t status = ATT_ERROR_NO_ERROR;

    if (!gfps->isPendingForWritingNameReq)
    {
        TRACE(0, "Pre fp write name request is not received.");
    }
    else
    {
        uint8_t rawName[FP_MAX_NAME_LEN];
        if (gfps->isInitialPairing)
        {
            succ = gfps_decrypt_name(gfps->aesKeyFromECDH, (uint8_t *)p->value,
                (uint8_t *)&(p->value[8]), (uint8_t *)&(p->value[16]), rawName, p->value_len-16);
        }
        else
        {
            succ = gfps_decrypt_name(gfps->keybase_pair_key, (uint8_t *)p->value,
                (uint8_t *)&(p->value[8]), (uint8_t *)&(p->value[16]), rawName, p->value_len-16);
        }

        TRACE(2,"write name successful flag %d %d", succ, gfps->head.con_idx);

        if (succ)
        {
            nv_record_fp_update_name(rawName, p->value_len-16);
            TRACE(1,"Rename BT name: [%s]", rawName);
            app_gfps_update_local_bt_name();
#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
            app_tws_send_fastpair_info_to_slave();
#endif
        }

        gfps->isPendingForWritingNameReq = false;
    }

    if (!succ)
    {
        status = ATT_ERROR_WR_NOT_PERMITTED;
    }

    return status;
}

static void app_gfps_write_accountkey_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p)
{
    NV_FP_ACCOUNT_KEY_ENTRY_T accountkey;

    TRACE(1,"length = %d value = 0x", p->value_len);
    DUMP8("%02X, ", p->value, FP_ACCOUNT_KEY_SIZE);
    gfps_crypto_decrypt(p->value, FP_ACCOUNT_KEY_SIZE, gfps->keybase_pair_key, accountkey.key);
    TRACE(0,"decrypt account key:");
    //TRACE(0,"===============================");
    DUMP8("%02X", accountkey.key, FP_ACCOUNT_KEY_SIZE);
    //TRACE(0,"===============================");

    nv_record_fp_account_key_add(&accountkey);

#ifdef SASS_ENABLED
    gfps_sass_set_inuse_acckey(accountkey.key, NULL);
#endif

#if defined(IBRT) && !defined(FREEMAN_ENABLED_STERO)
    app_tws_send_fastpair_info_to_slave();
#endif

    // update the BLE ADV as account key has been added
    if (!gfps_is_in_fastpairing_mode())
    {
        // restart the BLE adv if it's retro-active pairing
        app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    }
}

void app_gfps_set_battery_info_acquire_handler(gfps_get_battery_info_cb cb)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->get_battery_info_handler = cb;
}

void app_gfps_update_random_salt(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->advRandSalt = (uint8_t)rand();
}

uint8_t app_gfps_generate_accountkey_data(uint8_t *outputData)
{
    uint8_t keyCount = nv_record_fp_account_key_count();
    uint8_t dataLen = 0, useKey = 0;
    uint8_t salt[APP_GFPS_ADV_LEN_SALT+1] = {0};
    uint8_t index;

    uint8_t batteryFollowingData[1 + GFPS_BATTERY_VALUE_MAX_COUNT];
    uint8_t batteryLevelCount = 0;

    uint8_t accountKeyData[32];
    uint8_t accountKeyDataLen = 2;
    uint8_t hash256Result[32];

    uint8_t sizeOfFilter = (((uint8_t)(( float )1.2 * useKey)) + 3);
    uint8_t FArray[2 * FP_ACCOUNT_KEY_RECORD_NUM + 3];

#ifdef SASS_ENABLED
    uint8_t inUseKey[16] = {0};
    uint8_t VArray[FP_ACCOUNT_KEY_SIZE + APP_GFPS_ADV_LEN_SALT + GFPS_BATTERY_VALUE_MAX_COUNT+1 + SASS_ADV_LEN_MAX+1] = {0};
#ifdef SASS_SECURE_ENHACEMENT
    uint8_t sassAdv[SASS_ADV_LEN_MAX + 1] = {0};
    uint8_t sassAdvLen = SASS_ADV_LEN_MAX + 1;
#endif
#else
    uint8_t VArray[FP_ACCOUNT_KEY_SIZE + APP_GFPS_ADV_LEN_SALT + GFPS_BATTERY_VALUE_MAX_COUNT+1] = {0};
#endif

    if (0 == keyCount)
    {
        outputData[0] = 0;
        outputData[1] = 0;
        return 2;
    }
    useKey = (keyCount >= 3) ? 3 : keyCount;

    if (gfps_is_battery_enabled())
    {
        gfps_get_battery_levels(&batteryLevelCount, batteryFollowingData + 1);
        //include charging case
        if(batteryLevelCount >= GFPS_BATTERY_VALUE_MAX_COUNT)
        {
            batteryLevelCount = GFPS_BATTERY_VALUE_MAX_COUNT;
        }
        batteryFollowingData[0] = gfps_get_battery_datatype() | (batteryLevelCount << 4);
    }

    accountKeyData[0] = (APP_GFPS_ADV_VERSION << 4) | APP_GFPS_ADV_FLAG;
    accountKeyDataLen = 2;

    sizeOfFilter = (((uint8_t)(( float )1.2 * useKey)) + 3);
    memset(FArray, 0, sizeof(FArray));

    ble_bdaddr_t ble_addr = app_gfps_get_ble_addr(NULL);
    uint8_t *currentBleAddr = ble_addr.addr;
    salt[0] = (APP_GFPS_ADV_LEN_SALT << 4) | APP_GFPS_ADV_TYPE_SALT;

    for(index = 0; index < APP_GFPS_ADV_LEN_SALT; index++ )
    {
#if GFPS_ACCOUNTKEY_SALT_TYPE == USE_RANDOM_NUM_AS_SALT
        if (GFPS_INITIAL_ADV_RAND_SALT != g->advRandSalt)
        {
            salt[index + 1] = g->advRandSalt;
        }
        else
        {
            salt[index + 1] = (uint8_t)rand();
        }
#else
        salt[index + 1] = currentBleAddr[index%6];
#endif
    }

#ifdef SASS_ENABLED
    gfps_sass_get_inuse_acckey(inUseKey);
#ifdef SASS_SECURE_ENHACEMENT
    uint8_t iv[16] = {0};
    memcpy(iv, salt+1, APP_GFPS_ADV_LEN_SALT);
    gfps_sass_encrypt_connection_state(iv, inUseKey, sassAdv, &sassAdvLen, true, true, NULL);
#endif
#endif

    for (uint8_t keyIndex = 0; keyIndex < useKey; keyIndex++)
    {
        uint8_t offsetOfVArray = 0;
        nv_record_fp_account_key_get_by_index(keyCount - 1 - keyIndex, VArray);
        offsetOfVArray += FP_ACCOUNT_KEY_SIZE;

#ifdef SASS_ENABLED
        if(!memcmp(inUseKey, VArray, FP_ACCOUNT_KEY_SIZE))
        {
            uint8_t activeDev = gfps_sass_get_active_dev();
            if (((activeDev != 0xFF) && gfps_sass_is_sass_dev(activeDev)) || \
                (activeDev == 0xFF && gfps_sass_is_there_sass_dev()))
            {
                VArray[0] = IN_USE_ACCOUNT_KEY_HEADER;
               TRACE(0, "is inuse account key!!");
            }
            else
            {
                VArray[0] = MOST_RECENT_USED_ACCOUNT_KEY_HEADER;
                TRACE(0, "is most recently account key!!");
            }
        }
#endif
        memcpy(VArray + offsetOfVArray, salt+1, APP_GFPS_ADV_LEN_SALT);
        offsetOfVArray += APP_GFPS_ADV_LEN_SALT;

        if (gfps_is_battery_enabled())
        {
            memcpy(VArray+offsetOfVArray, batteryFollowingData, batteryLevelCount + 1);
            offsetOfVArray += (batteryLevelCount + 1);
        }

#ifdef SASS_ENABLED
#ifdef SASS_SECURE_ENHACEMENT
        memcpy(VArray+offsetOfVArray, sassAdv, sassAdvLen);
        offsetOfVArray += sassAdvLen;
#endif
#endif

        TRACE(0,"To hash256 on:");
        DUMP8("%02x ", VArray, offsetOfVArray);

        gfps_SHA256_hash(VArray, offsetOfVArray, hash256Result);

        // K = Xi % (s * 8)
        // F[K/8] = F[K/8] | (1 << (K % 8))
        uint32_t pX[8];
        for (index = 0; index < 8; index++)
        {
            pX[index] = (((uint32_t)(hash256Result[index * 4])) << 24) |
                        (((uint32_t)(hash256Result[index * 4 + 1])) << 16) |
                        (((uint32_t)(hash256Result[index * 4 + 2])) << 8) |
                        (((uint32_t)(hash256Result[index * 4 + 3])) << 0);
        }

        for (index = 0; index < 8; index++)
        {
            uint32_t K    = pX[index] % (sizeOfFilter * 8);
            FArray[K / 8] = FArray[K / 8] | (1 << (K % 8));
        }
    }

    memcpy(&accountKeyData[2], FArray, sizeOfFilter);

    accountKeyDataLen += sizeOfFilter;

    accountKeyData[1] = (sizeOfFilter<<4) | APP_GFPS_ADV_TYPE_ACCKEY_FILETER_SHOW_UI;

    accountKeyData[2+sizeOfFilter] = salt[0];
    for(index = 0; index < APP_GFPS_ADV_LEN_SALT; index++ )
    {
        accountKeyData[2 + sizeOfFilter + index + 1] = salt[index + 1];
    }
    accountKeyDataLen += (APP_GFPS_ADV_LEN_SALT + 1);

    memcpy(outputData, accountKeyData, accountKeyDataLen);
    dataLen += accountKeyDataLen;

    if (gfps_is_battery_enabled())
    {
        memcpy(outputData + dataLen, batteryFollowingData, batteryLevelCount + 1);
        dataLen += (batteryLevelCount + 1);
    }

#ifdef SASS_ENABLED
#ifdef SASS_SECURE_ENHACEMENT
    memcpy(outputData + dataLen, sassAdv, sassAdvLen);
    dataLen += sassAdvLen;
#else
    gfps_sass_encrypt_adv_data(FArray, sizeOfFilter, inUseKey, outputData, &dataLen);
#endif
#endif

    TRACE(1,"Generated accountkey data len:%d", dataLen);
    DUMP8("%02x ", outputData, dataLen);

    return dataLen;
}

#ifdef SPOT_ENABLED
static const uint8_t *app_gfps_generate_nonce(ble_gfps_t *gfps)
{
    rand_generator(gfps->nonce, 8); // generate 8-byte random number for nonce
    return gfps->nonce;
}

static const uint8_t *app_gfps_get_nonce(ble_gfps_t *gfps)
{
    return gfps->nonce;
}

static uint8_t app_gfps_get_protocol_version(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    return g->protocol_version;
}

void app_gfps_set_protocol_version(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->protocol_version = GFPS_BEACON_PROTOCOL_VERSION;
}

static void spot_ring_timer_stop(ble_gfps_t *gfps)
{
    if (gfps->spot_ring_timeout_timer_id)
    {
        co_timer_del(&gfps->spot_ring_timeout_timer_id);
        gfps->spot_ring_timeout_timer_id = 0;
    }
}

static void spot_find_devices_ring_timeout_handler(void *arg)
{
    ble_gfps_t *gfps = (ble_gfps_t *)arg;
    if (gfps != (ble_gfps_t *)ble_get_gfps(gfps->head.connhdl))
    {
        TRACE(0, "spot ring timeout invalid %04x", gfps->head.connhdl);
        return;
    }

    gfps->spot_ring_nums += 1;
    app_voice_start_gfps_find();

    if (gfps->spot_ring_nums * 2 >= gfps->remaining_ring_time / 10)
    {
        TRACE(0,"ring time out");
        gfps->spot_ring_nums = 0;
        spot_ring_timer_stop(gfps);
        app_voice_stop_gfps_find();
        gfps->ring_state = false;

        gfpsp_beacon_ring_resp beacon_rsp;
        beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STOPPED_TIMEOUT;
        beacon_rsp.data[1] = GFPS_BEACON_RINGING_NONE;
        memset(&beacon_rsp.data[2], 0x00, 2);

        uint8_t auth_segment[16];
        auth_segment[0] = app_gfps_get_protocol_version();
        memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
        auth_segment[9] = GFPS_BEACON_RING;
        auth_segment[10] = 0x0c;
        memcpy(&auth_segment[11], beacon_rsp.data, 4);
        auth_segment[15] = 0x01;
        gfps_beacon_encrpt_data(gfps->hashed_ring_key, auth_segment, 16, beacon_rsp.auth_data);

        beacon_rsp.data_id = GFPS_BEACON_RING;
        beacon_rsp.data_length = 0x0c;

       app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
    }
}

static void spot_ring_timer_set(ble_gfps_t *gfps)
{
    TRACE(1,"%s,", __func__);

    if (!gfps->spot_ring_timeout_timer_id)
    {
        gfps->spot_ring_nums = 0;
        gfps->spot_ring_timeout_timer_id = co_timer_new(&gfps->spot_ring_timeout_timer_id, 2*1000,
            spot_find_devices_ring_timeout_handler, gfps, 0x7fffffff);
    }

    co_timer_start(&gfps->spot_ring_timeout_timer_id);
}

static void spot_get_current_beacon_timer(ble_gfps_t *gfps)
{
    uint32_t current_time;
     current_time = btdrv_syn_get_curr_ticks()/2200;
    uint8_t time[4];
    memcpy(time, &current_time, sizeof(uint32_t));
    TRACE(1,"spot_get_current_beacon_timer, time is %d", current_time);

    gfps->beacon_time[0] = time[0]&0x00;
    gfps->beacon_time[1]=  time[1]&0xFC;
    gfps->beacon_time[2] = time[2]&0xFF;
    gfps->beacon_time[3] = time[3]&0xFF;
}

static void spot_generate_EID_handler(ble_gfps_t *gfps, uint8_t* orig_eph_identity_key)
{
    uint8_t Ephemeral_ID_seed[32];
    uint8_t random[32];
    spot_get_current_beacon_timer(gfps);
    
    memset(Ephemeral_ID_seed, 0xFF, 11);        //padding 0xff
    Ephemeral_ID_seed[11] = 0x0a;
    big_little_switch(gfps->beacon_time, &Ephemeral_ID_seed[12], 4);

    memset(&Ephemeral_ID_seed[16],0x00, 11);
    
    Ephemeral_ID_seed[27] = 0x0a;
    big_little_switch(gfps->beacon_time, &Ephemeral_ID_seed[28], 4);
    TRACE(0,"Ephemeral_ID_seed is");
    DUMP8("0x%x ",  &Ephemeral_ID_seed[12],4);

    gfps_crypto_encrypt(Ephemeral_ID_seed, 32, orig_eph_identity_key, random);
    gfps_crypto_encrypt(&Ephemeral_ID_seed[16], 32, orig_eph_identity_key, &random[16]);

    TRACE(0,"random is:");
    DUMP8("0X%x ", random, 32);
    //ECC to get ephemeral identifier
    uint8_t Ecc_private_key[20];
    uint8_t Ecc_public_key[20];

    TRACE(0,"gfps_beacon_ecc_mod");
    uint8_t random_output[48];
    big_little_switch(random, random_output, 32);
    memset(&random_output[32],0,16);

    uEcc_160r1_mod(Ecc_private_key, random_output);
    memcpy(gfps->Ecc_private_key, Ecc_private_key, 20);
    DUMP8("0X%x ",Ecc_private_key, 20);
    TRACE(0,"gfps_beacon_ecc_mult");
    uEcc_160r1_mult(Ecc_public_key, Ecc_private_key);
    DUMP8("0X%x ",Ecc_public_key, 20);

    big_little_switch(Ecc_public_key, gfps->adv_identifer, 20);
}

static HWTIMER_ID spot_EID_timeout_hard_timer_id = NULL;
static void spot_EID_hard_timer_rotation_handler(void const *param);

void ble_gfps_spot_rotate_hard_timer_start(ble_gfps_t *gfps)
{
    TRACE(0,"ble_gfps_spot_rotate_hard_timer_start");
    if (spot_EID_timeout_hard_timer_id == NULL) {
        spot_EID_timeout_hard_timer_id = hwtimer_alloc((HWTIMER_CALLBACK_T)spot_EID_hard_timer_rotation_handler, gfps);
    }

    hwtimer_start(spot_EID_timeout_hard_timer_id, MS_TO_TICKS(1020*1000));
}

static void spot_EID_hard_timer_rotation_handler(void const * param)
{
    ble_gfps_t *gfps = (ble_gfps_t *)param;

    if (gfps == NULL || (gfps != ble_get_gfps(gfps->head.connhdl)))
    {
        CO_LOG_ERR_1(BT_STS_INVALID_GFPS, gfps);
        return;
    }

    TRACE(0,"spot_EID_hard_timer_rotation_handler");
    spot_generate_EID_handler(gfps, nv_record_fp_get_eph_identity_key());
    //write to flash
    nv_record_fp_update_spot_adv_data(gfps->adv_identifer);
    if(gfps->enable_unwanted_tracking_mode == false)
    {
        gap_resolving_list_set_bonded_devices();
    }

    app_ble_start_connectable_adv(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);
    ble_gfps_spot_rotate_hard_timer_start(gfps);
}

static uint8_t app_gfps_write_beacon_actions_ind_hander(ble_gfps_t *gfps, gatt_server_char_write_t *p)
{
    TRACE(1, "app_gfps_write_beacon_actions_ind_hander, data_id is %d", p->value[0]);
    ble_gfps_global_t *g = ble_gfps_global();
    uint8_t data_id = p->value[0];
    uint8_t data_length = p->value[1];
    uint8_t status = GFPS_SUCCESS;

    if (data_id == GFPS_BEACON_READ_BEACON_PARAM)                //reading the beacon's state;
    {
        uint8_t keyCount = nv_record_fp_account_key_count();
        uint8_t protocol_version = g->protocol_version;
        uint8_t nonce[8];
        memcpy(nonce, app_gfps_get_nonce(gfps), 8);

        if (keyCount == 0)
        {
            status = GFPS_ERROR_INVALID_VALUE;
        }
        for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
        {
            uint8_t VArray[FP_ACCOUNT_KEY_SIZE];
            uint8_t calculatedHmacFirst8Bytes[8];
            uint8_t input_encrpyt_data[11];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], nonce, 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;

            nv_record_fp_account_key_get_by_index(keyIndex, VArray);
            gfps_beacon_encrpt_data(VArray, input_encrpyt_data, 11, calculatedHmacFirst8Bytes);

            if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
            {
               TRACE(0,"matched");
               status = GFPS_SUCCESS;
               memcpy(gfps->matched_account_key, VArray, FP_ACCOUNT_KEY_SIZE);
               break;
            }
            else
            {
                TRACE(0,"not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
        }

        if (status == GFPS_SUCCESS)
        {
            gfpsp_reading_beacon_additional_data additional_data;
            gfpsp_reading_beacon_state_resp beacon_rsp;
            uint8_t encrypted_data[16];
            uint32_t current_time;
            uint8_t auth_segment[28];
            ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
            current_time = btdrv_syn_get_curr_ticks()/2200;
            uint8_t time[4];
            memcpy(time, &current_time, sizeof(uint32_t));

            beacon_rsp.data_id = GFPS_BEACON_READ_BEACON_PARAM;
            beacon_rsp.data_length = 0x18;
            additional_data.power_value = bt_drv_reg_op_get_tx_pwr_dbm(p_ibrt_ctrl->tws_conhandle);
            TRACE(1,"tx power is %x", additional_data.power_value);
            big_little_switch(time, additional_data.clock_value, sizeof(uint32_t));
            //memcpy(beacon_rsp.clock_value, &time, 4);
            memset(additional_data.padgding, 0x00, 8);
            gfps_crypto_encrypt((uint8_t *)&additional_data, 16, gfps->matched_account_key, encrypted_data);
            memcpy(beacon_rsp.additional_data, encrypted_data, 16);

            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], nonce, 8);
            auth_segment[9] = data_id;
            auth_segment[10] = 0x18;
            memcpy(&auth_segment[11], encrypted_data, 16);
            auth_segment[27] = 0x01;

            gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 28, beacon_rsp.auth_data);

            memcpy(beacon_rsp.additional_data, encrypted_data, 16);

            gfps->beacon_time[0] = time[0]&0x00;
            gfps->beacon_time[1]=  time[1]&0xFC;
            gfps->beacon_time[2] = time[2]&0xFF;
            gfps->beacon_time[3] = time[3]&0xFF;
            TRACE(1,"time is %d, ", current_time);
            DUMP8("0x%x ", additional_data.clock_value, 4);
            DUMP8("0x%x ",gfps->beacon_time, 4);

            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
        }
    }
    else if (data_id == GFPS_BEACON_READ_PROVISION_STATE)             //0x01, reading the beacon's provisioning state
    {
        uint8_t keyCount = nv_record_fp_account_key_count();
        uint8_t protocol_version = g->protocol_version;
        uint8_t nonce[8];
        memcpy(nonce, app_gfps_get_nonce(gfps),8);
        uint8_t accountkey_Index =0;

        if (keyCount == 0)
        {
            status = GFPS_ERROR_INVALID_VALUE;
        }
        for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
        {
            uint8_t VArray[FP_ACCOUNT_KEY_SIZE];
            uint8_t calculatedHmacFirst8Bytes[8];

            uint8_t input_encrpyt_data[11];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], nonce, 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;

            nv_record_fp_account_key_get_by_index(keyIndex, VArray);
            gfps_beacon_encrpt_data(VArray, input_encrpyt_data, 11, calculatedHmacFirst8Bytes);

            if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
            {
               TRACE(0,"matched");
               status = GFPS_SUCCESS;
               memcpy(gfps->matched_account_key, VArray, FP_ACCOUNT_KEY_SIZE);
               accountkey_Index = keyIndex;
               break;
            }
            else
            {
                TRACE(0,"not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
        }

        if (status == GFPS_SUCCESS)
        {
            if (nv_record_fp_get_spot_adv_enable_value() == false)
            {
                TRACE(0,"spot adv is not enable");
                gfpsp_reading_beacon_provision_resp beacon_rsp;
                uint8_t auth_segment[13];

                auth_segment[0] = protocol_version;
                memcpy(&auth_segment[1], nonce, 8);
                auth_segment[9] = data_id;
                auth_segment[10] = 0x09;
                if(accountkey_Index == 0x00)
                    auth_segment[11] = 0x02;
                else
                    auth_segment[11] = 0x00;
                auth_segment[12] = 0x01;

                gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 13, beacon_rsp.auth_data);

                beacon_rsp.data_id = GFPS_BEACON_READ_PROVISION_STATE;
                beacon_rsp.data_length = 0x09;
                if(accountkey_Index == 0x00)
                    beacon_rsp.data = 0x02;
                else
                    beacon_rsp.data = 0x00;

                app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
            }
            else
            {
                TRACE(0,"spot adv has been enable yet");
                gfpsp_reading_EIK_beacon_provision_resp beacon_rsp;
                uint8_t auth_segment[33];

                auth_segment[0] = protocol_version;
                memcpy(&auth_segment[1], nonce, 8);
                auth_segment[9] = data_id;
                auth_segment[10] = 0x1d;
                if(accountkey_Index == 0x00)
                    auth_segment[11] = 0x03;
                else
                    auth_segment[11] = 0x01;
                memcpy(&auth_segment[12], gfps->adv_identifer, 20);
                auth_segment[32] = 0x01;
                DUMP8("%02x ", auth_segment, 33);

                gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 33, beacon_rsp.auth_data);

                beacon_rsp.data_id = GFPS_BEACON_READ_PROVISION_STATE;
                beacon_rsp.data_length = 0x1d;
                if(accountkey_Index == 0x00)
                    beacon_rsp.data = 0x03;
                else
                    beacon_rsp.data = 0x01;
                memcpy(beacon_rsp.EIK, gfps->adv_identifer, 20);

                app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
            }
        }
    }
    else if (data_id == GFPS_BEACON_SET_EPHEMERAL_IDENTITY_KEY)                //0x02, setting an ephemeral identity key, do not need nofity;
    {
         gap_set_rpa_timeout(0xA1B8);
         ble_gfps_spot_rotate_hard_timer_start(gfps);
         uint8_t owner_accountkey[16];
         nv_record_fp_account_key_get_by_index(0, owner_accountkey);
         uint8_t keyCount = nv_record_fp_account_key_count();
         uint8_t protocol_version = g->protocol_version;
         uint8_t nonce[8];
         memcpy(nonce, app_gfps_get_nonce(gfps),8);
         uint8_t orig_eph_identity_key[32];

         if (keyCount == 0)
         {
             status = GFPS_ERROR_INVALID_VALUE;
         }

         if (nv_record_fp_get_spot_adv_enable_value() == false)    //need to set EIK
         {
             uint8_t VArray[FP_ACCOUNT_KEY_SIZE];

             uint8_t calculatedHmacFirst8Bytes[8];

             uint8_t input_encrpyt_data[43];
             input_encrpyt_data[0] = protocol_version;
             memcpy(&input_encrpyt_data[1], nonce, 8);
             input_encrpyt_data[9] = data_id;
             input_encrpyt_data[10] = data_length;
             memcpy(&input_encrpyt_data[11], &p->value[10], 32);
             for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
             {
                 nv_record_fp_account_key_get_by_index(keyIndex, VArray);
                 gfps_beacon_encrpt_data(VArray, input_encrpyt_data, 43, calculatedHmacFirst8Bytes);

                 if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
                 {
                    TRACE(0,"matched");
                    status = GFPS_SUCCESS;
                    memcpy(gfps->matched_account_key, VArray, FP_ACCOUNT_KEY_SIZE);
                    break;
                 }
                 else
                 {
                     TRACE(0,"not matched");
                     status = GFPS_ERROR_UNAUTHENTICATED;
                 }
            }

            if(memcmp(owner_accountkey, gfps->matched_account_key, 16))
            {
                TRACE(0,"it is non-owner accout key");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
            else
            {
                TRACE(0,"it is owner account key");
            }

            if (status == GFPS_SUCCESS)
            {
                TRACE(0,"set key");
                gfps_crypto_decrypt(&p->value[10], 16, gfps->matched_account_key, orig_eph_identity_key);
                gfps_crypto_decrypt(&p->value[26], 16, gfps->matched_account_key, &orig_eph_identity_key[16]);

                TRACE(0,"orig_eph_identity_key is:");
                DUMP8("%02x", orig_eph_identity_key, 32);
                nv_record_fp_update_eph_identity_key(orig_eph_identity_key);

                spot_generate_EID_handler(gfps, orig_eph_identity_key);

                //write to flash
                nv_record_fp_update_spot_adv_data(gfps->adv_identifer);

                //start adv
                if (nv_record_fp_get_spot_adv_enable_value() == false)
                {
                    nv_record_fp_update_spot_adv_eanbled(true);
                    app_ble_start_connectable_adv(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);
                }

                //reply;
                uint8_t auth_segment[12];
                gfpsp_reading_set_beacon_provision_resp beacon_rsp;

                auth_segment[0] = protocol_version;
                memcpy(&auth_segment[1], nonce, 8);
                auth_segment[9] = data_id;
                auth_segment[10] = 0x08;
                auth_segment[11] = 0x01;

                gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 12, beacon_rsp.auth_data);

                beacon_rsp.data_id = data_id;
                beacon_rsp.data_length = 0x08;
                app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
            }
         }
         else                                                         //has been set EIK
         {
             TRACE(0,"has been set the EIK");
             uint8_t VArray[FP_ACCOUNT_KEY_SIZE];
             uint8_t calculatedHmacFirst8Bytes[8];
             uint8_t input_encrpyt_data[51];
             input_encrpyt_data[0] = protocol_version;
             memcpy(&input_encrpyt_data[1], nonce, 8);
             input_encrpyt_data[9] = data_id;
             input_encrpyt_data[10] = data_length;
             memcpy(&input_encrpyt_data[11], &p->value[10], 40);

             for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
             {
                 nv_record_fp_account_key_get_by_index(keyIndex, VArray);
                 gfps_beacon_encrpt_data(VArray, input_encrpyt_data, 51, calculatedHmacFirst8Bytes);

                 if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
                 {
                    TRACE(0,"matched");
                    status = GFPS_SUCCESS;
                    memcpy(gfps->matched_account_key, VArray, FP_ACCOUNT_KEY_SIZE);
                    break;
                 }
                 else
                 {
                     TRACE(0,"not matched");
                     status = GFPS_ERROR_UNAUTHENTICATED;
                 }
             }

             if(memcmp(owner_accountkey, gfps->matched_account_key, 16))
             {
                 TRACE(0,"it is non-owner accout key");
                 status = GFPS_ERROR_UNAUTHENTICATED;
             }
             else
             {
                 TRACE(0,"it is owner account key");
             }

             if (status == GFPS_SUCCESS)
             {
                 uint8_t Eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+8];
                 uint8_t hash_eph_result[32];

                 memcpy(Eph_identity_key,nv_record_fp_get_eph_identity_key(),FP_EPH_IDENTITY_KEY_LEN);
                 memcpy(&Eph_identity_key[32], app_gfps_get_nonce(gfps), 8);
                 gfps_SHA256_hash(Eph_identity_key, FP_EPH_IDENTITY_KEY_LEN+8, hash_eph_result);
                 DUMP8("%02x ", hash_eph_result, 8);
                 if(!memcmp(hash_eph_result, &p->value[42], 8))
                 {
                     TRACE(0,"eph key mathed");
                     status = GFPS_SUCCESS;
                 }
                 else
                 {
                     TRACE(0,"eph key not matched");
                     status = GFPS_ERROR_UNAUTHENTICATED;
                 }

                 if(status == GFPS_SUCCESS)
                 {
                     gfps_crypto_decrypt(&p->value[10], 16, gfps->matched_account_key, orig_eph_identity_key);
                     gfps_crypto_decrypt(&p->value[26], 16, gfps->matched_account_key, &orig_eph_identity_key[16]);
                     nv_record_fp_update_eph_identity_key(orig_eph_identity_key); //write the new eik to the flash;

                     //refresh adv
                     spot_generate_EID_handler(gfps, orig_eph_identity_key);
                     //write to flash
                     nv_record_fp_update_spot_adv_data(gfps->adv_identifer);
                     app_ble_refresh_adv_state(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);
                    
                     uint8_t auth_segment[12];
                     gfpsp_reading_set_beacon_provision_resp beacon_rsp;

                     auth_segment[0] = protocol_version;
                     memcpy(&auth_segment[1], nonce, 8);
                     auth_segment[9] = data_id;
                     auth_segment[10] = 0x08;
                     auth_segment[11] = 0x01;

                     gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 12, beacon_rsp.auth_data);

                     beacon_rsp.data_id = data_id;
                     beacon_rsp.data_length = 0x08;
                     app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
                 }
             }
         }
    }
    else if (data_id == GFPS_BEACON_CLEAR_EPHEMERAL_IDENTITY_KEY)        //clearing the ephemeral identity key
    {
        uint8_t keyCount = nv_record_fp_account_key_count();
        uint8_t protocol_version = g->protocol_version;
        uint8_t nonce[8];
        memcpy(nonce, app_gfps_get_nonce(gfps),8);

        if (keyCount == 0)
        {
            status = GFPS_ERROR_INVALID_VALUE;
        }
        for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++)
        {
            uint8_t VArray[FP_ACCOUNT_KEY_SIZE];
            uint8_t calculatedHmacFirst8Bytes[8];

            uint8_t input_encrpyt_data[19];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], nonce, 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;
            memcpy(&input_encrpyt_data[11], &p->value[10], 8);

            nv_record_fp_account_key_get_by_index(keyIndex, VArray);
            gfps_beacon_encrpt_data(VArray, input_encrpyt_data, 19, calculatedHmacFirst8Bytes);
            DUMP8("0x%02x ", calculatedHmacFirst8Bytes, 8);

            if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
            {
               TRACE(0,"matched");
               status = GFPS_SUCCESS;
               memcpy(gfps->matched_account_key, VArray, FP_ACCOUNT_KEY_SIZE);
               break;
            }
            else
            {
                TRACE(0,"not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
        }

        if(status == GFPS_SUCCESS)
        {
            uint8_t current_eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+8];
            uint8_t hash256ResultOfEphKey[32];
            memcpy(current_eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
            memcpy(&current_eph_identity_key[FP_EPH_IDENTITY_KEY_LEN], app_gfps_get_nonce(gfps), 8);

            gfps_SHA256_hash(current_eph_identity_key, (FP_EPH_IDENTITY_KEY_LEN+8), hash256ResultOfEphKey);
            if (!memcmp(&p->value[10], hash256ResultOfEphKey, 8))
            {
                TRACE(0,"eph key matched");
                status = GFPS_SUCCESS;
            }
            else
            {
                TRACE(0,"eph key not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
        }

        if (status == GFPS_SUCCESS)
        {
            nv_record_fp_reset_eph_identity_key();

            nv_record_fp_reset_spot_adv_enable_value();
            nv_record_fp_reset_spot_adv_data();
            app_ble_start_connectable_adv(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);

            uint8_t auth_segment[12];
            gfpsp_clearing_EIK_beacon_provision_resp beacon_rsp;

            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], nonce, 8);
            auth_segment[9] = data_id;
            auth_segment[10] = 0x08;
            auth_segment[11] = 0x01;
            gfps_beacon_encrpt_data(gfps->matched_account_key, auth_segment, 12, beacon_rsp.auth_data);

            beacon_rsp.data_id= data_id;
            beacon_rsp.data_length = 0x08;
            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));

        }
    }
    else if (data_id == GFPS_BEACON_READ_EPHEMERAL_IDENTITY_KEY)            //reading the ephemeral identity key
    {
        if (1)//gfps_is_in_fastpairing_mode())
        {
            uint8_t hashed_recover_key[GFPS_RECOVERY_KEY_SIZE+8];
            memset(hashed_recover_key,0, 16);

            uint8_t protocol_version = g->protocol_version;
            uint8_t hash256Result[32];
            uint8_t eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+1];
            //uint8_t hashResultOfrecoveryKey[32];
            uint8_t calculatedHmacFirst8Bytes[8];


            memcpy(eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
            eph_identity_key[FP_EPH_IDENTITY_KEY_LEN] = 0x01;
            gfps_SHA256_hash(eph_identity_key, 33, hash256Result);
            memcpy(hashed_recover_key, hash256Result, 8);                    //recovery key;

            uint8_t input_encrpyt_data[11];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;

            gfps_beacon_encrpt_data(hashed_recover_key, input_encrpyt_data, 11, calculatedHmacFirst8Bytes);
            if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
            {
                TRACE(0,"matched");
                status = GFPS_SUCCESS;
            }
            else
            {
                TRACE(0,"not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }

            if (status == GFPS_SUCCESS)
            {
                gfpsp_reading_beacon_identity_key_resp beacon_rsp;
                uint8_t encrpted_ep_key[32];
                DUMP8("%02x ", gfps->matched_account_key, 16);
                gfps_crypto_encrypt(eph_identity_key, 16, gfps->matched_account_key, encrpted_ep_key);
                gfps_crypto_encrypt(&eph_identity_key[16], 16, gfps->matched_account_key, &encrpted_ep_key[16]);

                beacon_rsp.data_id = data_id;
                beacon_rsp.data_length = 0x28;

                uint8_t auth_segment[44];
                auth_segment[0] = protocol_version;
                memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
                auth_segment[9] = data_id;
                auth_segment[10] = 0x28;
                memcpy(&auth_segment[11], encrpted_ep_key, 32);    //if eph_identity_key need account key to encrypted
                auth_segment[43] = 0x01;
                gfps_beacon_encrpt_data(hashed_recover_key, auth_segment, 44, beacon_rsp.auth_data);

                memcpy(beacon_rsp.data, encrpted_ep_key, 32);
                app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
            }

        }
        else
        {
            TRACE(0,"not in fast pair mode");
            status = GFPS_ERROR_NO_USER_CONSENT;
        }
    }
    else if (data_id == GFPS_BEACON_RING)                    //ringing, data id 0x05
    {
        memset(gfps->hashed_ring_key, 0, 16);

        uint8_t protocol_version = g->protocol_version;
        uint8_t hash256Result[32];
        uint8_t eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+1];
        uint8_t calculatedHmacFirst8Bytes[8];

        memcpy(eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
        eph_identity_key[FP_EPH_IDENTITY_KEY_LEN] = 0x02;
        gfps_SHA256_hash(eph_identity_key, 33, hash256Result);
        memcpy(gfps->hashed_ring_key, hash256Result, 8);                    //ring key;

        uint8_t input_encrpyt_data[15];
        input_encrpyt_data[0] = protocol_version;
        memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
        input_encrpyt_data[9] = data_id;
        input_encrpyt_data[10] = data_length;
        memcpy(&input_encrpyt_data[11], &p->value[10], 4);
        gfps_beacon_encrpt_data(gfps->hashed_ring_key, input_encrpyt_data, 15, calculatedHmacFirst8Bytes);

        if (gfps->enable_unwanted_tracking_mode && (gfps->control_flag==GFPS_BEACON_CONTROL_FLAG_SKIP_RING_AUT))
        {
            //do not need auth;
            status = GFPS_SUCCESS;
        }
        else
        {
            if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
            {
                TRACE(0,"ring, key matched");
                status = GFPS_SUCCESS;
            }
            else
            {
                TRACE(0,"ring ,key not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }
        }

        if (status == GFPS_SUCCESS)
        {
            uint8_t ring_state;
            memcpy(&ring_state,&p->value[10],1);
            uint16_t ring_time;
            uint8_t big_ring_time[2];
            uint8_t ring_rsp_time[2];
            uint16_t remaining_ring_time = 0x00;
            //uint8_t ring_volume;

            gfpsp_beacon_ring_resp beacon_rsp;

            big_little_switch(&p->value[11], big_ring_time, 2);
            memcpy(&ring_time, big_ring_time, 2);
            ring_time = ring_time/10;
            TRACE(2,"ring_state is %x, remaining_ring_time is %d",ring_state, ring_time);

            if (ring_state != 0x00)
            {
                spot_ring_timer_set(gfps);
            }

            if (ring_state == GFPS_BEACON_RINGING_ALL)
            {
                app_voice_start_gfps_find();
                //ring left and right;
                beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STATED;
                beacon_rsp.data[1] = GFPS_BEACON_RINGING_RIGHT_AND_LEFT;
                remaining_ring_time = (ring_time-gfps->spot_ring_nums*2)*10;

                memcpy(ring_rsp_time, &remaining_ring_time, 0x02);
                big_little_switch(ring_rsp_time, &beacon_rsp.data[2],2);
                gfps->ring_state = true;
            }
            else if (ring_state == GFPS_BEACON_RINGING_RIGHT)
            {
                 //ring right
                app_voice_start_gfps_find();

                beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STATED;
                beacon_rsp.data[1] = GFPS_BEACON_RINGING_RIGHT;
                remaining_ring_time = (ring_time-gfps->spot_ring_nums*2)*10;

                memcpy(ring_rsp_time, &remaining_ring_time, 0x02);
                big_little_switch(ring_rsp_time, &beacon_rsp.data[2],2);
                gfps->ring_state = true;
            }
            else if (ring_state == GFPS_BEACON_RINGING_LEFT)
            {
                //ring left
                app_voice_start_gfps_find();

                beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STATED;
                beacon_rsp.data[1] = GFPS_BEACON_RINGING_LEFT;
                remaining_ring_time = (ring_time-gfps->spot_ring_nums*2)*10;

                memcpy(ring_rsp_time, &remaining_ring_time, 0x02);
                big_little_switch(ring_rsp_time, &beacon_rsp.data[2], 2);
                gfps->ring_state = true;
            }
            else if (ring_state == GFPS_BEACON_RINGING_BOX)
            {
                //ring box, box cant't ring in now case, and tell it to the phone;
                //if box can ring, customer can change the reply
                beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_FAILED;
                beacon_rsp.data[1] = GFPS_BEACON_RINGING_BOX;
                remaining_ring_time = 0x00;

                memcpy(&beacon_rsp.data[2], &remaining_ring_time, 2);

            }
            else
            {
                //ring none
                app_voice_stop_gfps_find();
                beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STOPPED_REQUEST;
                beacon_rsp.data[1] = GFPS_BEACON_RINGING_NONE;
                remaining_ring_time = 0x00;
                memcpy(&beacon_rsp.data[2],&remaining_ring_time,2);

                gfps->spot_ring_nums =0;
                spot_ring_timer_stop(gfps);
            }
            gfps->remaining_ring_time = remaining_ring_time;

            uint8_t auth_segment[16];
            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
            auth_segment[9] = data_id;
            auth_segment[10] = 0x0c;
            memcpy(&auth_segment[11], beacon_rsp.data, 4);
            auth_segment[15] = 0x01;
            gfps_beacon_encrpt_data(gfps->hashed_ring_key, auth_segment, 16, beacon_rsp.auth_data);

            beacon_rsp.data_id = GFPS_BEACON_RING;
            beacon_rsp.data_length = 0x0c;
            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
        }
    }
    else if (data_id == GFPS_BEACON_READ_RING_STATE)     //read ring state,0x06
    {
        memset(gfps->hashed_ring_key, 0, 16);

        uint8_t protocol_version = g->protocol_version;
        uint8_t hash256Result[32];
        uint8_t eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+1];
        uint8_t calculatedHmacFirst8Bytes[8];
        uint8_t ring_rsp_time[2];

        memcpy(eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
        eph_identity_key[FP_EPH_IDENTITY_KEY_LEN] = 0x02;
        gfps_SHA256_hash(eph_identity_key, 33, hash256Result);
        memcpy(gfps->hashed_ring_key, hash256Result, 8);                    //ring key;

        uint8_t input_encrpyt_data[11];
        input_encrpyt_data[0] = protocol_version;
        memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
        input_encrpyt_data[9] = data_id;
        input_encrpyt_data[10] = data_length;
        gfps_beacon_encrpt_data(gfps->hashed_ring_key, input_encrpyt_data, 11, calculatedHmacFirst8Bytes);

        if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
        {
            TRACE(0,"matched");
            status = GFPS_SUCCESS;
            uint16_t remaining_time;
            gfpsp_reading_beacon_ring_state_resp beacon_rsp;

            beacon_rsp.data_id = GFPS_BEACON_READ_RING_STATE;
            beacon_rsp.data_length = 0x0b;
            if(!gfps->ring_state)
            {
                beacon_rsp.data[0] = GFPS_BEACON_INCAPABLE_OF_RING;
                remaining_time = 0x00;
            }
            else
            {
                beacon_rsp.data[0] = GFPS_BEACON_TWO_CAPABLE_OF_RING;
                remaining_time = gfps->remaining_ring_time;
            }
            memcpy(ring_rsp_time, &remaining_time,  sizeof(uint16_t));
            big_little_switch(ring_rsp_time, &beacon_rsp.data[1], sizeof(uint16_t));

            uint8_t auth_segment[15];
            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
            auth_segment[9] = data_id;
            auth_segment[10] = 0x0b;
            memcpy(&auth_segment[11], beacon_rsp.data, 3);
            auth_segment[14] = 0x01;
            gfps_beacon_encrpt_data(gfps->hashed_ring_key, auth_segment, 15, beacon_rsp.auth_data);

            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
        }
        else
        {
            TRACE(0,"not matched");
            status = GFPS_ERROR_NO_USER_CONSENT;
        }
    }
    else if (data_id == GFPS_BEACON_ACTIVATE_UNWANTED_TRACK_MODE)      //0x07
    {
        gap_set_rpa_timeout(0xA1B8);
        gfps->control_flag = p->value[10];  //if param is 0x01,just ring not need auth,
        gfps->enable_unwanted_tracking_mode = true;  //frame type is 0x41
        gfps->original_flag = 0x80;   //now don't support battery level indication;

        g->curr_unwanted_tracking_mode = gfps->enable_unwanted_tracking_mode;
        g->curr_original_flag = gfps->original_flag;

        uint8_t hashed_orignal_value[32];
        uint8_t hashed_output_value[32];
        memset(hashed_orignal_value, 0, 32);
        memcpy(hashed_orignal_value, gfps->Ecc_private_key, 20);
        gfps_SHA256_hash(hashed_orignal_value, 32, hashed_output_value);
        TRACE(1,"control_flag is %d", p->value[10]);
        DUMP8("%02x ", hashed_output_value, 32);
        gfps->hashed_flag = hashed_output_value[0];
        g->curr_hashed_flag = gfps->hashed_flag;
        //refresh adv
        if( nv_record_fp_get_spot_adv_enable_value() == true)
        {
             app_ble_refresh_adv_state(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);
        }

        uint8_t unwanted_track_key[GFPS_RING_KEY_SIZE+8];
        memset(unwanted_track_key, 0, 16);

        uint8_t protocol_version = g->protocol_version;
        uint8_t hash256Result[32];
        uint8_t eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+1];
        uint8_t calculatedHmacFirst8Bytes[8];

        memcpy(eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
        eph_identity_key[FP_EPH_IDENTITY_KEY_LEN] = 0x03;
        gfps_SHA256_hash(eph_identity_key, 33, hash256Result);
        memcpy(unwanted_track_key, hash256Result, 8);                    //unwanted_track_key key;

        if ((p->value[1] == 0x09) && (gfps->control_flag == GFPS_BEACON_CONTROL_FLAG_SKIP_RING_AUT))
        {
            uint8_t input_encrpyt_data[12];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;
            input_encrpyt_data[11] = p->value[10];
            gfps_beacon_encrpt_data(unwanted_track_key, input_encrpyt_data, 12, calculatedHmacFirst8Bytes);
        }
        else
        {
            uint8_t input_encrpyt_data[11];
            input_encrpyt_data[0] = protocol_version;
            memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
            input_encrpyt_data[9] = data_id;
            input_encrpyt_data[10] = data_length;
            gfps_beacon_encrpt_data(unwanted_track_key, input_encrpyt_data, 11, calculatedHmacFirst8Bytes);
        }

        if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
        {
            TRACE(0,"acitve unwanted, key matched");
            status = GFPS_SUCCESS;
        }
        else
        {
            TRACE(0,"acitve unwanted, key not matched");
            status = GFPS_ERROR_UNAUTHENTICATED;
        }

        if (status == GFPS_SUCCESS)
        {
            gfpsp_beacon_activate_unwanted_tracking_mode_resp beacon_rsp;
            beacon_rsp.data_id = data_id;
            beacon_rsp.data_length = 0x08;

            uint8_t auth_segment[12];
            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
            auth_segment[9] = data_id;
            auth_segment[10]= 0x08;
            auth_segment[11] = 0x01;

            gfps_beacon_encrpt_data(unwanted_track_key, auth_segment, 12, beacon_rsp.auth_data);

            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
        }
    }
    else if (data_id == GFPS_BEACON_DEACTIVATE_UNWANTED_TRACK_MODE)
    {
        hwtimer_stop(spot_EID_timeout_hard_timer_id);
        ble_gfps_spot_rotate_hard_timer_start(gfps);
        gfps->enable_unwanted_tracking_mode = false;
        gfps->original_flag= 0x00;

        g->curr_unwanted_tracking_mode = gfps->enable_unwanted_tracking_mode;
        g->curr_original_flag = gfps->original_flag;

        //refresh adv
        if (nv_record_fp_get_spot_adv_enable_value() == true)
        {
             app_ble_refresh_adv_state(BLE_FASTPAIR_SPOT_ADVERTISING_INTERVAL);
        }

        uint8_t unwanted_track_key[GFPS_RING_KEY_SIZE+8];
        memset(unwanted_track_key, 0, 16);

        uint8_t protocol_version = g->protocol_version;
        uint8_t hash256Result[32];
        uint8_t eph_identity_key[FP_EPH_IDENTITY_KEY_LEN+1];
        uint8_t calculatedHmacFirst8Bytes[8];
        uint8_t eph_identity_key_nonce[FP_EPH_IDENTITY_KEY_LEN+8];
        uint8_t hashed_of_eik_and_nonce[32];

        memcpy(eph_identity_key, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
        eph_identity_key[FP_EPH_IDENTITY_KEY_LEN] = 0x03;
        gfps_SHA256_hash(eph_identity_key, 33, hash256Result);
        memcpy(unwanted_track_key, hash256Result, 8);                    //unwanted_track_key key;

        memcpy(eph_identity_key_nonce, nv_record_fp_get_eph_identity_key(), FP_EPH_IDENTITY_KEY_LEN);
        memcpy(&eph_identity_key_nonce[32], app_gfps_get_nonce(gfps), 8);
        gfps_SHA256_hash(eph_identity_key_nonce, 40, hashed_of_eik_and_nonce);

        uint8_t input_encrpyt_data[19];
        input_encrpyt_data[0] = protocol_version;
        memcpy(&input_encrpyt_data[1], app_gfps_get_nonce(gfps), 8);
        input_encrpyt_data[9] = data_id;
        input_encrpyt_data[10] = data_length;
        memcpy(&input_encrpyt_data[11], &p->value[10], 8);
        gfps_beacon_encrpt_data(unwanted_track_key, input_encrpyt_data, 19, calculatedHmacFirst8Bytes);

        if (!memcmp(calculatedHmacFirst8Bytes, &p->value[2], 8))
        {
            TRACE(0,"matched");

            if(!memcmp(hashed_of_eik_and_nonce, &p->value[10], 8))
            {
                TRACE(0,"track key matched");
                status = GFPS_SUCCESS;
            }
            else
            {
                TRACE(0, "track key not matched");
                status = GFPS_ERROR_UNAUTHENTICATED;
            }

        }
        else
        {
            TRACE(0,"not matched");
            status = GFPS_ERROR_UNAUTHENTICATED;
        }

        if (status == GFPS_SUCCESS)
        {
            gfpsp_beacon_deactivate_unwanted_tracking_mode_resp beacon_rsp;
            beacon_rsp.data_id = data_id;
            beacon_rsp.data_length = 0x08;

            uint8_t auth_segment[12];
            auth_segment[0] = protocol_version;
            memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
            auth_segment[9] = data_id;
            auth_segment[10] = 0x08;
            auth_segment[11] = 0x01;
            gfps_beacon_encrpt_data(unwanted_track_key, auth_segment, 12, beacon_rsp.auth_data);

            app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
        }
    }
    else
    {
        TRACE(0, "error, no this data id");
    }

    return status;
}

void app_spot_press_stop_ring_handle(uint16_t connhdl, APP_KEY_STATUS *status, void *param)
{
    ble_gfps_t *gfps = NULL;
    gfpsp_beacon_ring_resp beacon_rsp;
    uint16_t remaining_ring_time;
    uint8_t rsp_time[2];

    gfps = (ble_gfps_t *)ble_get_gfps(connhdl);
    if (gfps == NULL)
    {
        TRACE(0, "app_spot_press_stop_ring_handle: invalid %04x", connhdl);
        return;
    }

    TRACE(0,"app_spot_press_stop_ring_handle");

    remaining_ring_time = (gfps->remaining_ring_time / 10 - gfps->spot_ring_nums * 2) * 10;
    app_voice_stop_gfps_find();
    beacon_rsp.data[0] = GFPS_BEACON_RINGING_STATE_STOPPED_PRESS;
    beacon_rsp.data[1] = GFPS_BEACON_INCAPABLE_OF_RING;

    memcpy(rsp_time, &remaining_ring_time ,2);

    big_little_switch(rsp_time, &beacon_rsp.data[2], 2);
    gfps->spot_ring_nums = 0;
    spot_ring_timer_stop(gfps);

    uint8_t auth_segment[16];
    auth_segment[0] = app_gfps_get_protocol_version();
    memcpy(&auth_segment[1], app_gfps_get_nonce(gfps), 8);
    auth_segment[9] = GFPS_BEACON_RING;
    auth_segment[10] = 0x0c;
    memcpy(&auth_segment[11], beacon_rsp.data, 4);
    auth_segment[15] = 0x01;
    gfps_beacon_encrpt_data(gfps->hashed_ring_key, auth_segment, 16, beacon_rsp.auth_data);

    beacon_rsp.data_id = GFPS_BEACON_RING;
    beacon_rsp.data_length = 0x0c;

    app_gfps_send_beacon_data_via_notification(gfps, (uint8_t *)&beacon_rsp, sizeof(beacon_rsp));
}
#endif

void app_bt_set_fast_pair_info(FastPairInfo fast_pair_info)
{
    ble_gfps_global_t *g = ble_gfps_global();
    memcpy(&g->fast_pair_info, &fast_pair_info, sizeof(fast_pair_info));
}

void app_gfps_set_tx_power_in_adv(char rssi)
{
    ble_gfps_global_t *g = ble_gfps_global();
    g->gfps_tx_power = rssi;
}

void app_bt_set_fast_pair_tx_power(int8_t tx_power)
{
    app_gfps_set_tx_power_in_adv(tx_power);
}

const uint8_t* app_bt_get_fast_pair_public_key(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    return g->fast_pair_info.public_anti_spoofing_key;
}

const uint8_t* app_bt_get_fast_pair_private_key(void)
{
    ble_gfps_global_t *g = ble_gfps_global();
    return g->fast_pair_info.private_anti_spoofing_key;
}

#endif /* GFPS_ENABLED */
