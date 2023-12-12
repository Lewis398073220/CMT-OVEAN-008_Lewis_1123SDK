/***************************************************************************
 *
 * Copyright 2015-2022 BES.
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
#ifndef _AHP_AHS_H_
#define _AHP_AHS_H_
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "ke_task.h"
#include "ke_msg.h"
#if (BLE_AHP_SERVER)
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#define ADVANCED_HEADPHONE_SERVICE_BRC_BIT_MASK    0x0001
#define ADVANCED_HEADPHONE_SERVICE_HTC_BIT_MASK    0x0001

/*
 * ENUMERATION
 ****************************************************************************************
 */
/// Binaural Rendoring Control Functionality status
/// Allow the user to turn head tracking (scene rotation) processing on/off via the client UI
typedef enum
{
    /// This value represents the BRC Functionality is closed
    AHP_AHS_BRC_DISABLED = 0x00,
    /// This value represents the BRC Functionlaity is opened
    AHP_AHS_BRC_ENABELD  = 0X01,
} AHP_AHS_BRC_STATUS_E;

/// Head Tracking Control Functionality status
/// Allow the user to turn head tracking (scene rotation) processing on/off via the client UI.
typedef enum
{
    /// This value represents the HTC Functionality is closed
    AHP_AHS_HTC_DISABLED = 0x00,
    AHP_AHS_HTC_ENABELD  = 0x01,
} AHP_AHS_HTC_STATUS_E;

/// Index of attributes for Advanced Headphone Service
enum ahp_ahs_att
{
    /// Advanced Headphone Service Declaration
    AHP_AHS_ATT_SVC,

    /// Binaural Rendoring Control Characteristic Declaration
    AHP_AHS_ATT_BRC_CHAR,
    /// Binaural Rendoring Control Characteristic Value
    AHP_AHS_ATT_BRC_VAL,
    /// Binaural Rendoring Control Client Characteristic Configuration Descriptor
    AHP_AHS_ATT_BRC_CHAR_CFG,

    /// Head Tracking Control Characteristic Declaration
    AHP_AHS_ATT_HTC_CHAR,
    /// Head Tracking Control Characteristic Value
    AHP_AHS_ATT_HTC_VAL,
    /// Head Tracking Control Characteristic Client Characteristic Configuration Descriptor
    AHP_AHS_ATT_HTC_CHAR_CFG,

    /// Maximum number of attributes that can be present in the service
    AHP_AHS_ATT_MAX_NB,
};

/// Messages for AHP Server Profile
enum ahps_msg_id
{
    /// AHP server BRC ccc changed
    AHPS_BRC_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_AHPS),

    /// AHP server HTC ccc changed
    AHPS_HTC_CCC_CHANGED,

    /// AHP server BRC Write without rsp
    AHPS_BRC_WRITE_CHAR_WITHOUT_RSP,

    /// AHP server HTC write without rsp
    AHPS_HTC_WRITE_CHAR_WITHOUT_RSP,

    /// AHP server BRC send notification
    AHPS_BRC_SEND_NOTIFICATION,

    /// AHP server HTC send notification
    AHPS_HTC_SEND_NOTIFICATION,
};

/*
 * STRUCTURE
 ****************************************************************************************
 */
/// Advanced Headphone Service Server environment structure
typedef struct ahp_ahs_env
{
    /// prf hdr
    prf_hdr_t hdr;
    /// Service start handle
    uint16_t shdl;
    /// GATT User local index
    uint8_t user_lid;
    ///state of different AHPS task instance
    ke_state_t state;

    /// flag tp mark whether AHP service BRC notification is enabled
    uint8_t isBRCNotificationEnabled[BLE_CONNECTION_MAX];
    /// Binaural Rendoring Control fucntionailty status
    uint8_t is_BRC_on;

    /// flag tp mark whether AHP service HTC notification is enabled
    uint8_t isHTCNotificationEnabled[BLE_CONNECTION_MAX];
    /// Head Tracking Control Fuctionailty status
    uint8_t is_HTC_on;
} ahp_ahs_env_t;

/// AHPS BRC notification config
struct ble_ahps_brc_notif_config_t
{
    bool is_BRCNotifEnabled;
};

/// AHPS HTC notification config
struct ble_ahps_htc_notif_config_t
{
    bool is_HTCNotifEnabled;
};

/// AHP Server BRC Rx data config
struct ble_ahps_brc_data_rx_ind_t
{
    uint8_t      BRC_status;
} ;

/// AHP Server HTC Rx data config
struct ble_ahps_htc_data_rx_ind_t
{
    uint8_t      HTC_status;
} ;

struct ble_ahps_brc_send_ntf_req_t
{
    uint8_t    connecionIndex;
    uint8_t    brc_status;
} ;

struct ble_ahps_htc_send_ntf_req_t
{
    uint8_t    connecionIndex;
    uint8_t    htc_status;
} ;


/*
 * Function Defination
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Retrieve AHP service profile interface
 *
 * @return AHP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* ahps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Retrieve AHP Server Task init interface
 *
 * @return AHP server Task init interface
 ****************************************************************************************
 */
void ahps_task_init(struct ke_task_desc *task_desc, void *p_env);


#endif /// (BLE_AHP_SERVER)
#endif /// _AHP_AHS_H_

