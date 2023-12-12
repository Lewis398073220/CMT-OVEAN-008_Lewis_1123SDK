/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#define APP_BLE_GATT_SUER_ID_INVALID 0xFF

/// APP BLE GATT EVENT 
typedef enum
{
    APP_BLE_GATT_OP_SVC_ADD=0,
    APP_BLE_GATT_OP_SVC_REMOVE,
    APP_BLE_GATT_OP_SVC_CTRL,
    APP_BLE_GATT_OP_SVC_GET_HASH,
    APP_BLE_GATT_OP_SVC_SEND_RELIABLE,
    APP_BLE_GATT_OP_SVC_SEND,
    APP_BLE_GATT_OP_SVC_SEND_MTP,
    APP_BLE_GATT_OP_SVC_SEND_MTP_CANCLE,
    APP_BLE_GATT_OP_CLI_DISCOVER,
    APP_BLE_GATT_OP_CLI_DISCOVER_INC,
    APP_BLE_GATT_OP_CLI_DISCOVER_CHAR,
    APP_BLE_GATT_OP_CLI_DISCOVER_DESC,
    APP_BLE_GATT_OP_CLI_DISCOVER_CANCEL,
    APP_BLE_GATT_OP_CLI_READ,
    APP_BLE_GATT_OP_CLI_TREA_BY_UUID,
    APP_BLE_GATT_OP_CLI_TREA_MULTIPLE,
    APP_BLE_GATT_OP_CLI_WRITE_RELIABLE,
    APP_BLE_GATT_OP_CLI_WEITE,
    APP_BLE_GATT_OP_CLI_WEITE_EXE,
    APP_BLE_GATT_OP_CLI_RECEIVE_CTRL,
    APP_BLE_GATT_OP_CLI_MTU_UPDATE,
} APP_BLE_GATT_CMP_OP_E;
/// APP BLE GATT EVENT 
typedef enum
{
    APP_BLE_GATT_CMP_EVENT = 0,
    /// when peer device requests to read an attribute.
    APP_BLE_GATT_SRV_READ_REQ_EVENT,
    /// When the user wants to send by reliable sending, get the event of sending data from the user layer
    APP_BLE_GATT_SRV_VAL_GET_REQ_EVENT,
    /// When the client writes reliably, it must first obtain the length of the value
    APP_BLE_GATT_SRV_GET_VAL_LEN_REQ_EVENT,
    /// Event triggered by GATT in order to inform GATT server user when an attribute value has been written by a
    /// peer device.
    APP_BLE_GATT_SRV_VAL_WRITE_REQ_EVENT,
    /// When the user wants to send by reliable write, get the event of sending data from the user layer
    APP_BLE_GATT_CLI_VAL_GET_REQ_EVENT,
    /// Inform GATT client user about reception of either a notification or an indication from peer device.
    APP_BLE_GATT_CLI_EVENT_REQ_EVENT,
    APP_BLE_GATT_CLI_DIS_SVC_EVENT,
    /// Event triggered when a service is found during service discovery procedure - Only Service information.
    APP_BLE_GATT_CLI_DIS_SVC_INFO_EVENT,
    /// Event triggered when an included service is found during discovery procedure.
    APP_BLE_GATT_CLI_DIS_INC_SVC_EVENT,
    /// Event triggered when a characteristic is found during discovery procedure.
    APP_BLE_GATT_CLI_DIS_CHAR_EVENT,
    /// Event triggered when a characteristic descriptor is found during discovery procedure.
    APP_BLE_GATT_CLI_DIS_DESC_EVENT,
    /// Event triggered when an attribute value has been read.
    APP_BLE_GATT_CLI_READ_RESP_EVENT,
    /// Event triggered when a service change has been received or if an attribute transaction triggers an
    /// out of sync error
    APP_BLE_GATT_CLI_SVC_CHANGED_EVENT,
    APP_BLE_GATT_EVENT_AMX,
} APP_BLE_GATT_EVENT_TYPE_E;
/// GATT User Role
typedef enum
{
    /// Client user role
    APP_BLE_GATT_ROLE_CLIENT = 0x00,
    /// Server user role
    APP_BLE_GATT_ROLE_SERVER = 0x01,
    /// Role not defined
    APP_BLE_GATT_ROLE_NONE   = 0xFF,
} APP_BLE_GATT_USER_ROLE_E;
///  GATT UUID Type
typedef enum
{
    /// 16-bit UUID value
    APP_BLE_GATT_UUID_16      = 0x00,
    /// 32-bit UUID value
    APP_BLE_GATT_UUID_32      = 0x01,
    /// 128-bit UUID value
    APP_BLE_GATT_UUID_128     = 0x02,
    /// Invalid UUID Type
    APP_BLE_GATT_UUID_INVALID = 0x03,
}APP_BLE_GATT_UUID_TYPE_E;
/// GATT attribute security level
typedef enum
{
    /// Attribute value is accessible on an unencrypted link.
    APP_BLE_GATT_SEC_NOT_ENC    = 0x00,
    /// Attribute value is accessible on an encrypted link or modified with using write signed procedure
    /// on unencrypted link if bonded using an unauthenticated pairing.
    APP_BLE_GATT_SEC_NO_AUTH    = 0x01,
    /// Attribute value is accessible on an encrypted link or modified with using write signed procedure
    /// on unencrypted link if bonded using an authenticated pairing.
    APP_BLE_GATT_SEC_AUTH       = 0x02,
    /// Attribute value is accessible on an encrypted link or modified with using write signed procedure
    /// on unencrypted link if bonded using a secure connection pairing.
    APP_BLE_GATT_SEC_SECURE_CON = 0x03,
}APP_BLE_GATT_SEC_LVL;
/// @verbatim
///    15   14    13  12 11 10  9  8   7    6    5   4   3    2    1    0
/// +-----+-----+---+---+--+--+--+--+-----+----+---+---+----+----+----+---+
/// | UUID_TYPE |  NIP  |  WP |  RP | EXT | WS | I | N | WR | WC | RD | B |
/// +-----+-----+---+---+--+--+--+--+-----+----+---+---+----+----+----+---+
///                                  <--------------- PROP -------------->
/// @endverbatim
/// GATT Attribute information Bit Field
typedef enum
{
    /// Broadcast descriptor present
    APP_BLE_GATT_ATT_B_BIT          = 0x0001,
    APP_BLE_GATT_ATT_B_POS          = 0,
    /// Read Access Mask
    APP_BLE_GATT_ATT_RD_BIT         = 0x0002,
    APP_BLE_GATT_ATT_RD_POS         = 1,
    /// Write Command Enabled attribute Mask
    APP_BLE_GATT_ATT_WC_BIT         = 0x0004,
    APP_BLE_GATT_ATT_WC_POS         = 2,
    /// Write Request Enabled attribute Mask
    APP_BLE_GATT_ATT_WR_BIT         = 0x0008,
    APP_BLE_GATT_ATT_WR_POS         = 3,
    /// Notification Access Mask
    APP_BLE_GATT_ATT_N_BIT          = 0x0010,
    APP_BLE_GATT_ATT_N_POS          = 4,
    /// Indication Access Mask
    APP_BLE_GATT_ATT_I_BIT          = 0x0020,
    APP_BLE_GATT_ATT_I_POS          = 5,
    /// Write Signed Enabled attribute Mask
    APP_BLE_GATT_ATT_WS_BIT         = 0x0040,
    APP_BLE_GATT_ATT_WS_POS         = 6,
    /// Extended properties descriptor present
    APP_BLE_GATT_ATT_EXT_BIT        = 0x0080,
    APP_BLE_GATT_ATT_EXT_POS        = 7,
    /// Read security level permission (see enum #APP_BLE_GATT_SEC_LVL).
    APP_BLE_GATT_ATT_RP_MASK        = 0x0300,
    APP_BLE_GATT_ATT_RP_LSB         = 8,
    /// Write security level permission (see enum #APP_BLE_GATT_SEC_LVL).
    APP_BLE_GATT_ATT_WP_MASK        = 0x0C00,
    APP_BLE_GATT_ATT_WP_LSB         = 10,
    /// Notify and Indication security level permission (see enum #APP_BLE_GATT_SEC_LVL).
    APP_BLE_GATT_ATT_NIP_MASK       = 0x3000,
    APP_BLE_GATT_ATT_NIP_LSB        = 12,
    /// Type of attribute UUID (see enum #APP_BLE_GATT_UUID_TYPE_E)
    APP_BLE_GATT_ATT_UUID_TYPE_MASK  = 0xC000,
    APP_BLE_GATT_ATT_UUID_TYPE_LSB   = 14,
    /// Attribute value property
    APP_BLE_GATT_ATT_PROP_MASK      = 0x00FF,
    APP_BLE_GATT_ATT_PROP_LSB       = 0,
}APP_BLE_GATT_ATT_INFO_E;
/// @verbatim
///       15     14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
/// +-----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// | NO_OFFSET |               WRITE_MAX_SIZE               |
/// +-----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                     INC_SVC_HANDLE                     |
/// +-----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                     EXT_PROP_VALUE                     |
/// +-----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// @endverbatim
///
/// GATT Attribute extended information Bit Field
typedef enum
{
    /// Maximum value authorized for an attribute write.
    /// Automatically reduce to Maximum Attribute value (GATT_MAX_VALUE) if greater
    APP_BLE_GATT_ATT_WRITE_MAX_SIZE_MASK  = 0x7FFF,
    APP_BLE_GATT_ATT_WRITE_MAX_SIZE_LSB   = 0,
    /// 1: Do not authorize peer device to read or write an attribute with an offset != 0
    /// 0: Authorize offset usage
    APP_BLE_GATT_ATT_NO_OFFSET_BIT        = 0x8000,
    APP_BLE_GATT_ATT_NO_OFFSET_POS        = 15,
    /// Include Service handle value
    APP_BLE_GATT_INC_SVC_HDL_BIT          = 0xFFFF,
    APP_BLE_GATT_INC_SVC_HDL_POS          = 0,
    /// Characteristic Extended Properties value
    APP_BLE_GATT_ATT_EXT_PROP_VALUE_MASK  = 0xFFFF,
    APP_BLE_GATT_ATT_EXT_PROP_VALUE_LSB   = 0,
}APP_BLE_GATT_ATT_XET_INFO_E;
/// @verbatim
///   7      6     5     4      3     2    1   0
/// +-----+-----+-----+------+-----+-----+---+---+
/// | RFU | UUID_TYPE | HIDE | DIS | EKS |  AUTH |
/// +-----+-----+-----+------+-----+-----+---+---+
/// @endverbatim
/// GATT information Bit Field
typedef enum
{
    /// Service minimum required security level (see enum #APP_BLE_GATT_SEC_LVL).
    APP_BLE_GATT_SVC_AUTH_MASK       = 0x03,
    APP_BLE_GATT_SVC_AUTH_LSB        = 0,
    /// If set, access to value with encrypted security requirement also requires a 128-bit encryption key size.
    APP_BLE_GATT_SVC_EKS_BIT         = 0x04,
    APP_BLE_GATT_SVC_EKS_POS         = 2,
    /// If set, service is visible but cannot be used by peer device
    APP_BLE_GATT_SVC_DIS_BIT         = 0x08,
    APP_BLE_GATT_SVC_DIS_POS         = 3,
    /// Hide the service
    APP_BLE_GATT_SVC_HIDE_BIT        = 0x10,
    APP_BLE_GATT_SVC_HIDE_POS        = 4,
    /// Type of service UUID (see enum #APP_BLE_GATT_UUID_TYPE_E)
    APP_BLE_GATT_SVC_UUID_TYPE_MASK  = 0x60,
    APP_BLE_GATT_SVC_UUID_TYPE_LSB   = 5,
}APP_BLE_GATT_SVC_INFO_E;
/// GATT Send Type
typedef enum
{
    /// Server initiated notification
    APP_BLE_GATT_NOTIFY     = 0x00,
    /// Server initiated indication
    APP_BLE_GATT_INDICATE   = 0x01,
}APP_BLE_GATT_SEND_TYPE_E;
/// GATT Discovery Type
typedef enum
{
    /// Discover all primary services
    APP_BLE_GATT_DISCOVER_SVC_PRIMARY_ALL         = 0x00,
    /// Discover primary services using UUID value
    APP_BLE_GATT_DISCOVER_SVC_PRIMARY_BY_UUID     = 0x01,
    /// Discover all secondary services
    APP_BLE_GATT_DISCOVER_SVC_SECONDARY_ALL       = 0x02,
    /// Discover secondary services using UUID value
    APP_BLE_GATT_DISCOVER_SVC_SECONDARY_BY_UUID   = 0x03,
}APP_BLE_GATT_DISCOVERY_TYPE_E;
/// GATT Characteristic Discovery Type
typedef enum
{
    /// Discover all characteristics
    APP_BLE_GATT_DISCOVER_CHAR_ALL      = 0x00,
    /// Discover characteristics using UUID value
    APP_BLE_GATT_DISCOVER_CHAR_BY_UUID  = 0x01,
}APP_BLE_GATT_DISCOVERY_CHAR_TYPE_E;
/// GATT Write Type
typedef enum
{
    /// Write attribute
    APP_BLE_GATT_WRITE              = 0x00,
    /// Write attribute without response
    APP_BLE_GATT_WRITE_NO_RESP      = 0x01,
    /// Write attribute signed
    APP_BLE_GATT_WRITE_SIGNED       = 0x02,
}APP_BLE_GATT_WRITE_TYPE_E;
/// Write execution mode
typedef enum
{
    /// Perform automatic write execution
    APP_BLE_GATT_WRITE_MODE_AUTO_EXECUTE    = 0x00,
    /// Force to use prepare write queue. Can be used to write multiple attributes
    APP_BLE_GATT_WRITE_MODE_QUEUE    = 0x01,
}APP_BLE_GATT_WRITE_MODE_E;
/// GATT service discovery information
typedef enum 
{
    /// Complete service present in structure
    APP_BLE_GATT_SVC_CMPLT = 0x00,
    /// First service attribute present in structure
    APP_BLE_GATT_SVC_START = 0x01,
    /// Last service attribute present in structure
    APP_BLE_GATT_SVC_END   = 0x02,
    /// Following service attribute present in structure
    APP_BLE_GATT_SVC_CONT  = 0x03,
}APP_BLE_GATT_SVC_DISC_INFO_E;
/// Service Discovery Attribute type
typedef enum
{
    /// No Attribute Information
    APP_BLE_GATT_ATT_NONE           = 0x00,
    /// Primary service attribute
    APP_BLE_GATT_ATT_PRIMARY_SVC    = 0x01,
    /// Secondary service attribute
    APP_BLE_GATT_ATT_SECONDARY_SVC  = 0x02,
    /// Included service attribute
    APP_BLE_GATT_ATT_INCL_SVC       = 0x03,
    /// Characteristic declaration
    APP_BLE_GATT_ATT_CHAR           = 0x04,
    /// Attribute value
    APP_BLE_GATT_ATT_VAL            = 0x05,
    /// Attribute descriptor
    APP_BLE_GATT_ATT_DESC           = 0x06,
}APP_BLE_GATT_ATT_TYPE_E;
/// Attribute Description structure
typedef struct
{
    /// Attribute UUID (LSB First)
    uint8_t  uuid[16];
    /// Attribute information bit field (see enum #APP_BLE_GATT_ATT_INFO_E)
    uint16_t info;
    /// Attribute extended information bit field (see enum #APP_BLE_GATT_ATT_XET_INFO_E)
    /// Note:
    ///   - For Included Services and Characteristic Declarations, this field contains targeted handle.
    ///   - For Characteristic Extended Properties, this field contains 2 byte value
    ///   - For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used.
    uint16_t ext_info;
} app_ble_gatt_att_desc_t;
typedef struct{
    /// info:@verbatim, see @APP_BLE_GATT_SVC_INFO_E
    uint8_t info ;
    uint8_t p_uuid[16];
    uint8_t nb_att;
    app_ble_gatt_att_desc_t * p_atts;
}app_ble_gatt_add_server_t;
/// Attribute structure
typedef struct
{
    /// Attribute handle
    uint16_t hdl;
    /// Value length
    uint16_t length;
} app_ble_gatt_att_t;
/// GATT_SRV_EVENT_RELIABLE_SEND Command structure definition
typedef struct
{
    /// Connection index
    uint8_t             conidx;
    /// Event type to trigger (see enum #APP_BLE_GATT_SEND_TYPE_E)
    uint8_t             evt_type;
    /// Number of attribute
    uint8_t             nb_att;
    /// List of attribute
    app_ble_gatt_att_t  *atts;
} app_ble_gatt_srv_send_reliable_t;
/// GATT_SRV_EVENT_SEND Command structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Event type to trigger (see enum #APP_BLE_GATT_SEND_TYPE_E)
    uint8_t         evt_type;
    /// Attribute handle
    uint16_t        hdl;
    /// Value length
    uint16_t        value_length;
    /// Value to transmit
    uint8_t         *value;
} app_ble_gatt_srv_send_t;
/// GATT_SRV_EVENT_MTP_SEND Command structure definition
typedef struct
{
    /// Connection index bit field
    uint32_t        conidx_bf;
    /// Event type to trigger (see enum #APP_BLE_GATT_SEND_TYPE_E)
    uint8_t         evt_type;
    /// Attribute handle
    uint16_t        hdl;
    /// Value length
    uint16_t        value_length;
    /// Value to transmit
    uint8_t         *value;
} app_ble_gatt_srv_send_mtp_t;
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Search start handle
    uint16_t        start_hdl;
    /// Search end handle
    uint16_t        end_hdl;
}app_ble_gatt_cli_discover_general_t;
/// structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// GATT Service discovery type (see enum #APP_BLE_GATT_DISCOVERY_TYPE_E)
    uint8_t         disc_type;
    /// Perform discovery of all information present in the service  (1: enable, 0: disable)
    uint8_t         full;
    /// Search start handle
    uint16_t        start_hdl;
    /// Search end handle
    uint16_t        end_hdl;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Searched Service UUID (meaningful only for discovery by UUID)
    uint8_t         uuid[16];
} app_ble_gatt_cli_discover_t;
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// GATT characteristic discovery type (see enum #APP_BLE_GATT_DISCOVERY_CHAR_TYPE_E)
    uint8_t         disc_type;
    /// Search start handle
    uint16_t        start_hdl;
    /// Search end handle
    uint16_t        end_hdl;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Searched UUID (meaningful only for discovery by UUID)
    uint8_t         uuid[16];
} app_ble_gatt_cli_discover_char_t;
/// GATT_CLI_READ Command structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Value offset
    uint16_t        offset;
    /// Value length to read (0 = read complete value)
    uint16_t        length;
} app_ble_gatt_cli_read_t;
/// GATT_CLI_READ_BY_UUID Command structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Search start handle
    uint16_t        start_hdl;
    /// Search end handle
    uint16_t        end_hdl;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Searched UUID
    uint8_t         uuid[16];
} app_ble_gatt_cli_read_by_uuid_t;
/// GATT_CLI_READ_MULTIPLE Command structure definition
typedef struct
{
    /// Connection index
    uint8_t             conidx;
    /// Number of attribute
    uint8_t             nb_att;
    /// List of attribute
    /// If Attribute length is zero (consider length unknown):
    ///     - Attribute protocol read multiple variable length procedure used
    app_ble_gatt_att_t  *atts;
} app_ble_gatt_cli_read_multiple_t;
/// GATT_CLI_WRITE_RELIABLE Command structure definition
typedef struct app_ble_gatt_cli_write_reliable
{
    /// Connection index
    uint8_t         conidx;
    /// GATT write type (see enum #APP_BLE_GATT_WRITE_TYPE_E)
    uint8_t         write_type;
    /// Write execution mode (see enum #APP_BLE_GATT_WRITE_MODE_E). Valid only for GATT_WRITE.
    uint8_t         write_mode;
    /// Attribute handle
    uint16_t        hdl;
    /// Value offset, valid only for GATT_WRITE
    uint16_t        offset;
    /// Value length to write
    uint16_t        length;
} app_ble_gatt_cli_write_reliable_t;
/// GATT_CLI_WRITE Command structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// GATT write type (see enum #APP_BLE_GATT_WRITE_TYPE_E)
    uint8_t         write_type;
    /// Attribute handle
    uint16_t        hdl;
    /// Value offset, valid only for GATT_WRITE
    uint16_t        offset;
    /// Value length to write
    uint16_t        value_length;
    /// Attribute value
    uint8_t         *value;
} app_ble_gatt_cli_write_t;
/// Receive (notification / indication) control structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// GATT client receive control, 0:disenable receive, 1:receive
    uint8_t         en;
    /// Search start handle
    uint16_t        start_hdl;
    /// Search end handle
    uint16_t        end_hdl;
} app_ble_gatt_cli_rec_ctrl_t;
typedef union{
    /// operation = APP_BLE_GATT_OP_SVC_ADD
    //  Service start handle associated to created service.
    uint16_t        start_hdl;
    /// operation = APP_BLE_GATT_OP_SVC_GET_HASH
    /// Database Hash.
    uint8_t         hash[16];
    /// operation = APP_BLE_GATT_OP_SVC_SEND_RELIABLE,
    /// operation = APP_BLE_GATT_OP_SVC_SEND,
    /// operation = APP_BLE_GATT_OP_SVC_SEND_MTP,
    /// operation = APP_BLE_GATT_OP_SVC_SEND_MTP_CANCLE
    /// operation = APP_BLE_GATT_OP_CLI_DISCOVER,
    /// operation = APP_BLE_GATT_OP_CLI_DISCOVER_INC,
    /// operation = APP_BLE_GATT_OP_CLI_DISCOVER_CHAR,
    /// operation = APP_BLE_GATT_OP_CLI_DISCOVER_DESC,
    /// operation = APP_BLE_GATT_OP_CLI_DISCOVER_CANCEL
    /// operation = APP_BLE_GATT_OP_CLI_WEITE,
    /// operation = APP_BLE_GATT_OP_CLI_WEITE_EXE,
    /// operation = APP_BLE_GATT_OP_CLI_RECEIVE_CTRL
    /// Connection index
    uint8_t         conidx;
}app_ble_gatt_cmp_param_u;
typedef struct{
    /// operation complete , see@APP_BLE_GATT_CMP_OP_E
    uint8_t                   operation;
    /// complete status, 0: success, other: fail
    uint16_t                  status;
    /// complete operation parameter
    app_ble_gatt_cmp_param_u  param;
}app_ble_gatt_cmp_event_t;
/// GATT_SRV_ATT_READ_GET request indication structure
typedef struct{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Value offset
    uint16_t        offset;
    /// Maximum value length to return
    uint16_t        max_length;
}app_ble_gatt_svc_read_req_event_t;
/// reliable send value get
typedef struct
{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Maximum value length to return
    uint16_t        max_length;
} app_ble_gatt_srv_val_get_event_t;
/// GATT_SRV_ATT_READ_GET confirm structure
typedef struct
{
    /// Token provided by GAT module into the GATT_REQ_IND message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Status of the request by GATT user (see enum #hl_err)
    uint16_t        status;
    /// Complete Length of the attribute value
    uint16_t        att_length;
    /// Value length
    uint16_t        value_length;
    /// Attribute value (starting from data offset)
    uint8_t         *value;
} app_ble_gatt_svc_val_basic_resp_t;
/// When the client writes reliably, it must first obtain the length of the value
typedef struct
{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
} app_ble_gatt_srv_get_val_len_event_t;
/// GATT_SRV_ATT_INFO_GET confirm structure
typedef struct
{
    /// Token provided by GAT module into the GATT_REQ_IND message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Status of the request by GATT user (see enum #hl_err)
    uint16_t        status;
    /// Attribute Value length
    uint16_t        att_length;
} app_ble_gatt_srv_get_val_len_resp_t;
/// GATT_SRV_ATT_VAL_SET request indication structure
typedef struct
{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Value offset
    uint16_t        offset;
    /// Value length to write
    uint16_t        value_length;
    /// Attribute value to update (starting from offset)
    uint8_t         *value;
} app_ble_gatt_srv_write_req_event_t;
/// GATT_SRV_ATT_VAL_SET confirm structure
typedef struct
{
    /// Token provided by GAT module into the GATT_REQ_IND message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Status of the request by GATT user (see enum #hl_err)
    uint16_t        status;
} app_ble_gatt_srv_write_resp_t;
/// GATT_CLI_ATT_VAL_GET request indication structure
typedef struct
{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Data offset
    uint16_t        offset;
    /// Maximum value length to return
    uint16_t        max_length;
} app_ble_gatt_cli_get_val_req_event_t;
/// GATT_CLI_ATT_VAL_GET confirm structure
typedef struct
{
    /// Token provided by GAT module into the GATT_REQ_IND message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Status of the request by GATT user (see enum #hl_err)
    uint16_t        status;
    /// Value length
    uint16_t        value_length;
    /// Attribute value (starting from data offset)
    uint8_t         *value;
} app_ble_gatt_cli_get_val_resp_t;
/// GATT_CLI_ATT_EVENT request indication structure
typedef struct
{
    /// Token provided by GATT module that must be used into the GAT_CFM message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Event type triggered (see enum #APP_BLE_GATT_SEND_TYPE_E)
    uint8_t         evt_type;
    /// 1: if event value if complete value has been received
    /// 0: if data received is equals to maximum attribute protocol value. In such case GATT Client User should
    ///    perform a read procedure.
    uint8_t         complete;
    /// Attribute handle
    uint16_t        hdl;
    /// Value length
    uint16_t        value_length;
    /// Attribute value
    uint8_t         *value;
} app_ble_gatt_cli_req_event_t;
/// GATT_SRV_ATT_VAL_SET confirm structure
typedef struct
{
    /// Token provided by GAT module into the GATT_REQ_IND message
    uint16_t        token;
    /// Connection index
    uint8_t         conidx;
    /// Status of the request by GATT user (see enum #hl_err)
    uint16_t        status;
} app_ble_gatt_cli_event_resp_t;
/// Attribute info structure for Service Discovery
typedef struct 
{
    /// Attribute Type (see enum #APP_BLE_GATT_ATT_TYPE_E)
    uint8_t  att_type;
    /// UUID type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t  uuid_type;
    /// UUID - LSB first (0 for GATT_ATT_NONE)
    uint8_t  uuid[16];
    /// Information about Service attribute union
    union app_ble_gatt_info
    {
        /// Service info structure
        //#APP_BLE_GATT_ATT_PRIMARY_SVC
        //#APP_BLE_GATT_ATT_SECONDARY_SVC
        //#APP_BLE_GATT_ATT_INCL_SVC)
        struct app_ble_gatt_svc_info
        {
            /// Service start handle
            uint16_t start_hdl;
            /// Service end handle
            uint16_t end_hdl;
        } svc;
        /// Characteristic info structure
        //#APP_BLE_GATT_ATT_CHAR
        struct app_ble_gatt_char_info
        {
            /// Value handle
            uint16_t val_hdl;
            /// Characteristic properties (see enum #APP_BLE_GATT_ATT_INFO_E - bits [0-7])
            uint8_t  prop;
        } charac;
    } info; //!< Information about Service attribute
} app_ble_gatt_svc_att_t;
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// First handle value of following list
    uint16_t        hdl;
    /// Discovery information (see enum #APP_BLE_GATT_SVC_DISC_INFO_E)
    uint8_t         disc_info;
    /// Number of attributes
    uint8_t         nb_att;
    /// Attribute information present in a service
    app_ble_gatt_svc_att_t *atts;
} app_ble_gatt_cli_dis_svc_event_t;
/// GATT_CLI_SVC_INFO Indication structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Service start handle
    uint16_t        start_hdl;
    /// Service end handle
    uint16_t        end_hdl;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Service UUID (LSB first)
    uint8_t         uuid[16];
} app_ble_gatt_cli_dis_svc_basic_event_t;
/// GATT_CLI_CHAR Indication structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Characteristic attribute handle
    uint16_t        char_hdl;
    /// Value handle
    uint16_t        val_hdl;
    /// Characteristic properties (see enum #APP_BLE_GATT_ATT_INFO_E - bits [0-7])
    uint8_t         prop;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Characteristic value UUID - LSB first
    uint8_t         uuid[16];
}app_ble_gatt_cli_dis_char_event_t;
/// GATT_CLI_DESC Indication structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Characteristic descriptor attribute handle
    uint16_t        desc_hdl;
    /// UUID Type (see enum #APP_BLE_GATT_UUID_TYPE_E)
    uint8_t         uuid_type;
    /// Attribute UUID - LSB first
    uint8_t         uuid[16];
} app_ble_gatt_cli_dis_desc_event_t;
/// GATT_CLI_ATT_VAL Indication structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// Attribute handle
    uint16_t        hdl;
    /// Data offset
    uint16_t        offset;
    /// Value length
    uint16_t        value_length;
    /// Attribute value starting from offset
    uint8_t         *value;
} app_ble_gatt_cli_read_resp_event_t;
/// GATT_CLI_SVC_CHANGED Indication structure definition
typedef struct
{
    /// Connection index
    uint8_t         conidx;
    /// True if an out of sync error has been received
    uint8_t         out_of_sync;
    /// Service start handle
    uint16_t        start_hdl;
    /// Service end handle
    uint16_t        end_hdl;
} app_ble_gatt_cli_svc_changed_event_t;
typedef union{
    app_ble_gatt_cmp_event_t               cmp;
    app_ble_gatt_svc_read_req_event_t      svc_read_req;
    app_ble_gatt_srv_val_get_event_t       svc_val_get_req;
    app_ble_gatt_srv_get_val_len_event_t   svc_get_val_len_req;
    app_ble_gatt_srv_write_req_event_t     svc_write_req;
    app_ble_gatt_cli_get_val_req_event_t   cli_get_val_req;
    app_ble_gatt_cli_req_event_t           cli_event_req;
    app_ble_gatt_cli_dis_svc_event_t       cli_dis_svc;
    app_ble_gatt_cli_dis_svc_basic_event_t cli_dis_svc_info;
    app_ble_gatt_cli_dis_svc_basic_event_t cli_dis_svc_inc;
    app_ble_gatt_cli_dis_char_event_t      cli_dis_char;
    app_ble_gatt_cli_dis_desc_event_t      cli_dis_desc;
    app_ble_gatt_cli_read_resp_event_t     cli_read_resp;
    app_ble_gatt_cli_svc_changed_event_t   cli_svc_change;
}app_ble_gatt_event_param_u;
typedef void (*app_ble_gatt_event_cb)(APP_BLE_GATT_EVENT_TYPE_E type, app_ble_gatt_event_param_u *param);
extern const struct app_subtask_handlers app_gatt_handler;
/// Register the app ble gatt to the ble host, not open to the upper layer
void app_ble_gatt_reg(APP_BLE_GATT_USER_ROLE_E role);
void app_ble_gatt_unreg(APP_BLE_GATT_USER_ROLE_E role);
void app_ble_gatt_set_mtu(uint8_t conidx, uint16_t mtu);

