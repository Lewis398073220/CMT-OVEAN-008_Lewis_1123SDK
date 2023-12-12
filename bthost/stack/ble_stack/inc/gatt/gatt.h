/**
 ****************************************************************************************
 *
 * @file gatt.h
 *
 * @brief Header file - GATT Native API.
 *
 * Copyright (C) RivieraWaves 2009-2019
 ****************************************************************************************
 */

#ifndef GATT_H_
#define GATT_H_

/**
 ****************************************************************************************
 * @addtogroup GATT_API Generic Attribute Profile (GATT)
 * @brief  The GATT module is responsible for service and client attribute protocol handling
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GATT_NATIVE_API Native API
 * @ingroup GATT_API
 * @brief Function based API
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "compiler.h"

#include <stdbool.h>
#include "gatt_msg.h"
#include "l2cap_ble.h"
#include "co_buf.h"

#include "ble_gatt_common.h"

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */
/// helper macro to define an attribute property
/// @param prop Property see enum #gatt_prop_bf
#define PROP(prop)          (GATT_PROP_##prop##_BIT)


/// helper macro to define an attribute option bit
/// @param opt see enum #gatt_att_info_bf or see enum #gatt_att_ext_info_bf
#define OPT(opt)          (GATT_ATT_##opt##_BIT)

/// helper macro to set attribute security level on a specific permission
/// @param  lvl_name Security level see enum #gatt_sec_lvl
/// @param  perm     Permission see enum #gatt_att_info_bf (only RP, WP, NIP authorized)
#define SEC_LVL(perm, lvl_name)  (((GATT_SEC_##lvl_name) << (GATT_ATT_##perm##_LSB)) & (GATT_ATT_##perm##_MASK))

/// helper macro to set attribute security level on a specific permission
/// @param  lvl_val  Security level value
/// @param  perm     Permission see enum #gatt_att_info_bf (only RP, WP, NIP authorized)
#define SEC_LVL_VAL(perm, lvl_val)  (((lvl_val) << (GATT_ATT_##perm##_LSB)) & (GATT_ATT_##perm##_MASK))

/// helper macro to set attribute UUID type
/// @param uuid_type UUID type (16, 32, 128)
#define ATT_UUID(uuid_type)      (((GATT_UUID_##uuid_type) << (GATT_ATT_UUID_TYPE_LSB)) & (GATT_ATT_UUID_TYPE_MASK))

/// helper macro to set service security level
/// @param  lvl_name Security level see enum #gatt_sec_lvl
#define SVC_SEC_LVL(lvl_name)    (((GATT_SEC_##lvl_name) << (GATT_SVC_AUTH_LSB)) & (GATT_SVC_AUTH_MASK))
/// helper macro to set service security level
/// @param  lvl_val Security level value
#define SVC_SEC_LVL_VAL(lvl_val)    (((lvl_val) << (GATT_SVC_AUTH_LSB)) & (GATT_SVC_AUTH_MASK))

/// helper macro to set service UUID type
/// @param uuid_type UUID type (16, 32, 128)
#define SVC_UUID(uuid_type)      (((GATT_UUID_##uuid_type) << (GATT_SVC_UUID_TYPE_LSB)) & (GATT_SVC_UUID_TYPE_MASK))

/*
 * DEFINES
 ****************************************************************************************
 */

/// Length of Attribute signature
#define GATT_SIGNATURE_LEN              (12)

/// Length of Notification header length used for ATT transmission
#define GATT_NTF_HEADER_LEN             (1 + GATT_HANDLE_LEN)
/// Length of Write no response header length used for ATT transmission
#define GATT_WRITE_NO_RESP_HEADER_LEN   (1 + GATT_HANDLE_LEN)
/// Length of Write header length used for ATT transmission
#define GATT_WRITE_HEADER_LEN           (1 + GATT_HANDLE_LEN)

/// Invalid GATT user local index
#define GATT_INVALID_USER_LID           (0xFF)
/// Maximum number of handle that can be simultaneously read.
#define GATT_RD_MULTIPLE_MAX_NB_ATT     (8)

/// Length of Content Control ID
#define GATT_CCID_LEN                   (1)
/// Buffer Header length that must be reserved for GATT processing
#define GATT_BUFFER_HEADER_LEN          (L2CAP_BUFFER_HEADER_LEN + 7) // required for attribute packing
/// Buffer Tail length that must be reserved for GATT processing
#define GATT_BUFFER_TAIL_LEN            (L2CAP_BUFFER_TAIL_LEN)
/// Buffer Tail length that must be reserved for GATT Write signed processing
#define GATT_BUFFER_SIGN_TAIL_LEN       (L2CAP_BUFFER_TAIL_LEN + GATT_SIGNATURE_LEN)


