#include "bluetooth_bt_api.h"
#include "cmsis.h"
#include "app_bt.h"
#include "app_bt_cmd.h"
#include "me_api.h"
#include "bt_if.h"
#include "cobuf.h"
#include "btapp.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "conmgr_api.h"
#include "cobuf.h"
#include "co_ppbuff.h"
#include "app_bt_func.h"
#include "app_a2dp.h"
#include "audio_policy.h"
#include "a2dp_api.h"
#include "avrcp_api.h"
#include "hfp_api.h"
#include "app_hfp.h"
#include "hfp_i.h"
#include "hci_api.h"
#include "me_mediator.h"
#include "hci_i.h"
#include "l2cap_api.h"
#include "l2cap_i.h"
#include "btgatt_api.h"
#include "app_btgatt.h"
#include "sdp_api.h"
#include "spp_api.h"
#include "app_fp_rfcomm.h"
#include "app_dip.h"
#include "dip_api.h"
#include "besaud_api.h"
#include "btm_i.h"
#include "app_a2dp_source.h"
#include "bt_source.h"
#include "app_bt_hid.h"
#include "app_map.h"
#include "map_api.h"
#ifdef SASS_ENABLED
#include "gfps_sass.h"
#endif

#ifdef BT_A2DP_SUPPORT

bt_status_t bt_a2dp_init(bt_a2dp_callback_t callback)
{
    if (callback)
    {
        bt_add_event_callback((bt_event_callback_t)callback, BT_EVENT_MASK_A2DP_SNK_GROUP);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_cleanup(void)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_connect(const bt_bdaddr_t *remote)
{
    app_bt_reconnect_a2dp_profile((bt_bdaddr_t *)remote);
    return BT_STS_PENDING;
}

#if defined(BT_SOURCE)

bt_status_t bt_a2dp_source_init(bt_a2dp_source_callback_t callback)
{
    if (callback)
    {
        bt_add_event_callback((bt_event_callback_t)callback, BT_EVENT_MASK_A2DP_SRC_GROUP);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_source_cleanup(void)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_source_connect(const bt_bdaddr_t *bd_addr)
{
    bt_source_reconnect_a2dp_profile(bd_addr);
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_source_disconnect(const bt_bdaddr_t *bd_addr)
{
    return bt_a2dp_disconnect(bd_addr);
}

bt_status_t bt_a2dp_source_start_stream(const bt_bdaddr_t *bd_addr)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    app_a2dp_source_start_stream(curr_device->device_id);
    return BT_STS_SUCCESS;
}

bt_status_t bt_a2dp_source_suspend_stream(const bt_bdaddr_t *bd_addr)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    app_a2dp_source_suspend_stream(curr_device->device_id);
    return BT_STS_SUCCESS;
}

#endif /* BT_SOURCE */
#endif /* BT_A2DP_SUPPORT */

#ifdef BT_AVRCP_SUPPORT

bt_status_t bt_avrcp_init(bt_avrcp_callback_t callback)
{
    if (callback)
    {
        bt_add_event_callback((bt_event_callback_t)callback, BT_EVENT_MASK_AVRCP_GROUP);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_avrcp_cleanup(void)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_avrcp_connect(const bt_bdaddr_t *remote)
{
    app_bt_reconnect_avrcp_profile((bt_bdaddr_t *)remote);
    return BT_STS_SUCCESS;
}

bt_status_t bt_avrcp_disconnect(const bt_bdaddr_t *remote)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(remote);
    if (curr_device)
    {
        app_bt_disconnect_avrcp_profile(curr_device->avrcp_channel);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_avrcp_send_passthrough_cmd(const bt_bdaddr_t *bd_addr, bt_avrcp_key_code_t key)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    app_bt_a2dp_send_key_request(curr_device->device_id, key);
    return BT_STS_SUCCESS;
}

bt_status_t bt_avrcp_send_get_play_status(const bt_bdaddr_t *bd_addr)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    return btif_avrcp_ct_get_play_status(curr_device->avrcp_channel);
}

bt_status_t bt_avrcp_send_set_abs_volume(const bt_bdaddr_t *bd_addr, uint8_t volume)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    return btif_avrcp_ct_set_absolute_volume(curr_device->avrcp_channel, volume);
}

bt_status_t bt_avrcp_send_get_play_status_rsp(const bt_bdaddr_t *bd_addr, bt_avrcp_play_status_t play_status, uint32_t song_len, uint32_t song_pos)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    return btif_avrcp_send_play_status_rsp(curr_device->avrcp_channel, song_len, song_pos, play_status);
}

bt_status_t bt_avrcp_send_volume_notify_rsp(const bt_bdaddr_t *bd_addr, uint8_t volume)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device || !curr_device->avrcp_conn_flag)
    {
        return BT_STS_FAILED;
    }
    if (curr_device->volume_report == BTIF_AVCTP_RESPONSE_INTERIM)
    {
        bt_status_t status = btif_avrcp_ct_send_volume_change_actual_rsp(curr_device->avrcp_channel, volume);
        if (BT_STS_FAILED != status)
        {
            curr_device->volume_report = BTIF_AVCTP_RESPONSE_CHANGED;
        }
        return status;
    }
    return BT_STS_FAILED;
}

