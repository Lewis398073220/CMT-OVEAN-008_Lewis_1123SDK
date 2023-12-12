/**
 ****************************************************************************************
 *
 * @file bap_uc_srv.h
 *
 * @brief Basic Audio Profile - Unicast Server - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_UC_SRV_H_
#define BAP_UC_SRV_H_

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV Unicast Server module
 * @ingroup BAP_UC
 * @brief Description of Unicast Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV_DEFINE Definitions
 * @ingroup BAP_UC_SRV
 * @brief Definitions for Unicast Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV_ENUM Enumerations
 * @ingroup BAP_UC_SRV
 * @brief Enumerations for Unicast Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV_NATIVE_API Native API
 * @ingroup BAP_UC_SRV
 * @brief Description of Native API for Unicast Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV_CALLBACK Callback Functions
 * @ingroup BAP_UC_SRV_NATIVE_API
 * @brief Description of callback functions for Unicast Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_UC_SRV_FUNCTION Functions
 * @ingroup BAP_UC_SRV_NATIVE_API
 * @brief Description of functions for Unicast Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"                // GAF Definitions
#include "iap.h"                // IAP Defines
#include "bap_uc.h"             // Basic Audio Profile - Unicast Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// @addtogroup BAP_UC_SRV_DEFINE
/// @{

/// Maximum number of ASE characteristics that can be supported
#define BAP_UC_SRV_NB_ASES_MAX      (15)

/// @} BAP_UC_SRV_DEFINE

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup BAP_UC_SRV_ENUM
/// @{

/// List of command types values for Unicast Server module
enum bap_uc_srv_cmd_type
{
    ///  Disable ASE
    BAP_UC_SRV_CMD_TYPE_DISABLE = 0,
    /// Release ASE
    BAP_UC_SRV_CMD_TYPE_RELEASE,
    /// Get Quality
    BAP_UC_SRV_CMD_TYPE_GET_QUALITY,
    /// Get ISO TX Sync
    BAP_UC_SRV_CMD_TYPE_GET_ISO_TX_SYNC,
};

/// Configuration bit field meaning for Unicast Server module
enum bap_uc_srv_cfg_bf
{
    /// Indicate if it is required (= 1) to check the Lock state
    /// Meaningful only if Coordinated Set Identification Profile Set Member Role is supported
    BAP_UC_SRV_CFG_CHECK_LOCK_POS = 0,
    BAP_UC_SRV_CFG_CHECK_LOCK_BIT = CO_BIT(BAP_UC_SRV_CFG_CHECK_LOCK_POS),
};

/// @} BAP_UC_SRV_ENUM

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup BAP_UC_SRV_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called each time a command has been completed
 *
 * @param[in] cmd_type      Command type (see #bap_uc_srv_cmd_type enumeration)
 * @param[in] status        Status
 * @param[in] ase_lid       ASE local index
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_cmp_evt)(uint8_t cmd_type, uint16_t status, uint8_t ase_lid);

/**
 ****************************************************************************************
 * @brief Callback function called each time BAP_UC_SRV_GET_QUALITY command has been completed
 *
 * @param[in] status                    Status
 * @param[in] ase_lid                   ASE local index
 * @param[in] tx_unacked_packets        Number of packets transmitted and unacked
 * @param[in] tx_flushed_packets        Number of flushed transmitted packets
 * @param[in] tx_last_subevent_packets  Number of packets transmitted during last subevent
 * @param[in] retransmitted_packets     Number of retransmitted packets
 * @param[in] crc_error_packets         Number of packets received with a CRC error
 * @param[in] rx_unreceived_packets     Number of unreceived packets
 * @param[in] duplicate_packets         Number of duplicate packet received
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_quality_cmp_evt)(uint16_t status, uint8_t ase_lid, uint32_t tx_unacked_packets,
                                              uint32_t tx_flushed_packets, uint32_t tx_last_subevent_packets,
                                              uint32_t retransmitted_packets, uint32_t crc_error_packets,
                                              uint32_t rx_unreceived_packets, uint32_t duplicate_packets);

typedef void (*bap_uc_srv_cb_iso_tx_sync_cmp_evt)(uint16_t status, uint8_t ase_lid, uint16_t con_hdl,
                                              uint16_t packet_seq_num, uint32_t tx_time_stamp, uint32_t time_offset);
/**
 ****************************************************************************************
 * @brief CIS rejected callback function
 *
 * @param[in] intf_lid          Interface local index
 * @param[in] con_hdl           Connection handle of Connected Isochronous Stream
 * @param[in] error             Reject reason
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_cis_rejected_evt)(uint16_t con_hdl, uint8_t error);

/**
 ****************************************************************************************
 * @brief CIG Terminated callback function
 *
 * @param[in] cig_id        CIG ID
 * @param[in] group_lid     Group local index
 * @param[in] stream_lid    Stream local index
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_cig_terminated_evt)(uint8_t cig_id, uint8_t group_lid, uint8_t stream_lid, uint8_t reason);

#ifdef BLE_STACK_PORTING_CHANGES
/**
 ****************************************************************************************
 * @brief Callback function called when send notification value for ASE Control operations
 *
 * @param[in] opcode         Opcode of the client-initiated ASE Control operation causing this response
 * @param[in] nb_ases        Total number of ASEs the server is providing a response for
 * @param[in] ase_lid        ASE Local index
 * @param[in] rsp_code       Response code
 * @param[in] reason         Reason
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_ase_ntf_value)(uint8_t opcode, uint8_t nb_ases, uint8_t ase_lid, uint8_t rsp_code, uint8_t reason);
#endif

/**
 ****************************************************************************************
 * @brief Callback function called when client configuration for Audio Stream Control Service has been updated
 *
 * @param[in] con_lid           Connection local index
  * @param[in] char_type         Char type
  * @param[in] cli_cfg_bf        Client configuration bit field
  * @param[in] ase_cli_cfg_bf    ASE Client configuration bit field
  ****************************************************************************************
  */
