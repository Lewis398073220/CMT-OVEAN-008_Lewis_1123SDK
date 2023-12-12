/***************************************************************************
 *
 * Copyright 2015-2023 BES.
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
#ifndef __BLE_GAP_I_H__
#define __BLE_GAP_I_H__
#include "bluetooth.h"
#include "gap_service.h"
#include "hci_i.h"
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint8_t opcode;
    uint8_t io_cap; // RFU if SC pairing intiated over BR/EDR
    uint8_t oob_flag; // 0x00 OOB auth data not present, 0x01 present, RFU if SC pairing intiated over BR/EDR
    uint8_t auth_req; // RFU if SC pairing intiated over BR/EDR except the CT2 bit
    uint8_t max_enc_key_size; // max encryption key size the device can support, 7 to 16 octets
    uint8_t init_key_dist; // which keys the initiator is requesting to distribute / generate or use during the Transport Specific Key Distribution phase
    uint8_t resp_key_dist; // which keys the initiator is requesting the responder to distribute / generate or use during the TSKD phase
} __attribute__ ((packed)) smp_pairing_req_t;

typedef void (*gap_user_confirm_func_t)(void *priv, bool user_confirmed);
typedef void (*gap_data_func_t)(void *priv, const uint8_t *data);
typedef void (*gap_passkey_func_t)(void *priv, uint32_t passkey);
typedef void (*gap_oob_auth_data_callback_t)(void *priv, const gap_smp_oob_auth_data_t *data);

bt_status_t gap_send_hci_le_encrypt(const uint8_t *key, const uint8_t *data,
        hci_cmd_evt_func_t cmd_cb, void *priv, void *cont, void *cmpl);
bt_status_t gap_send_hci_le_rand(hci_cmd_evt_func_t cmpl_status_cb, void *priv, void *cont);
bt_status_t gap_send_hci_le_read_local_p256_public_key(hci_cmd_evt_func_t cmpl_status_cb, void *priv, void *cont);
bt_status_t gap_send_hci_le_gen_dhkey(const uint8_t *pkx, const uint8_t *pky, bool use_debug_private_key,
        hci_cmd_evt_func_t cmpl_status_cb, void *priv, void *cont);
bt_status_t gap_send_hci_le_enable_encryption(gap_conn_item_t *conn, const uint8_t *rand, const uint8_t *ediv, const uint8_t *ltk,
        hci_cmd_evt_func_t cmd_cb, void *priv, void *cont);
bt_status_t gap_send_le_ltk_request_reply(gap_conn_item_t *conn, const uint8_t *ltk, bool positive_reply);
bt_status_t gap_ask_user_numeric_comparison(gap_conn_item_t *conn, uint32_t user_confirm_value, gap_user_confirm_func_t cb, void *priv);
bt_status_t gap_ask_input_6_digit_passkey(gap_conn_item_t *conn, gap_passkey_func_t passkey_cb, void *priv);
bt_status_t gap_ask_display_6_digit_passkey(gap_conn_item_t *conn, uint32_t passkey);
bt_status_t gap_get_tk_from_oob_data(gap_conn_item_t *conn, gap_data_func_t tk_cb, void *priv);
bt_status_t gap_get_peer_oob_auth_data(gap_conn_item_t *conn, gap_oob_auth_data_callback_t oob_cb, void *priv);
bt_status_t gap_get_local_oob_auth_data(gap_conn_item_t *conn, gap_oob_auth_data_callback_t oob_cb, void *priv);
bt_status_t gap_start_tx_block_authentication(gap_conn_item_t *conn, gap_who_started_auth_t who);
bt_status_t gap_start_rx_block_authentication(gap_conn_item_t *conn, gap_who_started_auth_t who);
bt_status_t gap_tx_rx_block_authentication(gap_conn_item_t *conn, gap_who_started_auth_t who);
bt_status_t gap_start_eatt_block_authentication(gap_conn_item_t *conn, gap_who_started_auth_t who);

void gap_prepare_ble_stack(void);
void gap_gen_local_random(uint8_t *random, uint8_t bytes);
bool gap_is_pairing_method_accept(const gap_bond_sec_t *sec, uint8_t accept_method);
smp_requirements_t gap_local_smp_requirements(gap_conn_item_t *conn, smp_pairing_req_t *peer_req);
int gatt_conn_event_handler(uintptr_t connhdl, gap_conn_event_t event, gap_conn_callback_param_t param);
bt_status_t gatt_conn_ready_handler(gap_conn_item_t *conn);
void gatt_continue_rx_packet(gap_conn_item_t *item);
void gatt_continue_tx_packet(gap_conn_item_t *item);
bool gap_load_gatt_client_cache(gap_bond_sec_t *bond);
void gap_free_gatt_client_cache(gap_bond_sec_t *bond);
void gap_clear_gatt_client_cache(const gap_bond_sec_t *bond);
bool gap_has_gatt_client_cache(gap_bond_sec_t *bond);
uint8_t gap_impl_start_advertising(bt_addr_type_t own_addr_type, gap_adv_callback_t cb, uint32_t cont_filter_legacy_conn_scan, const bt_bdaddr_t *custom_local_addr);
void gap_report_mtu_is_exchanged(gap_conn_item_t *conn, uint16_t mtu);
void data_path_init(uint8_t init_type);
void bap_audio_evt_handle(uint8_t subcode, const uint8_t *evt_data, uint8_t len);
ble_bdaddr_t gap_ctkd_get_le_identity_address(gap_conn_item_t *conn);
void gap_ctkd_notify_ltk_derived(gap_conn_item_t *bredr_conn, bt_addr_type_t peer_type, const bt_bdaddr_t *peer_addr, bool wait_peer_kdist);
void gap_ctkd_notify_link_key_derived(gap_conn_item_t *conn, const bt_bdaddr_t *bt_addr, const uint8_t *link_key, bool wait_peer_ia);
void gap_ltk_from_lk_generated(gap_conn_item_t *conn, const uint8_t *ltk, bool waiting_peer_kdist);
void gap_lk_from_ltk_generated(gap_conn_item_t *conn, const uint8_t *link_key, bool waiting_peer_ia);
void gap_recv_smp_encrypted(gap_conn_item_t *conn, bool new_pair, uint8_t error_code);
void gap_legacy_ltk_ediv_rand_generated(gap_conn_item_t *conn);
void gap_recv_peer_legacy_ltk_ediv_rand(gap_conn_item_t *conn);
void gap_recv_peer_irk_ia(gap_conn_item_t *conn);
void gap_recv_peer_csrk(gap_conn_item_t *conn);
bool gap_is_directly_send_sec_error_rsp(void);
void gap_set_min_conn_interval(uint16_t min_conn_interval_1_25ms);
void gap_set_default_key_size(uint8_t key_size);
void gap_set_default_auth_req(uint8_t mitm_auth);
void gap_pts_set_ble_l2cap_test(bool test);
bool gap_is_pts_ble_l2cap_test(void);
void gap_pts_set_use_passkey_entry(void);
void gap_pts_set_use_oob_method(void);
void gap_pts_set_no_mitm_auth(void);
void gap_pts_set_display_only(void);
void gap_pts_set_keyboard_only(void);
void gap_pts_set_no_bonding(void);
void gap_pts_set_dont_start_smp(bool dont_auto_start_smp);
bool gap_pts_get_dont_start_smp(void);
void gap_pts_gen_linkkey_from_ltk(void);
void gap_pts_set_dist_irk_only(void);
void gap_pts_set_dist_csrk(void);

/*SMP INTF*/
void smp_init(void);
void smp_pairing_end(gap_conn_item_t *conn, uint8_t error_code);
bt_status_t smp_start_authentication(gap_conn_item_t *conn, gap_who_started_auth_t who, uint32_t ca);
bt_status_t smp_bredr_ctkd_request(uint16_t connhdl);
void smp_continue_bredr_pairing(gap_conn_item_t *conn);
void smp_receive_peer_ltk_req(gap_conn_item_t *conn, struct hci_ev_le_ltk_request *p);
void smp_receive_enc_change(gap_conn_item_t *conn, uint8_t opcode, struct hci_ev_encryption_change_v2 *p);
bool smp_random(void *priv, gap_key_callback_t cmpl);
bool smp_e(const uint8_t *key_128_le, const uint8_t *plain_128_le, gap_key_callback_t cmpl, void *priv);
bool smp_gen_irk(gap_key_callback_t func, void *priv);
bool smp_gen_csrk(gap_key_callback_t func, void *priv);
bool smp_ah(const uint8_t *k_128_le, const uint8_t *r_24_le, gap_key_callback_t func, void* priv);
bool smp_aes_ccm(const uint8_t *key_128_le, const uint8_t *nonce_le, const uint8_t *m, uint16_t m_len, bool encrypt, uint8_t add_auth_data, gap_key_callback_t func, void *priv);
bool smp_aes_cmac(const uint8_t *k_128_le, const uint8_t *m_le, uint16_t m_len, gap_key_callback_t func, void *priv);
bool smp_f5_gen_key_T(const uint8_t *DHKey_256_le, gap_key_callback_t func, void *priv);
bool smp_f5_gen_mackey_ltk(const uint8_t *key_T_128_le, const uint8_t *Ra_128_le, const uint8_t *Rb_128_le,
        const uint8_t *A, const uint8_t *B, gap_key_callback_t mackey, gap_key_callback_t ltk, void *priv);
