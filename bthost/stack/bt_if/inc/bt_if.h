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
#ifndef __BT_IF_H_
#define __BT_IF_H_
#include <stdint.h>
#include "bluetooth.h"
#include "me_api.h"
#include "besbt.h"

#include "bt_callback_func.h"

//Application ID,indentify profle app context

#ifdef __cplusplus
extern "C" {
#endif

#define    BTIF_TWS_LINK_CONNECTED          (1 << 0)
#define    BTIF_TWS_L2CAP_CONNECTED         (1 << 1)

#if defined(IBRT)
uint64_t btif_app_get_app_id_from_spp_flag(uint8_t spp_flag);
uint8_t btif_app_get_spp_flag_from_app_id(uint64_t app_id);
void btif_ibrt_stack_clean_slave_status(const bt_bdaddr_t* remote);
#endif

#define RFCOMM_MAX_SYNC_DATA_SIZE (20)

// remain one Byte
#define RFCOMM_MAX_SYNC_TXRX_CREDIT_DATA_SIZE (14)

struct a2dp_command_t {
    bool is_valid;
    uint8_t transaction;
    uint8_t signal_id;
} __attribute__ ((packed));

struct btif_sync_txrx_credit_after_exif_t {
    bt_bdaddr_t remote_bdaddr;
    uint8_t rfcomm_sync_txrx_credit_data[RFCOMM_MAX_SYNC_TXRX_CREDIT_DATA_SIZE];
} __attribute__ ((packed));

struct btif_sync_data_to_new_master_t {
    bt_bdaddr_t remote_bdaddr;
    struct a2dp_command_t stream_cmd;
    uint8_t rfcomm_sync_data[RFCOMM_MAX_SYNC_DATA_SIZE];
} __attribute__ ((packed));

typedef struct{
    unsigned char   *dev_name;
    uint8_t         name_str_len;
    uint8_t         *uuid_buf;
    uint16_t        uuid_buf_len;
} btif_eir_raw_data_t;

bool btif_ibrt_master_wait_remote_new_master_ready(const bt_bdaddr_t *remote);

void btif_ibrt_master_tws_switch_set_start(const bt_bdaddr_t* remote);

void btif_ibrt_slave_tws_switch_set_start(const bt_bdaddr_t* remote);

void btif_ibrt_master_become_slave(const bt_bdaddr_t* remote);

void btif_ibrt_slave_become_master(const bt_bdaddr_t* remote);

void btif_ibrt_new_master_to_clear_ibrt_ignore_tx_data_start_bit(const bt_bdaddr_t* remote);

void btif_ibrt_old_master_receive_ready_req(struct btif_sync_data_to_new_master_t *sync_data, const bt_bdaddr_t *remote);

void btif_ibrt_new_master_receive_ready_rsp(struct btif_sync_data_to_new_master_t *sync_data);

enum pair_event
{
    PAIR_EVENT_NUMERIC_REQ,
    PAIR_EVENT_COMPLETE,
    PAIR_EVENT_FAILED,
};

typedef void (*btif_pairing_callback_t)(enum pair_event evt, void *data);

void btif_pairing_register_callback(btif_pairing_callback_t callback);

typedef void (*stack_ready_callback_t) (int status);
typedef void (*stack_start_init_callback_t) (void);

int bt_stack_register_ready_callback(stack_ready_callback_t ready_cb);
int bt_stack_register_init_start_callback(stack_start_init_callback_t cb);

void bt_stack_register_heap(void *start_address, uint32_t block_size);

int bt_stack_initilize(void);
void bt_chip_common_init(bool ble_only_enabled);

int bt_stack_config(const unsigned char *dev_name, uint8_t len);
void btif_update_bt_name(const unsigned char *dev_name, uint8_t len);
int bt_set_local_dev_name(const unsigned char *dev_name, uint8_t len);
int bt_set_local_clock(uint32_t clock);
void bt_process_stack_events(void);
uint8_t bt_get_max_sco_number(void);
void bt_set_max_sco_number(uint8_t sco_num);
bool btif_is_gatt_over_br_edr_enabled(void);
#if defined(__GATT_OVER_BR_EDR__)
bool btif_is_gatt_over_br_edr_allowed_send(uint8_t conidx);
#endif
bool btif_is_ctkd_over_br_edr_enabled(void);

void btif_set_btstack_chip_config(void *config);
void btif_set_extended_inquiry_response(uint8_t* eir, uint32_t len);
void btif_avrcp_ct_register_notification_event(uint8_t device_id, uint8_t event);
int btif_me_send_hci_cmd(uint16_t opcode, uint8_t *param_data_ptr, uint8_t param_len);

#if defined(IBRT)
uint32_t btif_save_app_bt_device_ctx(uint8_t *ctx_buffer,uint8_t psm_context_mask);
uint32_t btif_set_app_bt_device_ctx(uint8_t *ctx_buffer,uint8_t psm_context_mask,uint8_t bt_devices_idx, uint8_t rm_detbl_idx, uint8_t avd_ctx_device_idx);
bool btif_ibrt_master_wait_remote_new_master_ready(const bt_bdaddr_t *remote);
void btif_ibrt_master_tws_switch_set_start(const bt_bdaddr_t* remote);
void btif_ibrt_slave_tws_switch_set_start(const bt_bdaddr_t* remote);
void btif_ibrt_master_become_slave(const bt_bdaddr_t* remote);
void btif_ibrt_slave_become_master(const bt_bdaddr_t* remote);
#endif

void btif_config_gatt_over_br_edr(bool isEnable);
bool btif_is_gatt_over_br_edr_enabled(void);
bool btif_is_gatt_over_br_edr_allowed_send(uint8_t conidx);

typedef void (*bt_eir_fill_manufacture_data)(uint8_t *buff, uint32_t* offset);

void btif_eir_global_srv_uuid_gather_func_register(bt_gather_global_srv_uuids func);
void btif_eir_ble_audio_manufacture_info_register(bt_eir_fill_manufacture_data func);

uint32_t btif_get_class_of_device(void);
void btif_osapi_lock_stack(void);
void btif_osapi_unlock_stack(void);
int btif_osapi_lock_is_exist(void);
void btif_osapi_notify_evm(void);
struct btm_conn_item_t *btif_ibrt_create_new_snoop_link(bt_bdaddr_t *remote, uint16 conn_handle);
void btif_conn_ibrt_disconnected_handle(struct btm_conn_item_t *btm_conn);
int btif_check_buffer_available(int size);
int8 btif_l2cap_send_data( uint32 l2cap_handle, uint8 *data, uint32 datalen, void *context);
void btif_create_besaud_extra_channel(void* remote_addr, bt_l2cap_callback_t l2cap_callback);
int btif_check_l2cap_mtu_buffer_available(void);
void btif_colist_addto_head(struct list_node *n, struct list_node *head);
void btif_colist_addto_tail(struct list_node *n, struct list_node *head);
void btif_colist_insert_after(struct list_node *n, struct list_node *head);
void btif_colist_delete(struct list_node *entry);
int btif_colist_is_node_on_list(struct list_node *list, struct list_node *node);
int btif_colist_item_count(struct list_node *list);
struct list_node *btif_colist_get_head(struct list_node *head);
int btif_colist_is_list_empty(struct list_node *head);
BOOL btif_is_list_circular(list_entry_t * list);
void btif_insert_tail_list(list_entry_t * head, list_entry_t * entry);
list_entry_t *btif_remove_head_list(list_entry_t * head);
void btif_remove_entry_list(list_entry_t * entry);
struct coheap_buff_t *btif_coheap_bt_malloc_with_ca(uint16 size, uint32 ca, uint32 line, bool allow_fail_alloc);
void btif_coheap_free_with_ca(struct coheap_buff_t *p, uint32 ca, uint32 line);
void btif_coheap_dump_statistics(void);
void btif_coheap_enable_debug(bool enable);
void btif_ecc_gen_new_secret_key_192(uint8_t* secret_key192);
void btif_ecc_gen_new_public_key_192(uint8_t* secret_key,uint8_t* out_public_key);
void btif_hci_enable_cmd_evt_debug(bool enable);
void btif_hci_enable_tx_flow_debug(bool enable);
void btif_hci_enable_tx_0c35_without_alloc(bool enable);
void btif_hci_register_controller_state_check(void (*cb)(void));
void btif_hci_register_pending_too_many_rx_acl_packets(void (*cb)(void));
#ifdef IBRT
void btif_hci_set_start_ibrt_reserve_buff(bool reserve);
void btif_hci_register_acl_tx_buff_tss_process(void (*cb)(void));
#endif
uint8_t *btif_hci_get_curr_pending_cmd(uint16_t cmd_opcode);
uint16_t btif_hci_get_curr_cmd_opcode(void);
int btif_hci_count_free_bt_tx_buff(void);

void btif_pts_av_create_channel(bt_bdaddr_t *btaddr);
void btif_pts_av_disc_channel(void);
void btif_pts_av_close_channel(void);
void btif_pts_av_set_sink_delay(void);
void btif_pts_ar_connect(bt_bdaddr_t *btaddr);
void btif_pts_ar_disconnect(void);
void btif_pts_ar_panel_stop(void);
void btif_pts_ar_panel_play(void);
void btif_pts_ar_panel_pause(void);
void btif_pts_ar_panel_forward(void);
void btif_pts_ar_panel_backward(void);
void btif_pts_ar_volume_up(void);
void btif_pts_ar_volume_down(void);
void btif_pts_ar_volume_notify(void);
void btif_pts_ar_volume_change(void);
void btif_pts_ar_set_absolute_volume(void);
void btif_pts_hf_create_service_link(bt_bdaddr_t *btaddr);
void btif_pts_hf_disc_service_link(void);
void btif_pts_hf_create_audio_link(void);
void btif_pts_hf_disc_audio_link(void);
void btif_pts_hf_send_key_pressed(void);
void btif_pts_hf_redial_call(void);
void btif_pts_hf_dial_number(void);
void btif_pts_hf_dial_number_memory_index(void);
void btif_pts_hf_dial_number_invalid_memory_index(void);
void btif_pts_hf_answer_call(void);
void btif_pts_hf_hangup_call(void);
void btif_pts_hf_vr_enable(void);
void btif_pts_hf_vr_disable(void);
void btif_pts_hf_list_current_calls(void);
void btif_pts_hf_release_active_call_2(void);
void btif_pts_hf_hold_active_call(void);
void btif_pts_hf_hold_active_call_2(void);
void btif_pts_hf_release_active_call(void);
void btif_pts_hf_hold_call_transfer(void);
void btif_pts_hf_send_ind_1(void);
void btif_pts_hf_send_ind_2(void);
void btif_pts_hf_send_ind_3(void);
void btif_pts_hf_update_ind_value(void);
void btif_pts_hf_report_mic_volume(void);
void btif_pts_hf_attach_voice_tag(void);
void btif_pts_hf_ind_activation(void);
void btif_pts_rfc_register_channel(void);
void btif_pts_rfc_close(void);
void btif_pts_rfc_close_dlci_0(void);
void btif_pts_rfc_send_data(void);
void btif_pts_av_send_discover(void);
void btif_pts_av_send_getcap(void);
void btif_pts_av_send_setconf(void);
void btif_pts_av_send_getconf(void);
void btif_pts_av_send_reconf(void);
void btif_pts_av_send_open(void);
void btif_pts_av_send_close(void);
void btif_pts_av_send_abort(void);
void btif_pts_av_send_getallcap(void);
void btif_pts_av_send_suspend(void);
void btif_pts_av_send_start(void);
void btif_pts_av_create_media_channel(void);
void btif_register_is_pts_address_check_callback(bool (*cb)(void *remote));
void btif_pts_l2c_disc_channel(void);
void btif_pts_l2c_send_data(void);

void btif_pts_hf_acs_bv_09_i_set_enable(void);
void btif_pts_hf_acs_bv_09_i_set_disable(void);
void btif_pts_hf_acs_bi_13_i_set_enable(void);
void btif_pts_hf_acs_bi_13_i_set_disable(void);

void btif_pts_reject_INVALID_OBJECT_TYPE(void);
void btif_pts_reject_INVALID_CHANNELS(void);
void btif_pts_reject_INVALID_SAMPLING_FREQUENCY(void);
void btif_pts_reject_INVALID_DRC(void);
void btif_pts_reject_NOT_SUPPORTED_OBJECT_TYPE(void);
void btif_pts_reject_NOT_SUPPORTED_CHANNELS(void);
void btif_pts_reject_NOT_SUPPORTED_SAMPLING_FREQUENCY(void);
void btif_pts_reject_NOT_SUPPORTED_DRC(void);
void btif_pts_reject_INVALID_CODEC_TYPE(void);
void btif_pts_reject_INVALID_CHANNEL_MODE(void);
void btif_pts_reject_INVALID_SUBBANDS(void);
void btif_pts_reject_INVALID_ALLOCATION_METHOD(void);
void btif_pts_reject_INVALID_MINIMUM_BITPOOL_VALUE(void);
void btif_pts_reject_INVALID_MAXIMUM_BITPOOL_VALUE(void);
void btif_pts_reject_INVALID_BLOCK_LENGTH(void);
void btif_pts_reject_INVALID_CP_TYPE(void);
void btif_pts_reject_INVALID_CP_FORMAT(void);
void btif_pts_reject_NOT_SUPPORTED_CODEC_TYPE(void);

#if defined(BT_SOURCE)
void btif_pts_source_cretae_media_channel(void);
void btif_pts_source_send_close_cmd(void);
void btif_pts_source_send_discover_cmd(void);
void btif_pts_source_send_get_capability_cmd(void);
void btif_pts_source_send_set_configuration_cmd(void);
void btif_pts_source_send_get_configuration_cmd(void);
void btif_pts_source_send_reconfigure_cmd(void);
void btif_pts_source_send_open_cmd(void);
void btif_pts_source_send_abort_cmd(void);
void btif_pts_source_send_get_all_capability_cmd(void);
void btif_pts_source_send_suspend_cmd(void);
void btif_pts_source_send_start_cmd(void);
void btif_update_name_and_uuid(btif_eir_raw_data_t *eir_raw_data);

#endif

void btif_config_ctkd_over_br_edr(bool isEnable);
bool btif_is_ctkd_over_br_edr_enabled(void);

#ifdef __cplusplus
}
#endif /*  */

#endif /*__BT_IF_H_*/
