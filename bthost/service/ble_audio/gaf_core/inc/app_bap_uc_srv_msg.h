/**
 ****************************************************************************************
 *
 * @file app_bap_uc_srv_msg.h
 *
 * @brief BLE Audio Audio Stream Control Service Server
 *
 * Copyright 2015-2019 BES.
 *
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
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP_BAP
 * @{
 ****************************************************************************************
 */

#ifndef APP_BAP_UC_SRV_MSG_H_
#define APP_BAP_UC_SRV_MSG_H_

#if BLE_AUDIO_ENABLED
#include "app_bap.h"
#include "co_list.h"

#include "app_gaf_define.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define APP_BAP_DFT_ASCS_NB_ASE_CHAR_SINK         (2)
#define APP_BAP_DFT_ASCS_NB_ASE_CHAR_SRC          (1)
#define APP_BAP_DFT_ASCS_NB_ASE_CHAR              (APP_BAP_DFT_ASCS_NB_ASE_CHAR_SINK + APP_BAP_DFT_ASCS_NB_ASE_CHAR_SRC)
#define APP_BAP_DFT_ASCS_NB_ASE_CFG               (APP_BAP_DFT_ASCS_NB_ASE_CHAR) * (BLE_CONNECTION_MAX)

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
#define APP_BAP_ASCS_NON_AUDIO_FRAMING_TYPE             APP_ISO_UNFRAMED_MODE
#define APP_BAP_ASCS_NON_AUDIO_RETRANSSMIT_NUM            3
#define APP_BAP_ASCS_NON_AUDIO_MIN_PRES_DELAY_US          10
#define APP_BAP_ASCS_NON_AUDIO_MAX_PRES_DELAY_US          10
#define APP_BAP_ASCS_NON_AUDIO_MAX_SDU_SIZE              (14)
#define APP_BAP_ASCS_NON_AUDIO_MAX_TRANS_LATENCY_MS       15
#endif

/*
 * ENUMERATE
 ****************************************************************************************
 */
/// ASE State values
enum app_bap_uc_ase_state
{
    /// Idle
    APP_BAP_UC_ASE_STATE_IDLE = 0,
    /// Codec configured
    APP_BAP_UC_ASE_STATE_CODEC_CONFIGURED,
    /// QoS configured
    APP_BAP_UC_ASE_STATE_QOS_CONFIGURED,
    /// Enabling
    APP_BAP_UC_ASE_STATE_ENABLING,
    /// Streaming
    APP_BAP_UC_ASE_STATE_STREAMING,
    /// Disabling
    APP_BAP_UC_ASE_STATE_DISABLING,
    /// Releasing
    APP_BAP_UC_ASE_STATE_RELEASING,

    APP_BAP_UC_ASE_STATE_MAX,
};

enum app_bap_uc_opcode
{
    /// Configure Codec
    APP_BAP_UC_OPCODE_CFG_CODEC = 1,
    APP_BAP_UC_OPCODE_MIN = APP_BAP_UC_OPCODE_CFG_CODEC,
    /// Configure QoS
    APP_BAP_UC_OPCODE_CFG_QOS,
    /// Enable
    APP_BAP_UC_OPCODE_ENABLE,
    /// Receiver Start Ready
    APP_BAP_UC_OPCODE_RX_START_READY,
    /// Disable
    APP_BAP_UC_OPCODE_DISABLE,
    /// Receiver Stop Ready
    APP_BAP_UC_OPCODE_RX_STOP_READY,
    /// Update Metadata
    APP_BAP_UC_OPCODE_UPDATE_METADATA,
    /// Release
    APP_BAP_UC_OPCODE_RELEASE,

    APP_BAP_UC_OPCODE_MAX,
};

/// ASCS ASE Structure
typedef struct app_bap_ascs_ase
{
    /// ASE local index
    uint8_t ase_lid;
    /// Connection local index
    uint8_t con_lid;
    /// ASE Instance local index
    uint8_t ase_instance_idx;
    /// ASE Direction
    enum app_gaf_direction direction;
    /// ASE State
    uint8_t ase_state;
    /// Codec ID
    app_gaf_codec_id_t codec_id;
    /// Pointer to Codec Configuration structure
    app_gaf_bap_cfg_t *p_cfg;
    /// QoS Requirements
    app_gaf_bap_qos_req_t qos_req;
    /// CIG ID
    uint8_t cig_id;
    /// CIS ID
    uint8_t cis_id;
    /// CIS Connection Handle
    uint16_t cis_hdl;
    /// QoS Configuration structure
    app_gaf_bap_qos_cfg_t qos_cfg;
    /// Pointer to Metadata structure
    app_gaf_bap_cfg_metadata_t *p_metadata;
    /// CIG sync delay in us
    uint32_t cig_sync_delay;
    /// CIS sync delay in us
    uint32_t cis_sync_delay;
    /// iso interval in us
    uint32_t iso_interval_us;
    /// bn count from master to slave
    uint32_t bn_m2s;
    /// bn count from slave to master
    uint32_t bn_s2m;
    /// context type bring by enable req
    uint16_t init_context_bf;
} app_bap_ascs_ase_t;

