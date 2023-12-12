/**
 ****************************************************************************************
 *
 * @file bap_bc_bass_int.h
 *
 * @brief Basic Audio Profile - Internals
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef BAP_BC_INT_H_
#define BAP_BC_INT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bap.h"            // Basic Audio Profile Definitions
#include "gaf_inc.h"        // Generic Audio Framework Included Definitions
#if (GAF_BAP_BC_SCAN)
#include "bap_bc_scan.h"    // Scan definitions
#endif // (GAF_BAP_BC_SCAN)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

#if (GAF_BAP_BC_SCAN)
/**
 ****************************************************************************************
 * @brief Internal callback function called each time a Broadcast Scan command has been completed
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] cmd_type      Command type (see #bap_bc_scan_cmd_type enumeration)
 * @param[in] status        Status (see #gaf_err enumeration)
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_cmp_evt)(uint8_t pa_lid, uint8_t cmd_type, uint16_t status);

/**
 ****************************************************************************************
 * @brief Internal callback function called when periodic sync has been established
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] p_adv_id      Pointer to Periodic Advertising identification
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_pa_established)(uint8_t pa_lid, const bap_adv_id_t* p_adv_id);

/**
 ****************************************************************************************
 * @brief Internal callback function called as result of @see bap_bc_scan_pa_terminate function or if
 *        the periodic sync establishment is canceled or periodic sync is lost
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] reason        Stop reason (see #gaf_err enumeration)
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_pa_terminated)(uint8_t pa_lid, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Callback function called when an advertising report is received with audio announcement
 *        It provides information about Broadcast Group
 *
 * @param[in] pa_lid            Periodic Advertising local index
 * @param[in] nb_subgroups      Number of Subgroups in the Broadcast Group
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_group_report)(uint8_t pa_lid, uint8_t nb_subgroups);

/**
 ****************************************************************************************
 * @brief Callback function called when an advertising report is received with audio announcement
 *        It provides information about Broadcast Subgroup
 *
 * @param[in] pa_lid            Periodic Advertising local index
 * @param[in] sgrp_idx          Subgroup index
 * @param[in] stream_pos_bf     Stream position index bit field indicating which Streams are part of the Subgroup
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_subgroup_report)(uint8_t pa_lid, uint8_t sgrp_idx, uint32_t stream_pos_bf);

/**
 ****************************************************************************************
 * @brief Callback function called when a BIG Info report is received
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] p_report      Pointer to structure that contains Big Info data
 ****************************************************************************************
 */
typedef void (*bap_bc_scan_int_cb_big_info_report)(uint8_t pa_lid, const gap_big_info_t* p_report);

/// Set of callback functions for internal communication with Broadcast Assistant or Delegator module
typedef struct bap_bc_scan_int_cb
{
    /// Callback function called when a command has been completed
    bap_bc_scan_int_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when a Periodic Advertising Sync is established
    bap_bc_scan_int_cb_pa_established cb_pa_established;
    /// Callback function called when a Periodic Advertising Sync is lost, canceled or terminated
    bap_bc_scan_int_cb_pa_terminated cb_pa_terminated;
    /// Callback function called when an advertising report is received with audio announcement (Group information)
    bap_bc_scan_int_cb_group_report cb_group_report;
    /// Callback function called when an advertising report is received with audio announcement (Subgroup information)
    bap_bc_scan_int_cb_subgroup_report cb_subgroup_report;
    /// Callback function called when a BIG Info report is received
    bap_bc_scan_int_cb_big_info_report cb_big_info_report;
} bap_bc_scan_int_cb_t;
#endif // (GAF_BAP_BC_SCAN)

#if (GAF_BAP_BC_SINK && GAF_BAP_BC_DELEG)
/**
 ****************************************************************************************
 * @brief Callback function called each time a Broadcast sink command has been completed
 *
 * @param[in] grp_lid       Group local index
 * @param[in] cmd_type      Command type (see #bap_bc_sink_cmd_type enumeration)
 * @param[in] status        Status (see #gaf_err enumeration)
 ****************************************************************************************
 */
typedef void (*bap_bc_sink_int_cb_cmp_evt)(uint8_t grp_lid, uint8_t cmd_type, uint16_t status);

