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
#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED

#include "app_dbg_ble_audio_info.h"
#include "gaf_cfg.h"
#include "gaf_media_stream.h"
#include "app_vendor_cmd_evt.h"
#include "app_bap_uc_srv_msg.h"
#include "app_gaf_dbg.h"
#include "aob_volume_api.h"

static app_dbg_ble_con_info_t app_ble_con_dbg_info[BLE_CONNECTION_MAX];

static void app_dbg_ble_register_con_info_record_cb(gapm_dbg_ble_con_cmp_info_record_cb func)
{
    gapm_dbg_ble_register_con_info_record_cb(func);
}

static void app_dbg_ble_register_con_disconnect_cb(gapm_dbg_ble_dis_con_cmp_cb func)
{
    gapc_dbg_ble_register_disconnect_info_record_cb(func);
}

static void app_dbg_ble_con_info_record(uint8_t status, uint8_t conidx, uint8_t peer_addr_type,
    uint8_t *p_peer_addr, uint16_t con_interval, uint16_t con_latency, uint16_t sup_to)
{
    if (CO_ERROR_NO_ERROR != status)
    {
        LOG_W("%s fail-reason=%d", __func__, status);
        return;
    }
    bool isHaveSpaceToRecordConInfo = false;
    app_dbg_ble_con_info_t *con_info_to_record = NULL;

    app_dbg_ble_con_info_t *all_con_info = (app_dbg_ble_con_info_t*)app_dbg_ble_get_all_con_info();
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if ((conidx == all_con_info[i].conidx) || (GAP_INVALID_CONIDX == all_con_info[i].conidx))
        {
            isHaveSpaceToRecordConInfo = true;
            con_info_to_record = all_con_info + i;
            break;
        }
    }

    if (!isHaveSpaceToRecordConInfo)
    {
        ///Try to delete the disconnected le link info if existed to record new le link info.
        for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
        {
            if (CO_ERROR_NO_ERROR != all_con_info[i].status)
            {
                con_info_to_record = all_con_info + i;
                break;
            }
        }
    }

    if (NULL != con_info_to_record)
    {
        con_info_to_record->status = status;
        con_info_to_record->conidx = conidx;
        con_info_to_record->peer_addr_type = peer_addr_type;
        if (NULL != p_peer_addr)
        {
            memcpy(con_info_to_record->peer_addr, p_peer_addr, BD_ADDR_LEN);
        }
        con_info_to_record->con_interval = con_interval;
        con_info_to_record->con_latency = con_latency;
        con_info_to_record->sup_to = sup_to;
    }
}

static void app_dbg_ble_con_dis_info_record(uint8_t conidx, uint8_t dis_con_reason)
{
    app_dbg_ble_con_info_t *all_con_info = (app_dbg_ble_con_info_t*)app_dbg_ble_get_all_con_info();

    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        if (conidx == all_con_info[i].conidx)
        {
            all_con_info[i].status = dis_con_reason;
            break;
        }
    }
    app_dbg_ble_audio_cis_dbg_info_status_updated_when_con_dis(conidx, dis_con_reason);
}

app_dbg_ble_con_info_t* app_dbg_ble_get_all_con_info(void)
{
    return app_ble_con_dbg_info;
}

app_dbg_ble_con_info_t* app_ble_dbg_get_con_info_by_conidx(uint8_t conidx)
{
    app_dbg_ble_con_info_t *all_con_info = app_dbg_ble_get_all_con_info();
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (all_con_info[i].conidx == conidx)
        {
            return (&all_con_info[i]);
        }
    }
    return NULL;
}

static app_dbg_ble_channel_map_total_info_t app_dbg_ble_channel_map_total_info[BLE_CONNECTION_MAX];
app_dbg_ble_channel_map_total_info_t* app_ble_dbg_get_all_con_channel_map_info(void)
{
    return &app_dbg_ble_channel_map_total_info[0];
}

app_dbg_ble_channel_map_total_info_t* app_ble_dbg_get_channel_map_info_through_conidx(uint8_t conidx)
{
    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        if ((app_dbg_ble_channel_map_total_info[i].conidx == conidx)
            || (GAP_INVALID_CONIDX ==app_dbg_ble_channel_map_total_info[i].conidx))
        {
            return &app_dbg_ble_channel_map_total_info[i];
        }
    }
    return NULL;
}

