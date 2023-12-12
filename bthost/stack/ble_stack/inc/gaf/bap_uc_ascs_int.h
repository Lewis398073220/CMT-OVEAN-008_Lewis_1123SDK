/**
 ****************************************************************************************
 *
 * @file bap_uc_ascs_int.h
 *
 * @brief Basic Audio Profile - Unicast - Audio Stream Control Service Internal Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_UC_ASCS_INT_H_
#define BAP_UC_ASCS_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bap.h"            // Basic Audio Profile Definitions
#include "bap_uc.h"         // Basic Audio Profile - Unicast - Definitions
#include "gaf_inc.h"        // Generic Audio Framework Included Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// Minimum ASE ID
#define BAP_UC_ASE_ID_MIN       (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Position of fields in written ASE Control Point characteristic value
enum bap_uc_cp_pos
{
    /// Operation code
    BAP_UC_CP_OPCODE_POS = 0,
    /// Number of ASEs
    BAP_UC_CP_NB_ASES_POS,
    /// Minimal length of ASE Control Point characteristic value
    BAP_UC_CP_LEN_MIN,

    /// --------- Config Codec operation ---------
    /// ASE ID
    BAP_UC_CP_CODEC_ASE_ID_POS = 0,
    /// Target Latency
    BAP_UC_CP_CODEC_TGT_LATENCY_POS,
    /// Target PHY
    BAP_UC_CP_CODEC_TGT_PHY_POS,
    /// Codec ID
    BAP_UC_CP_CODEC_ID_POS,
    /// Length of Codec Specific Configuration
    BAP_UC_CP_CODEC_SPEC_CFG_LEN_POS = BAP_UC_CP_CODEC_ID_POS + GAF_CODEC_ID_LEN,
    /// Minimal length of ASE Control Point characteristic value for Configure Codec operation code
    BAP_UC_CP_CODEC_LEN_MIN,
    /// Codec Specific Configuration
    BAP_UC_CP_CODEC_SPEC_CFG_POS = BAP_UC_CP_CODEC_LEN_MIN,

    /// --------- Config QoS operation ---------
    /// ASE ID
    BAP_UC_CP_QOS_ASE_ID_POS = 0,
    /// CIG ID
    BAP_UC_CP_QOS_CIG_ID_POS,
    /// CIS ID
    BAP_UC_CP_QOS_CIS_ID_POS,
    /// SDU Interval
    BAP_UC_CP_QOS_SDU_INTV_POS,
    /// Framing
    BAP_UC_CP_QOS_FRAMING_POS = BAP_UC_CP_QOS_SDU_INTV_POS + 3,
    /// PHY
    BAP_UC_CP_QOS_PHY_POS,
    /// Maximum SDU Size
    BAP_UC_CP_QOS_MAX_SDU_SIZE_POS,
    /// Retransmission Number
    BAP_UC_CP_QOS_RETX_NB_POS = BAP_UC_CP_QOS_MAX_SDU_SIZE_POS + 2,
    /// Maximum Transport Latency
    BAP_UC_CP_QOS_MAX_TRANS_LATENCY_POS,
    /// Presentation Delay
    BAP_UC_CP_QOS_PRES_DELAY_POS = BAP_UC_CP_QOS_MAX_TRANS_LATENCY_POS + 2,
    /// Length of ASE Control Point characteristic value for Configure QoS operation code
    BAP_UC_CP_QOS_LEN = BAP_UC_CP_QOS_PRES_DELAY_POS + 3,

    /// --------- Enable/Update Metadata operation ---------
    /// ASE ID
    BAP_UC_CP_METADATA_ASE_ID_POS = 0,
    /// Metadata length
    BAP_UC_CP_METADATA_METADATA_LEN_POS,
    /// Minimal length of ASE Control Point characteristic value for Enable and Update Metadata operation code
    BAP_UC_CP_METADATA_LEN_MIN,
    /// Metadata
    BAP_UC_CP_METADATA_METADATA_POS = BAP_UC_CP_METADATA_LEN_MIN,
};

/// Position of fields in notified ASE characteristic value
enum bap_uc_cp_ntf_pos
{
    /// ASE Control Point operation code in error
    BAP_UC_CP_NTF_OPCODE_POS = 0,
    /// Number of ASEs
    BAP_UC_CP_NTF_NB_ASES_POS,

    BAP_UC_CP_NTF_LEN_MIN,

    /// ASE ID
    BAP_UC_CP_NTF_ASE_ID_POS = 0,
    /// Response Code
    BAP_UC_CP_NTF_RSP_CODE_POS,
    /// Reason
    BAP_UC_CP_NTF_REASON_POS,

    BAP_UC_CP_NTF_PER_ASE_LEN,
};

/// Position of fields in ASE characteristic value
enum bap_uc_ase_pos
{
    /// ASE ID
    BAP_UC_ASE_ID_POS = 0,
    /// ASE State
    BAP_UC_ASE_STATE_POS,
    /// Minimal length of ASE characteristic value
    BAP_UC_ASE_LEN_MIN,

    /// --------- Codec Configured State ---------
    /// Framing
    BAP_UC_ASE_CODEC_FRAMING_POS = BAP_UC_ASE_LEN_MIN,
    /// Preferred PHY
    BAP_UC_ASE_CODEC_PREF_PHY_POS,
    /// Preferred Retransmission Number
    BAP_UC_ASE_CODEC_PREF_RETX_NB_POS,
    /// Max Transport Latency
    BAP_UC_ASE_CODEC_MAX_TRANS_LATENCY_POS,
    /// Presentation Delay Min
    BAP_UC_ASE_CODEC_PRES_DELAY_MIN_POS = BAP_UC_ASE_CODEC_MAX_TRANS_LATENCY_POS + 2,
    /// Presentation Delay Max
    BAP_UC_ASE_CODEC_PRES_DELAY_MAX_POS = BAP_UC_ASE_CODEC_PRES_DELAY_MIN_POS + 3,
    /// Preferred Presentation Delay Min
    BAP_UC_ASE_CODEC_PREF_PRES_DELAY_MIN_POS = BAP_UC_ASE_CODEC_PRES_DELAY_MAX_POS + 3,
    /// Preferred Presentation Delay Max
    BAP_UC_ASE_CODEC_PREF_PRES_DELAY_MAX_POS = BAP_UC_ASE_CODEC_PREF_PRES_DELAY_MIN_POS + 3,
    /// Codec ID
    BAP_UC_ASE_CODEC_ID_POS = BAP_UC_ASE_CODEC_PREF_PRES_DELAY_MAX_POS + 3,
    /// Codec Specific Configuration Length
    BAP_UC_ASE_CODEC_SPEC_CFG_LEN_POS = BAP_UC_ASE_CODEC_ID_POS + GAF_CODEC_ID_LEN,
    /// Minimal length of ASE characteristic value for Codec Configured state
    BAP_UC_ASE_CODEC_LEN_MIN,
    /// Codec Specific Configuration
    BAP_UC_ASE_CODEC_SPEC_CFG_POS = BAP_UC_ASE_CODEC_LEN_MIN,

    /// --------- QoS Configured State ---------
    /// CIS ID
    BAP_UC_ASE_QOS_CIG_ID_POS = BAP_UC_ASE_LEN_MIN,
    /// CIG ID
    BAP_UC_ASE_QOS_CIS_ID_POS,
    /// SDU Interval
    BAP_UC_ASE_QOS_SDU_INTV_POS,
    /// Framing
    BAP_UC_ASE_QOS_FRAMING_POS = BAP_UC_ASE_QOS_SDU_INTV_POS + 3,
    /// PHY
    BAP_UC_ASE_QOS_PHY_POS,
    /// Maximum SDU Size
    BAP_UC_ASE_QOS_MAX_SDU_SIZE_POS,
    /// Retransmission Number
    BAP_UC_ASE_QOS_RETX_NB_POS = BAP_UC_ASE_QOS_MAX_SDU_SIZE_POS + 2,
    /// Maximum Transport Latency
    BAP_UC_ASE_QOS_MAX_TRANS_LATENCY_POS,
    /// Presentation Delay
    BAP_UC_ASE_QOS_PRES_DELAY_POS = BAP_UC_ASE_QOS_MAX_TRANS_LATENCY_POS + 2,
    /// Length of ASE characteristic additional parameters for QoS Configured state
    BAP_UC_ASE_QOS_LEN = BAP_UC_ASE_QOS_PRES_DELAY_POS + 3,

    /// --------- Enabling/Streaming/Disabling State ---------
#ifdef BLE_STACK_PORTING_CHANGES
    /// CIG ID
    BAP_UC_ASE_METADATA_CIG_ID_POS = BAP_UC_ASE_LEN_MIN,
    /// CIS ID
    BAP_UC_ASE_METADATA_CIS_ID_POS,
#else
    /// CIS ID
    BAP_UC_ASE_METADATA_CIS_ID_POS = BAP_UC_ASE_LEN_MIN,
    /// CIG ID
    BAP_UC_ASE_METADATA_CIG_ID_POS,
#endif
    /// Metadata length
    BAP_UC_ASE_METADATA_METADATA_LEN_POS,
    /// Minimal length of ASE characteristic value for Enabling/Streaming/Disabling state
    BAP_UC_ASE_METADATA_LEN_MIN,
    /// Metadata
    BAP_UC_ASE_METADATA_METADATA_POS = BAP_UC_ASE_METADATA_LEN_MIN,
};

/// List of descriptors for Audio Stream Control Service
enum bap_uc_desc_type
{
    /// Client Characteristic Configuration descriptor for ASE control point characteristic
    BAP_UC_DESC_TYPE_CCC_CP = 0,
    /// CLient Characteristic Configuration descriptor for ASE characteristic
    BAP_UC_DESC_TYPE_CCC_ASE,

    BAP_UC_DESC_TYPE_MAX,
};

/*
 * INTERNAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check QoS Requirements values
 *
 * @param[in] p_qos_req     Pointer to QoS Requirements structure
 *
 * @return An error status
 *  - GAF_ERR_NO_ERROR if all provided values are valid
 *  - GAF_ERR_INVALID_PARAM if an invalid value has been detected
 ****************************************************************************************
 */
uint16_t bap_uc_check_qos_req(bap_qos_req_t* p_qos_req);

/**
 ****************************************************************************************
 * @brief Check QoS Configuration values
 *
 * @param[in] p_qos_cfg     Pointer to QoS Configuration structure
 *
 * @return An error status
 *  - GAF_ERR_NO_ERROR if all provided values are valid
 *  - GAF_ERR_INVALID_PARAM if an invalid value has been detected
 ****************************************************************************************
 */
uint16_t bap_uc_check_qos_cfg(bap_qos_cfg_t* p_qos_cfg);

#endif // BAP_UC_H_