/// Audio Stream Control Service Server environment structure
typedef struct app_bap_ascs_env
{
    ///prefferred Mtu
    uint8_t preferred_mtu;
    /// Number of instances of the ASE characteristic sink
    /// Shall be in the range [0, 15]
    uint8_t nb_ase_chars_sink;
    /// Number of instances of the ASE characteristic src
    /// Shall be in the range [0, 15]
    uint8_t nb_ase_chars_src;
    /// Number of ASE configurations that can be maintained
    /// Shall be at least equal to nb_ase_chars
    /// Should be a multiple of nb_ase_chars
    /// Shall not be larger than nb_ase_chars * BLE_CONNECTION_MAX
    uint8_t nb_ases_cfg;
    /// ASCS ASE Information
    app_bap_ascs_ase_t *ase_info;

    /// For ACC discovery use
    uint8_t isFirstTimeAccStart[BLE_CONNECTION_MAX];

    /// test mode enable flag
    bool tm_enable;
}app_bap_ascs_env_t;

typedef struct
{
    /// Preferred SDU interval minimum in microseconds
    /// From 255us (0xFF) to 16777215us (0xFFFFFF)
    uint32_t sdu_intv_min_us;
    /// Preferred SDU interval maximum in microseconds
    /// From 255us (0xFF) to 16777215us (0xFFFFFF)
    uint32_t sdu_intv_max_us;
    /// Presentation delay minimum microseconds
    uint32_t pres_delay_min_us;
    /// Presentation delay maximum in microseconds
    uint32_t pres_delay_max_us;
    /// Minimum preferred presentation delay in microseconds
    uint32_t pref_pres_delay_min_us;
    /// Maximum preferred presentation delay in microseconds
    uint32_t pref_pres_delay_max_us;
    /// Preferred Transport latency maximum in milliseconds
    /// From 5ms (0x5) to 4000ms (0xFA0)
    uint16_t trans_latency_max_ms;
    /// Preferred maximum SDU size
    /// From 0 to 4095 bytes (0xFFF)
    uint16_t max_sdu_size;
} app_bap_uc_srv_cfg_info_t;

/// Structure for #bap_uc_srv_get_quality_cmp_evt_t
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
} __attribute__ ((__packed__)) app_bap_uc_srv_quality_rpt_evt_t;

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
typedef void (*app_bap_uc_srv_iso_quality_info_rcv_cb)(uint16_t status, uint8_t ase_lid,
                                                uint32_t tx_unacked_packets, uint32_t tx_flushed_packets,
                                                uint32_t tx_last_subevent_packets, uint32_t retx_packets,
                                                uint32_t crc_error_packets, uint32_t rx_unrx_packets,
                                                uint32_t duplicate_packets);

void app_bap_uc_srv_register_iso_quality_info_rcv_cb(app_bap_uc_srv_iso_quality_info_rcv_cb func);

typedef void(*app_bap_uc_srv_iso_tx_sync_info_rcv_cb)(uint8_t status, uint16_t con_hdl, uint16_t packet_seq_num,
                                                                        uint32_t tx_time_stamp, uint32_t time_offset);

void app_bap_uc_srv_register_iso_tx_sync_info_record_cb(app_bap_uc_srv_iso_tx_sync_info_rcv_cb func);
#endif   //#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED

typedef struct app_bap_uc_iso_tx_sync_dbg_info
{
    /// 0x00 - The HCI_LE_Read_ISO_TX_Sync command succeeded ; 0x01-0xFF Failed reason
    uint8_t  status;
    /// Connection handle of the CIS or BIS (Range: 0x0000-0x0EFF)
    uint16_t con_hdl;
    ///packet sequence number
    uint16_t packet_seq_num;
    ///The CIG reference point or BIG anchor point of a transmitted SDU
    ///derived using the Controller's free running reference clock (in microseconds).
    uint32_t tx_time_stamp;
    ///The time offset, in microseconds, that is associated with a transmitted SDU.
    uint32_t time_offset;
} __attribute__ ((__packed__)) app_bap_uc_iso_tx_sync_dbg_info_t;

typedef bool (*get_cfg_info_cb_func)(app_bap_uc_srv_cfg_info_t *info);

#ifdef __cplusplus
extern "C"{
#endif
uint32_t app_bap_uc_srv_cmp_evt_handler (void const *param);
uint32_t app_bap_uc_srv_rsp_handler (void const *param);
uint32_t app_bap_uc_srv_ind_handler (void const *param);
uint32_t app_bap_uc_srv_req_ind_handler (void const *param);
void app_bap_uc_srv_update_metadata_req(uint8_t ase_lid, app_gaf_bap_cfg_metadata_t* metadata);
void app_bap_uc_srv_iso_quality_ind_handler(uint16_t cisHdl, uint8_t *param);
void app_bap_uc_srv_restore_bond_data_req(uint8_t con_lid, uint8_t cli_cfg_bf, uint8_t ase_cli_cfg_bf);
void app_bap_uc_srv_store_codec_cfg_and_qos_req_for_ase(uint8_t ase_lid, app_gaf_bap_cfg_t *codec_cfg,
                                                        app_gaf_bap_qos_req_t *qos_req);
/**
 ****************************************************************************************
 * @brief configure codec
 *
 *
 * @param[in] ase_info          ase info
 *
 ****************************************************************************************
 */
void app_bap_uc_srv_configure_codec_req(app_bap_ascs_ase_t *ase_info, const app_gaf_bap_cfg_t * ntf_codec_cfg);


void app_bap_uc_srv_init(void);
void app_bap_uc_srv_info_init(void);

bool app_bap_uc_srv_acc_already_discovered(uint8_t con_lid);
void app_bap_uc_srv_set_acc_discovery_status(uint8_t con_lid,bool status);

#ifdef __cplusplus
}
#endif

#endif
#endif // APP_BAPS_MSG_H_

/// @} APP_BAP