/// app ble gatt service api
void app_ble_gatt_svc_add(uint8_t gatt_user_id, app_ble_gatt_add_server_t *svc_info);
void app_ble_gatt_svc_remove(uint8_t gatt_user_id, uint16_t          start_hdl);
void app_ble_gatt_svc_ctrl(uint8_t gatt_user_id, uint8_t enable, uint8_t visible, uint16_t start_hdl);
void app_ble_gatt_svc_get_hash(uint8_t gatt_user_id);
void app_ble_gatt_svc_send_reliable(uint8_t gatt_user_id, app_ble_gatt_srv_send_reliable_t *param);
void app_ble_gatt_svc_send(uint8_t gatt_user_id, app_ble_gatt_srv_send_t *param);
void app_ble_gatt_svc_send_mtp(uint8_t gatt_user_id, app_ble_gatt_srv_send_mtp_t *param);
void app_ble_gatt_svc_send_mtp_cancel(uint8_t gatt_user_id);
void app_ble_gatt_svc_read_resp(app_ble_gatt_svc_val_basic_resp_t *param);
void app_ble_gatt_svc_val_get_resp(app_ble_gatt_svc_val_basic_resp_t *param);
void app_ble_gatt_svc_get_val_len_resp(app_ble_gatt_srv_get_val_len_resp_t *param);
void app_ble_gatt_svc_write_resp(app_ble_gatt_srv_write_resp_t *param);
/// app ble gatt client api
void app_ble_gatt_cli_discover(uint8_t gatt_user_id, app_ble_gatt_cli_discover_t *param);
void app_ble_gatt_cli_discover_included(uint8_t gatt_user_id, app_ble_gatt_cli_discover_general_t *param);
void app_ble_gatt_cli_discover_char(uint8_t gatt_user_id, app_ble_gatt_cli_discover_char_t *param);
void app_ble_gatt_cli_discover_desc(uint8_t gatt_user_id, app_ble_gatt_cli_discover_general_t *param);
void app_ble_gatt_cli_discover_cancel(uint8_t gatt_user_id, uint8_t conidx);
void app_ble_gatt_cli_read(uint8_t gatt_user_id, app_ble_gatt_cli_read_t *param);
void app_ble_gatt_cli_read_by_uuid(uint8_t gatt_user_id, app_ble_gatt_cli_read_by_uuid_t *param);
void app_ble_gatt_cli_read_multiple(uint8_t gatt_user_id, app_ble_gatt_cli_read_multiple_t *param);
void app_ble_gatt_cli_write_reliable(uint8_t gatt_user_id, app_ble_gatt_cli_write_reliable_t *param);
void app_ble_gatt_cli_write(uint8_t gatt_user_id, app_ble_gatt_cli_write_t *param);
void app_ble_gatt_cli_write_exe(uint8_t gatt_user_id, uint8_t conidx, uint8_t execute);
void app_ble_gatt_cli_receive_ctrl(uint8_t gatt_user_id, app_ble_gatt_cli_rec_ctrl_t *param);
void app_ble_gatt_cli_mtu_update(uint8_t gatt_user_id, uint8_t conidx);
void app_ble_gatt_cli_get_val_resp(app_ble_gatt_cli_get_val_resp_t *param);
void app_ble_gatt_cli_event_resp(app_ble_gatt_cli_event_resp_t *param);
/// app ble gatt event register api
uint8_t app_ble_gatt_user_reg(app_ble_gatt_event_cb cb);
void    app_ble_gatt_user_unreg(uint8_t gatt_user_id);
void    app_ble_gatt_get_mtu(uint8_t conidx, uint16_t *mtu);