#ifdef BLE_STACK_PORTING_CHANGES
 typedef void (*bap_uc_srv_cb_bond_data)(uint8_t con_lid, uint8_t char_type, uint8_t cli_cfg_bf, uint16_t ase_cli_cfg_bf);
#else
 typedef void (*bap_uc_srv_cb_bond_data)(uint8_t con_lid, uint8_t cli_cfg_bf, uint16_t ase_cli_cfg_bf);
#endif

/**
 ****************************************************************************************
 * @brief Callback function called when state of an ASE has been updated
 *
 * @param[in] ase_lid           ASE local index
 * @param[in] con_lid           Connection local index
 *                              GAF_INVALID_LID indicates the ASE is no more bound with a connection
 * @param[in] state             State (see #bap_uc_ase_state enumeration)
 *                              Meaningful only is con_lid is not equal to GAF_INVALID_LID
 * @param[in] p_qos_cfg         Pointer to QoS Configuration
 *                              NULL if ASE state is at least QoS Configured
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_ase_state)(uint8_t ase_lid, uint8_t con_lid, uint8_t state,
                                        bap_qos_cfg_t* p_qos_cfg);

/**
 ****************************************************************************************
 * @brief Callback function called when state of a CIS has been updated
 *
 * @param[in] stream_lid    Stream local index
 * @param[in] con_lid       Connection local index
 * @param[in] ase_lid_sink  ASE local index for Sink direction
 * @param[in] ase_lid_src   ASE local index for Source direction
 * @param[in] cig_id        CIG ID
 * @param[in] cis_id        CIS ID
 * @param[in] conhdl        Connection handle
 *                          conhdl == GAP_INVALID_CONHDL indicates that CIS is disconnected
 *                          conhdl != GAP_INVALID_CONHDL indicates that CIS is connected
 * @param[in] p_cig_cfg     Pointer to CIG parameters
 *                          NULL if conhdl == GAP_INVALID_CONHDL
 * @param[in] p_cis_cfg     Pointer to CIS parameters
 *                          NULL if conhdl == GAP_INVALID_CONHDL
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_cis_state)(uint8_t stream_lid, uint8_t con_lid, uint8_t ase_lid_sink,
                                        uint8_t ase_lid_src, uint8_t cig_id, uint8_t cis_id, uint16_t conhdl,
                                        iap_ug_config_t* p_cig_cfg, iap_us_config_t* p_cis_cfg,
                                        uint8_t status, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Callback function called when a client device successfully write ASE Control Point characteristic with
 * Configure Codec operation
 *
 * @param[in] con_lid           Connection local index
 * @param[in] ase_instance_idx  ASE instance index
 * @param[in] ase_lid           ASE local index
 * @param[in] tgt_latency       Target Latency (see #bap_uc_tgt_latency enumeration)
 * @param[in] tgt_phy           Target PHY (see #bap_uc_tgt_phy enumeration)
 * @param[in] p_codec_id        Pointer to Codec ID
 * @param[in] p_cfg             Pointer to Codec Configuration structure
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_configure_codec_req)(uint8_t con_lid, uint8_t ase_instance_idx, uint8_t ase_lid,
                                                  uint8_t tgt_latency, uint8_t tgt_phy,
                                                  gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg);

/**
 ****************************************************************************************
 * @brief Callback function called when a client device successfully write ASE Control Point characteristic with
 * Enable operation
 *
 * @param[in] ase_lid           ASE local index
 * @param[in] p_metadata        Pointer to Metadata structure
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_enable_req)(uint8_t ase_lid, bap_cfg_metadata_ptr_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Callback function called when a client device successfully write ASE Control Point characteristic with
 * Update Metadata operation
 *
 * @param[in] ase_lid           ASE local index
 * @param[in] p_metadata        Pointer to Metadata structure
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_update_metadata_req)(uint8_t ase_lid, bap_cfg_metadata_ptr_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Callback function called when a client device successfully write ASE Control Point characteristic with
 * Release operation
 *
 * @param[in] ase_lid           ASE local index
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_release_req)(uint8_t ase_lid);

/**
 ****************************************************************************************
 * @brief Callback function called when data path is about to be started or stopped for an ASE
 *
 * @param[in] ase_lid       ASE local index
 * @param[in] start         Indicate if Data Path is about to be started or stopped for the ASE
 ****************************************************************************************
 */