bt_status_t bt_avrcp_report_status_change(const bt_bdaddr_t *bd_addr, bt_avrcp_status_change_event_t event_id, uint32_t param)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    if (event_id == BT_AVRCP_VOLUME_CHANGED)
    {
        return bt_avrcp_send_volume_notify_rsp(bd_addr, (uint8_t)param);
    }
    if (event_id == BT_AVRCP_PLAY_STATUS_CHANGED)
    {
        bt_avrcp_play_status_t play_status = (bt_avrcp_play_status_t)param;
        bt_avrcp_play_status_t prev_play_status = (bt_avrcp_play_status_t)curr_device->avrcp_playback_status;
        if (curr_device->avrcp_conn_flag && play_status <= BT_AVRCP_PLAY_STATUS_PAUSED)
        {
            curr_device->avrcp_playback_status = (uint8_t)play_status;
            if (curr_device->play_status_notify_registered && play_status != prev_play_status)
            {
                return btif_avrcp_send_play_status_change_actual_rsp(curr_device->avrcp_channel, (uint8_t)play_status);
            }
        }
        return BT_STS_FAILED;
    }
    TRACE(0, "bt_avrcp_send_notify_rsp: unsupported event %d", event_id);
    return BT_STS_FAILED;
}

#endif /* BT_AVRCP_SUPPORT */


#ifdef BT_HFP_SUPPORT

bt_status_t bt_hf_init(bt_hf_callback_t callback)
{
    if (callback)
    {
        bt_add_event_callback((bt_event_callback_t)callback, BT_EVENT_MASK_HFP_HF_GROUP);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_hf_cleanup(void)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_hf_connect(const bt_bdaddr_t *remote)
{
    app_bt_reconnect_hfp_profile((bt_bdaddr_t *)remote);
    return BT_STS_PENDING;
}

bt_status_t bt_hf_disconnect(const bt_bdaddr_t *remote)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(remote);
    if (curr_device)
    {
        app_bt_disconnect_hfp_profile(curr_device->hf_channel);
    }
    return BT_STS_PENDING;
}

bt_status_t bt_hf_connect_audio(const bt_bdaddr_t *bd_addr)
{
    uint8_t device_id = app_bt_get_device_id_byaddr((bt_bdaddr_t *)bd_addr);
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device == NULL)
    {
        return BT_STS_FAILED;
    }
    return btif_hf_create_audio_link(curr_device->hf_channel);
}

bt_status_t bt_hf_disconnect_audio(const bt_bdaddr_t *bd_addr)
{
    uint8_t device_id = app_bt_get_device_id_byaddr((bt_bdaddr_t *)bd_addr);
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device == NULL)
    {
        return BT_STS_FAILED;
    }
    return btif_hf_disc_audio_link(curr_device->hf_channel);
}