app_dbg_ble_channel_map_info_t* app_ble_dbg_get_latest_channel_map_info_through_conidx(uint8_t conidx)
{
    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        if (app_dbg_ble_channel_map_total_info[i].conidx == conidx)
        {
            return &app_dbg_ble_channel_map_total_info[i].channel_map_info[0];
        }
    }

    return NULL;
}

void app_ble_dbg_record_channel_map_info(uint16_t con_hdl, uint8_t *p_ChM_info, uint8_t info_len)
{
    TRACE(0," %s done",__func__);
    if ((NULL == p_ChM_info) || (info_len != sizeof(app_dbg_ble_channel_map_info_t)))
    {
        LOG_W("%s fail!!!", __func__);
        return;
    }

    uint8_t conidx = gapc_get_conidx(con_hdl);
    app_dbg_ble_channel_map_total_info_t *con_channel_map_info = app_ble_dbg_get_channel_map_info_through_conidx(conidx);
    if (NULL == con_channel_map_info)
    {
        LOG_W("%s fail no space record conidx=%d", __func__, conidx);
        return;
    }

    con_channel_map_info->conidx = conidx;
    uint8_t numOfChlMapInfoHaveSaved = con_channel_map_info->numInfoHaveSaved;
    if (numOfChlMapInfoHaveSaved < APP_DBG_BLE_CHANNEL_MAP_INFO_NUM)
    {
        memcpy(&con_channel_map_info->channel_map_info[numOfChlMapInfoHaveSaved], p_ChM_info, info_len);
        con_channel_map_info->numInfoHaveSaved++;
    }
    else
    {
        app_dbg_ble_channel_map_info_t info_to_copy[APP_DBG_BLE_CHANNEL_MAP_INFO_NUM];
        uint8_t num_to_copy = APP_DBG_BLE_CHANNEL_MAP_INFO_NUM - 1;
        memcpy(info_to_copy, con_channel_map_info->channel_map_info,
                APP_DBG_BLE_CHANNEL_MAP_INFO_NUM * sizeof(app_dbg_ble_channel_map_info_t));
        memcpy(con_channel_map_info->channel_map_info, &info_to_copy[1],
                num_to_copy * sizeof(app_dbg_ble_channel_map_info_t));
        memcpy(&con_channel_map_info->channel_map_info[num_to_copy], p_ChM_info, info_len);
    }

}

void app_dbg_ble_info_system_init(void)
{
    TRACE(0," %s done",__func__);
    app_dbg_ble_channel_map_total_info_t *con_channel_map_info = app_ble_dbg_get_all_con_channel_map_info();
    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        con_channel_map_info[i].conidx = GAP_INVALID_CONIDX;
    }

    app_dbg_ble_con_info_t *all_con_info = (app_dbg_ble_con_info_t *)app_dbg_ble_get_all_con_info();
    memset(all_con_info, 0x00, sizeof(app_dbg_ble_con_info_t) * BLE_CONNECTION_MAX);
    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        all_con_info[i].conidx = GAP_INVALID_CONIDX;
    }

    app_dbg_ble_register_con_info_record_cb(app_dbg_ble_con_info_record);
    app_dbg_ble_register_con_disconnect_cb(app_dbg_ble_con_dis_info_record);

    app_dbg_ble_audio_info_init();
}


///BLE Audio Debug Info Part:
static void app_dbg_ble_audio_register_cis_info_record_cb
    (iap_dbg_ble_cis_established_info_record_cb func)
{
    iap_dbg_ble_cis_established_evt_cb_register(func);
}

static app_dbg_ble_audio_cis_dbg_info_t app_ble_audio_cis_dbg_info[IAP_NB_STREAMS];   //IAP_NB_STREAMS
app_dbg_ble_audio_cis_dbg_info_t* app_dbg_ble_audio_total_cis_dbg_info_get(void)
{
    return app_ble_audio_cis_dbg_info;
}

app_dbg_ble_audio_cis_dbg_info_t* app_dbg_ble_audio_cis_dbg_info_get_from_conidx(uint8_t conidx)
{
    app_dbg_ble_audio_cis_dbg_info_t* total_cis_dbg_info = app_dbg_ble_audio_total_cis_dbg_info_get();
    for (uint8_t i = 0;i < IAP_NB_STREAMS;i++)
    {
        if (total_cis_dbg_info[i].conidx == conidx)
        {
            return &total_cis_dbg_info[i];
        }
    }

    return NULL;
}