/// extended characteristics
#define GATT_EXT_RELIABLE_WRITE         (0x0001)
/// extended writable auxiliary
#define GATT_EXT_WRITABLE_AUX           (0x0002)
/// extended reserved for future use
#define GATT_EXT_RFU                    (0xFFFC)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @verbatim
///    7    6    5   4   3    2    1    0
/// +-----+----+---+---+----+----+----+---+
/// | EXT | WS | I | N | WR | WC | RD | B |
/// +-----+----+---+---+----+----+----+---+
/// @endverbatim
/// GATT Attribute properties Bit Field
enum gatt_prop_bf
{
    /// Broadcast descriptor present
    GATT_PROP_B_BIT          = 0x0001,
    GATT_PROP_B_POS          = 0,
    /// Read Access Mask
    GATT_PROP_RD_BIT          = 0x0002,
    GATT_PROP_RD_POS          = 1,
    /// Write Command Enabled attribute Mask
    GATT_PROP_WC_BIT         = 0x0004,
    GATT_PROP_WC_POS         = 2,
    /// Write Request Enabled attribute Mask
    GATT_PROP_WR_BIT         = 0x0008,
    GATT_PROP_WR_POS         = 3,
    /// Notification Access Mask
    GATT_PROP_N_BIT          = 0x0010,
    GATT_PROP_N_POS          = 4,
    /// Indication Access Mask
    GATT_PROP_I_BIT          = 0x0020,
    GATT_PROP_I_POS          = 5,
    /// Write Signed Enabled attribute Mask
    GATT_PROP_WS_BIT         = 0x0040,
    GATT_PROP_WS_POS         = 6,
    /// Extended properties descriptor present
    GATT_PROP_EXT_BIT        = 0x0080,
    GATT_PROP_EXT_POS        = 7,
};

/// Format for Characteristic Presentation
enum gatt_format
{
    /// unsigned 1-bit: true or false
    GATT_FORMAT_BOOL     = 0x01,
    /// unsigned 2-bit integer
    GATT_FORMAT_2BIT,
    /// unsigned 4-bit integer
    GATT_FORMAT_NIBBLE,
    /// unsigned 8-bit integer
    GATT_FORMAT_UINT8,
    /// unsigned 12-bit integer
    GATT_FORMAT_UINT12,
    /// unsigned 16-bit integer
    GATT_FORMAT_UINT16,
    /// unsigned 24-bit integer
    GATT_FORMAT_UINT24,
    /// unsigned 32-bit integer
    GATT_FORMAT_UINT32,
    /// unsigned 48-bit integer
    GATT_FORMAT_UINT48,
    /// unsigned 64-bit integer
    GATT_FORMAT_UINT64,
    /// unsigned 128-bit integer
    GATT_FORMAT_UINT128,
    /// signed 8-bit integer
    GATT_FORMAT_SINT8,
    /// signed 12-bit integer
    GATT_FORMAT_SINT12,
    /// signed 16-bit integer
    GATT_FORMAT_SINT16,
    /// signed 24-bit integer
    GATT_FORMAT_SINT24,
    /// signed 32-bit integer
    GATT_FORMAT_SINT32,
    /// signed 48-bit integer
    GATT_FORMAT_SINT48,
    /// signed 64-bit integer
    GATT_FORMAT_SINT64,
    /// signed 128-bit integer
    GATT_FORMAT_SINT128,
    /// IEEE-754 32-bit floating point
    GATT_FORMAT_FLOAT32,
    /// IEEE-754 64-bit floating point
    GATT_FORMAT_FLOAT64,
    /// IEEE-11073 16-bit SFLOAT
    GATT_FORMAT_SFLOAT,
    /// IEEE-11073 32-bit FLOAT
    GATT_FORMAT_FLOAT,
    /// IEEE-20601 format
    GATT_FORMAT_DUINT16,
    /// UTF-8 string
    GATT_FORMAT_UTF8S,
    /// UTF-16 string
    GATT_FORMAT_UTF16S,
    /// Opaque structure
    GATT_FORMAT_STRUCT,
};

/// Client Characteristic Configuration Codes
enum gatt_ccc_val
{
    /// Stop notification/indication
    GATT_CCC_STOP_NTFIND = 0x0000,
    /// Start notification
    GATT_CCC_START_NTF   = 0x0001,
    /// Start indication
    GATT_CCC_START_IND   = 0x0002,
};

/*
 * CALLBACK DEFINITION
 ****************************************************************************************
 */

/// 16-bit UUID Attribute Description structure
typedef struct gatt_att16_desc
{
    /// Attribute UUID (16-bit UUID - LSB First)
    uint16_t uuid16;
    /// Attribute information bit field (see enum #gatt_att_info_bf)
    uint16_t info;
    /// Attribute extended information bit field (see enum #gatt_att_ext_info_bf)
    /// Note:
    ///   - For Included Services and Characteristic Declarations, this field contains targeted handle.
    ///   - For Characteristic Extended Properties, this field contains 2 byte value
    ///   - For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used.
    uint16_t ext_info;
} gatt_att16_desc_t;


/// GATT Database Hash callback set
typedef struct gatt_db_hash_cb
{
    /**
     ****************************************************************************************
     * @brief This function is called when hash value for local attribute database hash has
     *        been computed.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution.
     * @param[in] status        Status of the operation (see enum #hl_err)
     * @param[in] p_hash        Pointer to the 128-bit database hash value
     ****************************************************************************************
     */
    void (*cb_db_hash) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status, const uint8_t* p_hash);
} gatt_db_hash_cb_t;