bt_status_t bt_hf_start_voice_recognition(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        status = btif_hf_enable_voice_recognition(curr_device->hf_channel, true);
    }
    return status;
}

bt_status_t bt_hf_stop_voice_recognition(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        status = btif_hf_enable_voice_recognition(curr_device->hf_channel, false);
    }
    return status;
}

bt_status_t bt_hf_volume_control(const bt_bdaddr_t *bd_addr, bt_hf_volume_type_t type, int volume)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        if (type == BT_HF_VOLUME_TYPE_SPK)
        {
            btif_hf_report_speaker_volume(curr_device->hf_channel, (uint8_t)volume);
        }
        else
        {
            btif_hf_report_mic_volume(curr_device->hf_channel, (uint8_t)volume);
        }
        status = btif_hf_enable_voice_recognition(curr_device->hf_channel, false);
    }
    return status;
    
}

bt_status_t bt_hf_dial(const bt_bdaddr_t *bd_addr, const char *number)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        if (number == NULL)
        {
            status = btif_hf_redial_call(curr_device->hf_channel);
        }
        else
        {
            status = btif_hf_dial_number(curr_device->hf_channel, (uint8_t *)number, number ? strlen(number) : 0);
        }
    }
    return status;
}

bt_status_t bt_hf_dial_memory(const bt_bdaddr_t *bd_addr, int location)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        status = btif_hf_dial_memory(curr_device->hf_channel, location);
    }
    return status;
}

bt_status_t bt_hf_handle_call_action(const bt_bdaddr_t *bd_addr, bt_hf_call_action_t action, int idx)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return status;
    }
    if (action == BT_HF_CALL_ACTION_ATA)
    {
        return btif_hf_answer_call(curr_device->hf_channel);
    }
    if (action == BT_HF_CALL_ACTION_CHUP)
    {
        return btif_hf_hang_up_call(curr_device->hf_channel);
    }
    if (action == BT_HF_CALL_ACTION_REDIAL)
    {
        return btif_hf_redial_call(curr_device->hf_channel);
    }
    if (action >= BT_HF_CALL_ACTION_CHLD_0 && action <= BT_HF_CALL_ACTION_CHLD_4)
    {
        return btif_hf_call_hold(curr_device->hf_channel, (btif_hf_hold_call_t)(action - BT_HF_CALL_ACTION_CHLD_0), 0);
    }
    if (action == BT_HF_CALL_ACTION_CHLD_1x || action == BT_HF_CALL_ACTION_CHLD_2x)
    {
        return btif_hf_call_hold(curr_device->hf_channel,
            action == BT_HF_CALL_ACTION_CHLD_1x ? BTIF_HF_HOLD_RELEASE_ACTIVE_CALLS : BTIF_HF_HOLD_HOLD_ACTIVE_CALLS,
            idx);
    }
    if (action >= BT_HF_CALL_ACTION_BTRH_0 && action <= BT_HF_CALL_ACTION_BTRH_2)
    {
        char at_cmd[] = {'A', 'T', '+', 'B', 'T', 'R', 'H', '=', '0' + (action - BT_HF_CALL_ACTION_BTRH_0), '\r', 0};
        return bt_hf_send_at_cmd(bd_addr, at_cmd);
    }
    return BT_STS_FAILED;
}

bt_status_t bt_hf_query_current_calls(const bt_bdaddr_t *bd_addr)
{
    return bt_hf_send_at_cmd(bd_addr, "AT+CLCC\r");
}

bt_status_t bt_hf_query_current_operator_name(const bt_bdaddr_t *bd_addr)
{
    return bt_hf_send_at_cmd(bd_addr, "AT+COPS?\r");
}

bt_status_t bt_hf_retrieve_subscriber_info(const bt_bdaddr_t *bd_addr)
{
    return bt_hf_send_at_cmd(bd_addr, "AT+CNUM\r");
}

bt_status_t bt_hf_send_dtmf(const bt_bdaddr_t *bd_addr, char code)
{
    char at_cmd[] = {'A', 'T', '+', 'V', 'T', 'S', '=', code, '\r', 0};
    return bt_hf_send_at_cmd(bd_addr, at_cmd);
}

bt_status_t bt_hf_request_last_voice_tag_number(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        status = btif_hf_attach_voice_tag(curr_device->hf_channel);
    }
    return status;
}

bt_status_t bt_hf_send_at_cmd(const bt_bdaddr_t *bd_addr, const char *at_cmd_str)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (curr_device)
    {
        status = btif_hf_send_at_cmd(curr_device->hf_channel, at_cmd_str);
        status = BT_STS_SUCCESS;
    }
    return status;
}

#if defined(BT_HFP_AG_ROLE)

bt_status_t bt_ag_init(bt_ag_callback_t callback)
{
    if (callback)
    {
        bt_add_event_callback((bt_event_callback_t)callback, BT_EVENT_MASK_HFP_AG_GROUP);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_cleanup(void)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_connect(const bt_bdaddr_t *bd_addr)
{
    bt_source_reconnect_hfp_profile(bd_addr);
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_disconnect(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return status;
    }
    return btif_ag_disconnect_service_link(curr_device->hf_channel);
}

bt_status_t bt_ag_connect_audio(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return status;
    }
    return btif_ag_create_audio_link(curr_device->hf_channel);
}

bt_status_t bt_ag_disconnect_audio(const bt_bdaddr_t *bd_addr)
{
    bt_status_t status = BT_STS_FAILED;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return status;
    }
    return btif_ag_disc_audio_link(curr_device->hf_channel);
}

bt_status_t bt_ag_start_voice_recoginition(const bt_bdaddr_t *bd_addr)
{
    return bt_ag_send_at_result(bd_addr, "\r\nBVRA: 1\r\n");
}

bt_status_t bt_ag_stop_voice_recoginition(const bt_bdaddr_t *bd_addr)
{
    return bt_ag_send_at_result(bd_addr, "\r\nBVRA: 0\r\n");
}

bt_status_t bt_ag_volume_control(const bt_bdaddr_t *bd_addr, bt_hf_volume_type_t type, int volume)
{
    char result[32] = {0};
    snprintf(result, sizeof(result), (type == BT_HF_VOLUME_TYPE_SPK) ? "\r\n+VGS: %d\r\n" : "\r\n+VGM: %d\r\n", volume);
    return bt_ag_send_at_result(bd_addr, result);
}

bt_status_t bt_ag_set_curr_at_upper_handle(const bt_bdaddr_t *bd_addr)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    return btif_ag_set_curr_at_upper_handle(curr_device->hf_channel);
}

bt_status_t bt_ag_cops_response(const bt_bdaddr_t *bd_addr, const char *operator_name)
{
    char result[32] = {0};
    if (operator_name && operator_name[0])
    {
        snprintf(result, sizeof(result), "\r\n+COPS: 1,0,\"%s\"\r\n", operator_name);
    }
    else
    {
        snprintf(result, sizeof(result), "\r\n+COPS: 0\r\n");
    }
    return bt_ag_send_at_result(bd_addr, result);
}

bt_status_t bt_ag_clcc_response(const bt_bdaddr_t *bd_addr, const bt_ag_clcc_status_t *status)
{
    char clcc[128] = {0};
    snprintf(clcc, sizeof(clcc), "\r\n+CLCC: %d,%d,%d,%d,%d,\"%s\",%d\r\n",
        status->index, status->dir, status->state, status->mode, status->mpty, status->number, status->number_type);
    return bt_ag_send_at_result(bd_addr, clcc);
}