static void app_dbg_ble_audio_cis_dbg_info_record(uint8_t conidx, struct hci_le_cis_established_evt const* cis_info)
{
    bool isHaveSpaceToRecordCISInfo = false;
    app_dbg_ble_audio_cis_dbg_info_t* total_cis_dbg_info = app_dbg_ble_audio_total_cis_dbg_info_get();

    if (NULL == cis_info)
    {
        LOG_W("%s cis_info is NULL!", __func__);
        return;
    }

    if (CO_ERROR_NO_ERROR != cis_info->status)
    {
        LOG_W("%s fail! status=%d", __func__, cis_info->status);
        return;
    }

    app_dbg_ble_audio_cis_dbg_info_t* cis_dbg_info_to_record = NULL;
    for (uint8_t i = 0;i < IAP_NB_STREAMS;i++)
    {
        if ((GAP_INVALID_CONIDX == total_cis_dbg_info[i].conidx) || (total_cis_dbg_info[i].conidx == conidx))
        {
            cis_dbg_info_to_record = total_cis_dbg_info + i;
            isHaveSpaceToRecordCISInfo = true;
            break;
        }
    }

    if (!isHaveSpaceToRecordCISInfo)
    {
        for (uint8_t i = 0;i < IAP_NB_STREAMS;i++)
        {
            if (GAP_INVALID_CONIDX == total_cis_dbg_info[i].status)
            {
                cis_dbg_info_to_record = total_cis_dbg_info + i;
                break;
            }
        }
    }

    if (NULL != cis_dbg_info_to_record)
    {
        cis_dbg_info_to_record->conidx = conidx;
        cis_dbg_info_to_record->status = cis_info->status;
        cis_dbg_info_to_record->cis_con_hdl = cis_info->conhdl;
        cis_dbg_info_to_record->cig_sync_delay = cis_info->cig_sync_delay;
        cis_dbg_info_to_record->cis_sync_delay = cis_info->cis_sync_delay;
        cis_dbg_info_to_record->trans_latency_m2s = cis_info->trans_latency_m2s;
        cis_dbg_info_to_record->trans_latency_s2m = cis_info->trans_latency_s2m;
        cis_dbg_info_to_record->phy_m2s = cis_info->phy_m2s;
        cis_dbg_info_to_record->phy_s2m = cis_info->phy_s2m;
        cis_dbg_info_to_record->nse = cis_info->nse;
        cis_dbg_info_to_record->bn_m2s = cis_info->bn_m2s;
        cis_dbg_info_to_record->bn_s2m = cis_info->bn_s2m;
        cis_dbg_info_to_record->ft_m2s = cis_info->ft_m2s;
        cis_dbg_info_to_record->ft_s2m = cis_info->ft_s2m;
        cis_dbg_info_to_record->max_pdu_m2s = cis_info->max_pdu_m2s;
        cis_dbg_info_to_record->max_pdu_s2m = cis_info->max_pdu_s2m;
        cis_dbg_info_to_record->iso_interval = cis_info->iso_interval;
    }
}

void app_dbg_ble_audio_cis_dbg_info_status_updated_when_con_dis(uint8_t conidx, uint8_t dis_reason)
{
    app_dbg_ble_audio_cis_dbg_info_t* total_cis_dbg_info = app_dbg_ble_audio_total_cis_dbg_info_get();

    for (uint8_t i = 0;i < BLE_CONNECTION_MAX;i++)
    {
        if (conidx == total_cis_dbg_info[i].conidx)
        {
            total_cis_dbg_info[i].status = dis_reason;
            break;
        }
    }
}

static void app_dbg_ble_audio_register_plc_packet_occurs_cb(gaf_stream_common_plc_packet_occurs_cb func)
{
#ifdef GAF_DECODER_CROSS_CORE_USE_M55
    M55_gaf_stream_common_register_stream_plc_packet_occurs_cb(func);
#else
    gaf_stream_common_register_stream_plc_packet_occurs_cb(func);
#endif
}

/// cis heavy chopping info
static app_dbg_cis_heavy_chopping_info_t app_dbg_cis_heavy_chopping_param_info;
app_dbg_cis_heavy_chopping_info_t* app_dbg_get_cis_chopping_info(void)
{
    return &app_dbg_cis_heavy_chopping_param_info;
}

static void app_dbg_record_cis_heavy_chopping_info(uint8_t *pbuf, uint32_t length)
{
    app_dbg_cis_heavy_chopping_info_t *info = (app_dbg_cis_heavy_chopping_info_t *)pbuf;
    if (info)
    {
        memcpy(&app_dbg_cis_heavy_chopping_param_info, info, sizeof(app_dbg_cis_heavy_chopping_info_t));
    }
}