bool smp_f6(const uint8_t *W_128_le, const uint8_t *N1_128_le, const uint8_t *N2_128_le,
        const uint8_t *R_128_le, const uint8_t *IOcap_24_le, const uint8_t *A1_56_le, const uint8_t *A2_56_le,
        gap_key_callback_t func, void *priv);
bool smp_g2(const uint8_t *PKax_U_256_le, const uint8_t *PKbx_V_256_le, const uint8_t *Ra_X_128_le,
        const uint8_t *Rb_Y_128_le, gap_key_callback_t func, void *priv);
bool smp_h6(const uint8_t *W_128_le, const uint8_t *key_id_32_le, gap_key_callback_t func, void *priv);
bool smp_h7(const uint8_t *salt_128_le, const uint8_t *W_128_le, gap_key_callback_t func, void *priv);
bool smp_h8(const uint8_t *k_128_le, const uint8_t *s_128_le, const uint8_t *key_id_32_le, gap_key_callback_t func, void *priv);
bool smp_big_gsk_gen(const uint8_t *gltk_128_le, const uint8_t *rand_gskd_128_le, gap_key_callback_t func, void *priv);
bool smp_signature(uint8_t *m_le, uint16_t len, uint32_t sign_counter, gap_key_callback_t func, void *priv);
bool smp_linkkey_to_iltk(const uint8_t *linkkey, bool ct2, gap_key_callback_t func, void *priv);
bool smp_iltk_to_ltk(const uint8_t *iltk, gap_key_callback_t func, void *priv);
bool smp_ltk_to_ilk(const uint8_t *ltk, bool ct2, gap_key_callback_t func, void *priv);
bool smp_ilk_to_linkkey(const uint8_t *ilk, gap_key_callback_t func, void *priv);
void smp_aes_key_xor(const uint8_t *a, const uint8_t *b, gap_key_callback_t func, void *priv);
void smp_aes_key_shift(const uint8_t *key, gap_key_callback_t func, void *priv);
void smp_gen_secure_oob_auth_data(gap_oob_auth_data_callback_t cb, void *priv);
void smp_input_oob_legacy_tk(uint16_t peer_type_or_connhdl, const bt_bdaddr_t *peer_addr, const uint8_t *tk);
void smp_input_6_digit_passkey(uint16_t peer_type_or_connhdl, const bt_bdaddr_t *peer_addr, uint32_t passkey);
void smp_input_numeric_confirm(uint16_t peer_type_or_connhdl, const bt_bdaddr_t *peer_addr, bool user_confirmed);
void smp_input_peer_oob_auth_data(uint16_t peer_type_or_connhdl, const bt_bdaddr_t *peer_addr, const gap_smp_oob_auth_data_t *data);
void smp_input_local_oob_auth_data(uint16_t peer_type_or_connhdl, const bt_bdaddr_t *peer_addr, const gap_smp_oob_auth_data_t *data);

#if defined(__cplusplus)
}
#endif
#endif /* __BLE_GAP_I_H__ */