bt_status_t bt_ag_cind_response(const bt_bdaddr_t *bd_addr, const bt_ag_cind_status_t *status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    char result[40] = {0};
    bt_ag_ind_status_t ag_status;
    bt_hf_call_ind_t call = BT_HF_CALL_NO_CALLS_IN_PROGRESS;
    bt_hf_callsetup_ind_t callsetup = BT_HF_CALLSETUP_NONE;
    bt_hf_callheld_ind_t callheld = BT_HF_CALLHELD_NONE;
    if (status->num_active || status->num_held)
    {
        call = BT_HF_CALL_CALLS_IN_PROGRESS;
    }
    if (status->call_state == BT_HF_CALL_STATE_INCOMING || status->call_state == BT_HF_CALL_STATE_WAITING)
    {
        callsetup = BT_HF_CALLSETUP_INCOMING;
    }
    else if (status->call_state == BT_HF_CALL_STATE_DIALING)
    {
        callsetup = BT_HF_CALLSETUP_OUTGOING;
    }
    else if (status->call_state == BT_HF_CALL_STATE_ALERTING)
    {
        callsetup = BT_HF_CALLSETUP_ALERTING;
    }
    if (status->num_active && status->num_held)
    {
        callheld = BT_HF_CALLHELD_HOLD_AND_ACTIVE;
    }
    else if (status->num_held)
    {
        callheld = BT_HF_CALLHELD_HOLD;
    }
    ag_status.service = status->service;
    ag_status.call = call;
    ag_status.callsetup = callsetup;
    ag_status.callheld = callheld;
    ag_status.signal = status->signal;
    ag_status.roam = status->roam;
    ag_status.battchg = status->battery_level;
    snprintf(result, sizeof(result), "\r\n+CIND: %d,%d,%d,%d,%d,%d,%d\r\n",
        ag_status.service, ag_status.call, ag_status.callsetup, ag_status.callheld,
        ag_status.signal, ag_status.roam, ag_status.battchg);
    btif_ag_set_ind_status(curr_device->hf_channel, &ag_status);
    return bt_ag_send_at_result(bd_addr, result);
}