static app_dbg_cis_heavy_chopping_info_cb app_dbg_cis_chopping_info_cb = NULL;
void app_dbg_ble_register_cis_chopping_info_cb(app_dbg_cis_heavy_chopping_info_cb func)
{
     app_dbg_cis_chopping_info_cb= func;
}

app_dbg_cis_heavy_chopping_info_cb app_dbg_ble_get_cis_chopping_info_rcv_cb(void)
{
    return app_dbg_cis_chopping_info_cb;
}

/// le link loss info
static app_dbg_le_link_loss_info_t app_dbg_le_link_loss_info;
app_dbg_le_link_loss_info_t* app_dbg_get_le_link_loss_info(void)
{
    return &app_dbg_le_link_loss_info;
}

static void app_dbg_record_le_link_loss_info(uint8_t *pbuf, uint32_t length)
{
    app_dbg_le_link_loss_info_t *info = (app_dbg_le_link_loss_info_t *)pbuf;
    if (info)
    {
        memcpy(&app_dbg_le_link_loss_info, info, sizeof(app_dbg_le_link_loss_info_t));
    }
}

static app_dbg_le_link_loss_info_cb app_dbg_le_link_loss_info_callback = NULL;
void app_dbg_ble_register_le_link_loss_info_cb(app_dbg_le_link_loss_info_cb func)
{
     app_dbg_le_link_loss_info_callback= func;
}

app_dbg_le_link_loss_info_cb app_dbg_ble_get_le_link_loss_info_rcv_cb(void)
{
    return app_dbg_le_link_loss_info_callback;
}

/// total plc info
static app_dbg_ble_audio_stream_total_plc_info_t app_dbg_ble_audio_total_plc_info;
app_dbg_ble_audio_stream_total_plc_info_t* app_dbg_ble_audio_get_total_plc_info(void)
{
    return &app_dbg_ble_audio_total_plc_info;
}

app_dbg_ble_audio_stream_plc_packet_info_t* app_dbg_ble_audio_get_now_interval_plc_info(void)
{
    return &app_dbg_ble_audio_total_plc_info.interval_plc_info[0];
}

static void app_dbg_ble_audio_stream_plc_packet_info_record(uint16_t pkt_seq_nb,
    GAF_ISO_PKT_STATUS_E pkt_status, uint32_t time_stamp, uint32_t dmaIrqHappenTime)
{
    TRACE(0,"%s done",__func__);
    app_dbg_ble_audio_stream_packet_info_t new_pcl_packet;
    new_pcl_packet.dmaIrqHappenTime = dmaIrqHappenTime;
    new_pcl_packet.pkt_seq_nb = pkt_seq_nb;
    new_pcl_packet.pkt_status = pkt_status;
    new_pcl_packet.time_stamp = time_stamp;

    app_dbg_ble_audio_stream_plc_packet_info_t *now_interval_plc_info = app_dbg_ble_audio_get_now_interval_plc_info();
    now_interval_plc_info->numPlcHaveHappened++;

    if (now_interval_plc_info->numPlcHaveStored < APP_DBG_PLC_INFO_NUM_IN_ONE_INTERVAL)
    {
        uint8_t index_to_store = now_interval_plc_info->numPlcHaveStored;
        memcpy(&now_interval_plc_info->plc_packet_info[index_to_store], &new_pcl_packet, sizeof(new_pcl_packet));
        now_interval_plc_info->numPlcHaveStored++;
    }
    else
    {
        uint8_t plc_packet_to_move = APP_DBG_PLC_INFO_NUM_IN_ONE_INTERVAL - 1;
        app_dbg_ble_audio_stream_packet_info_t copy_plc_packet[APP_DBG_PLC_INFO_NUM_IN_ONE_INTERVAL];
        memcpy(copy_plc_packet, &now_interval_plc_info->plc_packet_info[0], sizeof(copy_plc_packet));
        memcpy(&now_interval_plc_info->plc_packet_info[0], &copy_plc_packet[1],
                plc_packet_to_move * sizeof(app_dbg_ble_audio_stream_packet_info_t));
        memcpy(&now_interval_plc_info->plc_packet_info[plc_packet_to_move], &new_pcl_packet, sizeof(new_pcl_packet));
    }
}

