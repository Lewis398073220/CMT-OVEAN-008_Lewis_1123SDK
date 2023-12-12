/**
 ****************************************************************************************
 *
 * @file acc_ots.h
 *
 * @brief Audio Content Control - Object Transfer Server - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_OTS_H_
#define ACC_OTS_H_

/**
 ****************************************************************************************
 * @defgroup ACC_OTS Object Transfer Service Server
 * @ingroup ACC_OT
 * @brief Description of Object Transfer Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTS_ENUM Enumerations
 * @ingroup ACC_OTS
 * @brief Enumerations for Object Transfer Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTS_NATIVE_API Native API
 * @ingroup ACC_OTS
 * @brief Description of Native API for Object Transfer Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTS_CALLBACK Callback Functions
 * @ingroup ACC_OTS_NATIVE_API
 * @brief Description of callback functions for Object Transfer Service Server module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTS_FUNCTION Functions
 * @ingroup ACC_OTS_NATIVE_API
 * @brief Description of functions for Object Transfer Service Server module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "acc_ot.h"            // Audio Content Control - Object Transfer Definitions

#if (GAF_ACC_OTS)

#include "gaf.h"               // GAF Defines
#include "ots.h"               // Object Transfer Server Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup ACC_OTS_ENUM
/// @{

/// List of GAF_CMD command codes for Object Transfer Server
enum acc_ots_cmd_codes
{
    /// Disconnect Channel
    ACC_OTS_COC_DISCONNECT = GAF_CODE(ACC, OTS, 0),
    /// Send on Channel
    ACC_OTS_COC_SEND = GAF_CODE(ACC, OTS, 1),
    /// Release Channel
    ACC_OTS_COC_RELEASE = GAF_CODE(ACC, OTS, 2),
};

/// @} ACC_OTS_ENUM

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// @addtogroup ACC_OTS_CALLBACK
/// @{

/// Set of callback functions for Object Transfer Server
typedef ots_cb_t acc_ots_cb_t;

/// @} ACC_OTS_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup ACC_OTS_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Create and configure Object Transfer Client module
 *
 * @param[in] nb_transfers      Number of Object Transfer Service instances
 * @param[in] p_cb              Pointer to set of callback functions for communications with
 * upper layers
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t acc_ots_configure(uint8_t nb_transfers, const acc_ots_cb_t* p_cb);

/// Wrapper for functions defined in ots.h
#define acc_ots_coc_disconnect(con_lid)                                                     \
                            ots_coc_disconnect(con_lid)
#define acc_ots_coc_release(con_lid)                                                        \
                            ots_coc_release(con_lid)
#define acc_ots_coc_send(con_lid, length, p_sdu)                                            \
                            ots_coc_send(con_lid, length, p_sdu)
#define acc_ots_add(cfg_bf, shdl, oacp_features, olcp_features,                             \
                    p_transfer_lid, p_start_handle)                                         \
                            ots_add(cfg_bf, shdl, oacp_features, olcp_features,             \
                                    p_transfer_lid, p_start_handle)
#define acc_ots_restore_bond_data(con_lid, transfer_lid, cli_cfg_bf, evt_cfg_bf,            \
                                  nb_changes, p_changed_info)                               \
                            ots_restore_bond_data(con_lid, transfer_lid, cli_cfg_bf,        \
                                                  evt_cfg_bf, nb_changes, p_changed_info)
#define acc_ots_object_add(p_object_id, current_size, allocated_size,                       \
                           p_first_created_time, p_last_modified_time,                      \
                           properties, uuid_type, p_uuid, p_object_lid)                     \
                            ots_object_add(p_object_id, current_size, allocated_size,       \
                                           p_first_created_time, p_last_modified_time,      \
                                           properties, uuid_type, p_uuid, p_object_lid)
#define acc_ots_object_remove(object_lid)                                                   \
                            ots_object_remove(object_lid)
#define acc_ots_object_change(con_lid, transfer_lid, object_lid)                            \
                            ots_object_change(con_lid, transfer_lid, object_lid)
#define acc_ots_object_changed(flags, p_object_id)                                          \
                            ots_object_changed(flags, p_object_id)
#define acc_ots_set(object_lid, set_type, value)                                            \
                            ots_set(object_lid, set_type, value)
#define acc_ots_set_time(object_lid, p_time)                                                \
                            ots_set_time(object_lid, p_time)
#define acc_ots_cfm_get_name(status, con_lid, token, name_len, p_name)                      \
                            ots_cfm_get_name(status, con_lid,                               \
                                             token, name_len, p_name)
#define acc_ots_cfm_set_name(status, con_lid, token)                                        \
                            ots_cfm_set_name(status, con_lid, token)
#define acc_ots_cfm_object_control(status, con_lid, transfer_lid,                           \
                                   token, result_code, checksum)                            \
                            ots_cfm_object_control(status, con_lid,                         \
                                                   transfer_lid, token,                     \
                                                   result_code, checksum)
#define acc_ots_cfm_object_execute(status, con_lid, transfer_lid, token,                    \
                                   result_code, rsp_len, p_rsp)                             \
                            ots_cfm_object_execute(status, con_lid, transfer_lid, token,    \
                                                   result_code, rsp_len, p_rsp)
#define acc_ots_cfm_filter_get(status, con_lid, transfer_lid,                               \
                               token, filter_val)                                           \
                            ots_cfm_filter_get(status, con_lid, transfer_lid,               \
                                               token, filter_val)
#define acc_ots_cfm_filter_get_time(status, con_lid, transfer_lid,                          \
                                    token, filter_val, p_time_start, p_time_end)            \
                            ots_cfm_filter_get_time(status, con_lid, transfer_lid,          \
                                                    token, filter_val,                      \
                                                    p_time_start, p_time_end)
#define acc_ots_cfm_filter_get_size(status, con_lid, transfer_lid,                          \
                                    token, filter_val, size_min, size_max)                  \
                            ots_cfm_filter_get_size(status, con_lid, transfer_lid,          \
                                                    token, filter_val,                      \
                                                    size_min, size_max)
#define acc_ots_cfm_filter_get_name(status, con_lid, transfer_lid,                          \
                                    token, filter_val, name_len, p_name)                    \
                            ots_cfm_filter_get_name(status, con_lid, transfer_lid,          \
                                                    token, filter_val,                      \
                                                    name_len, p_name)
#define acc_ots_cfm_filter_get_type(status, con_lid, transfer_lid,                          \
                                    token, uuid_type, p_uuid)                               \
                            ots_cfm_filter_get_type(status, con_lid, transfer_lid,          \
                                                    token, uuid_type, p_uuid)
#define acc_ots_cfm_list_control(status, con_lid, transfer_lid,                             \
                                 token, result_code, nb_object)                             \
                            ots_cfm_list_control(status, con_lid,                           \
                                                 transfer_lid, token, result_code,          \
                                                 nb_object)
#define acc_ots_cfm_filter_set(status, con_lid, transfer_lid, token)                        \
                            ots_cfm_filter_set(status, con_lid, transfer_lid, token)
#define acc_ots_cfm_coc_connect(status, con_lid, token, local_max_sdu)                      \
                            ots_cfm_coc_connect(status, con_lid, token, local_max_sdu)

/// @} ACC_OTS_FUNCTION
#endif //(GAF_ACC_OTS)

#endif // ACC_OTS_H_