/**
 ****************************************************************************************
 * @brief Inform upper layer about status of synchronization with a Broadcast Group
 *
 * @param[in] grp_lid           Group local index
 * @param[in] state             Broadcast Sink state (see #bap_bc_sink_state enumeration)
 * @param[in] stream_pos_bf     Stream position bit field in Broadcast Group to receive
 ****************************************************************************************
 */
typedef void (*bap_bc_sink_int_cb_status)(uint8_t grp_lid, uint8_t state, uint32_t stream_pos_bf);

/**
 ****************************************************************************************
 * @brief Get information about how to establish BIG Sink:
 *           - Periodic ADV sync local index
 *           - If encrypted, value of broadcast code
 *
 * @param[in]  grp_lid           Group local index
 * @param[out] pp_bcast_code     Pointer to broadcast code information, returns NULL if not encrypted
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
typedef uint16_t (*bap_bc_sink_int_cb_info_get)(uint8_t grp_lid, const gaf_bcast_code_t** pp_bcast_code);

/// Set of callback functions for internal communication with Broadcast Delegator module
typedef struct bap_bc_sink_int_cb
{
    /// Callback function called when a command has been completed
    bap_bc_sink_int_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when status of synchronization with a Broadcast Group changed
    bap_bc_sink_int_cb_status cb_status;
    /// Callback used to get get information about how to establish BIG Sink
    bap_bc_sink_int_cb_info_get cb_info_get;
} bap_bc_sink_int_cb_t;
#endif //(GAF_BAP_BC_SCAN && GAF_BAP_BC_DELEG)

#if (GAF_BAP_BC_SRC && GAF_BAP_BC_ASSIST)
/**
 ****************************************************************************************
 * @brief Internal callback function called each time a Broadcast Source command has been completed
 *
 * @param[in] cmd_type      Command type (see #bap_bc_scan_cmd_type enumeration)
 * @param[in] status        Status (see #gaf_err enumeration)
 * @param[in] pa_lid        Periodic Advertising local index
 ****************************************************************************************
 */
typedef void (*bap_bc_src_int_cb_cmp_evt)(uint8_t cmd_type, uint16_t status, uint8_t pa_lid);

/// Set of callback functions for internal communication with Broadcast Assistant module
typedef struct bap_bc_src_int_cb
{
    /// Callback function called when a command has been completed
    bap_bc_src_int_cb_cmp_evt cb_cmp_evt;
} bap_bc_src_int_cb_t;
#endif // (GAF_BAP_BC_SRC && GAF_BAP_BC_ASSIST)

/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */

#if (GAF_BAP_BC_DELEG)
/**
 ****************************************************************************************
 * @brief Retrieve Broadcast Delegator scan interface
 *
 * @return Scan callback interface (shall not be NULL)
 ****************************************************************************************
 */
const bap_bc_scan_int_cb_t* bap_bc_deleg_get_cb_scan(void);

/**
 ****************************************************************************************
 * @brief Establish a Periodic Adv Sync initiated by delegator
 *        Upper layer application will be inform about establishment to accept or reject it.
 *
 * @param[in]  src_lid            Source local index in delegator
 * @param[in]  con_lid            Connection local index that requests the sync establishment
 * @param[in]  p_adv_addr         Pointer to Periodic Advertising identification
 * @param[in]  past               True if sync transfer is expected, False otherwise
 *
 * @param[out] p_pa_lid           Pointer to allocated Periodic Advertising sync local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_synchronize_from_deleg(uint8_t src_lid, uint8_t con_lid, const bap_adv_id_t* p_adv_addr,
                                            bool past, uint8_t* p_pa_lid);

/**
 ****************************************************************************************
 * @brief Terminate a Periodic Adv Sync initiated by delegator
 *        Upper layer application will be inform about termination to accept or reject it.
 *
 * @param[in] pa_lid            Periodic Advertising local index
 * @param[in] con_lid           Connection local index that requests the sync terminate
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_terminate_from_deleg(uint8_t pa_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Retrieve Broadcast delegator sink interface
 *
 * @return SINK callback interface (shall not be NULL)
 ****************************************************************************************
 */
