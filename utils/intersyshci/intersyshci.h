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
#ifndef __INTERSYSHCI_H__
#define __INTERSYSHCI_H__
#if defined(__cplusplus)
extern "C" {
#endif

void BESHCI_Open(void);
void BESHCI_Close(void);

int bes_hci_send_data(const uint8_t *buf, uint16_t size);
void bes_hci_bt_rx_isr(void (*cb)(const uint8_t *data, uint16_t len));

typedef bool (*intersys_hci_cmd_filter_handler_func)(uint8_t* pbuf, uint32_t length);
void intersys_register_hci_cmd_filter_handler_callback(intersys_hci_cmd_filter_handler_func func);

typedef void (*intersys_log_report_func)(uint8_t* data);
void intersys_register_log_report_handler_callback(intersys_log_report_func func);

void beshci_set_intersys_trace_enable(bool enable);
bool beshci_get_intersys_trace_enable(void);
void beshci_set_supv_hci_buff_trace_enable(void);
bool beshci_get_supv_hci_buff_trace_enable(void);
void beshci_set_supv_hci_buff_trace_enable_high(void);
bool beshci_get_supv_hci_buff_trace_enable_high(void);
void beshci_set_a2dp_stream_trace_enable(bool enable);
bool beshci_get_a2dp_stream_trace_enable(void);
void beshci_set_acl_packet_trace_enable(bool enable);
bool beshci_get_acl_packet_trace_enable(void);
void beshci_set_iso_packet_trace_enable(bool enable);
bool beshci_get_iso_packet_trace_enable(void);
void beshci_enable_sleep_checker(bool enable);

void bes_get_hci_rx_flowctrl_info(void);
void bes_get_hci_tx_flowctrl_info(void);

#if defined(__cplusplus)
}
#endif
#endif /* __INTERSYSHCI_H__ */