static void app_dbg_ble_audio_update_total_plc_info(uint8_t con_lid, uint32_t dmaChunkIntervalUs)
{
    TRACE(0," %s done",__func__);
    static uint32_t one_interval_timer_start = 0;
    app_dbg_ble_audio_stream_total_plc_info_t* total_plc_info = app_dbg_ble_audio_get_total_plc_info();
    total_plc_info->conidx = con_lid;
    if (one_interval_timer_start >= APP_DBG_PLC_INTERVAL_SIZE_IN_US)
    {
        total_plc_info->numIntervalsPlcHaveStored += ((total_plc_info->numIntervalsPlcHaveStored ==
                                                        APP_DBG_PLC_INFO_STORED_INTERVAL_NUM) ? 0 : 1);
        uint8_t interval_to_move = ((total_plc_info->numIntervalsPlcHaveStored == APP_DBG_PLC_INFO_STORED_INTERVAL_NUM) ?
                                   (APP_DBG_PLC_INFO_STORED_INTERVAL_NUM - 1) : total_plc_info->numIntervalsPlcHaveStored);
        app_dbg_ble_audio_stream_plc_packet_info_t copy_plc_info[APP_DBG_PLC_INFO_STORED_INTERVAL_NUM];
        memcpy(copy_plc_info, &total_plc_info->interval_plc_info[0], sizeof(copy_plc_info));
        memcpy(&total_plc_info->interval_plc_info[1], copy_plc_info, (interval_to_move * sizeof(app_dbg_ble_audio_stream_plc_packet_info_t)));
        memset(&total_plc_info->interval_plc_info[0], 0x00, sizeof(app_dbg_ble_audio_stream_plc_packet_info_t));
        one_interval_timer_start = 0;
    }
    else
    {
        one_interval_timer_start += dmaChunkIntervalUs;
    }
}

static void app_dbg_ble_audio_register_stream_get_packet_cb(gaf_stream_common_get_packet_cb func)
{
#ifdef GAF_DECODER_CROSS_CORE_USE_M55
    M55_gaf_stream_common_register_stream_get_packet_cb(func);
#else
    gaf_stream_common_register_stream_get_packet_cb(func);
#endif
}

static app_dbg_ble_audio_stream_latest_packets_info_t app_dbg_ble_audio_latest_stream_packets_info;
app_dbg_ble_audio_stream_latest_packets_info_t* app_dbg_ble_audio_get_latest_packets_info(void)
{
    return &app_dbg_ble_audio_latest_stream_packets_info;
}

static void app_dbg_ble_audio_record_latest_stream_packets_info(uint8_t conidx, uint16_t pkt_seq_nb,
    GAF_ISO_PKT_STATUS_E pkt_status, uint32_t time_stamp, uint32_t dmaIrqHappenTime)
{
    TRACE(0,"%s done",__func__);
    app_dbg_ble_audio_stream_packet_info_t new_packet_info;
    new_packet_info.pkt_seq_nb = pkt_seq_nb;
    new_packet_info.pkt_status = pkt_status;
    new_packet_info.time_stamp = time_stamp;
    new_packet_info.dmaIrqHappenTime = dmaIrqHappenTime;

    app_dbg_ble_audio_stream_latest_packets_info_t* latest_info = app_dbg_ble_audio_get_latest_packets_info();
    latest_info->conidx = conidx;
    uint8_t numPacketsHaveStored = latest_info->latestPacketsNumHaveStored;
    if (numPacketsHaveStored < APP_DBG_LATEST_PACKET_INFO_NUMBER)
    {
        uint8_t index_to_store = numPacketsHaveStored;
        memcpy(&latest_info->latest_packets[index_to_store], &new_packet_info, sizeof(new_packet_info));
        latest_info->latestPacketsNumHaveStored++;
    }
    else
    {
        uint8_t packet_to_move = numPacketsHaveStored - 1;
        app_dbg_ble_audio_stream_packet_info_t copy_latest_info[APP_DBG_LATEST_PACKET_INFO_NUMBER];
        memcpy(copy_latest_info, latest_info->latest_packets, sizeof(copy_latest_info));
        memcpy(&latest_info->latest_packets[0], &copy_latest_info[1],
            packet_to_move * sizeof(app_dbg_ble_audio_stream_packet_info_t));
        memcpy(&latest_info->latest_packets[packet_to_move], &new_packet_info, sizeof(new_packet_info));
    }
}

static void app_dbg_ble_audio_register_dma_irq_happens_cb(gaf_stream_common_dma_irq_happens_cb func)
{
#ifdef GAF_DECODER_CROSS_CORE_USE_M55
    M55_gaf_stream_common_register_dma_irq_happens_cb(func);
#else
    gaf_stream_common_register_dma_irq_happens_cb(func);
#endif
}

