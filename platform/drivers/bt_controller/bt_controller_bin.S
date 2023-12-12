/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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

#define TO_STRING_A(s)                  # s
#define TO_STRING(s)                    TO_STRING_A(s)

.globl __bt_controller_code_start_flash
.globl __bt_controller_code_end_flash
.section .rodata_bt_controller_code, "a", %progbits
.balign 4

__bt_controller_code_start_flash:
	.incbin TO_STRING(BT_CONTROLLER_BIN_NAME)
__bt_controller_code_end_flash:

