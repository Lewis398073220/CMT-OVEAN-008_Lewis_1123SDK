#ifndef GFPSP_H_
#define GFPSP_H_

/**
 ****************************************************************************************
 * @addtogroup GFPSP google fast pair s
 * @ingroup DIS
 * @brief Device Information Service Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_GFPS_PROVIDER)
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"
#include "ble_gfps_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// GFPSP Attributes database handle list 
enum gfpsp_att_db_handles
{
    GFPSP_IDX_SVC,

    GFPSP_IDX_MODEL_ID_CHAR,
    GFPSP_IDX_MODEL_ID_VAL,
    GFPSP_IDX_MODEL_ID_CFG,

    GFPSP_IDX_KEY_BASED_PAIRING_CHAR,
    GFPSP_IDX_KEY_BASED_PAIRING_VAL,
    GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG,
    
    GFPSP_IDX_PASSKEY_CHAR,
    GFPSP_IDX_PASSKEY_VAL,
    GFPSP_IDX_PASSKEY_NTF_CFG,
    
    GFPSP_IDX_ACCOUNT_KEY_CHAR,
    GFPSP_IDX_ACCOUNT_KEY_VAL,
    GFPSP_IDX_ACCOUNT_KEY_CFG,

    GFPSP_IDX_NAME_CHAR,
    GFPSP_IDX_NAME_VAL,
    GFPSP_IDX_NAME_CFG,
    
#ifdef SPOT_ENABLED
    GFPSP_IDX_BEACON_ACTIONS_CHAR,
    GFPSP_IDX_BEACON_ACTIONS_VAL,
    GFPSP_IDX_BEACON_ACTIONS_CFG,
#endif 

    GFPSP_IDX_MSG_STREAM_CHAR,
    GFPSP_IDX_MSG_STREAM_VAL, 
    GFPSP_IDX_MSG_STREAM_CFG,

    GFPSP_IDX_NB,
};

/// Value element
struct gfpsp_val_elmt
{
    /// list element header
    struct co_list_hdr hdr;
    /// value identifier
    uint8_t value;
    /// value length
    uint8_t length;
    /// value data
    uint8_t data[__ARRAY_EMPTY];
};

///Device Information Service Server Environment Variable
typedef PRF_ENV_TAG(gfpsp)
{
    /// profile environment
    prf_hdr_t prf_env;
    /// List of values set by application
    struct co_list values;
    /// Service Attribute Start Handle
    uint16_t start_hdl;
    /// flag to mark whether notification or indication is enabled
    uint8_t isNotificationEnabled[BLE_CONNECTION_MAX];
    /// Services features
    uint16_t features;
    /// Last requested value
    uint8_t  req_val;
    /// Last connection index which request value
    uint8_t  req_conidx;

    /// GFPSP task state
    ke_state_t state[GFPSP_IDX_MAX];
    /// GATT User local index
    uint8_t     user_lid;
} PRF_ENV_T(gfpsp);

typedef struct {
    uint8_t conidx;
    uint16_t chan_id;
    uint16_t mtu;
}gfpsp_l2cap_env_t;

typedef struct {
    gfpsp_l2cap_env_t l2capEnv[BT_DEVICE_NUM];
}gfpsp_ble_env_t;

#ifdef SPOT_ENABLED
typedef struct _gfpsp_reading_beacon_additional_data{
    int8_t power_value;
    uint8_t clock_value[4];
    uint8_t SECP_method;
    uint8_t numbesr_of_ringing;
    uint8_t ringing_capability;
    uint8_t padgding[8];
}gfpsp_reading_beacon_additional_data;
typedef struct _gfpsp_reading_beacon_state_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t additional_data[16];
}gfpsp_reading_beacon_state_resp;

typedef struct _gfpsp_reading_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data;
}gfpsp_reading_beacon_provision_resp;

typedef struct _gfpsp_reading_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data;
    uint8_t EIK[20];
}gfpsp_reading_EIK_beacon_provision_resp;

typedef struct _gfpsp_set_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_reading_set_beacon_provision_resp;

typedef struct _gfpsp_clearing_EIK_beacon_provision_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_clearing_EIK_beacon_provision_resp;

typedef struct _gfpsp_reading_beacon_identity_key_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[32];
}gfpsp_reading_beacon_identity_key_resp;

typedef struct _gfpsp_beacon_ring_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[4];
}gfpsp_beacon_ring_resp;

typedef struct _gfpsp_beacon_read_ring_state_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
    uint8_t data[3];
}gfpsp_reading_beacon_ring_state_resp;

typedef struct _gfpsp_beacon_activate_unwanted_tracking_mode_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_beacon_activate_unwanted_tracking_mode_resp;

typedef struct _gfpsp_beacon_deactivate_unwanted_tracking_mode_resp{
    uint8_t data_id;
    uint8_t data_length;
    uint8_t auth_data[8];
}gfpsp_beacon_deactivate_unwanted_tracking_mode_resp;
#endif


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve DIS service profile interface
 *
 * @return DIS service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* gfpsp_prf_itf_get(void);


/**
 ****************************************************************************************
 * @brief Check if an attribute shall be added or not in the database
 *
 * @param features DIS features
 *
 * @return Feature config flag
 ****************************************************************************************
 */
uint32_t gfpsp_compute_cfg_flag(uint16_t features);

/**
 ****************************************************************************************
 * @brief Check if the provided value length matches characteristic requirements
 * @param char_code Characteristic Code
 * @param val_len   Length of the Characteristic value
 *
 * @return status if value length is ok or not
 ****************************************************************************************
 */
uint8_t gfpsp_check_val_len(uint8_t char_code, uint8_t val_len);

/**
 ****************************************************************************************
 * @brief Retrieve handle attribute from value
 *
 * @param[in] env   Service environment variable
 * @param[in] value Value to search
 *
 * @return Handle attribute from value
 ****************************************************************************************
 */
uint16_t gfpsp_value_to_handle(PRF_ENV_T(gfpsp)* env, uint8_t value);

/**
 ****************************************************************************************
 * @brief Retrieve value from attribute handle
 *
 * @param[in] env    Service environment variable
 * @param[in] handle Attribute handle to search
 *
 * @return  Value from attribute handle
 ****************************************************************************************
 */
uint8_t gfpsp_handle_to_value(PRF_ENV_T(gfpsp)* env, uint16_t handle);

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void gfpsp_task_init(struct ke_task_desc *task_desc);

uint8_t gfps_ble_l2cap_send(uint8_t conidx, uint8_t *ptrData, uint32_t length);

void gfps_ble_l2cap_disconnect(uint8_t conidx);

uint8_t gfpsp_is_connected(uint8_t conidx);

#endif //BLE_GFPSP_SERVER

/// @} GFPSP

#endif // GFPSP_H_
