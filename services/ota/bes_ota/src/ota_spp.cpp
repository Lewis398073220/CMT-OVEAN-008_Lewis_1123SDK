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
 *
 *    Leonardo        2018/12/20
 ****************************************************************************/
#include "cmsis_os.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "app_audio.h"
#include "bluetooth_bt_api.h"
#include "app_bt_stream.h"
#include "nvrecord_env.h"
#include "cqueue.h"
#include "ota_dbg.h"
#include "ota_spp.h"
#include "ota_bes.h"
#include "ota_basic.h"

static app_spp_ota_tx_done_t app_spp_ota_tx_done_func = NULL;

OtaContext ota;

/****************************************************************************
 * OTA SPP SDP Entries
 ****************************************************************************/

static const uint8_t OTA_SPP_UUID_128[16] = BES_OTA_UUID_128;
/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
static const U8 OtaSppClassId[] = {
#ifdef IS_BES_OTA_USE_CUSTOM_RFCOMM_UUID
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    SDP_UUID_128BIT(OTA_SPP_UUID_128), /* 128 bit UUID in Big Endian */
#else
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */
#endif
};

static const U8 OtaSppProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12), /* Data element sequence, 12 bytes */

    /* Each element of the list is a Protocol descriptor which is a
     * data element sequence. The first element is L2CAP which only
     * has a UUID element.
     */
    SDP_ATTRIB_HEADER_8BIT(3), /* Data element sequence for L2CAP, 3
                                  * bytes
                                  */

    SDP_UUID_16BIT(PROT_L2CAP), /* Uuid16 L2CAP */

    /* Next protocol descriptor in the list is RFCOMM. It contains two
     * elements which are the UUID and the channel. Ultimately this
     * channel will need to filled in with value returned by RFCOMM.
     */

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_BES_OTA)};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 OtaSppProfileDescList[] = {
#ifdef IS_BES_OTA_USE_CUSTOM_RFCOMM_UUID
     SDP_ATTRIB_HEADER_8BIT(22), /* Data element sequence, 22 bytes */

    /* Data element sequence for ProfileDescriptor, 20 bytes */
    SDP_ATTRIB_HEADER_8BIT(20),

    SDP_UUID_128BIT(OTA_SPP_UUID_128), /* Uuid128 SPP */
#else
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
#endif
    SDP_UINT_16BIT(0x0102)          /* As per errata 2239 */
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 OtaSppServiceName[] = {
    SDP_TEXT_8BIT(7), /* Null terminated text string */
    'B', 'E', 'S', 'O', 'T', 'A', '\0'};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static bt_sdp_record_attr_t OtaSppSdpAttributes[] = { // list attr id in ascending order

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, OtaSppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, OtaSppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, OtaSppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), OtaSppServiceName),
};

void ota_disconnect(void)
{
    LOG_D("%s %d ",__func__,__LINE__);

    if (!app_is_in_ota_mode())
    {
        return;
    }

    if (ota.ota_spp_dev)
    {
        bt_spp_disconnect(ota.ota_spp_dev->rfcomm_handle, BTIF_BEC_LOCAL_TERMINATED);
    }

    return;
}

static int ota_spp_bes_handle_data_event_func(const bt_bdaddr_t *remote, bt_spp_event_t event, bt_spp_callback_param_t *param)
{
    uint8_t *pData = (uint8_t *)param->rx_data_ptr;
    uint16_t dataLen = param->rx_data_len;

    LOG_D("SPP1 bes_ota receive pData %p length=%d", pData, dataLen);
    DUMP8("0x%02x ", pData, (dataLen > 15) ? 15 : dataLen);
    app_statistics_get_ota_pkt_in_role_switch();
    ota_control_handle_received_data(pData, false, dataLen);

    return 0;
}

static int spp_bes_ota_callback(const bt_bdaddr_t *remote, bt_spp_event_t event, bt_spp_callback_param_t *param)
{
    bt_spp_channel_t *spp_chan = param->spp_chan;
    if (BT_SPP_EVENT_OPENED == event)
    {
        LOG_D("spp_bes_ota_callback ::BTIF_SPP_EVENT_REMDEV_CONNECTED %02x", event);
        ota_basic_enable_datapath_status(OTA_BASIC_SPP_DATAPATH_ENABLED, spp_chan->remote.address);

        ota.permissionToApply = 0;
        ota.ota_spp_dev = param->spp_chan;

        app_ota_connected(APP_OTA_CONNECTED);
        ota_control_update_MTU(OTA_SPP_MAX_PACKET_SIZE);
        ota_control_set_datapath_type(DATA_PATH_SPP);
        ota_control_register_transmitter(app_ota_send_data_via_spp);
    }
    else if (BT_SPP_EVENT_CLOSED == event)
    {
        LOG_D("spp_bes_ota_callback ::BTIF_SPP_EVENT_REMDEV_DISCONNECTED %02x", event);

        ota_basic_disable_datapath_status(OTA_BASIC_SPP_DATAPATH_ENABLED, spp_chan->remote.address);

        app_ota_disconnected(APP_OTA_DISCONNECTED, APP_OTA_LINK_TYPE_SPP);
        ota_control_set_datapath_type(0);
        app_spp_ota_tx_done_func = NULL;
        osDelay(100);
    }
    else if (BT_SPP_EVENT_TX_DONE == event)
    {
        if (app_spp_ota_tx_done_func)
        {
            app_spp_ota_tx_done_func();
        }
    }
    else if (BT_SPP_EVENT_RX_DATA == event)
    {
        ota_spp_bes_handle_data_event_func(remote, event, param);
    }
    else
    {
        LOG_D("spp_bes_ota_callback ::unknown event %02x", event);
    }
    return 0;
}

void app_ota_send_cmd_via_spp(uint8_t *ptrData, uint16_t length)
{
    if (!app_is_in_ota_mode() && (ota_control_get_datapath_type() != DATA_PATH_SPP))
    {
        return;
    }

    bt_spp_write(ota.ota_spp_dev->rfcomm_handle, ptrData, length);
}

void app_ota_send_data_via_spp(uint8_t *ptrData, uint32_t length)
{
    if (!app_is_in_ota_mode() && (ota_control_get_datapath_type() != DATA_PATH_SPP))
    {
        return;
    }

    bt_spp_write(ota.ota_spp_dev->rfcomm_handle, ptrData, length);
}

void app_spp_ota_register_tx_done(app_spp_ota_tx_done_t callback)
{
    app_spp_ota_tx_done_func = callback;
}

void app_spp_ota_init(void)
{
    bt_spp_create_port(RFCOMM_CHANNEL_BES_OTA, OtaSppSdpAttributes, ARRAY_SIZE(OtaSppSdpAttributes));

    bt_spp_set_callback(RFCOMM_CHANNEL_BES_OTA, OTA_SPP_RECV_BUFFER_SIZE, spp_bes_ota_callback, NULL);

    bt_spp_listen(RFCOMM_CHANNEL_BES_OTA, false, NULL);

    ota.ota_spp_dev = bt_spp_create_channel(BT_DEVICE_ID_1, RFCOMM_CHANNEL_BES_OTA);
}
