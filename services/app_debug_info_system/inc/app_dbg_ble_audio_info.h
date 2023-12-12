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

#ifndef __APP_DBG_BLE_AUDIO_INFO__
#define __APP_DBG_BLE_AUFIO_INFO__

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED

#include "gapm_actv.h"
#include "gapc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "gaf_media_common.h"
#include "gaf_codec_cc_bth.h"

#ifdef __cplusplus
}
#endif

#define APP_DBG_PLC_INFO_STORED_INTERVAL_NUM   5
#define APP_DBG_PLC_INFO_NUM_IN_ONE_INTERVAL   10
#define APP_DBG_LATEST_PACKET_INFO_NUMBER   20
#define APP_DBG_PLC_INTERVAL_SIZE_IN_US   (60 * 1000 * 1000)   //one minute by default
#define APP_DBG_BLE_CHANNEL_MAP_INFO_NUM   5
#define APP_DBG_CALL_CAPTURE_DMA_INFO_NUM   10

typedef struct app_dbg_ble_con_info
{
    //0x00 Connection successfully completed,
    //0x01 to 0xFF Connection failed to Complete(See Controller Error Codes).
    uint8_t  status;
    ///connection handle
    uint8_t  conidx;
    ///peer address type
    uint8_t  peer_addr_type;
    ///peer address
    uint8_t  peer_addr[BD_ADDR_LEN];
    ///Connection interval
    uint16_t con_interval;
    ///Connection latency
    uint16_t con_latency;
    ///Link supervision timeout
    uint16_t sup_to;
} app_dbg_ble_con_info_t;

typedef struct app_dbg_ble_audio_cis_dbg_info
{
    /// connection local id.
    uint8_t conidx;
    /// 0x00 - The Broadcast Isochronous Group has been completed ; 0x01-0xFF Failed reason
    uint8_t  status;
    /// Connection handle of the Connected Isochronous Stream (Range: 0x0000-0x0EFF)
    uint16_t cis_con_hdl;
    /// The CIG synchronization delay time in microseconds
    uint32_t cig_sync_delay;
    /// The CIS synchronization delay time in microseconds
    uint32_t cis_sync_delay;
    /// The maximum time, in microseconds, for transmission of SDUs of all CISes from master to slave
    /// (range 0x0000EA to 0x7FFFFF)
    uint32_t trans_latency_m2s;
    /// The maximum time, in microseconds, for transmission of SDUs of all CISes from slave to master
    /// (range 0x0000EA to 0x7FFFFF)
    uint32_t trans_latency_s2m;
    /// Master to slave PHY, 0x01: 1Mbps, 0x02: 2Mbps, 0x03: LE-Coded
    uint8_t  phy_m2s;
    /// Slave to master PHY, 0x01: 1Mbps, 0x02: 2Mbps, 0x03: LE-Coded
    uint8_t  phy_s2m;
    /// Maximum number of subevents in each isochronous event (Range: 0x01-0x1E)
    uint8_t  nse;
    /// The burst number for master to slave transmission (0x00: no isochronous data from the master to the slave, range 0x01-0x0F)
    uint8_t  bn_m2s;
    /// The burst number for slave to master transmission (0x00: no isochronous data from the slave to the master, range 0x01-0x0F)
    uint8_t  bn_s2m;
    /// The flush timeout, in multiples of the ISO_Interval, for each payload sent from the master to the slave (Range: 0x01-0x1F)
    uint8_t  ft_m2s;
    /// The flush timeout, in multiples of the ISO_Interval, for each payload sent from the slave to the master (Range: 0x01-0x1F)
    uint8_t  ft_s2m;
    /// Maximum size, in octets, of the payload from master to slave (Range: 0x00-0xFB)
    uint16_t  max_pdu_m2s;
    /// Maximum size, in octets, of the payload from slave to master (Range: 0x00-0xFB)
    uint16_t  max_pdu_s2m;
    /// ISO interval (1.25ms unit, range: 5ms to 4s)
    uint16_t iso_interval;
} __attribute__ ((__packed__)) app_dbg_ble_audio_cis_dbg_info_t;

typedef struct
{
    /// Packet Sequence Number
    uint16_t pkt_seq_nb;
    GAF_ISO_PKT_STATUS_E pkt_status;
    uint32_t time_stamp;
    uint32_t dmaIrqHappenTime;
} __attribute__ ((__packed__)) app_dbg_ble_audio_stream_packet_info_t;

typedef struct
{
    uint32_t numPlcHaveHappened;
    uint8_t numPlcHaveStored;
    app_dbg_ble_audio_stream_packet_info_t plc_packet_info[APP_DBG_PLC_INFO_NUM_IN_ONE_INTERVAL];
} __attribute__ ((__packed__)) app_dbg_ble_audio_stream_plc_packet_info_t;

typedef struct
{
    uint8_t conidx;
    uint8_t numIntervalsPlcHaveStored;
    // interval_plc_info[0] store the now interval plc info.
    app_dbg_ble_audio_stream_plc_packet_info_t interval_plc_info[APP_DBG_PLC_INFO_STORED_INTERVAL_NUM];
} __attribute__ ((__packed__)) app_dbg_ble_audio_stream_total_plc_info_t;