static void app_dbg_ble_audio_register_capture_dma_irq_cb(gaf_stream_dma_irq_cb func)
{
    gaf_stream_capture_register_dma_irq_cb(func);
}

static app_dbg_ble_call_capture_dma_info_t app_dbg_ble_call_capture_dma_info;
app_dbg_ble_call_capture_dma_info_t* app_dbg_ble_get_call_capture_dma_irq_info(void)
{
    return &app_dbg_ble_call_capture_dma_info;
}

static void app_dbg_ble_call_capture_dma_irq_info_record(uint8_t conidx,
    uint32_t dmaIrqHappeningTimeUs, uint16_t seq_num)
{
    TRACE(0,"%sdone",__func__);
    app_dbg_ble_call_capture_dma_info_t *total_dma_info = app_dbg_ble_get_call_capture_dma_irq_info();
    total_dma_info->conidx = conidx;
    if (total_dma_info->infoHaveBeenSavedNum < APP_DBG_CALL_CAPTURE_DMA_INFO_NUM)
    {
        uint8_t index_to_save = total_dma_info->infoHaveBeenSavedNum;
        total_dma_info->packet_dma_info[index_to_save].dmaIrqHappeningTimeUs = dmaIrqHappeningTimeUs;
        total_dma_info->packet_dma_info[index_to_save].seq_num = seq_num;
        total_dma_info->infoHaveBeenSavedNum++;
    }
    else
    {
        uint8_t num_to_move = APP_DBG_CALL_CAPTURE_DMA_INFO_NUM - 1;
        app_dbg_ble_call_capture_packet_dma_info_t copy_dma_info[APP_DBG_CALL_CAPTURE_DMA_INFO_NUM];
        memcpy(&copy_dma_info[0], &total_dma_info->packet_dma_info[0], sizeof(copy_dma_info));
        memcpy(&total_dma_info->packet_dma_info[0], &copy_dma_info[1],
                num_to_move * sizeof(app_dbg_ble_call_capture_packet_dma_info_t));
        total_dma_info->packet_dma_info[num_to_move].dmaIrqHappeningTimeUs = dmaIrqHappeningTimeUs;
        total_dma_info->packet_dma_info[num_to_move].seq_num = seq_num;
    }
}

static app_dbg_ble_audio_stream_info_t app_dbg_stream_param_info;
app_dbg_ble_audio_stream_info_t* app_dbg_ble_get_audio_stream_info(void)
{
    uint8_t con_lid = app_dbg_ble_audio_latest_stream_packets_info.conidx;
    GAF_AUDIO_STREAM_ENV_T* stream_env = gaf_audio_get_stream_env_from_con_lid(con_lid);
    if (NULL != stream_env)
    {
        app_gaf_arc_vcs_volume_ind_t* p_volume_info = aob_volume_info_get();
        app_dbg_stream_param_info.volume = p_volume_info->volume;
        app_dbg_stream_param_info.mute = p_volume_info->mute;
        app_dbg_stream_param_info.conidx = stream_env->stream_info.con_lid;
        app_dbg_stream_param_info.contextType = stream_env->stream_info.contextType;
        app_dbg_stream_param_info.palyback_info.trigger_strat_ticks = stream_env->stream_context.playbackTriggerStartTicks;
        app_dbg_stream_param_info.palyback_info.sample_rate = stream_env->stream_info.playbackInfo.sample_rate;
        app_dbg_stream_param_info.palyback_info.num_channels = stream_env->stream_info.playbackInfo.num_channels;
        app_dbg_stream_param_info.palyback_info.bits_depth = stream_env->stream_info.playbackInfo.bits_depth;
        app_dbg_stream_param_info.palyback_info.frame_ms = stream_env->stream_info.playbackInfo.frame_ms;
        app_dbg_stream_param_info.palyback_info.encoded_frame_size = stream_env->stream_info.playbackInfo.encoded_frame_size;
        app_dbg_stream_param_info.capture_info.trigger_strat_ticks = stream_env->stream_context.captureTriggerStartTicks;
        app_dbg_stream_param_info.capture_info.sample_rate = stream_env->stream_info.captureInfo.sample_rate;
        app_dbg_stream_param_info.capture_info.num_channels = stream_env->stream_info.captureInfo.num_channels;
        app_dbg_stream_param_info.capture_info.bits_depth = stream_env->stream_info.captureInfo.bits_depth;
        app_dbg_stream_param_info.capture_info.frame_ms = stream_env->stream_info.captureInfo.frame_ms;
        app_dbg_stream_param_info.capture_info.encoded_frame_size = stream_env->stream_info.captureInfo.encoded_frame_size;
    }

    return &app_dbg_stream_param_info;
}

