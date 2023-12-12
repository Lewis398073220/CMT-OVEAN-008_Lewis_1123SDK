/**

 * @copyright Copyright (c) 2015-2022 BES Technic.
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
 */

#ifndef __GAF_ULL_HID_SUPPORT_H__
#define __GAF_ULL_HID_SUPPORT_H__

uint8_t gaf_ull_get_sensor_data(uint8_t con_lid);
#ifdef AOB_MOBILE_ENABLED
void gaf_mobile_ull_stream_update_and_stop_handler(uint8_t ase_lid);
void gaf_mobile_ull_stream_update_and_start_handler(uint8_t ase_lid);
#endif

#endif