/// GATT server user callback set
typedef struct gatt_srv_cb
{
    /**
     ****************************************************************************************
     * @brief This function is called when GATT server user has initiated event send to peer
     *        device or if an error occurs.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] status        Status of the procedure (see enum #hl_err)
     ****************************************************************************************
     */
    void (*cb_event_sent) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief This function is called when peer want to read local attribute database value.
     *
     *        #gatt_srv_att_read_get_cfm shall be called to provide attribute value
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] hdl           Attribute handle
     * @param[in] offset        Value offset
     * @param[in] max_length    Maximum value length to return
     ****************************************************************************************
     */
    void (*cb_att_read_get) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                             uint16_t max_length);

    /**
     ****************************************************************************************
     * @brief This function is called when GATT server user has initiated event send procedure,
     *
     *        #gatt_srv_att_event_get_cfm shall be called to provide attribute value
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution.
     * @param[in] hdl           Attribute handle
     * @param[in] max_length    Maximum value length to return
     ****************************************************************************************
     */
    void (*cb_att_event_get) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy, uint16_t hdl,
                              uint16_t max_length);

    /**
     ****************************************************************************************
     * @brief This function is called during a write procedure to get information about a
     *        specific attribute handle.
     *
     *        #gatt_srv_att_info_get_cfm shall be called to provide attribute information
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] hdl           Attribute handle
     ****************************************************************************************
     */
    void (*cb_att_info_get) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl);

    /**
     ****************************************************************************************
     * @brief This function is called during a write procedure to modify attribute handle.
     *
     *        #gatt_srv_att_val_set_cfm shall be called to accept or reject attribute
     *        update.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] hdl           Attribute handle
     * @param[in] offset        Value offset
     * @param[in] p_data        Pointer to buffer that contains data to write starting from offset
     ****************************************************************************************
     */
    void (*cb_att_val_set) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                            co_buf_t* p_data);
} gatt_srv_cb_t;



/// GATT client user callback set
typedef struct gatt_cli_cb
{
    /**
     ****************************************************************************************
     * @brief This function is called when GATT client user discovery procedure is over.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] status        Status of the procedure (see enum #hl_err)
     ****************************************************************************************
     */
    void (*cb_discover_cmp) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief This function is called when GATT client user read procedure is over.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] status        Status of the procedure (see enum #hl_err)
     ****************************************************************************************
     */
    void (*cb_read_cmp) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief This function is called when GATT client user write procedure is over.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] status        Status of the procedure (see enum #hl_err)
     ****************************************************************************************
     */
    void (*cb_write_cmp) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

    /**
     ****************************************************************************************
     * @brief This function is called when GATT client user has initiated a write procedure.
     *
     *        #gatt_cli_att_val_get_cfm shall be called to provide attribute value.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution - 0x0000 else.
     * @param[in] hdl           Attribute handle
     * @param[in] offset        Value offset
     * @param[in] max_length    Maximum value length to return
     ****************************************************************************************
     */
    void (*cb_att_val_get) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy,
                            uint16_t hdl, uint16_t offset, uint16_t max_length);

    /**
     ****************************************************************************************
     * @brief This function is called when a full service has been found during a discovery procedure.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] hdl           First handle value of following list
     * @param[in] disc_info     Discovery information (see enum #gatt_svc_disc_info)
     * @param[in] nb_att        Number of attributes
     * @param[in] p_atts        Pointer to attribute information present in a service
     ****************************************************************************************
     */
    void (*cb_svc) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint8_t disc_info,
                    uint8_t nb_att, const gatt_svc_att_t* p_atts);

    /**
     ****************************************************************************************
     * @brief This function is called when a service has been found during a discovery procedure.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] start_hdl     Service start handle
     * @param[in] end_hdl       Service end handle
     * @param[in] uuid_type     UUID Type (see enum #gatt_uuid_type)
     * @param[in] p_uuid        Pointer to service UUID (LSB first)
     ****************************************************************************************
     */
    void (*cb_svc_info) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t start_hdl, uint16_t end_hdl,
                         uint8_t uuid_type, const uint8_t* p_uuid);
    /**
     ****************************************************************************************
     * @brief This function is called when an include service has been found during a discovery procedure.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] inc_svc_hdl   Include service attribute handle
     * @param[in] start_hdl     Service start handle
     * @param[in] end_hdl       Service end handle
     * @param[in] uuid_type     UUID Type (see enum #gatt_uuid_type)
     * @param[in] p_uuid        Pointer to service UUID (LSB first)
     ****************************************************************************************
     */
    void (*cb_inc_svc) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t inc_svc_hdl,
                        uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid);
    /**
     ****************************************************************************************
     * @brief This function is called when a characteristic has been found during a discovery procedure.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] char_hdl      Characteristic attribute handle
     * @param[in] val_hdl       Value handle
     * @param[in] prop          Characteristic properties (see enum #gatt_att_info_bf - bits [0-7])
     * @param[in] uuid_type     UUID Type (see enum #gatt_uuid_type)
     * @param[in] p_uuid        Pointer to characteristic value UUID (LSB first)
     ****************************************************************************************
     */
    void (*cb_char) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t char_hdl, uint16_t val_hdl,
                     uint8_t prop, uint8_t uuid_type, const uint8_t* p_uuid);
    /**
     ****************************************************************************************
     * @brief This function is called when a descriptor has been found during a discovery procedure.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] desc_hdl      Characteristic descriptor attribute handle
     * @param[in] uuid_type     UUID Type (see enum #gatt_uuid_type)
     * @param[in] p_uuid        Pointer to attribute UUID (LSB first)
     ****************************************************************************************
     */
    void (*cb_desc) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t desc_hdl,
                     uint8_t uuid_type, const uint8_t* p_uuid);
    /**
     ****************************************************************************************
     * @brief This function is called during a read procedure when attribute value is retrieved
     *        form peer device.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] dummy         Dummy parameter provided by upper layer for command execution
     * @param[in] hdl           Attribute handle
     * @param[in] offset        Value offset
     * @param[in] p_data        Pointer to buffer that contains attribute value starting from offset
     ****************************************************************************************
     */
    void (*cb_att_val) (uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint16_t offset,
                        co_buf_t* p_data);
    /**
     ****************************************************************************************
     * @brief This function is called when a notification or an indication is received onto
     *        register handle range (#gatt_cli_event_register).
     *
     *        #gatt_cli_att_event_cfm must be called to confirm event reception.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] token         Procedure token that must be returned in confirmation function
     * @param[in] evt_type      Event type triggered (see enum #gatt_evt_type)
     * @param[in] complete      True if event value if complete value has been received
     *                          False if data received is equals to maximum attribute protocol value.
     *                          In such case GATT Client User should perform a read procedure.
     * @param[in] hdl           Attribute handle
     * @param[in] p_data        Pointer to buffer that contains attribute value
     ****************************************************************************************
     */
    void (*cb_att_val_evt) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint8_t evt_type, bool complete,
                            uint16_t hdl, co_buf_t* p_data);

    /**
     ****************************************************************************************
     * @brief Event triggered when a service change has been received or if an attribute
     *        transaction triggers an out of sync error.
     *
     * @param[in] conidx        Connection index
     * @param[in] user_lid      GATT user local identifier
     * @param[in] out_of_sync   True if an out of sync error has been received
     * @param[in] start_hdl     Service start handle
     * @param[in] end_hdl       Service end handle
     ****************************************************************************************
     */
    void (*cb_svc_changed) (uint8_t conidx, uint8_t user_lid, bool out_of_sync, uint16_t start_hdl, uint16_t end_hdl);
} gatt_cli_cb_t;