typedef struct
{
    uint8_t conidx;
    uint8_t latestPacketsNumHaveStored;
    app_dbg_ble_audio_stream_packet_info_t latest_packets[APP_DBG_LATEST_PACKET_INFO_NUMBER];
} __attribute__ ((__packed__)) app_dbg_ble_audio_stream_latest_packets_info_t;

typedef struct
{
    uint16_t seq_num;
    uint32_t dmaIrqHappeningTimeUs;
} __attribute__ ((__packed__)) app_dbg_ble_call_capture_packet_dma_info_t;

typedef struct
{
    uint8_t conidx;
    uint8_t infoHaveBeenSavedNum;
    app_dbg_ble_call_capture_packet_dma_info_t packet_dma_info[APP_DBG_CALL_CAPTURE_DMA_INFO_NUM];
} __attribute__ ((__packed__)) app_dbg_ble_call_capture_dma_info_t;

typedef struct
{
    uint32_t trigger_strat_ticks;
    uint32_t sample_rate;
    uint8_t  num_channels;
    uint8_t  bits_depth;
    float    frame_ms;
    uint16_t encoded_frame_size;
} __attribute__ ((__packed__)) app_dbg_ble_capture_info_t;

typedef struct
{
    uint32_t trigger_strat_ticks;
    uint32_t sample_rate;
    uint8_t  num_channels;
    uint8_t  bits_depth;
    float    frame_ms;
    uint16_t encoded_frame_size;
} __attribute__ ((__packed__)) app_dbg_ble_playback_info_t;

typedef struct
{
    uint8_t conidx;
    uint8_t volume;
    uint8_t mute;   //1-means mute, 0-means unmute.
    GAF_AUDIO_STREAM_CONTEXT_TYPE_E contextType;
    app_dbg_ble_playback_info_t palyback_info;
    app_dbg_ble_capture_info_t capture_info;
} __attribute__ ((__packed__)) app_dbg_ble_audio_stream_info_t;

typedef struct app_dbg_ble_trc_llcp_rx_head {
    uint16_t con_hdl;
    uint8_t  length;
    uint8_t  llcp_opcode;
} __attribute__ ((__packed__)) app_dbg_ble_trc_llcp_rx_head_t;

typedef struct {
    uint8_t  ChM[5];
    uint16_t instant;
} __attribute__ ((__packed__)) app_dbg_ble_channel_map_info_t;

typedef struct {
    uint8_t conidx;
    uint8_t numInfoHaveSaved;
    app_dbg_ble_channel_map_info_t channel_map_info[APP_DBG_BLE_CHANNEL_MAP_INFO_NUM];
} __attribute__ ((__packed__)) app_dbg_ble_channel_map_total_info_t;

#define APP_TRC_VS_LE_MAX_AGC 0x32
typedef struct ble_dbg_vs_agc_rssi_t {
    uint16_t ll_evt_cnt;
    uint8_t  rx_gain;
    int8_t   rssi_dbm;
} __attribute__ ((__packed__)) ble_dbg_vs_agc_rssi_t;

typedef struct app_ble_dbg_agc_rssi_info {
    uint8_t group_cnt;   //It means how many group agc-rssi info has been stored.
    ble_dbg_vs_agc_rssi_t agc_rssi[APP_TRC_VS_LE_MAX_AGC];
} __attribute__ ((__packed__)) app_ble_dbg_agc_rssi_info_t;

typedef struct {
    /// 0x00 - The HCI_LE_Read_ISO_TX_Sync command succeeded ; 0x01-0xFF Failed reason
    uint8_t  status;
    /// Connection handle of the CIS or BIS (Range: 0x0000-0x0EFF)
    uint16_t conidx;
    ///packet sequence number
    uint16_t packet_seq_num;
    ///The CIG reference point or BIG anchor point of a transmitted SDU
    ///derived using the Controller's free running reference clock (in microseconds).
    uint32_t tx_time_stamp;
    ///The time offset, in microseconds, that is associated with a transmitted SDU.
    uint32_t time_offset;
} __attribute__ ((__packed__)) app_dbg_ble_audio_iso_tx_sync_info_t;

typedef struct
{
    ///Cmd return status
    uint16_t status;
    /// ASE local index
    uint8_t ase_lid;
    /// Number of packets transmitted and unacked
    uint32_t tx_unacked_packets;
    /// Number of flushed transmitted packets
    uint32_t tx_flushed_packets;
    /// Number of packets transmitted during last subevent
    uint32_t tx_last_subevent_packets;
    /// Number of retransmitted packets
    uint32_t retx_packets;
    /// Number of packets received with a CRC error
    uint32_t crc_error_packets;
    /// Number of unreceived packets
    uint32_t rx_unrx_packets;
    /// Number of duplicate packets received
    uint32_t duplicate_packets;
} __attribute__ ((__packed__)) app_dbg_ble_iso_link_quality_info_t;