bt_status_t bt_ag_device_status_change(const bt_bdaddr_t *bd_addr, const bt_ag_device_status_t *status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    bt_ag_ind_status_t ag_status = btif_ag_get_ind_status(curr_device->hf_channel);
    if (status->service != ag_status.service)
    {
        btif_ag_send_service_status(curr_device->hf_channel, status->service);
    }
    if (status->signal != ag_status.signal)
    {
        btif_ag_send_mobile_signal_level(curr_device->hf_channel, status->signal);
    }
    if (status->roam != ag_status.roam)
    {
        btif_ag_send_mobile_roam_status(curr_device->hf_channel, status->roam);
    }
    if (status->battery_level != ag_status.battchg)
    {
        btif_ag_send_mobile_battery_level(curr_device->hf_channel, status->battery_level);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_phone_status_change(const bt_bdaddr_t *bd_addr, const bt_ag_phone_status_t *status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    bt_ag_ind_status_t ag_status = btif_ag_get_ind_status(curr_device->hf_channel);
    bt_hf_call_ind_t call = BT_HF_CALL_NO_CALLS_IN_PROGRESS;
    bt_hf_callsetup_ind_t callsetup = BT_HF_CALLSETUP_NONE;
    bt_hf_callheld_ind_t callheld = BT_HF_CALLHELD_NONE;
    if (status->num_active || status->num_held)
    {
        call = BT_HF_CALL_CALLS_IN_PROGRESS;
    }
    if (status->call_state == BT_HF_CALL_STATE_INCOMING || status->call_state == BT_HF_CALL_STATE_WAITING)
    {
        callsetup = BT_HF_CALLSETUP_INCOMING;
    }
    else if (status->call_state == BT_HF_CALL_STATE_DIALING)
    {
        callsetup = BT_HF_CALLSETUP_OUTGOING;
    }
    else if (status->call_state == BT_HF_CALL_STATE_ALERTING)
    {
        callsetup = BT_HF_CALLSETUP_ALERTING;
    }
    if (status->num_active && status->num_held)
    {
        callheld = BT_HF_CALLHELD_HOLD_AND_ACTIVE;
    }
    else if (status->num_held)
    {
        callheld = BT_HF_CALLHELD_HOLD;
    }
    if (callsetup == BT_HF_CALLSETUP_INCOMING)
    {
        if (ag_status.call)
        {
            btif_ag_send_calling_ring(curr_device->hf_channel, status->number);
        }
        else
        {
            btif_ag_send_call_waiting_notification(curr_device->hf_channel, status->number);
        }
    }
    if (call != ag_status.call)
    {
        btif_ag_send_call_active_status(curr_device->hf_channel, call);
    }
    if (callsetup != ag_status.callsetup)
    {
        btif_ag_send_callsetup_status(curr_device->hf_channel, callsetup);
    }
    if (callheld != ag_status.callheld)
    {
        btif_ag_send_callheld_status(curr_device->hf_channel, callheld);
    }
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_send_at_result(const bt_bdaddr_t *bd_addr, const char *at_result)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    return btif_ag_send_result_code(curr_device->hf_channel, at_result, at_result ? strlen(at_result) : 0);
}

bt_status_t bt_ag_send_response_code(const bt_bdaddr_t *bd_addr, bt_hf_at_response_t code, int cme_error)
{
    const char *at_result = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return BT_STS_FAILED;
    }
    if (code == BT_HF_AT_RESPONSE_ERROR_CME)
    {
        if (btif_ag_cmee_enabled(curr_device->hf_channel))
        {
            char result[32] = {0};
            snprintf(result, sizeof(result), "\r\n+CME ERROR: %d\r\n", cme_error);
            return bt_ag_send_at_result(bd_addr, result);
        }
        code = BT_HF_AT_RESPONSE_ERROR;
    }
    switch (code)
    {
        case BT_HF_AT_RESPONSE_OK: at_result = "\r\nOK\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR: at_result = "\r\nERROR\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR_NO_CARRIER: at_result = "\r\nNO CARRIER\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR_BUSY: at_result = "\r\nBUSY\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR_NO_ANSWER: at_result = "\r\nNO ANSWER\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR_DELAYED: at_result = "\r\nDELAYED\r\n"; break;
        case BT_HF_AT_RESPONSE_ERROR_BLACKLISTED: at_result = "\r\nBLACKLISTED\r\n"; break;
        default: TRACE(0, "bt_ag_send_response_code: invalid code %d", code); break;
    }
    if (at_result == NULL)
    {
        return BT_STS_FAILED;
    }
    return bt_ag_send_at_result(bd_addr, at_result);
}

bt_status_t bt_ag_set_sco_allowed(const bt_bdaddr_t *bd_addr, bool sco_enable)
{
    return BT_STS_SUCCESS;
}

bt_status_t bt_ag_send_bsir(const bt_bdaddr_t *bd_addr, bool in_band_ring_enable)
{
    char at_result[] = {'\r', '\n', '+', 'B', 'S', 'I', 'R', ':', ' ', in_band_ring_enable ? '1' : '0', '\r', '\n', 0};
    return bt_ag_send_at_result(bd_addr, at_result);
}

int bt_ag_is_noise_reduction_supported(const bt_bdaddr_t *bd_addr)
{
    uint32_t hf_features = 0;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return false;
    }
    hf_features = btif_ag_get_hf_features(curr_device->hf_channel);
    return (hf_features & BT_HF_FEAT_ECNR) != 0;
}

int bt_ag_is_voice_recognition_supported(const bt_bdaddr_t *bd_addr)
{
    uint32_t hf_features = 0;
    struct BT_DEVICE_T *curr_device = app_bt_get_connected_device_byaddr(bd_addr);
    if (!curr_device)
    {
        return false;
    }
    hf_features = btif_ag_get_hf_features(curr_device->hf_channel);
    return (hf_features & BT_HF_FEAT_VR) != 0;
}

#endif /* BT_HFP_AG_ROLE */
#endif /* BT_HFP_SUPPORT */