/*
 * FUCTION DEFINITION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Compare if two UUIDs matches
 *
 * @param[in]  p_uuid_a     UUID A value
 * @param[in]  uuid_a_type  UUID A type (see enum #gatt_uuid_type)
 * @param[in]  p_uuid_b     UUID B value
 * @param[in]  uuid_b_type  UUID B type (see enum #gatt_uuid_type)
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool gatt_uuid_comp(const uint8_t *p_uuid_a, uint8_t uuid_a_type, const uint8_t *p_uuid_b, uint8_t uuid_b_type);

/**
 ****************************************************************************************
 * @brief Check if two UUIDs matches (2nd UUID is a 16 bits UUID with LSB First)
 *
 * @param[in]  p_uuid_a     UUID A value
 * @param[in]  uuid_a_type  UUID A type (see enum #gatt_uuid_type)
 * @param[in]  uuid_b       UUID B 16 bit value
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool gatt_uuid16_comp(const uint8_t *p_uuid_a, uint8_t uuid_a_type, uint16_t uuid_b);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth 16-bits UUID for 128-bit input
 *
 * @param[in]  p_uuid128      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 16-bit UUID, false else.
 ****************************************************************************************
 */
bool gatt_is_uuid16(const uint8_t *p_uuid128);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth 32 bits UUID for 128-bit input
 *
 * @param[in]  p_uuid128      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 32-bits UUID, false else.
 ****************************************************************************************
 */
bool gatt_is_uuid32(const uint8_t *p_uuid128);

/**
 ****************************************************************************************
 * @brief Convert UUID value to 128 bit UUID
 *
 * @param[in]  p_uuid      UUID to convert to 128-bit UUID
 * @param[in]  uuid_type   UUID type (see enum #gatt_uuid_type)
 * @param[out] p_uuid128   converted 32-bit Bluetooth UUID to 128-bit UUID
 *
 ****************************************************************************************
 */
void gatt_uuid128_convert(const uint8_t *p_uuid, uint8_t uuid_type, uint8_t *p_uuid128);

/**
 ****************************************************************************************
 * @brief Extract UUID from a 16-bit or 128-bit UUID (reduce result to a minimum length)
 *
 * @param[out] p_out_uuid        Output UUID (16-bit / 32-bit or 128-bit UUID LSB-first)
 * @param[out] p_out_uuid_type   Output UUID type (see enum #gatt_uuid_type)
 * @param[in]  p_in_uuid         Pointer to the 16-bit or 128-bit UUID (LSB-first)
 * @param[in]  in_uuid_len       Length of the input UUID: 2 or 16 bytes.
 *
 * @return Status of the function execution (see enum #hl_err)
 */
uint16_t gatt_uuid_extract(uint8_t *p_out_uuid, uint8_t* p_out_uuid_type, const uint8_t *p_in_uuid, uint8_t in_uuid_len);

