/**
 ****************************************************************************************
 *
 * @file acc_otc.h
 *
 * @brief Audio Content Control - Object Transfer Client - Definitions
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef ACC_OTC_H_
#define ACC_OTC_H_

/**
 ****************************************************************************************
 * @defgroup ACC_OTC Object Transfer Service Client
 * @ingroup ACC_OT
 * @brief Description of Object Transfer Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTC_ENUM Enumerations
 * @ingroup ACC_OTC
 * @brief Enumerations for Object Transfer Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTC_NATIVE_API Native API
 * @ingroup ACC_OTC
 * @brief Description of Native API for Object Transfer Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTC_CALLBACK Callback Functions
 * @ingroup ACC_OTC_NATIVE_API
 * @brief Description of callback functions for Object Transfer Service Client module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup ACC_OTC_FUNCTION Functions
 * @ingroup ACC_OTC_NATIVE_API
 * @brief Description of functions for Object Transfer Service Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "acc_ot.h"            // Audio Content Control - Object Tranfer Definitions

#if (GAF_ACC_OTC)

#include "gaf.h"               // GAF Defines
#include "otc.h"               // Object Transfer Client Definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup ACC_OTC_ENUM
/// @{

/// List of ACC_CMD command codes for Object Transfer Client
enum acc_otc_cmd_codes
{
    /// Discover
    ACC_OTC_DISCOVER = GAF_CODE(ACC, OTC, 0),
    /// Get
    ACC_OTC_GET = GAF_CODE(ACC, OTC, 1),
    /// Get Configuration
    ACC_OTC_GET_CFG = GAF_CODE(ACC, OTC, 2),
    /// Set Configuration
    ACC_OTC_SET_CFG = GAF_CODE(ACC, OTC, 3),
    /// Set Name
    ACC_OTC_SET_NAME = GAF_CODE(ACC, OTC, 4),
    /// Set Time
    ACC_OTC_SET_TIME = GAF_CODE(ACC, OTC, 5),
    /// Set Properties
    ACC_OTC_SET_PROPERTIES = GAF_CODE(ACC, OTC, 6),
    /// Create Object
    ACC_OTC_OBJECT_CREATE = GAF_CODE(ACC, OTC, 7),
    /// Control Object
    ACC_OTC_OBJECT_CONTROL = GAF_CODE(ACC, OTC, 8),
    /// Manipulate Object
    ACC_OTC_OBJECT_MANIPULATE = GAF_CODE(ACC, OTC, 9),
    /// Exexute Object
    ACC_OTC_OBJECT_EXECUTE = GAF_CODE(ACC, OTC, 10),
    /// List Control
    ACC_OTC_LIST_CONTROL = GAF_CODE(ACC, OTC, 11),
    /// List Goto
    ACC_OTC_LIST_GOTO = GAF_CODE(ACC, OTC, 12),
    /// Filter Set
    ACC_OTC_FILTER_SET = GAF_CODE(ACC, OTC, 13),
    /// Filter Set Time
    ACC_OTC_FILTER_SET_TIME = GAF_CODE(ACC, OTC, 14),
    /// Filter Set Size
    ACC_OTC_FILTER_SET_SIZE = GAF_CODE(ACC, OTC, 15),
    /// Filter Set Name
    ACC_OTC_FILTER_SET_NAME = GAF_CODE(ACC, OTC, 16),
    /// Filter Set Type
    ACC_OTC_FILTER_SET_TYPE = GAF_CODE(ACC, OTC, 17),
    /// Connection Oriented Channel - Connect
    ACC_OTC_COC_CONNECT = GAF_CODE(ACC, OTC, 18),
    /// Connection Oriented Channel - Disconnect
    ACC_OTC_COC_DISCONNECT = GAF_CODE(ACC, OTC, 19),
    /// Connection Oriented Channel - Send
    ACC_OTC_COC_SEND = GAF_CODE(ACC, OTC, 20),
    /// Connection Oriented Channel - Release
    ACC_OTC_COC_RELEASE = GAF_CODE(ACC, OTC, 21),
};

/// @} ACC_OTC_ENUM

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// @addtogroup ACC_OTC_CALLBACK
/// @{

/// Set of callback functions for Object Transfer Client
typedef otc_cb_t acc_otc_cb_t;

/// @} ACC_OTC_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup ACC_OTC_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Create and configure Object Transfer Client module
 *
 * @param[in] p_cb      Pointer to set of callback functions for communications with
 * upper layers
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t acc_otc_configure(const acc_otc_cb_t* p_cb);

/// Wrapper for functions defined in otc.h
#define acc_otc_discover(con_lid, nb_ots_max, svc_type, shdl, ehdl)                                                               \
                                (otc_discover(con_lid, nb_ots_max, svc_type, shdl, ehdl))
#if (GAF_DBG)
#define acc_otc_get(con_lid, transfer_lid, get_type, char_type)                                             \
                                (otc_get(con_lid, transfer_lid, get_type, char_type))
#define acc_otc_get_cfg(con_lid, transfer_lid, char_type)                                               \
                                (otc_get_cfg(con_lid, transfer_lid, char_type))
#define acc_otc_set_cfg(con_lid, transfer_lid, char_type, enable)                                       \
                                (otc_set_cfg(con_lid, transfer_lid, char_type, enable))
#endif //(GAF_DBG)
#define acc_otc_set_name(con_lid, transfer_lid, name_len, p_name)                                           \
                                (otc_set_name(con_lid, transfer_lid, name_len, p_name))
#define acc_otc_set_time(con_lid, transfer_lid, char_type, p_time)                                          \
                                (otc_set_time(con_lid, transfer_lid, char_type, p_time))
#define acc_otc_set_properties(con_lid, transfer_lid, properties)                                           \
                                (otc_set_properties(con_lid, transfer_lid, properties))
#define acc_otc_object_create(con_lid, transfer_lid, size, uuid_type, p_uuid)                               \
                                (otc_object_create(con_lid, transfer_lid, size, uuid_type, p_uuid))
#define acc_otc_object_control(con_lid, transfer_lid, opcode)                                               \
                                (otc_object_control(con_lid, transfer_lid, opcode))
#define acc_otc_object_manipulate(con_lid, transfer_lid, opcode, offset, length, mode)                      \
                                (otc_object_manipulate(con_lid, transfer_lid, opcode, offset, length, mode))
#define acc_otc_object_execute(con_lid, transfer_lid, param_len, p_param)                                   \
                                (otc_object_execute(con_lid, transfer_lid, param_len, p_param))
#define acc_otc_list_control(con_lid, transfer_lid, opcode, order)                                          \
                                (otc_list_control(con_lid, transfer_lid, opcode, order))
#define acc_otc_list_goto(con_lid, transfer_lid, opcode, p_object_id)                                       \
                                (otc_list_goto(con_lid, transfer_lid, opcode, p_object_id))
#define acc_otc_filter_set(con_lid, transfer_lid, filter_lid, filter_val)                                   \
                                (otc_filter_set(con_lid, transfer_lid, filter_lid, filter_val))
#define acc_otc_filter_set_time(con_lid, transfer_lid, filter_lid, filter_val, p_time_start, p_time_end)    \
                                (otc_filter_set_time(con_lid, transfer_lid, filter_lid, filter_val,         \
                                                    p_time_start, p_time_end))
#define acc_otc_filter_set_size(con_lid, transfer_lid, filter_lid, filter_val, size_min, size_max)          \
                                (otc_filter_set_size(con_lid, transfer_lid, filter_lid, filter_val,         \
                                                     size_min, size_max))
#define acc_otc_filter_set_name(con_lid, transfer_lid, filter_lid, filter_val, name_len, p_name)            \
                                (otc_filter_set_name(con_lid, transfer_lid, filter_lid, filter_val,         \
                                                    name_len, p_name))
#define acc_otc_filter_set_type(con_lid, transfer_lid, filter_lid, uuid_type, p_uuid)                       \
                                (otc_filter_set_type(con_lid, transfer_lid, filter_lid, uuid_type, p_uuid))
#define acc_otc_coc_connect(con_lid, local_max_sdu)                                                         \
                                (otc_coc_connect(con_lid, local_max_sdu))
#define acc_otc_coc_disconnect(con_lid)                                                                     \
                                (otc_coc_disconnect(con_lid))
#define acc_otc_coc_send(con_lid, length, p_sdu)                                                            \
                                (otc_coc_send(con_lid, length, p_sdu))
#define acc_otc_coc_release(con_lid)                                                                        \
                                (otc_coc_release(con_lid))
#define acc_otc_restore_bond_data(con_lid, nb_ots, p_ots_info)                                                       \
                                (otc_restore_bond_data(con_lid, nb_ots, p_ots_info))

/// @} ACC_OTC_FUNCTION
#endif //(GAF_ACC_OTC)

#endif // ACC_OTC_H_
