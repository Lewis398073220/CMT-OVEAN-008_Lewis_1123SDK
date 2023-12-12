#ifndef _AI_H_
#define _AI_H_

/**
 ****************************************************************************************
 * @addtogroup SMARTVOICETASK Task
 * @ingroup SMARTVOICE
 * @brief Smart Voice Profile Task.
 *
 * The SMARTVOICETASK is responsible for handling the messages coming in and out of the
 * @ref SMARTVOICE collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "prf_utils.h"
#include "ke_task.h"

#define ATT_DECL_PRIMARY_SERVICE_UUID       { 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID       { 0x02, 0x29 }

#define AI_MAX_LEN                      (509)
#define INVALID_AI_GATT_SERVER_ENTRY_INDEX 0xFFFF

typedef enum{
    /// Server initiated notification
    AI_GATT_NOTIFY     = 0x00,
    /// Server initiated indication
    AI_GATT_INDICATE   = 0x01,
} AI_EVENT_TYPE_E;

typedef enum{
    /// Server initiated notification
    AI_CMD     = 0x00,
    /// Server initiated indication
    AI_DATA    = 0x01,
} AI_DATA_TYPE_E;

/// Possible states of the SMARTVOICE task
typedef enum
{
    /// Idle state
    AI_IDLE,
    /// Connected state
    AI_BUSY,
    /// Number of defined states.
    AI_STATE_MAX,
}AI_STATE_TYPE_E;

typedef enum
{
    SVC_AI_AMA,
    SVC_AI_DMA,
    SVC_AI_GMA,
    SVC_AI_SMART,
    SVC_AI_TENCENT,
    SVC_AI_RECORDING,
    SVC_AI_CUSTOMIZE,
} AI_SVC_TYPE_E;

/// Datapath Profile Server environment variable
typedef PRF_ENV_TAG(ai)
{
    /// pointer to the allocated memory used by profile during runtime.
    prf_hdr_t   *p_env;
    /// Application Task Number - if MSB bit set, Multi-Instantiated task
    uint16_t    app_task;
    /// Profile Task  Number    - if MSB bit set, Multi-Instantiated task
    uint16_t    prf_task;
    /// flag to mark whether notification or indication is enabled
    ///   7      6     5     4       3          2          1       0
    /// +-----+-----+-----+-----+----------+----------+--------+--------+
    /// | RFU | RFU | RFU | RFU | IND_DATA | NTF_DATA | IND_CMD|NTF_CMD |
    /// +-----+-----+-----+-----+----------+----------+--------+--------+
    uint16_t    ntfIndEnableFlag[BLE_CONNECTION_MAX];
    /// GATT User local index
    uint8_t     srv_user_lid;
    /// Service Start Handle
    uint16_t    shdl;
    /// State of different task instances
    uint8_t     state;
} PRF_ENV_T(ai);

enum
{
    AMA_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_AMA),
};

enum
{
    DMA_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_DMA),
};

enum
{
    GMA_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_GMA),
};

enum
{
    SMART_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_SMART),
};

enum
{
    TENCENT_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_TENCENT),
};

enum
{
    CUSTOMIZE_DATA_DEND_CFM = TASK_FIRST_MSG(TASK_ID_CUSTOMIZE),
};

enum
{
    RECORDING_DATA_DEND_CFM = MSG_ID(AI, 0x00),
};


typedef enum
{
    PRF_AI_SVC_ADD_DONE_IND     = MSG_ID(AI, 0x01),
    PRF_AI_CONNECT_IND          = MSG_ID(AI, 0x02),
    PRF_AI_DISCONNECT_IND       = MSG_ID(AI, 0x03),
    PRF_AI_TX_DONE_IND          = MSG_ID(AI, 0x04),
    PRF_AI_CMD_RECEIVED_IND     = MSG_ID(AI, 0x05),
    PRF_AI_DATA_RECEIVED_IND    = MSG_ID(AI, 0x06),
    PRF_AI_CMD_CHANGGE_CCC_IND  = MSG_ID(AI, 0x07),
    PRF_AI_DATA_CHANGGE_CCC_IND = MSG_ID(AI, 0x08),
} PRF_AI_EVENT_TYPE_E;

/// Parameters of the @ref AMA_DATA_DEND_CFM message
typedef struct ai_data_send_cfm
{
    /// Connection index
    uint8_t  conidx;
    /// gatt event type, see@AI_EVENT_TYPE_E
    uint8_t  gatt_event_type;
    /// send data type. see@AI_DATA_TYPE_E
    uint8_t  data_type;
    uint32_t data_len;
    uint8_t  data[__ARRAY_EMPTY];
} ai_data_send_cfm_t;

/// Parameters of the @ref AMA_DATA_DEND_CFM message
typedef struct ai_add_svc_ind
{
    /// see@AI_SVC_TYPE_E
    uint8_t  ai_type;
    /// att start handle
    uint32_t start_hdl;
    /// att number
    uint8_t  att_num;
} ai_add_svc_ind_t;

/// Parameters of the @ref AMA_DATA_DEND_CFM message
typedef struct ai_change_ccc_ind
{
    /// Connection index
    uint8_t  conidx;
    /// see@AI_SVC_TYPE_E
    uint8_t  ai_type;
    /// data
    uint8_t  ntf_ind_flag;
} ai_change_ccc_ind_t;

/// Parameters of the @ref AMA_DATA_DEND_CFM message
typedef struct ai_event_ind
{
    /// Connection index
    uint8_t  conidx;
    /// see@AI_SVC_TYPE_E
    uint8_t  ai_type;
    /// data lenth
    uint32_t data_len;
    /// data
    uint8_t  data[__ARRAY_EMPTY];
} ai_event_ind_t;

#endif