/**
 ****************************************************************************************
 * @brief Command used to register a GATT user. This must be done prior to any GATT
 *        procedure execution.
 *
 *        A GATT client user can initiate any client procedure, and shall be able to
 *        handle all client initiated message events
 *
 *        Same module can register multiple GATT users.
 *
 * @param[in]  pref_mtu     Preferred MTU for attribute exchange.
 * @param[in]  prio_level   User attribute priority level
 * @param[in]  p_cb         Pointer to set of callback functions to be used for communication
 *                          with the GATT server user
 * @param[out] p_user_lid   Pointer where GATT user local identifier will be set
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_user_cli_register(uint16_t pref_mtu, uint8_t prio_level, const gatt_cli_cb_t* p_cb, uint8_t* p_user_lid);

/**
 ****************************************************************************************
 * @brief Command used to register a GATT user. This must be done prior to any GATT
 *        procedure execution.
 *
 *        A GATT server can manipulate local attribute database and initiate server
 *        procedures. It shall be able to handle all server initiated events
 *
 *        Same module can register multiple GATT users.
 *
 * @param[in]  pref_mtu     Preferred MTU for attribute exchange.
 * @param[in]  prio_level   User attribute priority level
 * @param[in]  p_cb         Pointer to set of callback functions to be used for communication
 *                          with the GATT server user
 * @param[out] p_user_lid   Pointer where GATT user local identifier will be set
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_user_srv_register(uint16_t pref_mtu, uint8_t prio_level, const gatt_srv_cb_t* p_cb, uint8_t* p_user_lid);

/**
 ****************************************************************************************
 * @brief Command used to notify send done callback in l2cap.
 *
 * @param[in]  user_lid     GATT user.
 * @param[in]  ntf
 * true: user get send done callback from gatt.
 * false: user get send done callback from l2cap.
 *
 * @return void
 ****************************************************************************************
 */
void gatt_user_set_send_done_in_l2cap(uint8_t user_lid, bool ntf);

/**
 ****************************************************************************************
 * @brief Indicate if the user enabled send callback from l2cap or gatt.
 *
 * @param[in]  user_lid     GATT user.
 *
 * @return status of this user getting send done callback from l2cap or not
 ****************************************************************************************
 */
bool gatt_user_is_send_done_in_l2cap(uint8_t user_lid);

