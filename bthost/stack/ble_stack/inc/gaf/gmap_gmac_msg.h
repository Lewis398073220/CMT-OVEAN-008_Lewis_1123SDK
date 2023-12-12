/**
 ****************************************************************************************
 *
 * @file gmap_gmac_msg.h
 *
 * @brief Gaming Audio Profile - Gaming Audio Service Client - Message API Definitions
 *
 * GmapCopyright (C) Bestechnic 2015-2022
 *
 ****************************************************************************************
 */

#ifndef GMAP_GMAC_MSG_H_
#define GMAP_GMAC_MSG_H_

/**
 ****************************************************************************************
 * @defgroup GMAP_GMAC_MSG Message API
 * @ingroup GMAP_GMAC
 * @brief Description of Message API for Gaming Audio Service Client module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gmap_msg.h"       // Message API Definitions
#include "gmap_gmac.h"      // Gaming Audio Profile - Gaming Audio Service Client Definitions

/// @addtogroup GMAP_GMAC_MSG
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of GAF_CMD command code values for Gaming Audio Profile Client module
enum gmap_gmac_msg_cmd_code
{
    /// Gaming Audio Service Client - Discover (see #gmap_gmac_discover_cmd_t)
    GMAP_GMAC_DISCOVER = GAF_CODE(GMAP, GMAC, GMAP_GMAC_CMD_TYPE_DISCOVER),
    /// Gaming Audio Service Client - Get Role (see #gmap_gmac_get_role_cmd_t)
    GMAP_GMAC_GET_ROLE = GAF_CODE(GMAP, GMAC, GMAP_GMAC_CMD_TYPE_GET_ROLE),
    /// Gaming Audio Service Client - Get UGT Feat (see #gmap_gmac_get_ugt_feat_cmd_t)
    GMAP_GMAC_GET_UGT_FEAT = GAF_CODE(GMAP, GMAC, GMAP_GMAC_CMD_TYPE_GET_UGT_FEAT),
};

/// List of GAF_REQ request code values for Gaming Audio Profile Client module
enum gmap_gmac_msg_req_code
{
    /// Gaming Audio Service Client - Restore Bond Data (see #gmap_gmac_restore_bond_data_req_t)
    GMAP_GMAC_RESTORE_BOND_DATA = GAF_CODE(GMAP, GMAC, 0),
};

/// List of GAF_IND indication code values for Gaming Audio Profile Client module
enum gmap_gmac_msg_ind_code
{
    /// Gaming Audio Service Client - Restore Bond Data (see #gmap_gmac_bond_data_ind_t)
    GMAP_GMAC_BOND_DATA = GAF_CODE(GMAP, GMAC, 0),
    /// Gaming Audio Service Client - Service Changed (see #gmap_gmac_svc_changed_ind_t)
    GMAP_GMAC_SVC_CHANGED = GAF_CODE(GMAP, GMAC, 1),
    /// Gaming Audio Service Client - Role (see #gmap_gmac_role_ind_t)
    GMAP_GMAC_ROLE = GAF_CODE(GMAP, GMAC, 2),
    /// Gaming Audio Service Client - Role (see #gmap_gmac_ugt_feat_ind_t)
    GMAP_GMAC_UGT_FEAT = GAF_CODE(GMAP, GMAC, 3),
};

/*
 * API MESSAGES
 ****************************************************************************************
 */

/// Structure for #GMAP_GMAC_RESTORE_BOND_DATA request message
typedef struct gmap_gmac_restore_bond_data_req
{
    /// Request code (shall be set to #GMAP_GMAC_RESTORE_BOND_DATA)
    uint16_t req_code;
    /// Connection local index
    uint8_t con_lid;
    /// Content description of Gaming Audio Service
    gmap_gmac_gmas_t gmas_info;
} gmap_gmac_restore_bond_data_req_t;

/// Structure of response message for Gaming Audio Service Client module
typedef struct gmap_gmac_rsp
{
    /// Request code (see #gmap_gmac_msg_req_code enumeration)
    uint16_t req_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
} gmap_gmac_rsp_t;

/// Structure for #GMAP_GMAC_DISCOVER command message
typedef struct gmap_gmac_discover_cmd
{
    /// Command code (shall be set to #GMAP_GMAC_DISCOVER)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
    /// Start handle for the discovery. Set GATT_INVALID_HDL if not provided
    uint16_t shdl;
    /// End handle for the discovery. Set GATT_INVALID_HDL if not provided
    uint16_t ehdl;
} gmap_gmac_discover_cmd_t;

/// Structure for #GMAP_GMAC_GET_ROLE command message
typedef struct gmap_gmac_get_role_cmd
{
    /// Command code (shall be set to #GMAP_GMAC_GET_ROLE)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
} gmap_gmac_get_role_cmd_t;

/// Structure for #GMAP_GMAC_GET_UGT_FEAT command message
typedef struct gmap_gmac_get_ugt_feat_cmd
{
    /// Command code (shall be set to #GMAP_GMAC_GET_UGT_FEAT)
    uint16_t cmd_code;
    /// Connection local index
    uint8_t con_lid;
} gmap_gmac_get_ugt_feat_cmd_t;

/// Structure for command complete event message for Gaming Audio Service Client module
typedef struct gmap_gmac_cmp_evt
{
    /// Command code (see #gmap_gmac_msg_cmd_code enumeration)
    uint16_t cmd_code;
    /// Status
    uint16_t status;
    /// Connection local index
    uint8_t con_lid;
} gmap_gmac_cmp_evt_t;

/// Structure for #GMAP_GMAC_BOND_DATA indication message
typedef struct gmap_gmac_bond_data_ind
{
    /// Indication code (set to #GMAP_GMAC_BOND_DATA)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Content description of Gaming Audio Service
    gmap_gmac_gmas_t gmas_info;
} gmap_gmac_bond_data_ind_t;

/// Structure for #GMAP_GMAC_SVC_CHANGED indication message
typedef struct gmap_gmac_svc_changed_ind
{
    /// Indication code (set to #GMAP_GMAC_SVC_CHANGED)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
} gmap_gmac_svc_changed_ind_t;

/// Structure for #GMAP_GMAC_ROLE indication message
typedef struct gmap_gmac_role_ind
{
    /// Indication code (set to #GMAP_GMAC_ROLE)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Role bit field (see #gmap_role_bf enumeration)
    uint8_t role_bf;
} gmap_gmac_role_ind_t;

/// Structure for #GMAP_GMAC_UGT_FEAT indication message
typedef struct gmap_gmac_ugt_feat_ind
{
    /// Indication code (set to #GMAP_GMAC_UGT_FEAT)
    uint16_t ind_code;
    /// Connection local index
    uint8_t con_lid;
    /// Role bit field (see #gmap_ugt_feature_bf enumeration)
    uint8_t ugt_feat;
} gmap_gmac_ugt_feat_ind_t;

/// @} HAP_HAC_MSG

#endif // GMAP_GMAC_MSG_H_