typedef void (*bap_uc_srv_cb_dp_update_req)(uint8_t ase_lid, bool start);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Audio Stream Control Service Server
typedef struct bap_uc_srv_cb
{
    /// Callback function called each time a command has been completed
    bap_uc_srv_cb_cmp_evt cb_cmp_evt;
    /// Callback function called each time BAP_UC_SRV_GET_QUALITY command has been completed
    bap_uc_srv_cb_quality_cmp_evt cb_quality_cmp_evt;
    /// Callback function called each time when LE Read ISO TX Sync command cmp event comes
    bap_uc_srv_cb_iso_tx_sync_cmp_evt cb_iso_tx_sync_cmp_evt;
    /// Callback function called when client configuration for Audio Stream Control Service has been updated
    bap_uc_srv_cb_bond_data cb_bond_data;
    /// Callback function called when state of an ASE has been updated
    bap_uc_srv_cb_ase_state cb_ase_state;
    /// Callback function called when state of a CIS has been updated
    bap_uc_srv_cb_cis_state cb_cis_state;
    /// Callback function called when a client device successfully write ASE Control Point characteristic with
    /// Configure Codec operation
    bap_uc_srv_cb_configure_codec_req cb_configure_codec_req;
    /// Callback function called when a client device successfully write ASE Control Point characteristic with
    /// Enable operation
    bap_uc_srv_cb_enable_req cb_enable_req;
    /// Callback function called when a client device successfully write ASE Control Point characteristic with
    /// Update Metadata operation
    bap_uc_srv_cb_update_metadata_req cb_update_metadata_req;
    /// Callback function called when a client device successfully write ASE Control Point characteristic with
    /// Release operation
    bap_uc_srv_cb_release_req cb_release_req;
    /// Callback function called when data path is about to be started or stopped for an ASE
    bap_uc_srv_cb_dp_update_req cb_dp_update_req;
    /// Callback function called when CIS rejected
    bap_uc_srv_cb_cis_rejected_evt cb_cis_rejected_evt;
    /// Callback function called when CIG terminated
    bap_uc_srv_cb_cig_terminated_evt cb_cig_terminated_evt;
#ifdef BLE_STACK_PORTING_CHANGES
    /// Callback function called when send notification value for ASE Control operations
    bap_uc_srv_cb_ase_ntf_value cb_ase_ntf_value;
#endif
} bap_uc_srv_cb_t;