/**
 ****************************************************************************************
 * @brief Command used to unregister a GATT user (client or server).
 *
 * @param[in]  user_lid     GATT User Local identifier
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_user_unregister(uint8_t user_lid);

/**
 ****************************************************************************************
 * @brief Command used to add a service into local attribute database.
 *
 *        Service and attributes UUIDs in service must be 16-bit
 *
 *        If start handle is set to zero (invalid attribute handle), GATT looks for a
 *        free handle block matching with number of attributes to reserve.
 *        Else, according to start handle, GATT checks if attributes to reserve are
 *        not overlapping part of existing database.
 *
 *        An added service is automatically visible for peer device.
 *
 *        @note First attribute in attribute array must be a Primary or a Secondary service
 *
 * @param[in]     user_lid     GATT User Local identifier
 * @param[in]     info         Service Information bit field (see enum #gatt_svc_info_bf)
 * @param[in]     uuid16       Service UUID (16-bit UUID - LSB First)
 * @param[in]     nb_att       Number of attribute(s) in service
 * @param[in]     p_att_mask   Pointer to mask of attribute to insert in database:
 *                               - If NULL insert all attributes
 *                               - If bit set to 1: attribute inserted
 *                               - If bit set to 0: attribute not inserted
 * @param[in]     p_atts       Pointer to List of attribute (with 16-bit uuid) description present in service.
 * @param[in]     nb_att_rsvd  Number of attribute(s) reserved for the service (shall be equals or greater nb_att)
 *                             Prevent any services to be inserted between start_hdl and (start_hdl + nb_att_rsvd - 1)
 * @param[in,out] p_start_hdl  Pointer to Service Start Handle (0 = chosen by GATT module)
 *                             Pointer updated with service start handle associated to
 *                             created service.
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_svc16_add(uint8_t user_lid, uint8_t info, uint16_t uuid16, uint8_t nb_att, const uint8_t* p_att_mask,
                           const gatt_att16_desc_t* p_atts, uint8_t nb_att_rsvd, uint16_t* p_start_hdl);

/**
 ****************************************************************************************
 * @brief Command used to add a service into local attribute database.
 *
 *        If start handle is set to zero (invalid attribute handle), GATT looks for a
 *        free handle block matching with number of attributes to reserve.
 *        Else, according to start handle, GATT checks if attributes to reserve are
 *        not overlapping part of existing database.
 *
 *        An added service is automatically visible for peer device.
 *
 *        @note First attribute in attribute array must be a Primary or a Secondary service
 *
 * @param[in]     user_lid     GATT User Local identifier
 * @param[in]     info         Service Information bit field (see enum #gatt_svc_info_bf)
 * @param[in]     p_uuid       Pointer to service UUID (LSB first)
 * @param[in]     nb_att       Number of attribute(s) in service
 * @param[in]     p_att_mask   Pointer to mask of attribute to insert in database:
 *                               - If NULL insert all attributes
 *                               - If bit set to 1: attribute inserted
 *                               - If bit set to 0: attribute not inserted
 * @param[in]     p_atts       Pointer to List of attribute description present in service.
 * @param[in]     nb_att_rsvd  Number of attribute(s) reserved for the service (shall be equals or greater nb_att)
 *                             Prevent any services to be inserted between start_hdl and (start_hdl + nb_att_rsvd -1)
 * @param[in,out] p_start_hdl  Pointer to Service Start Handle (0 = chosen by GATT module)
 *                             Pointer updated with service start handle associated to
 *                             created service.
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_svc_add(uint8_t user_lid, uint8_t info, const uint8_t* p_uuid, uint8_t nb_att, const uint8_t* p_att_mask,
                         const gatt_att_desc_t* p_atts, uint8_t nb_att_rsvd, uint16_t* p_start_hdl);

/**
 ****************************************************************************************
 * @brief Command used to remove a service from local attribute database.
 *
 *        Only GATT user responsible of service can remove it.
 *
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  start_hdl    Service Start Handle
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_svc_remove(uint8_t user_lid, uint16_t start_hdl);

/**
 ****************************************************************************************
 * @brief Function use to verify if several services can be allocated on a contiguous
 *        handle range.
 *
 * @param[in]     user_lid     GATT User Local identifier
 * @param[in]     nb_att       Number of attribute(s) to reserve
 * @param[in,out] p_start_hdl  Pointer to Service Start Handle (0 = chosen by GATT module)
 *                             Pointer updated with service start handle associated to
 *                             first attribute range available.
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_handle_range_reserve(uint8_t user_lid, uint8_t nb_att, uint16_t* p_start_hdl);

/**
 ****************************************************************************************
 * @brief Command used to control visibility and usage authorization of a local service.
 *        A hidden service is present in database but cannot be discovered or manipulated
 *        by a peer device.
 *        A disabled service can be discovered by a peer device but it is not authorized to
 *        use it.
 *
 *        Only GATT user responsible of service can update its properties
 *
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  start_hdl    Service Start Handle
 * @param[in]  enable       True: Authorize usage of the service
 *                          False: reject usage of the service
 * @param[in]  visible      Service visibility (see enum #gatt_svc_visibility)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_svc_ctrl(uint8_t user_lid, uint16_t start_hdl, uint8_t enable, uint8_t visible);

/**
 ****************************************************************************************
 * @brief Command used to retrieve (or compute) the local database hash value.
 *
 * @param[in]  conidx   Connection index (not used but returned as requested)
 * @param[in]  user_lid GATT User Local identifier (must be a server user)
 * @param[in]  dummy    Dummy parameter whose meaning is upper layer dependent and which
 *                      is returned in command complete.
 * @param[in]  p_cb     Callback where database hash is returned
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_db_hash_get(uint8_t conidx, uint8_t user_lid, uint16_t dummy, const gatt_db_hash_cb_t* p_cb);

/**
 ****************************************************************************************
 * @brief Command used to retrieve information of an attribute.
 *
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  hdl          Attribute Handle
 * @param[out] p_info       Attribute information bit field
 *                          (see enum #gatt_att_info_bf)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_att_info_get(uint8_t user_lid, uint16_t hdl, uint16_t* p_info);

/**
 ****************************************************************************************
 * @brief Command used to set information of an attribute.
 *
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  hdl          Attribute Handle
 * @param[in]  info         Attribute information bit field (see enum #gatt_att_info_bf)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_db_att_info_set(uint8_t user_lid, uint16_t hdl, uint16_t info);

/**
 ****************************************************************************************
 * @brief Command used by a GATT server user to send notifications or indications for
 *        some attributes values to peer device.
 *        Number of attributes must be set to one for GATT_INDICATE event type.
 *
 *        This function is consider reliable because GATT user is aware of maximum packet
 *        size that can be transmitted over the air.
 *
 *        Attribute value will be requested by GATT using #gatt_srv_cb_t.cb_att_event_get function
 *        Wait for #gatt_srv_cb_t.cb_event_sent execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  evt_type     Event type to trigger (see enum #gatt_evt_type)
 * @param[in]  nb_att       Number of attribute
 * @param[in]  p_atts       Pointer to List of attribute
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_srv_event_reliable_send(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t evt_type,
                                      uint8_t nb_att, const gatt_att_t* p_atts);

/**
 ****************************************************************************************
 * @brief Command used by a GATT server user to send notifications or indications.
 *
 *        Since user is not aware of MTU size of the bearer used for attribute
 *        transmission it cannot be considered reliable. If size of the data buffer is too
 *        big, data is truncated to max supported length.
 *
 *        Wait for #gatt_srv_cb_t.cb_event_sent execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  evt_type     Event type to trigger (see enum #gatt_evt_type)
 * @param[in]  hdl          Attribute handle
 * @param[in]  p_data       Data buffer that must be transmitted
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_srv_event_send(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t evt_type, uint16_t hdl,
                             co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief Command used by a GATT server user to send notifications or indications of an
 *        attribute to multiple connected devices.
 *
 *        Since user is not aware of MTU size of the bearer used for attribute
 *        transmission it cannot be considered reliable. If size of the data buffer is too
 *        big, data is truncated to max supported length.
 *
 *        Event are sent sequentially over each connection and can take some time.
 *        The cb_event_sent callback is called once when procedure is completed.
 *
 *        It is possible to cancel an on-going multi-point procedure using
 *        #gatt_srv_event_mtp_cancel function. In such case GATT user must ensure
 *        that the couple user_lid + dummy parameters are unique for GATT module.
 *
 *        Wait for #gatt_srv_cb_t.cb_event_sent execution before starting a new procedure
 *
 * @param[in]  conidx_bf    Connection index bit field
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  evt_type     Event type to trigger (see enum #gatt_evt_type)
 * @param[in]  hdl          Attribute handle
 * @param[in]  p_data       Data buffer that must be transmitted
 * @param[in]  filter       True to filter intermediate command completed event triggered
 *                          when event is sent onto a specific connection.
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_srv_event_mtp_send(uint32_t conidx_bf, uint8_t user_lid, uint16_t dummy, uint8_t evt_type,
                                 uint16_t hdl, co_buf_t* p_data, bool filter);

/**
 ****************************************************************************************
 * @brief Command used by a GATT server user to cancel a multi connection event transmission
 *
 *        @note Once procedure is done, #cb_event_sent function is called.
 *
 * @param[in]  user_lid     GATT User Local identifier used in #gatt_srv_event_mtp_send
 * @param[in]  dummy        Dummy parameter used in #gatt_srv_event_mtp_send
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_srv_event_mtp_cancel(uint8_t user_lid, uint16_t dummy);

/**
 ****************************************************************************************
 * @brief Upper layer provide attribute value requested by GATT Layer for a read procedure
 *        If rejected, value is not used.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 * @param[in]  status       Status of attribute value get (see enum #hl_err)
 * @param[in]  att_length   Complete Length of the attribute value
 * @param[in]  p_data       Pointer to buffer that contains attribute data
 *                          (starting from offset and does not exceed maximum size provided)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_srv_att_read_get_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t status,
                                   uint16_t att_length, co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief Upper layer provide attribute value requested by GATT Layer for an event procedure.
 *        If rejected, value is not used.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 * @param[in]  status       Status of attribute value get (see enum #hl_err)
 * @param[in]  att_length   Complete Length of the attribute value
 * @param[in]  p_data       Pointer to buffer that contains attribute data
 *                          (data size does not exceed maximum size provided)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_srv_att_event_get_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t status,
                                    uint16_t att_length, co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief Upper layer provide information about attribute requested by GATT Layer.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 * @param[in]  status       Status of attribute info get (see enum #hl_err)
 * @param[in]  att_length   Attribute value length
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_srv_att_info_get_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t status, uint16_t att_length);

/**
 ****************************************************************************************
 * @brief Upper layer provide status of attribute value modification by GATT server user.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 * @param[in]  status       Status of attribute value set (see enum #hl_err)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_srv_att_val_set_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t status);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to discover primary or secondary services,
 *        exposed by peer device in its attribute database.
 *
 *        All services can be discovered or filtering services having a specific UUID.
 *        The discovery is done between start handle and end handle range.
 *        For a complete discovery start handle must be set to 0x0001 and end handle to
 *        0xFFFF.
 *
 *        Wait for #gatt_cli_cb_t.cb_discover_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  disc_type    GATT Service discovery type (see enum #gatt_svc_discovery_type)
 * @param[in]  full         Perform discovery of all information present in the service
 *                          (True: enable, False: disable)
 * @param[in]  start_hdl    Search start handle
 * @param[in]  end_hdl      Search end handle
 * @param[in]  uuid_type    UUID Type (see enum #gatt_uuid_type)
 * @param[in]  p_uuid       Pointer to searched Service UUID (meaningful only for
 *                          discovery by UUID)
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_discover_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t disc_type, bool full,
                               uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to discover included services, exposed
 *        by peer device in its attribute database.
 *
 *        The discovery is done between start handle and end handle range.
 *        For a complete discovery start handle must be set to 0x0001 and end handle to
 *        0xFFFF.
 *
 *        Wait for #gatt_cli_cb_t.cb_discover_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  start_hdl    Search start handle
 * @param[in]  end_hdl      Search end handle
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_discover_inc_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy,
                                   uint16_t start_hdl, uint16_t end_hdl);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to discover all or according to a specific
 *        UUID characteristics exposed by peer device in its attribute database.
 *
 *        The discovery is done between start handle and end handle range.
 *        For a complete discovery start handle must be set to 0x0001 and end handle to
 *        0xFFFF.
 *
 *        Wait for #gatt_cli_cb_t.cb_discover_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  disc_type    GATT characteristic discovery type (see enum #gatt_char_discovery_type)
 * @param[in]  start_hdl    Search start handle
 * @param[in]  end_hdl      Search end handle
 * @param[in]  uuid_type    UUID Type (see enum #gatt_uuid_type)
 * @param[in]  p_uuid       Pointer to searched Attribute Value UUID (meaningful only
 *                          for discovery by UUID)
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_discover_char(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t disc_type,
                                uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to discover characteristic descriptor
 *        exposed by peer device in its attribute database.
 *
 *        The discovery is done between start handle and end handle range.
 *        For a complete discovery start handle must be set to 0x0001 and end handle to
 *        0xFFFF.
 *
 *        Wait for #gatt_cli_cb_t.cb_discover_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  start_hdl    Search start handle
 * @param[in]  end_hdl      Search end handle
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_discover_desc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t start_hdl, uint16_t end_hdl);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to cancel an on-going discovery procedure.
 *        The dummy parameter in the request must be equals to dummy parameter used for
 *        service discovery command.
 *
 *        The discovery is aborted as soon as on-going discovery attribute transaction
 *        is over.
 *
 *        Wait for #gatt_cli_cb_t.cb_discover_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_discover_cancel(uint8_t conidx, uint8_t user_lid, uint16_t dummy);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to read value of an attribute (identified
 *        by its handle) present in peer database.
 *
 *        Wait for #gatt_cli_cb_t.cb_read_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  hdl          Attribute handle
 * @param[in]  offset       Value offset
 * @param[in]  length       Value length to read (0 = read all)
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_read(uint8_t conidx, uint8_t user_lid, uint16_t dummy,
                       uint16_t hdl, uint16_t offset, uint16_t length);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to read value of an attribute with a given
 *        UUID in peer database.
 *
 *        Wait for #gatt_cli_cb_t.cb_read_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  start_hdl    Search start handle
 * @param[in]  end_hdl      Search end handle
 * @param[in]  uuid_type    UUID Type (see enum #gatt_uuid_type)
 * @param[in]  p_uuid       Pointer to searched attribute UUID (LSB First)
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_read_by_uuid(uint8_t conidx, uint8_t user_lid, uint16_t dummy,
                               uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to read multiple attribute at the same time.
 *        If one of attribute length is unknown, the read multiple variable length
 *        procedure is used.
 *
 *        Wait for #gatt_cli_cb_t.cb_read_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  nb_att       Number of attribute
 * @param[in]  p_atts       Pointer to list of attribute
 *                          If Attribute length is zero (consider length unknown):
  *                            - Attribute protocol read multiple variable length
  *                              procedure used
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_read_multiple(uint8_t conidx, uint8_t user_lid, uint16_t dummy,
                                uint8_t nb_att, const gatt_att_t* p_atts);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to request to write value of an attribute
 *        in peer database.
 *
 *        This function is consider reliable because GATT user is aware of maximum packet
 *        size that can be transmitted over the air.
 *
 *        Attribute value will be requested by GATT using #gatt_cli_cb_t.cb_att_val_get function
 *
 *        Wait for #gatt_cli_cb_t.cb_write_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  write_type   GATT write type (see enum #gatt_write_type)
 * @param[in]  write_mode   Write execution mode (see enum #gatt_write_mode).
 *                          Valid only for GATT_WRITE.
 * @param[in]  hdl          Attribute handle
 * @param[in]  offset       Value offset, valid only for GATT_WRITE
 * @param[in]  length       Value length to write
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_write_reliable(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t write_type, uint8_t write_mode,
                                 uint16_t hdl, uint16_t offset, uint16_t length);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to request to write value of an attribute
 *        in peer database.
 *
 *        Since user is not aware of MTU size of the bearer used for attribute
 *        transmission it cannot be considered reliable.
 *
 *        For a GATT_WRITE_NO_RESP if attribute bearer max transmission size isn't sufficient,
 *        a GATT_WRITE (with response) procedure will be used.
 *
 *        For a GATT_WRITE_SIGNED, if attribute bearer max transmission size isn't sufficient,
 *        the procedure is aborted with L2CAP_ERR_INVALID_MTU error code.
 *
 *        Wait for #gatt_cli_cb_t.cb_write_cmp execution before starting a new procedure
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  write_type   GATT write type (see enum #gatt_write_type)
 * @param[in]  hdl          Attribute handle
 * @param[in]  offset       Value offset, valid only for GATT_WRITE
 * @param[in]  p_data       Data buffer that must be transmitted
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_write(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint8_t write_type,
                        uint16_t hdl, uint16_t offset, co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief Upper layer provide attribute value requested by GATT Layer, length shall be
 *        set to zero if request is rejected.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 * @param[in]  status       Status of attribute value get (see enum #hl_err)
 * @param[in]  p_data       Pointer to buffer that contains data to write (starting
 *                          from offset)
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_cli_att_val_get_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t status, co_buf_t* p_data);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to request peer server to execute prepare
 *        write queue.
 *
 *        Wait for #gatt_cli_cb_t.cb_write_cmp execution before starting a new procedure
 *
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  dummy        Dummy parameter whose meaning is upper layer dependent and
 *                          which is returned in command complete.
 * @param[in]  execute      True: Perform pending write operations
 *                          False: Cancel pending write operations
 *
 * @return Status of the function execution (see enum #hl_err)
 *         Consider status only if an error occurs; else wait for execution completion
 ****************************************************************************************
 */
