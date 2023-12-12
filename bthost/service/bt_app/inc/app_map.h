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
#ifndef __APP_BTMAP_SMS_H__
#define __APP_BTMAP_SMS_H__

#ifdef BT_MAP_SUPPORT

#include "map_service.h"

#ifdef __cplusplus
extern "C" {
#endif

void bt_map_set_obex_over_rfcomm(void);
void bt_map_clear_obex_over_rfcomm(void);
void bt_map_client_test(const bt_bdaddr_t *remote);
bt_map_state_t bt_map_get_state(const bt_bdaddr_t *remote);

#define bes_bt_map_connect bt_map_connect
#define bes_bt_map_disconnect bt_map_disconnect
#define bes_bt_map_send_sms bt_map_send_sms
#define bes_bt_map_is_connected bt_map_is_connected
#define bes_bt_map_get_state bt_map_get_state

#ifdef BT_MAP_TEST_SUPPORT
void app_bt_pts_map_connect(void);
void app_bt_pts_map_disconnect(void);
void app_bt_pts_map_open_all_mas_channel(void);
void app_bt_pts_map_mns_obex_disc_req(void);
void app_bt_pts_map_close_mns_channel(void);
void app_bt_pts_map_client_test(void);
void app_bt_pts_map_send_sms(void);
void app_bt_pts_map_obex_disc_req(const char* name, uint32_t len);
void app_bt_pts_map_obex_conn_req(const char* name, uint32_t len);
void app_bt_pts_map_dont_auto_conn_req(const char* name, uint32_t len);
void app_bt_pts_map_connect_mas(const char* name, uint32_t len);
void app_bt_pts_map_open_mas_channel(const char* name, uint32_t len);
void app_bt_pts_map_close_mas_channel(const char* name, uint32_t len);
void app_bt_pts_map_enter_to_root_folder(const char* name, uint32_t len);
void app_bt_pts_map_enter_to_parent_folder(const char* name, uint32_t len);
void app_bt_pts_map_enter_to_msg_folder(const char* name, uint32_t len);
void app_bt_pts_map_enter_to_child_folder(const char* name, uint32_t len);
void app_bt_pts_map_send_gsm_sms(const char* name, uint32_t len);
void app_bt_pts_map_send_cdma_sms(const char* name, uint32_t len);
void app_bt_pts_map_send_mms(const char* name, uint32_t len);
void app_bt_pts_map_send_im(const char* name, uint32_t len);
void app_bt_pts_map_send_email(const char* name, uint32_t len);
void app_bt_pts_map_replace_email(const char* name, uint32_t len);
void app_bt_pts_map_forward_email(const char* name, uint32_t len);
void app_bt_pts_map_forward_including_attachment(const char* name, uint32_t len);
void app_bt_pts_map_push_to_conversation(const char* name, uint32_t len);
void app_bt_pts_map_put_start(const char* name, uint32_t len);
void app_bt_pts_map_put_continue(const char* name, uint32_t len);
void app_bt_pts_map_put_end(const char* name, uint32_t len);
void app_bt_pts_map_get_instance_info(const char* name, uint32_t len);
void app_bt_pts_map_get_object_test(const char* name, uint32_t len);
void app_bt_pts_map_update_inbox(const char* name, uint32_t len);
void app_bt_pts_map_notify_register(const char* name, uint32_t len);
void app_bt_pts_map_notify_filter(const char* name, uint32_t len);
void app_bt_pts_map_set_msg_read(const char* name, uint32_t len);
void app_bt_pts_map_set_msg_unread(const char* name, uint32_t len);
void app_bt_pts_map_set_msg_delete(const char* name, uint32_t len);
void app_bt_pts_map_set_msg_undelete(const char* name, uint32_t len);
void app_bt_pts_map_set_msg_extdata(const char* name, uint32_t len);
void app_bt_pts_map_get_folder_listing_size(const char* name, uint32_t len);
void app_bt_pts_map_get_folder_listing(const char* name, uint32_t len);
void app_bt_pts_map_set_srm_in_wait(const char* name, uint32_t len);
void app_bt_pts_map_get_message(const char* name, uint32_t len);
void app_bt_pts_map_get_msg_listing_size(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_type(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_handle(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_readstatus(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_priority(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_originator(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_recipient(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_period_begin(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_period_end(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_period_bend(const char* name, uint32_t len);
void app_bt_pts_map_get_msg_listing_size_of_convoid(const char* name, uint32_t len);
void app_bt_pts_map_get_message_listing_of_convoid(const char* name, uint32_t len);
void app_bt_pts_map_get_owner_status(const char* name, uint32_t len);
void app_bt_pts_map_set_owner_status(const char* name, uint32_t len);
void app_bt_pts_map_get_convo_listing_size(const char* name, uint32_t len);
void app_bt_pts_map_get_convo_listing(const char* name, uint32_t len);
void app_bt_pts_map_get_convo_listing_by_readstatus(const char* name, uint32_t len);
void app_bt_pts_map_get_convo_listing_by_recipient(const char* name, uint32_t len);
void app_bt_pts_map_get_convo_listing_by_last_activity(const char* name, uint32_t len);
#endif /* BT_MAP_TEST_SUPPORT */

#ifdef __cplusplus
}
#endif

#endif /* BT_MAP_SUPPORT */

#endif /*__APP_BTMAP_SMS_H__*/