const bap_bc_sink_int_cb_t* bap_bc_deleg_get_cb_sink(void);

/**
 ****************************************************************************************
 * @brief Establish a BIG Sink initiated by delegator
 *        Upper layer application will be inform about establishment to accept or reject it.
 *        and provide Sink parameters
 *
 * @param[in] con_lid           Connection local index that requests the sync terminate
 * @param[in] src_lid           Source local index
 * @param[in] pa_lid            PA local index
 * @param[in] p_bcast_id        Pointer to Broadcast ID
 * @param[in] stream_pos_bf     Stream position bit field indicating Streams to synchronize with.
 * @param[out] p_grp_lid        Pointer at which allocated Group local index is returned
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_sink_enable_from_deleg(uint8_t con_lid, uint8_t src_lid, uint8_t pa_lid,
                                       const bap_bcast_id_t* p_bcast_id, uint32_t stream_pos_bf, uint8_t* p_grp_lid);

/**
 ****************************************************************************************
 * @brief Terminate a BIG Sink initiated by delegator
 *        Upper layer application will be inform about termination to accept or reject it.
 *
 * @param[in]  grp_lid           Group local index
 * @param[in]  con_lid           Connection local index that requests the sync terminate
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_sink_disable_from_deleg(uint8_t grp_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint32_t bap_bc_sink_is_known(const bap_bcast_id_t* p_bcast_id, uint8_t* p_grp_lid);
#endif // (GAF_BAP_BC_DELEG)

#if (GAF_BAP_BC_DELEG)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_report_ctrl_loc(uint8_t pa_lid, uint8_t report_filter_bf);
#endif //(GAF_BAP_BC_DELEG)

#if (GAF_BAP_BC_ASSIST)
/**
 ****************************************************************************************
 * @brief Retrieve Broadcast Assistant scan interface
 *
 * @return Scan callback interface (shall not be NULL)
 ****************************************************************************************
 */
const bap_bc_scan_int_cb_t* bap_bc_assist_get_cb_scan(void);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_synchronize_from_assist(const bap_adv_id_t* p_adv_addr, uint8_t* p_pa_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_terminate_from_assist(uint8_t pa_lid);

/**
 ****************************************************************************************
 * @brief Transfer information about synchronization with a Periodic Advertising
 *        Once the transfer has been performed, @see cb_cmp_evt callback is executed.
 *
 * @param[in] pa_lid          Periodic Advertising local index
 * @param[in] con_lid         Connection local index
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_scan_pa_transfer(uint8_t pa_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
bool bap_bc_src_is_local(bap_adv_id_t* p_src_addr, uint8_t* p_grp_lid, uint8_t* p_pa_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_src_pa_transfer(uint8_t grp_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_src_check_grp(uint8_t grp_lid, uint8_t nb_subgroups);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_src_get_adv_id(uint8_t grp_lid, bap_adv_id_t* p_adv_id);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
const bap_bcast_id_t* bap_bc_src_get_bcast_id(uint8_t grp_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
const bap_cfg_metadata_t* bap_bc_src_get_metadata(uint8_t grp_lid, uint8_t sgrp_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_src_get_metadata_len_total(uint8_t grp_lid);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_bc_src_get_sgrp_bis_bf(uint8_t grp_lid, uint8_t sgrp_lid);
#endif //(GAF_BAP_BC_ASSIST)

#if (GAF_BAP_BC_ASSIST || GAF_BAP_BC_DELEG)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
bool bap_bc_scan_pa_is_known(const bap_adv_id_t* p_adv_addr, uint8_t* p_pa_lid);
#endif //(GAF_BAP_BC_ASSIST || GAF_BAP_BC_DELEG)

#if (GAF_BAP_BC_ASSIST && GAF_BAP_BC_SRC)
/**
 ****************************************************************************************
 * @brief Retrieve Broadcast Assistant module interface for Broadcast Source module
 ****************************************************************************************
 */
const bap_bc_src_int_cb_t* bap_bc_assist_get_itf_src(void);
#endif //(GAF_BAP_BC_ASSIST && GAF_BAP_BC_SRC)

#endif // BAP_BC_INT_H_
