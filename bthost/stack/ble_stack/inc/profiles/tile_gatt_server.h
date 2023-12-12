#ifndef TILE_GATT_SERVER_H
#define TILE_GATT_SERVER_H

#include "rwip_config.h"

#if (BLE_TILE)
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"
#include "tile_storage.h"
#include "tile_timer_driver.h"
#include "tile_features.h"
#include "tile_lib.h"
#include "tile_gap_driver.h"
#include "ble_dp_common.h"

typedef PRF_ENV_TAG(tile_gatt)
{
  // By making this the first struct element, tile_env can be stored in the
  // 'env' field of prf_task_env.
  prf_hdr_t     prf_env;
  uint8_t       isNotificationEnabled[BLE_CONNECTION_MAX];
  uint16_t      start_hdl; // Service Start Handle
  ke_state_t    state; // State of different task instances
      /// GATT User local index
    uint8_t     srv_user_lid;
} PRF_ENV_T(tile_gatt);

enum {
    /// Idle state
    TILE_IDLE,
    /// Connected state
    TILE_BUSY,

    /// Number of defined states.
    TILE_STATE_MAX,
};

/*enum TILE_MODE
{
  TILE_MODE_MANUFACTURING = 0x0,
  TILE_MODE_SHIPPING      = 0x1,
  TILE_MODE_ACTIVATED     = 0x2
};
*/

typedef enum {
   TILE_IDX_SVC,

   TILE_IDX_ID_CHAR,
   TILE_IDX_ID_VAL,
   
   TILE_IDX_TOA_CMD_CHAR,
   TILE_IDX_TOA_CMD_VAL,
   
   TILE_IDX_TOA_RSP_CHAR,
   TILE_IDX_TOA_RSP_VAL,
   TILE_IDX_TOA_RSP_NTF_CFG,

   TILE_IDX_NUM,
} tile_gatt_handles;

enum tile_msg_id {
    TILE_CHANNEL_CONNECTED_IND = TASK_FIRST_MSG(TASK_ID_TILE),
    TILE_CHANNEL_DISCONNECTED_IND,
    TILE_TOA_TX_DATA_SENT_CMD,
    TILE_TOA_TX_DATA_SENT_DONE_IND,
    TILE_TOA_RECEIVED_IND,
};

struct ble_tile_sent_ntf_t         {
    uint8_t  conidx;
    uint16_t data_len;
    uint8_t  data[0];
};

struct tile_gatt_connection_event_t {
    uint8_t  conidx;
    uint16_t handle;
    uint8_t  status;
};

struct ble_tile_tx_notif_config_t {
    uint8_t conidx;
    bool    isNotificationEnabled;
};

struct tile_gatt_rx_data_ind_t {
    uint8_t  conidx;
    uint16_t length;
    uint8_t  data[0];
};

struct tile_gatt_tx_data_req_t {
  uint8_t  connectionIndex;
  uint16_t length;
  uint8_t  data[0];
};

struct tile_gatt_tx_complete_ind_t {
    uint8_t conidx;
    bool success;
};

const struct prf_task_cbs* tile_prf_itf_get(void);

#endif /* (BLE_TILE) */

#endif /* TILE_GATT_SERVER_H */