typedef struct app_dbg_cis_heavy_chopping_info
{
    /// Status
    uint8_t     opcode;
    uint8_t     hand_id;
    uint8_t     status;
    uint8_t     conn_handle;
    uint16_t    evt_cnt;
    uint8_t     channel_map[LE_CHNL_MAP_LEN];
    uint8_t     tx_pwr;
    uint8_t     avg_rssi;
    uint32_t    last_crc_ok_ts;
} __attribute__ ((__packed__)) app_dbg_cis_heavy_chopping_info_t;

typedef struct app_dbg_le_link_loss_info{
    uint8_t  opcode;
    uint16_t hand_id;
    uint8_t  status;

    /*basic infromation*/
    uint16_t  conn_handle;
    uint8_t   lsto_tx_pwr;
    uint8_t   lsto_channel_map[5];
    uint16_t  lsto_value;
    uint16_t  conn_interval;
    uint16_t  subrate_factor;
    uint16_t  periph_latency;

    /*last sync end infromation*/
    uint8_t   head_sync;
    uint16_t  last_sync_evt_cnt;
    uint32_t  last_sync_matched_clk;
    uint8_t   last_sync_rssi;
    uint8_t   last_sync_channel;

    /*last tx end infromation*/
    uint16_t  last_succ_rx_evt_cnt;
    uint16_t  total_rx_cnt_rec;
    uint16_t  last_rx_evt_cnt_rec[25];
    uint8_t   last_rx_channel[25];

    /*ll msg infromation*/
    //last two tx end ll msg
    uint8_t   head_tx[2];
    uint8_t   tx_opcode[2];
    uint16_t  first_tx_evt_cnt[2];
    uint16_t  last_tx_evt_cnt[2];
    uint8_t   total_tx_cnt[2];
    uint16_t  tx_instant[2];

    /*ll msg infromation*/
    //last two rx end ll msg
    uint8_t   head_rx[2];
    uint8_t   rx_opcode[2];
    uint16_t  first_rx_evt_cnt[2];
    uint16_t  last_rx_evt_cnt[2];
    uint8_t   total_rx_cnt[2];
    uint16_t  rx_instant[2];
} __attribute__ ((__packed__)) app_dbg_le_link_loss_info_t;


#ifdef __cplusplus
extern "C" {
#endif
void app_dbg_ble_info_system_init(void);

app_dbg_ble_con_info_t* app_dbg_ble_get_all_con_info(void);

app_dbg_ble_con_info_t* app_ble_dbg_get_con_info_by_conidx(uint8_t conidx);

app_dbg_ble_audio_cis_dbg_info_t* app_dbg_ble_audio_total_cis_dbg_info_get(void);

app_dbg_ble_audio_cis_dbg_info_t* app_dbg_ble_audio_cis_dbg_info_get_from_conidx(uint8_t conidx);

void app_dbg_ble_audio_cis_dbg_info_status_updated_when_con_dis(uint8_t conidx, uint8_t dis_reason);

void app_dbg_ble_audio_info_init(void);

app_dbg_ble_channel_map_total_info_t* app_ble_dbg_get_all_con_channel_map_info(void);

app_dbg_ble_channel_map_total_info_t* app_ble_dbg_get_channel_map_info_through_conidx(uint8_t conidx);

app_dbg_ble_channel_map_info_t* app_ble_dbg_get_latest_channel_map_info_through_conidx(uint8_t conidx);

void app_ble_dbg_record_channel_map_info(uint16_t con_hdl, uint8_t *p_ChM_info, uint8_t info_len);

app_dbg_ble_audio_stream_total_plc_info_t* app_dbg_ble_audio_get_total_plc_info(void);

app_dbg_ble_audio_stream_plc_packet_info_t* app_dbg_ble_audio_get_now_interval_plc_info(void);

app_dbg_ble_audio_stream_latest_packets_info_t* app_dbg_ble_audio_get_latest_packets_info(void);

app_dbg_ble_call_capture_dma_info_t* app_dbg_ble_get_call_capture_dma_irq_info(void);

app_dbg_ble_audio_stream_info_t* app_dbg_ble_get_audio_stream_info(void);

uint8_t* app_dbg_trc_vs_le_agc_info_get(void);

void app_dbg_trc_vs_le_agc_info_record(uint8_t *p_buf, uint16_t buf_len);

app_dbg_ble_audio_iso_tx_sync_info_t* app_dbg_ble_get_iso_tx_sync_info(void);

app_dbg_cis_heavy_chopping_info_t* app_dbg_get_cis_chopping_info(void);

app_dbg_ble_iso_link_quality_info_t* app_dbg_ble_get_iso_link_quality_info(void);

typedef void (*app_dbg_cis_heavy_chopping_info_cb)(uint8_t* info, uint32_t info_len);

app_dbg_cis_heavy_chopping_info_cb app_dbg_ble_get_cis_chopping_info_rcv_cb(void);

typedef void (*app_dbg_le_link_loss_info_cb)(uint8_t* info, uint32_t info_len);

app_dbg_le_link_loss_info_cb app_dbg_ble_get_le_link_loss_info_rcv_cb(void);

void app_dbg_dump_ble_info(void);

#ifdef __cplusplus
}
#endif

#endif   //IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED

#endif   //__APP_DBG_BLE_AUDIO_INFO__
