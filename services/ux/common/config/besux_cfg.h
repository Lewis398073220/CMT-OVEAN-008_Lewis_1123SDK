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
/***
 * besbt_cfg.h
 */

#ifndef __BESUX_CFG_H__
#define __BESUX_CFG_H__
#include <stdbool.h>

#ifdef BT_HFP_SUPPORT
#include "hfp_api.h"
#include "app_hfp.h"
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#endif

#include "dip_api.h"
#include "app_dip.h"

#ifdef __cplusplus
extern "C" {
#endif

struct besux_cfg_t{
    bool ux_hfp_support;
    bool ux_map_support;
    bool ux_hid_support;
    bool ux_dip_support;
    bool ux_set_master_on_tws_disconnected;
    bool ux_always_in_discoverable_mode;
    bool ux_tws_rs_by_btc_support;
    bool ux_tws_rs_without_mobile;
    bool ux_role_switch_monitor;
    bool ux_set_right_is_master;
    bool ux_use_safe_disconnect;
    bool ux_report_evt_to_cudtomux_support;
    bool ux_codec_error_handling;
    bool ux_bes_ota_support;
    bool ux_gam_voice_support;
    bool ux_interaction_support;
    bool ux_prompt_self_mgr_support;
    bool ux_media_player_support;
    bool ux_bt_sync_support;
    bool ux_ble_ctkd_support;
    bool ux_glasses_project_support;
    bool ux_bisto_support;
    bool ux_ai_voice_support;
    bool ux_me_mediator_support;
};

#ifndef BT_HFP_SUPPORT
#define BTIF_HF_CALL_SETUP_NONE   0
typedef uint8_t bt_hfp_chan_state_t;
typedef uint8_t hfp_sco_codec_t;
typedef uint8_t ibrt_hfp_status_t;
typedef uint8_t btif_hf_channel_t;
#endif

#ifndef BISTO_ENABLED
#define  GSOUND_CHANNEL_CONTROL   0
#define  GSOUND_CHANNEL_AUDIO     1
#endif

typedef void (*besux_dip_info_queried_callback)(uint8_t *remote_addr, app_dip_pnp_info_t *pnp_info);

struct ux_hfp_support_cb_t{
    int (*besux_hfp_siri_voice)(bool en);
    bool (*besux_hfp_is_sco_active)(void);
    bt_hfp_chan_state_t (*besux_get_hf_chan_state)(btif_hf_channel_t* chan_h);
    void (*besux_register_sco_link)(uint8_t device_id, struct bdaddr_t *remote);
    uint32_t (*besux_restore_hfp_app_ctx)(bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_save_hfp_app_ctx)(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
    bool (*besux_is_sco_connected)(uint8_t device_id);
    void (*besux_send_call_hold_request)(uint8_t device_id, btif_hf_hold_call_t action);
    uint8_t (*besux_get_call_setup)(void);
    bool (*besux_hfp_profile_connecting)(bt_bdaddr_t *bdaddr_p);
    bt_status_t (*besux_force_disconnect_hfp_profile)(uint8_t device_id,uint8_t reason);
    uint32_t (*besux_hfp_profile_restore_ctx)(bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_hfp_profile_save_ctx)(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
    bt_status_t (*besux_hf_sync_conn_audio_connected)(hfp_sco_codec_t codec,uint16_t conhdl);
    bt_status_t (*besux_hf_sync_conn_audio_disconnected)(uint16_t conhdl);
    bool (*besux_hfp_is_profile_initiator)(const bt_bdaddr_t* remote);
    hfp_sco_codec_t (*besux_hf_get_negotiated_codec)(btif_hf_channel_t* chan_h);
    void (*besux_receive_peer_sco_codec_info)(const void* remote, uint8_t codec);
    int (*besux_hfp_ibrt_service_connected_mock)(uint8_t device_id);
    void (*besux_hfp_update_local_volume)(int id, uint8_t localVol);
    int (*besux_sync_get_hfp_status)(uint8_t device_id,ibrt_hfp_status_t *hfp_status);
    int (*besux_sync_set_hfp_status)(uint8_t device_id,ibrt_hfp_status_t *hfp_status);
};

struct ux_map_support_cb_t {
    uint32_t (*besux_map_profile_save_ctx)(const bt_bdaddr_t *remote, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_save_map_app_ctx)(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_map_profile_restore_ctx)(const bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_restore_map_app_ctx)(bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
};

struct ux_hid_support_cb_t {
    uint32_t (*besux_hid_profile_save_ctx)(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_save_hid_app_ctx)(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_hid_profile_restore_ctx)(bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
    uint32_t (*besux_restore_hid_app_ctx)(bt_bdaddr_t *bdaddr_p, uint8_t *buf, uint32_t buf_len);
    bt_status_t (*besux_hid_profile_connect)(const bt_bdaddr_t *remote, int capture);
    bt_status_t (*besux_hid_profile_disconnect)(const bt_bdaddr_t *remote);
};

struct ux_use_safe_disconnect_cb_t{
    bool (*besux_custom_stop_ibrt_ongoing)(void);
    void (*besux_custom_all_safe_disconnect)(void);
};

struct ux_dip_support_cb_t{
    btif_dip_pnp_info* (*besux_dip_get_device_info)(bt_bdaddr_t *remote);
    void (*besux_dip_sync_init)(void);
    void (*besux_register_dip_info_queried_callback)(besux_dip_info_queried_callback func);
};

struct ux_besota_support_cb_t{
    void (*besux_set_ota_role_switch_initiator)(bool is_initiate);
    void (*besux_ota_send_role_switch_req)(void);
    uint8_t (*besux_get_bes_ota_state)(void);
    bool (*besux_ota_is_in_progress)(void);
    void (*besux_ota_send_role_switch_complete)(void);
};

struct ux_ai_voice_support_cb_t {
    uint32_t (*besux_ai_tws_role_switch_prepare)(uint32_t *wait_ms);
    void (*besux_ai_tws_role_switch)(void);
    void (*besux_ai_tws_master_role_switch_prepare)(void);
    void (*besux_ai_tws_role_switch_prepare_done)(void);
    void (*besux_ai_if_mobile_connect_handle)(void *_addr);
    void (*besux_ai_tws_gsound_sync_init)(void);
    void (*besux_ai_tws_role_switch_complete)(void);
    void (*besux_ai_tws_sync_init)(void);
    uint8_t (*besux_ai_tws_get_local_role)(void);
    void (*besux_tws_update_roleswitch_initiator)(uint8_t role);
    bool (*besux_tws_request_roleswitch)(void);
    void (*besux_gsound_set_ble_connect_state)(uint8_t chnl, bool state);
    void (*besux_gsound_on_bt_link_disconnected)(uint8_t *addr);
    void (*besux_gsound_on_system_role_switch_done)(uint8_t newRole);
    void (*besux_gsound_tws_role_update)(uint8_t newRole);
};

struct ux_other_macro_support_cb_t{
    void (*besux_gma_secret_key_send)(void);
    bool (*besux_bixby_hotword_detect_value_get)(void);
    void (*besux_media_PlayAudio)(AUD_ID_ENUM id,uint8_t device_id);
    void (*besux_trigger_media_play)(AUD_ID_ENUM id, uint8_t device_id, uint16_t aud_pram);
    void (*besux_media_PlayAudio_standalone)(AUD_ID_ENUM id, uint8_t device_id);
};

struct ux_bt_sync_support_cb_t{
    bool (*besux_bt_sync_enable)(uint32_t opCode, uint8_t length, uint8_t *p_buff);
    void (*besux_bt_sync_tws_cmd_handler)(uint8_t *p_buff, uint16_t length);
    void (*besux_bt_sync_send_tws_cmd_done)(uint8_t *ptrParam, uint16_t paramLen);
};

struct ux_me_mediator_support_cb_t{
    bool (*besux_mediator_event_handler)(uint16_t evt_id, void* pdata);
};

extern const struct besux_cfg_t besux_cfg;

const struct ux_hfp_support_cb_t *besux_get_hfp_support_cbs(void);

const struct ux_map_support_cb_t *besux_get_map_support_cbs(void);

const struct ux_hid_support_cb_t *besux_get_hid_support_cbs(void);

const struct ux_use_safe_disconnect_cb_t *besux_get_safe_disconnect_cbs(void);

const struct ux_besota_support_cb_t *besux_get_besota_support_cbs(void);

const struct ux_dip_support_cb_t *besux_get_dip_support_cbs(void);

const struct ux_other_macro_support_cb_t *besux_get_other_macro_support_cbs(void);

const struct ux_ai_voice_support_cb_t *besux_get_ai_voice_support_cbs(void);

const struct ux_bt_sync_support_cb_t *besux_get_bt_sync_support_cbs(void);

const struct ux_me_mediator_support_cb_t *besux_get_me_mediator_support_cbs(void);

#ifdef __cplusplus
}
#endif
#endif /* __BESUX_CFG_H__ */