/// @} BAP_UC_SRV_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if (GAF_BAP_UC_SRV)
/// @addtogroup BAP_UC_SRV_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Set bonding information related to Audio Stream Control Service after connection with a peer device
 *
 * @param[in] con_lid           Connection local index
 * @param[in] cli_cfg_bf        Client configuration bit field for Audio Stream Control Service
 *                              Each bit correspond to a characteristic in the range [0, BAP_UC_CHAR_TYPE_ASE[
 * @param[in] ase_cli_cfg_bf    Client configuration bit field for instances of the ASE characteristics
 *                              Each bit correspond to an instance of the ASE characteristic
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_restore_bond_data(uint8_t con_lid, uint8_t cli_cfg_bf, uint16_t ase_cli_cfg_bf);

/**
 ****************************************************************************************
 * @brief Set stored Codec Configuration for an ASE after connection with a peer device
 *
 * @param[in] con_lid           Connection local index
 * @param[in] ase_lid           ASE local index
 * @param[in] ase_instance_idx  ASE instance index
 * @param[in] p_codec_id        Pointer to Codec ID
 * @param[in] p_qos_req         Pointer to QoS Requirements structure
 * @param[in] ctl_delay_us      Controller delay in microseconds
 * @param[in] dp_id             Data Path ID
 * @param[in] p_cfg             Pointer to Codec Configuration structure (allocated by Upper Layer)\n
 *                                  - Can be NULL
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_restore_bond_data_codec(uint8_t con_lid, uint8_t ase_lid, uint8_t ase_instance_idx,
                                            gaf_codec_id_t* p_codec_id, bap_qos_req_t* p_qos_req,
                                            uint32_t ctl_delay_us, uint8_t dp_id, const bap_cfg_t* p_cfg);

/**
 ****************************************************************************************
 * @brief Configure Codec for an ASE
 *
 * @param[in] con_lid           Connection local index
 * @param[in] ase_lid           ASE local index
 * @param[in] ase_instance_idx  ASE instance index
 * @param[in] p_codec_id        Pointer to Codec ID
 * @param[in] p_qos_req         Pointer to QoS Requirements structure
 * @param[in] ctl_delay_us      Controller delay in microseconds
 * @param[in] dp_id             Data Path ID
 * @param[in] p_cfg             Pointer to Codec Configuration structure (allocated by Upper Layer)\n
 *                                  - Can be NULL
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_configure_codec(uint8_t con_lid, uint8_t ase_lid, uint8_t ase_instance_idx,
                                    gaf_codec_id_t* p_codec_id, bap_qos_req_t* p_qos_req, uint32_t ctl_delay_us,
                                    uint8_t dp_id, const bap_cfg_t* p_cfg);

/**
 ****************************************************************************************
 * @brief Update Metadata for an ASE
 *
 * @param[in] ase_lid       ASE local index
 * @param[in] p_metadata    Pointer to Metadata structure (allocated by Upper Layer)
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_update_metadata(uint8_t ase_lid, bap_cfg_metadata_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Disable an ASE
 *
 * @param[in] ase_lid       ASE local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_disable(uint8_t ase_lid);

/**
 ****************************************************************************************
 * @brief Release an ASE
 *
 * @param[in] ase_lid       ASE local index
 * @param[in] idle          Indicate if ASE next state will be Idle or Codec Configured
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_release(uint8_t ase_lid, bool idle);

/**
 ****************************************************************************************
 * @brief Get link quality information for stream an ASE is part of
 *
 * @param[in] ase_lid       ASE local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t bap_uc_srv_get_quality(uint8_t ase_lid);

/**
 ****************************************************************************************
 * @brief To get ISO link Tx sync info
 *
 * @param[in] ase_lid       ASE local index
 *
 * @return An error status
 ****************************************************************************************
 */

uint16_t bap_uc_srv_get_iso_tx_sync(uint8_t ase_lid);

/**
 ****************************************************************************************
 * @brief Confirmation for BAP_UC_SRV_CONFIGURE_CODEC request indication
 *
 * @param[in] rsp_code          Response code (see #bap_uc_cp_rsp_code enumeration)
 * @param[in] reason            Reason (see #bap_uc_cp_reason enumeration)
 * @param[in] ase_lid           ASE local index
 * @param[in] p_qos_req         Pointer to QoS Requirements structure
 * @param[in] p_cfg             Pointer to Codec Configuration structure (allocated by Upper Layer)\n
 *                                  - Can be NULL
 * @param[in] ctl_delay_us      Controller delay in microseconds
 * @param[in] dp_id             Data Path ID
 ****************************************************************************************
 */
void bap_uc_srv_configure_codec_cfm(uint8_t rsp_code, uint8_t reason, uint8_t ase_lid, bap_qos_req_t* p_qos_req,
                                    const bap_cfg_t* p_cfg, uint32_t ctl_delay_us, uint8_t dp_id);

/**
 ****************************************************************************************
 * @brief Confirmation for BAP_UC_SRV_ENABLE request indication
 *
 * @param[in] rsp_code      Response code (see #bap_uc_cp_rsp_code enumeration)
 * @param[in] reason        Reason (see #bap_uc_cp_reason enumeration)
 * @param[in] p_metadata    Pointer to Metadata structure (allocated by Upper Layer)
 ****************************************************************************************
 */
void bap_uc_srv_enable_cfm(uint8_t rsp_code, uint8_t reason, const bap_cfg_metadata_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Confirmation for BAP_UC_SRV_UPDATE_METADATA request indication
 *
 * @param[in] rsp_code      Response code (see #bap_uc_cp_rsp_code enumeration)
 * @param[in] reason        Reason (see #bap_uc_cp_reason enumeration)
 * @param[in] p_metadata    Pointer to Metadata structure (allocated by Upper Layer)
 ****************************************************************************************
 */
void bap_uc_srv_update_metadata_cfm(uint8_t rsp_code, uint8_t reason, const bap_cfg_metadata_t* p_metadata);

/**
 ****************************************************************************************
 * @brief Confirmation for BAP_UC_SRV_RELEASE request indication
 *
 * @param[in] rsp_code      Response code (see #bap_uc_cp_rsp_code enumeration)
 * @param[in] reason        Reason (see #bap_uc_cp_reason enumeration)
 * @param[in] idle          Indicate if next state of ASE being released will be Idle or Codec Configured
 ****************************************************************************************
 */
void bap_uc_srv_release_cfm(uint8_t rsp_code, uint8_t reason, bool idle);

/**
 ****************************************************************************************
 * @brief Confirmation function for BAP_UC_CLI_DP_UPDATE request indication
 *
 * @param[in] ase_lid           ASE local index
 * @param[in] accept            Indicate if data path update is accepted or not
 * During ENABLE procedure if accept = true, data path is started,
 * Else nothing is done. Application will then have to initiate either start of DISABLE or RELEASE procedure.
 *
 * During DISABLE procedure if accept = true, data path is started,
 * Else nothing is done. Application will then have to initiate start of RELEASE procedure
 ****************************************************************************************
 */
void bap_uc_srv_dp_update_cfm(uint8_t ase_lid, bool accept);

/**
 ****************************************************************************************
 * @return Return if BAP Unicast Server module has been configured
 ****************************************************************************************
 */
bool bap_uc_srv_is_configured(void);

/// @} BAP_UC_SRV_FUNCTION
#endif //(GAF_BAP_UC_SRV)

#endif // BAP_UC_SRV_H_