uint16_t gatt_cli_write_exe(uint8_t conidx, uint8_t user_lid, uint16_t dummy, bool execute);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to register for reception of events
 *        (notification / indication) for a given handle range.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  start_hdl    Attribute start handle
 * @param[in]  end_hdl      Attribute end handle
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_cli_event_register(uint8_t conidx, uint8_t user_lid, uint16_t start_hdl, uint16_t end_hdl);

/**
 ****************************************************************************************
 * @brief Command used by a GATT client user to stop reception of events (notification /
 *        indication) onto a specific handle range.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  start_hdl    Attribute start handle
 * @param[in]  end_hdl      Attribute end handle
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_cli_event_unregister(uint8_t conidx, uint8_t user_lid, uint16_t start_hdl, uint16_t end_hdl);

/**
 ****************************************************************************************
 * @brief Upper layer provide status of attribute event handled by GATT client user.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 * @param[in]  token        Procedure token provided in corresponding callback
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_cli_att_event_cfm(uint8_t conidx, uint8_t user_lid, uint16_t token);

/**
 ****************************************************************************************
 * @brief Request a MTU exchange on legacy attribute bearer.
 *        There is no callback executed when the procedure is over.
 *
 * @param[in]  conidx       Connection index
 * @param[in]  user_lid     GATT User Local identifier
 *
 * @return Status of the function execution (see enum #hl_err)
 ****************************************************************************************
 */
uint16_t gatt_cli_mtu_exch(uint8_t conidx, uint8_t user_lid);


/**
 ****************************************************************************************
 * @brief Retrieve minimum MTU of Maximum MTU negotiated for all available ATT bearers.
 *
 * @param[in]  conidx       Connection index
 *
 * @return Minimum of Maximum MTU negotiated for all available ATT bearers
 ****************************************************************************************
 */
uint16_t gatt_bearer_mtu_min_get(uint8_t conidx);

/// @} GATT_NATIVE_API

#endif // GATT_H_
