/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#ifdef __IAG_BLE_INCLUDE__
#include "bluetooth_ble_api.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "string.h"

#ifdef __AI_VOICE_BLE_ENABLE__
#include "ai_transport.h"
#include "ai_thread.h"
#include "ai_control.h"
#endif
#include "app_ai_if_ble.h"

bool app_ai_if_ble_check_if_notification_processing_is_busy(uint8_t conidx)
{
    return false;
}

uint16_t app_ai_if_ble_get_conhdl_from_conidx(uint8_t conidx)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    return bes_ble_gap_get_conhdl_from_conidx(conidx);
#else
    return 0xFFFF;
#endif
}

void app_ai_if_ble_disconnect_ble(uint8_t conidx)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_start_disconnect(conidx);
#endif
}

void app_ai_if_ble_set_adv(uint16_t advInterval)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}

void app_ai_if_ble_update_conn_param(uint8_t  conidx,
                                    uint32_t min_interval_in_ms,
                                    uint32_t max_interval_in_ms,
                                    uint32_t supervision_timeout_in_ms,
                                    uint8_t  slaveLantency)
{
#ifdef __AI_VOICE_BLE_ENABLE__
     bes_ble_gap_conn_update_param(conidx, min_interval_in_ms, max_interval_in_ms, supervision_timeout_in_ms, slaveLantency);
#endif
}

void app_ai_if_ble_update_conn_param_mode(bool isEnabled)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_update_conn_param_mode(BLE_CONN_PARAM_MODE_AI_STREAM_ON, isEnabled);
#endif
}

void app_ai_if_ble_register_data_fill_handle(void *func, bool enable)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_register_data_fill_handle(USER_AI, ( BLE_DATA_FILL_FUNC_T )func, enable);
#endif
}

void app_ai_if_ble_data_fill_enable(bool enable)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_data_fill_enable(USER_AI, enable);
#endif
}

void app_ai_if_ble_disconnect_all(void)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_disconnect_all();
#endif
}

void app_ai_if_ble_force_switch_adv(bool onOff)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    bes_ble_gap_force_switch_adv(BLE_SWITCH_USER_AI, onOff);
#endif
}

#endif