static void app_dbg_ble_audio_register_tx_sync_info_rcv_cb(app_bap_uc_srv_iso_tx_sync_info_rcv_cb func)
{
    app_bap_uc_srv_register_iso_tx_sync_info_record_cb(func);
}

static app_dbg_ble_audio_iso_tx_sync_info_t app_dbg_ble_iso_tx_sync_info;
app_dbg_ble_audio_iso_tx_sync_info_t* app_dbg_ble_get_iso_tx_sync_info(void)
{
    return &app_dbg_ble_iso_tx_sync_info;
}

static void app_dbg_ble_audio_iso_tx_sync_info_record(uint8_t status, uint16_t con_hdl, uint16_t packet_seq_num,
                                                                        uint32_t tx_time_stamp, uint32_t time_offset)
{
    if (GAP_ERR_NO_ERROR != status)
    {
        LOG_W("%s fail status = %d", __func__, status);
        return;
    }
    TRACE(0,"%s done",__func__);
    uint8_t conidx = gapc_get_conidx(con_hdl);
    app_dbg_ble_audio_iso_tx_sync_info_t* dbg_iso_tx_sync_info = app_dbg_ble_get_iso_tx_sync_info();
    dbg_iso_tx_sync_info->status = status;
    dbg_iso_tx_sync_info->conidx = conidx;
    dbg_iso_tx_sync_info->packet_seq_num = packet_seq_num;
    dbg_iso_tx_sync_info->tx_time_stamp = tx_time_stamp;
    dbg_iso_tx_sync_info->time_offset = time_offset;
}

static void app_dbg_ble_register_iso_link_quality_dbg_info_record(app_bap_uc_srv_iso_quality_info_rcv_cb func)
{
    app_bap_uc_srv_register_iso_quality_info_rcv_cb(func);
}

static app_dbg_ble_iso_link_quality_info_t app_dbg_iso_link_quality_info;
app_dbg_ble_iso_link_quality_info_t* app_dbg_ble_get_iso_link_quality_info(void)
{
    return &app_dbg_iso_link_quality_info;
}

static void app_dbg_ble_iso_link_quality_dbg_info_record(uint16_t status, uint8_t ase_lid,
                                                            uint32_t tx_unacked_packets, uint32_t tx_flushed_packets,
                                                            uint32_t tx_last_subevent_packets, uint32_t retx_packets,
                                                            uint32_t crc_error_packets, uint32_t rx_unrx_packets,
                                                            uint32_t duplicate_packets)
{
    if (GAP_ERR_NO_ERROR != status)
    {
        LOG_W("%s fail status = %d", __func__, status);
        return;
    }
    TRACE(0,"%s done",__func__);
    app_dbg_ble_iso_link_quality_info_t *iso_link_quality_info = app_dbg_ble_get_iso_link_quality_info();
    iso_link_quality_info->status = status;
    iso_link_quality_info->ase_lid = ase_lid;
    iso_link_quality_info->tx_unacked_packets = tx_unacked_packets;
    iso_link_quality_info->tx_flushed_packets = tx_flushed_packets;
    iso_link_quality_info->tx_last_subevent_packets = tx_last_subevent_packets;
    iso_link_quality_info->retx_packets = retx_packets;
    iso_link_quality_info->crc_error_packets = crc_error_packets;
    iso_link_quality_info->rx_unrx_packets = rx_unrx_packets;
    iso_link_quality_info->duplicate_packets = duplicate_packets;
}

void app_dbg_ble_audio_info_init(void)
{
    app_dbg_ble_audio_cis_dbg_info_t* total_cis_dbg_info = app_dbg_ble_audio_total_cis_dbg_info_get();
    for (uint8_t i = 0;i < IAP_NB_STREAMS;i++)
    {
        total_cis_dbg_info[i].conidx = GAP_INVALID_CONIDX;
    }

    app_dbg_ble_audio_register_cis_info_record_cb(app_dbg_ble_audio_cis_dbg_info_record);
    app_dbg_ble_audio_register_stream_get_packet_cb(app_dbg_ble_audio_record_latest_stream_packets_info);
    app_dbg_ble_audio_register_dma_irq_happens_cb(app_dbg_ble_audio_update_total_plc_info);
    app_dbg_ble_audio_register_plc_packet_occurs_cb(app_dbg_ble_audio_stream_plc_packet_info_record);
    app_dbg_ble_register_cis_chopping_info_cb(app_dbg_record_cis_heavy_chopping_info);
    app_dbg_ble_register_le_link_loss_info_cb(app_dbg_record_le_link_loss_info);
    app_dbg_ble_audio_register_capture_dma_irq_cb(app_dbg_ble_call_capture_dma_irq_info_record);
    app_dbg_ble_audio_register_tx_sync_info_rcv_cb(app_dbg_ble_audio_iso_tx_sync_info_record);
    app_dbg_ble_register_iso_link_quality_dbg_info_record(app_dbg_ble_iso_link_quality_dbg_info_record);
}

static app_ble_dbg_agc_rssi_info_t app_ble_agc_info;
uint8_t* app_dbg_trc_vs_le_agc_info_get(void)
{
    return (uint8_t*)&app_ble_agc_info;
}

void app_dbg_trc_vs_le_agc_info_record(uint8_t *p_buf, uint16_t buf_len)
{
    uint8_t rcv_group_cnt = ((app_ble_dbg_agc_rssi_info_t*)p_buf)->group_cnt;
    uint8_t *rcv_agc_rssi = (uint8_t*)(((app_ble_dbg_agc_rssi_info_t*)p_buf)->agc_rssi);
    app_ble_dbg_agc_rssi_info_t *agc_info = (app_ble_dbg_agc_rssi_info_t*)app_dbg_trc_vs_le_agc_info_get();
    uint16_t now_group_cnt = agc_info->group_cnt;
    uint16_t total_group_cnt = rcv_group_cnt + now_group_cnt;

    if (total_group_cnt <= APP_TRC_VS_LE_MAX_AGC)
    {
        memcpy(&agc_info->agc_rssi[now_group_cnt], rcv_agc_rssi, rcv_group_cnt * sizeof(ble_dbg_vs_agc_rssi_t));
        agc_info->group_cnt = total_group_cnt;
    }
    else
    {
        if (rcv_group_cnt >= APP_TRC_VS_LE_MAX_AGC)
        {
            memcpy(&agc_info->agc_rssi[0], rcv_agc_rssi, APP_TRC_VS_LE_MAX_AGC * sizeof(ble_dbg_vs_agc_rssi_t));
            agc_info->group_cnt = APP_TRC_VS_LE_MAX_AGC;
        }
        else
        {
            ble_dbg_vs_agc_rssi_t agc_copy_buf[APP_TRC_VS_LE_MAX_AGC] = {0};
            memcpy(&agc_info->agc_rssi[0], agc_copy_buf, APP_TRC_VS_LE_MAX_AGC * sizeof(ble_dbg_vs_agc_rssi_t));

            uint16_t agc_moved_copy_size = (APP_TRC_VS_LE_MAX_AGC - rcv_group_cnt) * sizeof(ble_dbg_vs_agc_rssi_t);
            memcpy(&agc_info->agc_rssi[rcv_group_cnt], agc_copy_buf, agc_moved_copy_size);

            memcpy(&agc_info->agc_rssi[0], rcv_agc_rssi, rcv_group_cnt * sizeof(ble_dbg_vs_agc_rssi_t));
            agc_info->group_cnt = APP_TRC_VS_LE_MAX_AGC;
        }
    }

}

void app_dbg_dump_ble_info(void)
{
    ///LE channel map info print
    app_dbg_ble_channel_map_total_info_t* all_con_channel_map = app_ble_dbg_get_all_con_channel_map_info();
    for (uint8_t i =0;i < BLE_CONNECTION_MAX;i++)
    {
        LOG_I("LE-con_hdl=0x%x Instant=%d Channel Map:",
            all_con_channel_map[i].conidx, all_con_channel_map[i].channel_map_info[0].instant);
        DUMP8("%02x ", all_con_channel_map[i].channel_map_info[0].ChM, 5);
    }

    /// LE Audio PLC packet info

    /// To get stream configuration
    app_dbg_ble_audio_stream_info_t*  ble_audio_stream_info= app_dbg_ble_get_audio_stream_info();
    if (NULL == ble_audio_stream_info)
    {
        LOG_W("LEA-INFO fail no ble_audio_stream_info !!!");
        return;
    }
}

#endif   // IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
