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
#undef MOUDLE
#define MOUDLE TEST
#include "bluetooth_bt_api.h"
#include "bluetooth_ble_api.h"
#include "app_trace_rx.h"
#include "app_key.h"
#include "nvrecord_bt.h"
#include "nvrecord_ble.h"
#include "app_ble_test.h"
#include "app_bt_func.h"
#include "apps.h"
#include "app_bt.h"
#include "btapp.h"
#include "gap_i.h"
#include "btm_i.h"
#include "l2cap_i.h"
#include "gatt_service.h"
#include "bap_service.h"
#include "app_ble.h"

#if BLE_AUDIO_ENABLED
#include "bap_uc_test.h"
#include "app_bap.h"
#include "aob_conn_api.h"
#include "aob_media_api.h"
#include "aob_call_api.h"
#include "aob_bis_api.h"
#include "aob_pacs_api.h"
#include "aob_csip_api.h"
#include "aob_volume_api.h"
#endif

#include "gaf_media_stream.h"
#include "app_audio_active_device_manager.h"

#ifdef APP_TRACE_RX_ENABLE
#ifdef AOB_MOBILE_ENABLED
static const CIS_TIMING_CONFIGURATION_T dft_cis_timing =
{
    .m2s_bn = 1,
    .m2s_nse = 2,
    .m2s_ft = 4,
    .s2m_bn = 1,
    .s2m_nse = 2,
    .s2m_ft = 4,
    .frame_cnt_per_sdu = 1,
    .iso_interval = 8,  // 10ms = 8 * 1.25ms
};

static const CIS_TIMING_CONFIGURATION_T dft_cis_timing_7_5ms =
{
    .m2s_bn = 1,
    .m2s_nse = 2,
    .m2s_ft = 4,
    .s2m_bn = 1,
    .s2m_nse = 2,
    .s2m_ft = 4,
    .frame_cnt_per_sdu = 1,
    .iso_interval = 6,  // 10ms = 8 * 1.25ms
};
#endif /// AOB_MOBILE_ENABLED

typedef void (*app_ble_test_cmd_handler_t)(void);

typedef struct {
    const char* string;
    app_ble_test_cmd_handler_t function;
} app_ble_test_handler_t;

typedef void (*app_ble_test_cmd_with_param_handler_t)(const char* cmdParam, uint32_t cmdParamLen);

typedef struct {
    const char* string;
    app_ble_test_cmd_with_param_handler_t function;
} app_ble_test_with_param_handler_t;

static uint8_t g_test_le_pts_addr[] = { // lsb to msb
#if 1
    0x00, 0x45, 0x82, 0xf5, 0xe8, 0x07, 0xc0
#elif 0
    0x00, 0xed, 0xb6, 0xf4, 0xdc, 0x1b, 0x00
#elif 0
    0x00, 0x81, 0x33, 0x33, 0x22, 0x11, 0x11
#elif 0
    0x00, 0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
    0x00, 0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
};

static uint8_t g_test_bt_pts_addr[] = { // lsb to msb
#if 1
    0xed, 0xb6, 0xf4, 0xdc, 0x1b, 0x00
#elif 0
    0x81, 0x33, 0x33, 0x22, 0x11, 0x11
#elif 0
    0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
    0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
};

POSSIBLY_UNUSED static void _str_to_hex(const char *pSrc, uint8_t *pDst, int len)
{
    uint8_t shift_H = 0;
    uint8_t shift_L = 0;
    memset(&pDst[0], 0, len);

    for (uint8_t i = 0; i < len; i++)
    {
        if (pSrc[i * 2] <= '9')
        {
            shift_H = 0 - '0';
        }
        else if (pSrc[i * 2] <= 'F')
        {
            shift_H = 0 - 'A' + 0x0A;
        }
        else
        {
            shift_H = 0 - 'a' + 0x0A;
        }

        if (pSrc[i * 2 + 1] <= '9')
        {
            shift_L = 0 - '0';
        }
        else if (pSrc[i * 2 + 1] <= 'F')
        {
            shift_L = 0 - 'A' + 0x0A;
        }
        else
        {
            shift_L = 0 - 'a' + 0x0A;
        }

        pDst[i] = (((pSrc[i * 2 ] + shift_H) << 4) & 0xF0) + ((pSrc[i*2 + 1] + shift_L) & 0xF);
    }
}

#ifdef AOB_MOBILE_ENABLED
static void ble_audio_test_update_cis_timing(CIS_TIMING_CONFIGURATION_T new_cis_timing)
{
    app_bap_update_cis_timing(&new_cis_timing);
}

static void ble_audio_test_update_sdu_intv(uint32_t sdu_intv_m2s_us, uint32_t sdu_intv_s2m_us)
{
    app_bap_update_sdu_intv(sdu_intv_m2s_us, sdu_intv_s2m_us);
}

static void ble_audio_test_start_unidirectional_stream(uint8_t direction, uint8_t sample_rate, uint16_t frame_octet, uint16_t context_bf)
{
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        sample_rate, frame_octet, (enum app_gaf_direction)direction, AOB_CODEC_ID_LC3, context_bf
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_start_stream(&ase_to_start, i, false);
    }
}

static void ble_audio_test_start_bidirectional_stream(uint8_t sample_rate_sink, uint8_t sample_rate_src,
                                                    uint16_t frame_octet_sink, uint16_t frame_octet_src, uint16_t context_bf)
{
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start[] =
    {
        {sample_rate_sink, frame_octet_sink, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, context_bf},
        {sample_rate_src, frame_octet_src, APP_GAF_DIRECTION_SRC, AOB_CODEC_ID_LC3, context_bf},
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_start_stream(&ase_to_start[0], i, true);
    }
}

#endif /// AOB_MOBILE_ENABLED

const bt_bdaddr_t *app_test_get_le_address(bt_addr_type_t *peer_type)
{
    *peer_type = (bt_addr_type_t)g_test_le_pts_addr[0];
    return (bt_bdaddr_t *)(g_test_le_pts_addr + 1);
}

const bt_bdaddr_t *app_test_get_bt_address(void)
{
    return (bt_bdaddr_t *)g_test_bt_pts_addr;
}

static bool app_ble_test_is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F');
}

static const char *app_ble_test_get_hex_digit(const char* param, const char *pend, char *out)
{
    for (; param < pend; param += 1)
    {
        if (app_ble_test_is_hex_digit(*param))
        {
            *out = *param;
            return param + 1;
        }
    }

    return NULL;
}

static uint8_t app_ble_test_get_hex_value(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f')
    {
        return 10 + c - 'a';
    }

    if (c >= 'A' && c <= 'F')
    {
        return 10 + c - 'A';
    }

    return 0;
}

static uint16_t app_ble_scan_hex_data_from_string(const char **p, const char *param_end, uint8_t *data_out, int data_max_len)
{
    const char *param_start = *p;
    const char *param = *p;
    const uint8_t *data_end = data_out + data_max_len;
    uint8_t *data = data_out;
    char high = 0;
    char low = 0;

    for (; data < data_end; data += 1)
    {
        param = app_ble_test_get_hex_digit(param, param_end, &high);
        if (param == NULL)
        {
            break;
        }

        param = app_ble_test_get_hex_digit(param, param_end, &low);
        if (param == NULL)
        {
            break;
        }

        *data = (app_ble_test_get_hex_value(high) << 4) | app_ble_test_get_hex_value(low);
    }

    TRACE(0, "ble test %d %d '%s'", param_end-param_start, data-data_out, param_start);

    DUMP8("%02x", data_out, data-data_out);

    *p = param;

    return data-data_out;
}

static void app_test_set_local_addr(const char *BT00LE01_BdAddrLsb2Msb, uint32_t str_len)
{
    const char *pend = BT00LE01_BdAddrLsb2Msb + str_len;
    uint8_t is_le_addr = 0;
    uint8_t bd_addr[6] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&BT00LE01_BdAddrLsb2Msb, pend, &is_le_addr, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&BT00LE01_BdAddrLsb2Msb, pend, bd_addr, 6);
    if (len != 6)
    {
        return;
    }
    if (is_le_addr)
    {
        gap_set_le_public_address((bt_bdaddr_t *)bd_addr);
    }
    else
    {
        gap_set_bt_public_address((bt_bdaddr_t *)bd_addr);
    }
}

static void app_test_set_le_address(const char *Type_BdAddrLsb2Msb, uint32_t str_len)
{
    const char *pend = Type_BdAddrLsb2Msb + str_len;
    uint8_t type_bdaddr[7];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Type_BdAddrLsb2Msb, pend, type_bdaddr, 7);
    if (len != 7)
    {
        return;
    }
    memcpy(g_test_le_pts_addr, type_bdaddr, 7);
}

static void app_test_set_bt_address(const char *BdAddrLsb2Msb, uint32_t str_len)
{
    const char *pend = BdAddrLsb2Msb + str_len;
    uint8_t bdaddr[6];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&BdAddrLsb2Msb, pend, bdaddr, 6);
    if (len != 6)
    {
        return;
    }
    memcpy(g_test_bt_pts_addr, bdaddr, 6);
}

static void smp_test_ctkd_set_ble_public_address(void)
{
    g_test_le_pts_addr[0] = 0;
    memcpy(g_test_le_pts_addr+1, g_test_bt_pts_addr, 6);
    gap_set_le_public_address(gap_factory_bt_address());
}

static void smp_test_ctkd_set_bt_public_address(void)
{
    gap_set_bt_public_address(gap_factory_le_address());
    gap_set_le_public_address(gap_factory_le_address());
}

static void smp_test_key_callback(void *priv, int error_code, const uint8_t *key)
{
    if (error_code || key == NULL)
    {
        CO_LOG_ERR_1(error_code, key);
    }
    else
    {
        CO_LOG_INFO_4(BT_STS_SECURITY_KEY, CO_COMBINE_UINT32_LE(key+12), CO_COMBINE_UINT32_LE(key+8),
            CO_COMBINE_UINT32_LE(key+4), CO_COMBINE_UINT32_LE(key));
    }
}

static void smp_test_cmac(const char *K_m, uint32_t str_len)
{
    const char *pend = K_m + str_len;
    uint8_t key[16];
    uint8_t m[64];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&K_m, pend, key, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&K_m, pend, m, 64);
    smp_aes_cmac(key, m, len, smp_test_key_callback, NULL);
}

static void smp_test_ase_cmac(const uint8_t *m, uint16_t len)
{
    const uint8_t key[16] = {
        0x3c, 0x4f, 0xcf, 0x09,
        0x88, 0x15, 0xf7, 0xab,
        0xa6, 0xd2, 0xae, 0x28,
        0x16, 0x15, 0x7e, 0x2b};
    smp_aes_cmac(key, m, len, smp_test_key_callback, NULL);
}

static void smp_test_ase_cmac_0(void)
{
    smp_test_ase_cmac(NULL, 0);
}

static void smp_test_ase_cmac_16(void)
{
    uint8_t m[16] = {
        0x2a, 0x17, 0x93, 0x73,
        0x11, 0x7e, 0x3d, 0xe9,
        0x96, 0x9f, 0x40, 0x2e,
        0xe2, 0xbe, 0xc1, 0x6b};
    smp_test_ase_cmac(m, 16);
}

static void smp_test_ase_cmac_40(void)
{
    uint8_t m[40] = {
        0x11, 0xe4, 0x5c, 0xa3,
        0x46, 0x1c, 0xc8, 0x30,
        0x51, 0x8e, 0xaf, 0x45,
        0xac, 0x6f, 0xb7, 0x9e,
        0x9c, 0xac, 0x03, 0x1e,
        0x57, 0x8a, 0x2d, 0xae,
        0x2a, 0x17, 0x93, 0x73,
        0x11, 0x7e, 0x3d, 0xe9,
        0x96, 0x9f, 0x40, 0x2e,
        0xe2, 0xbe, 0xc1, 0x6b};
    smp_test_ase_cmac(m, 40);
}

static void smp_test_ase_cmac_64(void)
{
    uint8_t m[64] = {
        0x10, 0x37, 0x6c, 0xe6,
        0x7b, 0x41, 0x2b, 0xad,
        0x17, 0x9b, 0x4f, 0xdf,
        0x45, 0x24, 0x9f, 0xf6,
        0xef, 0x52, 0x0a, 0x1a,
        0x19, 0xc1, 0xfb, 0xe5,
        0x11, 0xe4, 0x5c, 0xa3,
        0x46, 0x1c, 0xc8, 0x30,
        0x51, 0x8e, 0xaf, 0x45,
        0xac, 0x6f, 0xb7, 0x9e,
        0x9c, 0xac, 0x03, 0x1e,
        0x57, 0x8a, 0x2d, 0xae,
        0x2a, 0x17, 0x93, 0x73,
        0x11, 0x7e, 0x3d, 0xe9,
        0x96, 0x9f, 0x40, 0x2e,
        0xe2, 0xbe, 0xc1, 0x6b};
    smp_test_ase_cmac(m, 64);
}

static void smp_test_f4_numeric(const char *PKbx_PKax_Rb, uint32_t str_len)
{
    const char *pend = PKbx_PKax_Rb + str_len;
    uint8_t Z_PKax_PKbx[1+64] = {0};
    uint8_t Rb[16] = {0};
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&PKbx_PKax_Rb, pend, Z_PKax_PKbx+33, 32);
    if (len != 32)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&PKbx_PKax_Rb, pend, Z_PKax_PKbx+1, 32);
    if (len != 32)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&PKbx_PKax_Rb, pend, Rb, 16);
    if (len != 16)
    {
        return;
    }
    smp_aes_cmac(Rb, Z_PKax_PKbx, 65, smp_test_key_callback, NULL);
}

static void smp_test_f4(void)
{
    char PKbx_PKax_Rb[] =
        "e69d350e 480103cc dbfdf4ac 1191f4ef"
        "b9a5f9e9 a7832c5e 2cbe97f2 d203b020" // PKbx
        "fdc57ff4 49dd4f6b fb7c9df1 c29acb59"
        "2ae7d4ee fbfc0a90 9abbf632 3d8b1855" // PKax;
        "abae2b71 ecb2ffff 3e7377d1 5484cbd5" // Rb;
        ;
    smp_test_f4_numeric(PKbx_PKax_Rb, strlen(PKbx_PKax_Rb));
}

static void smp_test_encrypt_data(const char *Key_Plain, uint32_t str_len)
{
    const char *pend = Key_Plain + str_len;
    uint8_t key[16];
    uint8_t plain_text[16];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Key_Plain, pend, key, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Key_Plain, pend, plain_text, 16);
    if (len != 16)
    {
        return;
    }
    smp_e(key, plain_text, smp_test_key_callback, NULL);
}

/**
 * m[53] = 0001c1cf 2d
 *         7013a700 cebf3737 125600cf c43dfff7
 *         8365216e 5fa725cc e7e8a6ab ae2b71ec
 *         b2ffff3e 7377d154 84cbd565 6c746200
 * smp_aes_cmac(key_T, m, 53):
 *      0. m_len = 53; curr_block = 1; num_blocks = (53 + 15) / 16 = 4
 *         smp_e_res: {00000000 00000000 00000000 00000000}
 *      1. smp_e_res ^= m[53 - curr_block * 16] ~~ m[53-16]
 *         aes_block: {b2ffff3e 7377d154 84cbd565 6c746200} smp_e(key_T, aes_block); curr_block = 2;
 *         smp_f5_step|b2ffff3e 7377d154 84cbd565 6c746200
 *      2. smp_e_res: {bb6885e9 d8800224 6d7d9b65 24c4e0b4}
 *         smp_e_res^={8365216e 5fa725cc e7e8a6ab ae2b71ec}
 *         aes_block: {} smp_e(key_T, aes_block); curr_block = 3;
 *         smp_f5_step|
 *      3. smp_e_res: {}
 *         smp_e_res^={7013a700 cebf3737 125600cf c43dfff7}
 *         aes_block: {} smp_e(key_T, aes_block); curr_block = 4;
 *         smp_f5_step|
 *      4. aes_block: {}
 *         smp_f5_step|00000000 00000000 00000000 00000000
 *      5. smp_e_res: {}
 *         remain_len = (53 % 16) = 5
 *         msb = most sinificant bit of smp_e_res = (smp_e_res[15] >> 7);
 *         sub_key = smp_e_res << 1;
 *         if (msb) sub_key[0] ^= 0x87;
 *         msb = sub_key[15] >> 7;
 *         sub_key = sub_key << 1;
 *         if (msb) sub_key[0] ^= 0x87;
 *         m_last[11 12 13 14 15] = {00 01 c1 cf 2d}
 *         m_last[10] = 0x80
 *         m_last: {00000000 00000000 00008000 01c1cf2d}
 *         m_last ^= sub_key; aes_blcok ^= m_last;
 *         aes_blcok: {} smp_e(key_T, aes_block);
 *         smp_f5_step|
 *      6. smp_e_res: {}
 */

static void smp_test_shift(const char*K, uint32_t str_len)
{
    const char *pend = K + str_len;
    uint8_t key[16];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&K, pend, key, 16);
    if (len != 16)
    {
        return;
    }
    smp_aes_key_shift(key, smp_test_key_callback, NULL);
}

static void smp_test_xor(const char *Ka_Kb, uint32_t str_len)
{
    const char *pend = Ka_Kb + str_len;
    uint8_t Ka[16];
    uint8_t Kb[16];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Ka_Kb, pend, Ka, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Ka_Kb, pend, Kb, 16);
    if (len != 16)
    {
        return;
    }
    smp_aes_key_xor(Ka, Kb, smp_test_key_callback, NULL);
}

static void smp_test_f5_step(const char *M, uint32_t str_len)
{
    const char *pend = M + str_len;
    uint8_t key_T[16] = {
        0x89, 0x69, 0xac, 0x8d,
        0xdb, 0x4b, 0x62, 0x97,
        0x88, 0x32, 0x88, 0xde,
        0x20, 0x8f, 0x12, 0x3c};
    uint8_t m[16];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&M, pend, m, 16);
    if (len != 16)
    {
        return;
    }
    smp_e(key_T, m, smp_test_key_callback, NULL);
}

static void smp_test_f5_key_T(void)
{
    char K_m[] =
        "be83605A db0b3760 38a5f5aa 9183886C" // salt
        "98a6bf73 f3348d86 f166f8b4 136b7999"
        "9b7d390a a6101034 05adc857 a33402ec" // DHKey
        ;
    smp_test_cmac(K_m, strlen(K_m));
}

static uint8_t g_app_ble_test_Ra[16];
static uint8_t g_app_ble_test_Rb[16];
static uint8_t g_app_ble_test_A[7];
static uint8_t g_app_ble_test_B[7];

static void smp_test_f5_gen_key(void)
{
    uint8_t key_T[16] = {
        0x89, 0x69, 0xac, 0x8d,
        0xdb, 0x4b, 0x62, 0x97,
        0x88, 0x32, 0x88, 0xde,
        0x20, 0x8f, 0x12, 0x3c};
    smp_f5_gen_mackey_ltk(key_T, g_app_ble_test_Ra, g_app_ble_test_Rb, g_app_ble_test_A,
        g_app_ble_test_B, smp_test_key_callback, smp_test_key_callback, NULL);
}

static void smp_test_gen_ltk_callback(void *priv, int error_code, const uint8_t *key)
{
    if (error_code || key == NULL)
    {
        CO_LOG_ERR_1(error_code, key);
    }
    else
    {
        CO_LOG_INFO_4(BT_STS_SECURITY_KEY, CO_COMBINE_UINT32_LE(key+12), CO_COMBINE_UINT32_LE(key+8),
            CO_COMBINE_UINT32_LE(key+4), CO_COMBINE_UINT32_LE(key));

        smp_f5_gen_mackey_ltk(key, g_app_ble_test_Ra, g_app_ble_test_Rb, g_app_ble_test_A,
            g_app_ble_test_B, smp_test_key_callback, smp_test_key_callback, NULL);
    }
}

static void smp_test_f5_gen_ltk(const char *DHKey_Ra_Rb_A_B, uint32_t str_len)
{
    // DHKEY 32-byte | Ra 16-byte | Rb 16-byte | A_and_TYPE 7-byte | B_and_TYPE 7-byte
    const char *pend = DHKey_Ra_Rb_A_B + str_len;
    uint8_t DHKey[32];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&DHKey_Ra_Rb_A_B, pend, DHKey, 32);
    if (len != 32)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&DHKey_Ra_Rb_A_B, pend, g_app_ble_test_Ra, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&DHKey_Ra_Rb_A_B, pend, g_app_ble_test_Rb, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&DHKey_Ra_Rb_A_B, pend, g_app_ble_test_A, 7);
    if (len != 7)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&DHKey_Ra_Rb_A_B, pend, g_app_ble_test_B, 7);
    if (len != 7)
    {
        return;
    }
    smp_f5_gen_key_T(DHKey, smp_test_gen_ltk_callback, NULL);
}

static void smp_test_f5(void)
{
    char DHKey_Ra_Rb_A_B[] =
        "98a6bf73 f3348d86 f166f8b4 136b7999"
        "9b7d390a a6101034 05adc857 a33402ec" // DHKey
        "abae2b71 ecb2ffff 3e7377d1 5484cbd5" // Ra
        "cfc43dff f7836512 6e5fa725 cce7e8a6" // Rb
        "cebf3737125600" // A
        "c1cf2d7013a700" // B
        ;
    smp_test_f5_gen_ltk(DHKey_Ra_Rb_A_B, strlen(DHKey_Ra_Rb_A_B));
}

static void smp_test_f6_dhkey_check(const char *MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, uint32_t str_len)
{
    // MacKey 16-byte | Ra 16-byte | Rb 16-byte | rb 16-byte | IOCAPa 3-byte | A_and_TYPE 7-byte | B_and_TYPE 7-byte
    const char *pend = MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B + str_len;
    uint8_t MacKey[16];
    uint8_t Ra[16];
    uint8_t Rb[16];
    uint8_t rb[16];
    uint8_t IOCAPa[3];
    uint8_t A_and_TYPE[7];
    uint8_t B_and_TYPE[7];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, MacKey, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, Ra, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, Rb, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, rb, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, IOCAPa, 3);
    if (len != 3)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, A_and_TYPE, 7);
    if (len != 7)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, pend, B_and_TYPE, 7);
    if (len != 7)
    {
        return;
    }
    smp_f6(MacKey, Ra, Rb, rb, IOCAPa, A_and_TYPE, B_and_TYPE, smp_test_key_callback, NULL);
}

static void smp_test_f6(void)
{
    char MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B[] =
        "206e63ce 206a3ffd 024a08a1 76f16529" // MacKey
        "abae2b71 ecb2ffff 3e7377d1 5484cbd5" // Ra
        "cfc43dff f7836521 6e5fa725 cce7e8a6" // Rb
        "c80f2d0c d242da08 54bb53b4 3b34a312" // rb
        "020101" // IOcap
        "cebf3737125600" // A
        "c1cf2d7013a700" // B
        ;
    smp_test_f6_dhkey_check(MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B, strlen(MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B));
}

static void smp_test_g2_user_confirm_value(const char *PKax_PKbx_Ra_Rb, uint32_t str_len)
{
    const char *pend = PKax_PKbx_Ra_Rb + str_len;
    uint8_t PKax[32];
    uint8_t PKbx[32];
    uint8_t Ra[16];
    uint8_t Rb[16];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&PKax_PKbx_Ra_Rb, pend, PKax, 32);
    if (len != 32)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&PKax_PKbx_Ra_Rb, pend, PKbx, 32);
    if (len != 32)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&PKax_PKbx_Ra_Rb, pend, Ra, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&PKax_PKbx_Ra_Rb, pend, Rb, 16);
    if (len != 16)
    {
        return;
    }
    smp_g2(PKax, PKbx, Ra, Rb, smp_test_key_callback, NULL);
}

static void smp_test_g2(void)
{
    char PKax_PKbx_Ra_Rb[] =
        "e69d350e 480103cc dbfdf4ac 1191f4ef"
        "b9a5f9e9 a7832c5e 2cbe97f2 d203b020" // PKax
        "fdc57ff4 49dd4f6b fb7c9df1 c29acb59"
        "2ae7d4ee fbfc0a90 9abbf632 3d8b1855" // PKbx
        "abae2b71 ecb2ffff 3e7377d1 5484cbd5" // Ra
        "cfc43dff f7836521 6e5fa725 cce7e8a6" // Rb
        ;
    smp_test_g2_user_confirm_value(PKax_PKbx_Ra_Rb, strlen(PKax_PKbx_Ra_Rb));
}

static void smp_test_ah_irk_prand(const char *IRK_Prand, uint32_t str_len)
{
    const char *pend = IRK_Prand + str_len;
    uint8_t IRK[16];
    uint8_t prand[3];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&IRK_Prand, pend, IRK, 16);
    if (len != 16)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&IRK_Prand, pend, prand, 3);
    if (len != 3)
    {
        return;
    }
    smp_ah(IRK, prand, smp_test_key_callback, NULL);
}

static void smp_test_ah(void)
{
    char IRK_Prand[] =
        "9b7d390a a6101034 05adc857 a33402ec" // LTK
        "948170" // prand
        ;
    smp_test_ah_irk_prand(IRK_Prand, strlen(IRK_Prand));
}

static void smp_test_set_use_passkey(void)
{
    gap_pts_set_use_passkey_entry();
}

static void smp_test_set_use_oob(void)
{
    gap_pts_set_use_oob_method();
}

static void smp_test_set_no_mitm_auth(void)
{
    gap_pts_set_no_mitm_auth();
}

static void smp_test_set_display_only(void)
{
    gap_pts_set_display_only();
}

static void smp_test_set_keyboard_only(void)
{
    gap_pts_set_keyboard_only();
}

static void smp_test_set_no_bonding(void)
{
    gap_pts_set_no_bonding();
}

static void smp_test_dont_auto_start_auth(const char *DontAutoStartSmp, uint32_t str_len)
{
    const char *pend = DontAutoStartSmp + str_len;
    uint8_t dont_auto_start_smp = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&DontAutoStartSmp, pend, &dont_auto_start_smp, 1);
    if (len != 1)
    {
        return;
    }
    gap_pts_set_dont_start_smp(dont_auto_start_smp);
}

static void smp_test_gen_linkkey_from_ltk(void)
{
    gap_pts_gen_linkkey_from_ltk();
}

static void smp_test_set_dist_irk_only(void)
{
    gap_pts_set_dist_irk_only();
}

static void smp_test_set_dist_csrk(void)
{
    gap_pts_set_dist_csrk();
}

static void smp_test_start_authentication(const char *Conidx, uint32_t str_len)
{
    const char *pend = Conidx + str_len;
    uint8_t conidx = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    gap_start_authentication(gap_zero_based_ble_conidx_as_hdl(conidx), GAP_AUTH_STARTED_BY_BLE_TEST);
}

static void smp_test_input_oob_legacy_tk(const char *TK, uint32_t str_len)
{
    bt_addr_type_t peer_type;
    const bt_bdaddr_t *peer_addr = NULL;
    const char *pend = TK + str_len;
    uint8_t tk[16];
    uint16_t len = 0;

    len = app_ble_scan_hex_data_from_string(&TK, pend, tk, 16);
    if (len != 16)
    {
        return;
    }

    peer_addr = app_test_get_le_address(&peer_type);
    gap_input_oob_legacy_tk(peer_type, peer_addr, tk);
}

static void smp_test_input_6_digit_passkey(const char *Passkey, uint32_t str_len)
{
    bt_addr_type_t peer_type;
    const bt_bdaddr_t *peer_addr = NULL;
    const char *pend = Passkey + str_len;
    uint8_t digit[3];
    uint16_t len = 0;
    uint32_t passkey = 0;

    len = app_ble_scan_hex_data_from_string(&Passkey, pend, digit, 3);
    if (len != 3)
    {
        return;
    }

    passkey = ((digit[0]>>4)&0x0f) * 100000 +
              ((digit[0]>>0)&0x0f) * 10000 +
              ((digit[1]>>4)&0x0f) * 1000 +
              ((digit[1]>>0)&0x0f) * 100 +
              ((digit[2]>>4)&0x0f) * 10 +
              ((digit[2]>>0)&0x0f) * 1;

    peer_addr = app_test_get_le_address(&peer_type);
    gap_input_6_digit_passkey(peer_type, peer_addr, passkey);
}

static void smp_test_lirk(void)
{
    POSSIBLY_UNUSED const uint8_t *irk = gap_get_local_irk();
    CO_LOG_INFO_4(BT_STS_LOCAL_IRK, CO_COMBINE_UINT32_LE(irk+12), CO_COMBINE_UINT32_LE(irk+8),
        CO_COMBINE_UINT32_LE(irk+4), CO_COMBINE_UINT32_LE(irk));
}

static void smp_test_lcsrk(void)
{
    POSSIBLY_UNUSED const uint8_t *csrk = gap_get_local_csrk();
    CO_LOG_INFO_4(BT_STS_LOCAL_CSRK, CO_COMBINE_UINT32_LE(csrk+12), CO_COMBINE_UINT32_LE(csrk+8),
        CO_COMBINE_UINT32_LE(csrk+4), CO_COMBINE_UINT32_LE(csrk));
}

static void smp_test_h6(void)
{
    const uint8_t key[16] = {
        0x9b, 0x7d, 0x39, 0x0a,    0xa6, 0x10, 0x10, 0x34,
        0x05, 0xad, 0xc8, 0x57,    0xa3, 0x34, 0x02, 0xec
        };
    const uint8_t key_id[4] = {
        0x72, 0x62, 0x65, 0x6c
        };
    smp_h6(key, key_id, smp_test_key_callback, NULL);
}

static void smp_test_h7(void)
{
    const uint8_t salt[16] = {
        0x31, 0x70, 0x6d, 0x74,    0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00, 0x00
        };
    const uint8_t key[16] = {
        0x9b, 0x7d, 0x39, 0x0a,    0xa6, 0x10, 0x10, 0x34,
        0x05, 0xad, 0xc8, 0x57,    0xa3, 0x34, 0x02, 0xec
        };
    smp_h7(salt, key, smp_test_key_callback, NULL);
}

static void smp_test_iltk_to_ltk(void *priv, int error_code, const uint8_t *key)
{
    if (error_code || key == NULL)
    {
        CO_LOG_ERR_1(error_code, key);
    }
    else
    {
        smp_iltk_to_ltk(key, smp_test_key_callback, NULL);
    }
}

static void smp_test_ilk_to_linkkey(void *priv, int error_code, const uint8_t *key)
{
    if (error_code || key == NULL)
    {
        CO_LOG_ERR_1(error_code, key);
    }
    else
    {
        smp_ilk_to_linkkey(key, smp_test_key_callback, NULL);
    }
}

static void smp_test_linkkey_to_ltk(const char *ct2_linkkey, uint32_t str_len)
{
    const char *pend = ct2_linkkey + str_len;
    uint8_t linkkey[16];
    uint8_t ct2 = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&ct2_linkkey, pend, &ct2, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&ct2_linkkey, pend, linkkey, 16);
    if (len != 16)
    {
        return;
    }
    smp_linkkey_to_iltk(linkkey, ct2 ? true : false, smp_test_iltk_to_ltk, NULL);
}

static void smp_test_ltk_to_linkkey(const char *ct2_ltk, uint32_t str_len)
{
    const char *pend = ct2_ltk + str_len;
    uint8_t ltk[16];
    uint8_t ct2 = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&ct2_ltk, pend, &ct2, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&ct2_ltk, pend, ltk, 16);
    if (len != 16)
    {
        return;
    }
    smp_ltk_to_ilk(ltk, ct2 ? true : false, smp_test_ilk_to_linkkey, NULL);
}

static void smp_test_ltk_to_lk(void)
{
    char ct2_ltk[] =
        "01" // ct2
        "64bf4f33 336c06bd 584b26e3 bcf98d36" // LTK
        ;
    smp_test_ltk_to_linkkey(ct2_ltk, strlen(ct2_ltk));

    ct2_ltk[1] = '0';
    smp_test_ltk_to_linkkey(ct2_ltk, strlen(ct2_ltk));
}

static void smp_test_lk_to_ltk(void)
{
    char ct2_linkkey[] =
        "01" // ct2
        "00010203 04050607 08090001 02030405" // linkkey
        ;
    smp_test_linkkey_to_ltk(ct2_linkkey, strlen(ct2_linkkey));

    ct2_linkkey[1] = '0';
    smp_test_linkkey_to_ltk(ct2_linkkey, strlen(ct2_linkkey));
}

static void smp_test_set_key_size(const char *Default_Conidx_KeyLen_MITM, uint32_t str_len)
{
    const char *pend = Default_Conidx_KeyLen_MITM + str_len;
    uint8_t set_default_key_size = 0;
    uint8_t conidx = 0;
    uint8_t key_size = 0;
    uint8_t mitm_auth = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Default_Conidx_KeyLen_MITM, pend, &set_default_key_size, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Default_Conidx_KeyLen_MITM, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Default_Conidx_KeyLen_MITM, pend, &key_size, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Default_Conidx_KeyLen_MITM, pend, &mitm_auth, 1);
    if (len != 1)
    {
        return;
    }
    TRACE(0, "smp_test_set_key_size %d mitm_auth %d", key_size, mitm_auth);
    if (set_default_key_size)
    {
        gap_set_default_key_size(key_size);
        gap_set_default_auth_req(mitm_auth);
    }
    else
    {
        gap_set_key_strength(gap_zero_based_ble_conidx_as_hdl(conidx), key_size);
        gap_set_security_level(gap_zero_based_ble_conidx_as_hdl(conidx), mitm_auth ? GAP_SEC_AUTHENTICATED : GAP_SEC_UNAUTHENTICATED);
    }
}

#if BLE_GATT_CLIENT_SUPPORT
static void gattc_test_clear_cache(const char *Index, uint32_t str_len)
{
    const char *pend = Index + str_len;
    uint8_t index = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Index, pend, &index, 1);
    if (len != 1)
    {
        return;
    }
    gap_clear_client_cache_by_handle(gap_zero_based_ble_conidx_as_hdl(index));
}
#endif

GATT_DECL_PRI_SERVICE(g_app_test_service, 0x187F);

GATT_DECL_CHAR(g_app_test_character_7f02,
    0xFFDF,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_app_char_long_value_7f04,
    0xFFE0,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD|GATT_NTF_PROP|GATT_IND_PROP,
    ATT_SEC_NONE);

GATT_DECL_DESCRIPTOR(g_app_test_descriptor_7f05,
    0x2970,
    ATT_RD_PERM|ATT_WR_PERM);

GATT_DECL_CCCD_DESCRIPTOR(g_app_char_cccd_7f06,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_app_test_mitm_auth_7f08,
    0xFFE1,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD|GATT_SIGNED_WR,
    ATT_RD_MITM_AUTH|ATT_WR_MITM_AUTH);

GATT_DECL_CHAR(g_app_cannot_read_write_7f0a,
    0xFFE2,
    GATT_NO_PROP,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_app_char_require_author_7f0c,
    0xFFE3,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD,
    ATT_RD_AUTHOR|ATT_WR_AUTHOR);

GATT_DECL_CHAR(g_app_char_insuff_enc_key_7f0e,
    0xFFE4,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD,
    ATT_RD_ENC|ATT_WR_ENC);

GATT_DECL_CHAR(g_app_char_invalid_offset_7f10,
    0xFFE5,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD,
    ATT_SEC_NONE);

GATT_DECL_CHAR(g_app_char_fixed_size_7f12,
    0xFFE6,
    GATT_RD_REQ|GATT_WR_REQ|GATT_WR_CMD|GATT_NTF_PROP|GATT_IND_PROP,
    ATT_SEC_NONE);

GATT_DECL_CCCD_DESCRIPTOR(g_app_fixed_char_cccd_7f13,
    ATT_SEC_NONE);

static const gatt_attribute_t g_app_test_service_attr_list[] = {
    /* Service */
    gatt_attribute(g_app_test_service),
    /* Characteristics */
    gatt_attribute(g_app_test_character_7f02),
    gatt_attribute(g_app_char_long_value_7f04),
    gatt_attribute(g_app_test_descriptor_7f05),
    gatt_attribute(g_app_char_cccd_7f06),
    gatt_attribute(g_app_test_mitm_auth_7f08),
    gatt_attribute(g_app_cannot_read_write_7f0a),
    gatt_attribute(g_app_char_require_author_7f0c),
    gatt_attribute(g_app_char_insuff_enc_key_7f0e),
    gatt_attribute(g_app_char_invalid_offset_7f10),
    gatt_attribute(g_app_char_fixed_size_7f12),
    gatt_attribute(g_app_fixed_char_cccd_7f13),
};

static void gatts_test_send_notify(const char *Ntf00Ind01_HandleCount_CharValueHandls, uint32_t str_len)
{
    const char *pend = Ntf00Ind01_HandleCount_CharValueHandls + str_len;
    gatt_char_notify_by_handle_t notify[3];
    uint8_t send_indication = 0;
    uint8_t handle_count = 0;
    uint8_t char_value_handles[2*3] = {0};
    uint16_t len = 0;
    uint8_t notify_data[2] = {0xAA, 0xBB};
    len = app_ble_scan_hex_data_from_string(&Ntf00Ind01_HandleCount_CharValueHandls, pend, &send_indication, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Ntf00Ind01_HandleCount_CharValueHandls, pend, &handle_count, 1);
    if (len != 1)
    {
        return;
    }
    if (handle_count)
    {
        handle_count = (handle_count > ARRAY_SIZE(notify)) ? ARRAY_SIZE(notify) : handle_count;
        len = app_ble_scan_hex_data_from_string(&Ntf00Ind01_HandleCount_CharValueHandls, pend, char_value_handles, 2*handle_count);
        if (len != 2*handle_count)
        {
            return;
        }
    }
    if (handle_count)
    {
        for (int i = 0; i < handle_count; i += 1)
        {
            notify[i].char_value_handle = CO_COMBINE_UINT16_BE(char_value_handles + 2 * i);
            notify[i].value_len = sizeof(notify_data);
            notify[i].value = notify_data;
            notify[i].force_send_value = false;
        }
        if (send_indication)
        {
            gatts_send_indication_by_handle(GAP_ALL_CONNS, notify);
        }
        else
        {
            if (handle_count == 1)
            {
                gatts_send_notification_by_handle(GAP_ALL_CONNS, notify);
            }
            else
            {
                gatts_send_multi_notifications_by_handle(GAP_ALL_CONNS, notify, handle_count);
            }
        }
    }
    else
    {
        gatt_char_notify_t param = {0};
        param.service = g_app_test_service;
        param.character = g_app_char_long_value_7f04;
        param.force_send_value = true;
        if (send_indication)
        {
            gatts_send_value_indication(GAP_ALL_CONNS, &param, notify_data, sizeof(notify_data));
        }
        else
        {
            gatts_send_value_notification(GAP_ALL_CONNS, &param, notify_data, sizeof(notify_data));
        }
    }
}

int app_ble_test_server_callback(gatt_svc_t *svc, gatt_server_event_t event, gatt_server_callback_param_t param)
{
    static uint8_t long_value[32] = {0};
    if (event == GATT_SERV_EVENT_DESC_READ)
    {
        gatt_server_desc_read_t *desc_read = param.desc_read;
        if (desc_read->descriptor == g_app_test_descriptor_7f05)
        {
            uint8_t long_data_len = 32;
            if (desc_read->value_offset <= 32)
            {
                gatts_write_read_rsp_data(desc_read->ctx, long_value+desc_read->value_offset, long_data_len-desc_read->value_offset);
                return true;
            }
            else
            {
                desc_read->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
        }
        return false;
    }
    else if (event == GATT_SERV_EVENT_DESC_WRITE)
    {
        gatt_server_desc_write_t *desc_write = param.desc_write;
        if (desc_write->descriptor == g_app_test_descriptor_7f05)
        {
            if (desc_write->value_offset <= 32)
            {
                if (desc_write->value_offset + desc_write->value_len > 32)
                {
                     desc_write->rsp_error_code = ATT_ERROR_INVALID_ATTR_VALUE_LEN;
                    return false;
                }
                memcpy(long_value+desc_write->value_offset, desc_write->value, desc_write->value_len);
            }
            else
            {
                desc_write->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
            return true;
        }
        return false;
    }
    else if (event == GATT_SERV_EVENT_CHAR_READ)
    {
        gatt_server_char_read_t *char_read = param.char_read;
        if (char_read->character == g_app_test_character_7f02)
        {
            uint8_t long_data_len = 22; // MTU-1
            if (char_read->value_offset <= 22)
            {
                gatts_write_read_rsp_data(char_read->ctx, long_value+char_read->value_offset, long_data_len-char_read->value_offset);
                return true;
            }
            else
            {
                char_read->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
        }
        else if (char_read->character == g_app_char_long_value_7f04)
        {
            uint8_t long_data_len = 255;
            if (char_read->value_offset <= 255)
            {
                gatts_write_read_rsp_data(char_read->ctx, long_value+char_read->value_offset, long_data_len-char_read->value_offset);
                return true;
            }
            else
            {
                char_read->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
        }
        else if (char_read->character == g_app_test_mitm_auth_7f08)
        {
            uint8_t data[2] = {0xAA, 0xBB};
            gatts_write_read_rsp_data(char_read->ctx, data, sizeof(data));
            return true;
        }
        else if (char_read->character == g_app_char_require_author_7f0c)
        {
            char_read->rsp_error_code = ATT_ERROR_INSUFF_AUTHOR;
            return false;
        }
        else if (char_read->character == g_app_char_insuff_enc_key_7f0e)
        {
            char_read->rsp_error_code = ATT_ERROR_ENC_KEY_TOO_SHORT;
            return false;
        }
        else if (char_read->character == g_app_char_invalid_offset_7f10)
        {
            char_read->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
            return false;
        }
        else if (char_read->character == g_app_char_fixed_size_7f12)
        {
            uint8_t long_data_len = 10; // < MTU-3
            if (char_read->value_offset <= 10)
            {
                gatts_write_read_rsp_data(char_read->ctx, long_value+char_read->value_offset, long_data_len-char_read->value_offset);
                return true;
            }
            else
            {
                char_read->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
        }
    }
    else if (event == GATT_SERV_EVENT_CHAR_WRITE)
    {
        gatt_server_char_write_t *char_wirte = param.char_write;
        if (char_wirte->character == g_app_test_character_7f02)
        {
            if (char_wirte->value_offset <= 22) // MTU-1
            {
                if (char_wirte->value_offset + char_wirte->value_len > 22)
                {
                     char_wirte->rsp_error_code = ATT_ERROR_INVALID_ATTR_VALUE_LEN;
                    return false;
                }
                memcpy(long_value+char_wirte->value_offset, char_wirte->value, char_wirte->value_len);
            }
            else
            {
                char_wirte->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
            return true;
        }
        else if (char_wirte->character == g_app_char_long_value_7f04)
        {
            if (char_wirte->value_offset <= 255)
            {
                if (char_wirte->value_offset + char_wirte->value_len > 255)
                {
                    char_wirte->rsp_error_code = ATT_ERROR_INVALID_ATTR_VALUE_LEN;
                    return false;
                }
                if (char_wirte->value_offset + char_wirte->value_len <= sizeof(long_value))
                {
                    memcpy(long_value+char_wirte->value_offset, char_wirte->value, char_wirte->value_len);
                }
            }
            else
            {
                char_wirte->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
            return true;
        }
        else if (char_wirte->character == g_app_test_mitm_auth_7f08)
        {
            return true;
        }
        else if (char_wirte->character == g_app_char_require_author_7f0c)
        {
            char_wirte->rsp_error_code = ATT_ERROR_INSUFF_AUTHOR;
            return false;
        }
        else if (char_wirte->character == g_app_char_insuff_enc_key_7f0e)
        {
            char_wirte->rsp_error_code = ATT_ERROR_ENC_KEY_TOO_SHORT;
            return false;
        }
        else if (char_wirte->character == g_app_char_invalid_offset_7f10)
        {
            char_wirte->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
            return false;
        }
        else if (char_wirte->character == g_app_char_fixed_size_7f12)
        {
            if (char_wirte->value_offset <= 10) // < MTU-3
            {
                if (char_wirte->value_offset + char_wirte->value_len > 10)
                {
                     char_wirte->rsp_error_code = ATT_ERROR_INVALID_ATTR_VALUE_LEN;
                    return false;
                }
                memcpy(long_value+char_wirte->value_offset, char_wirte->value, char_wirte->value_len);
            }
            else
            {
                char_wirte->rsp_error_code = ATT_ERROR_INVALID_OFFSET;
                return false;
            }
            return true;
        }
    }
    return 0;
}

#if BLE_GATT_CLIENT_SUPPORT
static uint8_t g_128_uuid_le[16] = {0};
static uint16_t g_read_char_by_uuid = 0;
static const uint8_t *g_read_char_by_128_uuid = NULL;
static uint8_t g_test_prf_id = 0;

static void gattc_test_discover_all_services(const char *Conidx, uint32_t str_len)
{
    const char *pend = Conidx + str_len;
    uint8_t conidx = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    g_read_char_by_uuid = 0;
    g_read_char_by_128_uuid = NULL;
    gattc_discover_all_services(g_test_prf_id, gap_zero_based_ble_conidx_as_hdl(conidx));
}

static void gattc_test_discover_char_by_uuid(const char *Conidx_CharUuid_Char128Uuid, uint32_t str_len)
{
    const char *pend = Conidx_CharUuid_Char128Uuid + str_len;
    uint8_t conidx = 0;
    uint8_t uuid_be[2] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_CharUuid_Char128Uuid, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_CharUuid_Char128Uuid, pend, uuid_be, 2);
    if (len != 2)
    {
        return;
    }
    g_read_char_by_uuid = CO_COMBINE_UINT16_BE(uuid_be);
    if (g_read_char_by_uuid == 0)
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_CharUuid_Char128Uuid, pend, g_128_uuid_le, 16);
        if (len != 16)
        {
            return;
        }
    }
    if (g_read_char_by_uuid)
    {
        g_read_char_by_128_uuid = NULL;
    }
    else
    {
        g_read_char_by_128_uuid = g_128_uuid_le;
    }
    gattc_discover_all_services(g_test_prf_id, gap_zero_based_ble_conidx_as_hdl(conidx));
}

static int app_ble_test_client_callback(gatt_prf_t *prf, gatt_profile_event_t event, gatt_profile_callback_param_t param)
{
    switch (event)
    {
        case GATT_PROF_EVENT_SERVICE:
            {
                gatt_profile_service_t *p = param.service;
                if (g_read_char_by_uuid || g_read_char_by_128_uuid)
                {
                    gattc_read_character_by_uuid_of_service(prf, p->service->attr_handle, p->service->end_handle,
                        g_read_char_by_uuid, g_read_char_by_uuid ? NULL : g_read_char_by_128_uuid);
                }
                else
                {
                    gattc_discover_all_characters(prf, p->service);
                }
            }
            break;
        case GATT_PROF_EVENT_CHARACTER:
            {
                gatt_profile_character_t *p = param.character;
                bool notify = (p->char_prop & GATT_NTF_PROP) ? true : false;
                bool indicate = (p->char_prop & GATT_IND_PROP) ? true : false;
                TRACE(0, "app_ble_test_client char_prop %02x value_handle %04x", p->char_prop, p->char_value_handle);
                if (notify || indicate)
                {
                    gattc_discover_descriptor_by_value_handle(prf, p->char_value_handle, p->char_end_handle, p->char_prop);
                }
            }
            break;
        case GATT_PROF_EVENT_DESCRIPTOR:
            {
                gatt_profile_descriptor_t *p = param.descriptor;
                uint16_t cccd_config = (p->char_prop & GATT_NTF_PROP) ? GATT_CCCD_SET_NOTIFICATION : 0;
                cccd_config |= (p->char_prop & GATT_IND_PROP) ? GATT_CCCD_SET_INDICATION : 0;
                TRACE(0, "app_ble_test_client desc_uuid %04x desc_handle %04x", p->desc_uuid, p->desc_handle);
                if (cccd_config && p->desc_uuid == GATT_DESC_UUID_CHAR_CLIENT_CONFIG)
                {
                    uint8_t data_le[2] = {CO_SPLIT_UINT16_LE(cccd_config)};
                    gattc_write_descriptor_by_handle(prf, p->desc_handle, data_le, sizeof(uint16_t));
                }
            }
            break;
        default:
            break;
    }
    return 0;
}
#endif

static void gatts_test_change_service(void)
{
    static bool service_added = false;
    gatts_cfg_t cfg = {0};
    if (service_added)
    {
        return;
    }
    service_added = true;
    cfg.eatt_preferred = true;
    gatts_register_service(g_app_test_service_attr_list, ARRAY_SIZE(g_app_test_service_attr_list), app_ble_test_server_callback, &cfg);
    app_ble_gap_update_local_database_hash();
}

#if BLE_GATT_CLIENT_SUPPORT
static void gattc_test_add_profile(void)
{
    static bool profile_added = false;
    if (profile_added)
    {
        return;
    }
    profile_added = true;
    g_test_prf_id = gattc_register_profile(app_ble_test_client_callback, NULL);
    TRACE(0, "g_test_prf_id %d", g_test_prf_id);
}
#endif

#if (BLE_AUDIO_ENABLED)
static void ble_audio_set_audio_configuration_test(const char *cmdParam, uint32_t cmdParam_len)
{
    uint32_t aud_cfg_select = 0;
    /// We use first byte to fetch idx select
    if (cmdParam_len > 0)
    {
        sscanf(cmdParam, "%d", &aud_cfg_select);
    }

#if !defined (BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
    if (aud_cfg_select > AOB_AUD_CFG_TWS_MONO)
    {
        TRACE(1, "%s unsupport stereo %d", __func__, aud_cfg_select);
        return;
    }
#endif

    /// trans app_mode to bes aob aud cfg
    nvrec_appmode_e bes_app_mode = NV_APP_MONE_MIN;
    switch (aud_cfg_select)
    {
        case AOB_AUD_CFG_TWS_MONO:
            bes_app_mode = NV_APP_EARBUDS_MONO;
            break;
        case AOB_AUD_CFG_TWS_STEREO_ONE_CIS:
            bes_app_mode = NV_APP_EARBUDS_STEREO_ONE_CIS;
            break;
        case AOB_AUD_CFG_TWS_STEREO_TWO_CIS:
            bes_app_mode = NV_APP_EARBUDS_STEREO_TWO_CIS;
            break;
        case AOB_AUD_CFG_FREEMAN_STEREO_ONE_CIS:
            bes_app_mode = NV_APP_HEADSET_STEREO_ONE_CIS;
            break;
        case AOB_AUD_CFG_FREEMAN_STEREO_TWO_CIS:
            bes_app_mode = NV_APP_HEADSET_STEREO_TWO_CIS;
            break;
        default:
            TRACE(1, "%s unsupport %d", __func__, aud_cfg_select);
            return;
    }

    app_switch_mode(bes_app_mode, true);
}

static void bap_test_start_uc_srv_ase_operation(const char *cmdParam, uint32_t cmdParam_len)
{
    if (cmdParam != NULL)
    {
        uint8_t op_code_ase_lid_list[2] = {0};

        _str_to_hex(cmdParam, op_code_ase_lid_list, 2 * sizeof(uint8_t));

        if (op_code_ase_lid_list[0] == ASCS_OPCODE_DISABLE)
        {
            aob_media_disable_stream(op_code_ase_lid_list[1]);
        }
        else if (op_code_ase_lid_list[0] == ASCS_OPCODE_RELEASE)
        {
            aob_media_release_stream(op_code_ase_lid_list[1]);
        }
        else
        {
            TRACE(1, "OPCODE 0%d: DISABLE 05, RELEASE 08", op_code_ase_lid_list[0]);
            TRACE(1, "eg. [AOB,uc_srv_ase_op|0501], will disable ase_lid = 1");
        }
    }
}

static void bap_test_start_direct_connection(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t dev_num = cmdParam_len / (2*sizeof(ble_bdaddr_t));

    dev_num = (dev_num > BLE_CONNECTION_MAX) ?  BLE_CONNECTION_MAX : dev_num;

    if (dev_num == 0)
    {
        TRACE(1, "err dev num %d, use param: [addr1type1|...addrNtypeN] format please",
              dev_num);
        TRACE(1, "ex. [11222333336001|11222333336100]");
        return;
    }

    ble_bdaddr_t remote_device[BLE_CONNECTION_MAX] =
    {
        {
            .addr = {0x50,0x33,0x33,0x23,0x22,0x11},    // Default address
            .addr_type = GAPM_STATIC_ADDR,
        },
        {
            .addr = {0x51,0x33,0x33,0x23,0x22,0x11},    // Default address
            .addr_type = GAPM_STATIC_ADDR,
        }
    };

    if (cmdParam != NULL)
    {
        for (uint8_t i = 0; i < dev_num; i++)
        {
            /// Split with '|', so add 1
            _str_to_hex(cmdParam + i * (2*sizeof(ble_bdaddr_t) + 1), (uint8_t *)&remote_device[i], sizeof(ble_bdaddr_t));
        }

        app_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);

        if (dev_num == 1)
        {
            app_ble_start_connect(&remote_device[0], APP_GAPM_STATIC_ADDR);
        }
        else
        {
            app_ble_start_auto_connect(remote_device, dev_num, APP_GAPM_STATIC_ADDR, 500);
        }
    }
}

#ifdef APP_BLE_BIS_DELEG_ENABLE
static void bap_test_start_deleg_add_source(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[sizeof(app_gaf_bap_adv_id_t) + 12] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    app_gaf_bap_adv_id_t *p_addr = (app_gaf_bap_adv_id_t *)param;

    app_gaf_bap_bcast_id_t *p_bc_id = (app_gaf_bap_bcast_id_t *)&param[sizeof(app_gaf_bap_adv_id_t)];

    aob_bis_deleg_add_source(p_addr, p_bc_id, param[sizeof(app_gaf_bap_adv_id_t) + 3], NULL, 0,
                                *(uint16_t *)&param[sizeof(app_gaf_bap_adv_id_t) + 4],
                                *(uint32_t *)&param[sizeof(app_gaf_bap_adv_id_t) + 6]);
}
#endif /// APP_BLE_BIS_DELEG_ENABLE

#ifdef AOB_MOBILE_ENABLED
static void bap_test_start_assist_add_source(const char *cmdParam, uint32_t cmdParam_len)
{
    if (cmdParam != NULL)
    {
        uint8_t param[sizeof(app_gaf_bap_adv_id_t) + 9] = {0};
        _str_to_hex(cmdParam, (uint8_t *)&param[0], sizeof(param));
        DUMP8("%02x ", &param[0], sizeof(param));
#if defined(APP_BLE_BIS_ASSIST_ENABLE)
        aob_bis_assist_add_source(0, (app_gaf_bap_adv_id_t *)param,
                                    param + sizeof(app_gaf_bap_adv_id_t),
                                    *(param + sizeof(app_gaf_bap_adv_id_t) + 3),
                                    *(param + sizeof(app_gaf_bap_adv_id_t) + 4),
                                    *(uint32_t *)(param + sizeof(app_gaf_bap_adv_id_t) + 5));
#endif
        return;
    }
}

static void bap_test_assist_set_bcast_code(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[16] = {0};
    _str_to_hex(cmdParam, (uint8_t *)&param[0], sizeof(param));

#ifdef APP_BLE_BIS_ASSIST_ENABLE
    aob_bis_assist_set_src_id_key(NULL, param);
#endif /// APP_BLE_BIS_ASSIST_ENABLE
}

static void bap_test_start_assist_remove_source(void)
{
#ifdef APP_BLE_BIS_ASSIST_ENABLE
    aob_bis_assist_remove_source(0, 0);
#endif /// APP_BLE_BIS_ASSIST_ENABLE
}

static void bap_test_start_pacc_set_audio_location(const char *cmdParam, uint32_t cmdParam_len)
{
    if (cmdParam != NULL)
    {
        uint32_t location_bf = 0;
        _str_to_hex(cmdParam, (uint8_t *)&location_bf, sizeof(location_bf));
        DUMP8("%02x ", &location_bf, sizeof(location_bf));
        aob_pacs_mobile_set_location(0, location_bf);
        return;
    }
}

static void bap_test_start_uc_cli_cfg_codec(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[6] = {0};
    
    _str_to_hex(cmdParam, param, sizeof(param));

    if (param[2] == 0)
    {
        // Use dft cis timing @see dft_cis_timing
        ble_audio_test_update_cis_timing(dft_cis_timing);
        // Use 10ms sdu interval
        ble_audio_test_update_sdu_intv(10000, 10000);
    }
    else if (param[2] == 1)
    {
        // Use dft cis timing @see dft_cis_timing
        ble_audio_test_update_cis_timing(dft_cis_timing_7_5ms);
        // Use 10ms sdu interval
        ble_audio_test_update_sdu_intv(7500, 7500);
    }
    else
    {
        TRACE(1, "eg. [AOB,aob_uc_cli_cfg_codec|000100064000]");
        TRACE(1, "will codec cfg ase_lid 0, with cis id 1, fs enum 6, fr octs 0x40");
    }

    TRACE(1, "ase_lid = %d, cis_id = %d, fs = %d, octs = %d",
                                param[0],
                                param[1],
                                param[3],
                                *(uint16_t *)&param[4]);

    aob_media_mobile_cfg_codec(param[0],
                                param[1],
                                AOB_CODEC_ID_LC3,
                                param[3],
                                *(uint16_t *)&param[4]);
}

static void bap_test_start_uc_cli_cfg_qos(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[2] = {0};
    
    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mobile_cfg_qos(param[0],
                                param[1]);
}

static void bap_test_start_uc_cli_enable(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[10] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mobile_enable(param[0], *(uint16_t *)&param[1], param[3], &param[4]);
}

static void bap_test_start_uc_cli_disable(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t ase_lid = 0;

    _str_to_hex(cmdParam, &ase_lid, sizeof(ase_lid));

    aob_media_mobile_disable(ase_lid);
}

static void bap_test_start_uc_cli_release(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t ase_lid = 0;

    _str_to_hex(cmdParam, &ase_lid, sizeof(ase_lid));

    aob_media_mobile_release(ase_lid);
}

static void bap_test_start_uc_cli_upd_md(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[8] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    app_gaf_bap_cfg_metadata_t *p_md_param = (app_gaf_bap_cfg_metadata_t *)&param[1];

    aob_media_mobile_update_metadata(param[0], p_md_param);
}

static void bap_test_start_unidirectionla_stream(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[6] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    ble_audio_test_start_unidirectional_stream(param[0], param[1], *(uint16_t *)&param[2], *(uint16_t *)&param[4]);
}

static void bap_test_start_bidirectionla_stream(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[8] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    ble_audio_test_start_bidirectional_stream(param[0], param[1], *(uint16_t *)&param[2], *(uint16_t *)&param[4], *(uint16_t *)&param[6]);
}

#ifdef APP_BLE_BIS_SRC_ENABLE
static void bap_test_start_bis_src_grp(const char *cmdParam, uint32_t cmdParam_len)
{
    /*02021027000028000200000028000301010001000000*/
    /*
    uint8_t nb_subgrp;1
    uint8_t nb_stream;1
    uint32_t sdu_intvl;4
    uint16_t max_sdu_size;2
    uint32_t location_bf;4
    uint16_t frame_octet;2
    uint8_t sampling_freq;1
    uint8_t frame_dur;1
    uint8_t frames_sdu;1
    p_cfg->add_cfg.len;1
    */
    // Total:18
    /*
    uint8_t bc_id[3];3
    bool encrypt;1
    uint8_t bcast_code[16];16
    */
    // Total:20

    //Total:20+18 = 38
    uint8_t param[38] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_bis_src_update_src_group_info(0, param[0], param[1], *(uint32_t *)&param[2], *(uint16_t *)&param[6]);

    app_gaf_bap_cfg_t *p_cfg = (app_gaf_bap_cfg_t *)&param[8];

    p_cfg->add_cfg.len = 0;

    aob_bis_src_subgrp_param_t subgrp_param =
    {
        .sgrp_lid = 0,
        .location_bf = p_cfg->param.location_bf,
        .frame_octet = p_cfg->param.frame_octet,
        .sampling_freq = p_cfg->param.sampling_freq,
        .frame_dur = p_cfg->param.frame_dur,
        .frames_sdu = p_cfg->param.frames_sdu,
        .context_bf = APP_GAF_BAP_CONTEXT_TYPE_UNSPECIFIED,
    };

    memcpy(subgrp_param.codec_id, AOB_CODEC_ID_LC3, sizeof(*AOB_CODEC_ID_LC3));

    uint8_t subgrp_idx = 0;

    for (subgrp_idx = 0; subgrp_idx < param[0]; subgrp_idx++)
    {
        subgrp_param.sgrp_lid = subgrp_idx;
        aob_bis_src_set_subgrp_param(0, &subgrp_param);
    }

    uint8_t stream_idx = 0;

    for (stream_idx = 0; stream_idx < param[1]; stream_idx++)
    {
        aob_bis_src_update_stream_codec_cfg(0, stream_idx, p_cfg);
    }

    app_bap_bc_src_set_bcast_id(0, &param[18], 3);
    app_bap_bc_src_set_encrypt(0, param[21], (app_gaf_bc_code_t *)&param[22]);

    aob_bis_src_start(0, NULL);
}

static void bap_test_update_subgrp_metadata(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[8] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    app_gaf_bap_cfg_metadata_t *p_md_param = (app_gaf_bap_cfg_metadata_t *)&param[1];

    aob_bis_src_update_metadata(0, param[0], p_md_param);
}

static void bap_test_stop_bis_src_grp(void)
{
    aob_bis_src_stop(0);
}

static void bap_test_start_bis_src_streaming(void)
{
    // enable src, then start audio streaming
    aob_bis_src_enable(0);
}

static void bap_test_stop_bis_src_streaming(void)
{
    // stop audio straming, then disable src
    aob_bis_src_stop_streaming(0);
}
#endif

static void vcp_test_set_absolute_volume(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t le_vol = 0;

    _str_to_hex(cmdParam, &le_vol, sizeof(le_vol));

    aob_mobile_vol_set_abs_le_vol(0, le_vol);
}

static void vcp_test_set_volume_mute(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t mute = 0;

    _str_to_hex(cmdParam, &mute, sizeof(mute));

    if (mute == 0)
    {
        aob_mobile_vol_mute(0);
    }
    else
    {
        aob_mobile_vol_unmute(0);
    }
}

static void vcp_test_set_volume_offset(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[3] = {0};

    _str_to_hex(cmdParam, &param[0], sizeof(param));

    int16_t offset = *(uint16_t *)&param[1];

    offset *=  param[0] == 0 ? (-1) : (1);

    aob_mobile_vol_set_volume_offset(0, 0, offset);
}

static void micp_test_set_mic_mute(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t mute = 0;

    _str_to_hex(cmdParam, &mute, sizeof(mute));

    aob_media_mobile_micc_set_mute(0, mute);
}

static void micp_test_read_mic_mute(void)
{
    aob_media_mobile_micc_read_mute(0);
}

static void mcp_mcs_set_player_name(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[50] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mobile_set_player_name(param[0], &param[2], param[1]);
}

static void mcp_mcs_set_curr_track_info(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[50] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mobile_change_track(param[0], 12000, &param[2], param[1]);
}

static void mcp_mcs_media_action(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[8] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mobile_position_ctrl(param[0], param[1], *(int32_t *)&param[2], *(int8_t *)&param[6]);
}

static void micp_test_set_mic_input_gain(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[3] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    int16_t gain = *(uint16_t *)&param[1];

    gain *= param[0] == 0 ? (-1) : (1);

    aob_mobile_aic_set_input_gain(0, 0, gain);
}

static void micp_test_set_mic_input_mute(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t mute = 0;

    _str_to_hex(cmdParam, &mute, sizeof(mute));

    aob_mobile_aic_set_input_mute(0, 0, mute);
}

static void micp_test_set_mic_input_gain_mode(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t gain_mode = 0;

    _str_to_hex(cmdParam, &gain_mode, sizeof(gain_mode));

    aob_mobile_aic_set_input_gain_mode(0, 0, gain_mode);
}

static void aicp_test_read_character(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t char_type = 0;

    _str_to_hex(cmdParam, &char_type, sizeof(char_type));

    aob_mobile_aic_read_input_char(0, 0, char_type);
}

#endif /// AOB_MOBILE_ENABLED

static void vocs_test_set_vocs_location(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[5];

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_vol_set_audio_location(param[0], *(uint32_t *)&param[1]);
}

static void vocs_test_set_vocs_description(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[11];

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_vol_set_output_description(param[0], &param[1], sizeof(param) - sizeof(param[0]));
}

static void mics_test_set_mic_mute(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t mute = 0;

    _str_to_hex(cmdParam, &mute, sizeof(mute));

    aob_media_mics_set_mute(mute);
}
#endif /// BLE_AUDIO_ENABLED

static void ble_test_del_index_record(const char *Index, uint32_t str_len)
{
    const char *pend = Index + str_len;
    uint8_t index = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Index, pend, &index, 1);
    if (len != 1)
    {
        return;
    }
    nv_record_ble_delete_entry_by_index(index);
}

static void ble_test_del_all_record(void)
{
    nv_record_ble_delete_all_entry();
}

static void ble_test_del_record(void)
{
    const bt_bdaddr_t *peer_addr = NULL;
    bt_addr_type_t addr_type = BT_ADDR_TYPE_PUBLIC;
    peer_addr = app_test_get_le_address(&addr_type);
    gap_del_ble_device_record(peer_addr);
}

static void ble_test_dump_record(void)
{
    gap_bond_sec_t record = {{{0}}, (bt_addr_type_t)0};
    uint32_t count = 0;
    uint32_t i = 0;

    count = gap_count_ble_device_record();
    for (; i < count; i += 1)
    {
        if (!gap_get_ble_device_record_by_index(i, &record))
        {
            return;
        }
        TRACE(0, "ble record [%d/%d] %02x %08x:%04x flags %08x", i, count,
            record.peer_type, CO_COMBINE_UINT32_LE(record.peer_addr.address + 2), CO_COMBINE_UINT16_LE(record.peer_addr.address),
            (record.device_bonded<<28)|(record.secure_pairing<<24)|(record.bonded_with_num_compare<<20)|(record.bonded_with_passkey_entry<<16)|
            (record.peer_irk_distributed<<12)|(record.peer_csrk_distributed<<8)|(record.enc_key_size));
        TRACE(0, "ble record ltk %08x %08x %08x %08x rand %08x %08x ediv %04x",
            CO_COMBINE_UINT32_LE(record.ltk+12), CO_COMBINE_UINT32_LE(record.ltk+8),
            CO_COMBINE_UINT32_LE(record.ltk+4), CO_COMBINE_UINT32_LE(record.ltk),
            CO_COMBINE_UINT32_LE(record.rand+4), CO_COMBINE_UINT32_LE(record.rand),
            CO_COMBINE_UINT16_LE(record.ediv));
        TRACE(0, "ble record irk %08x %08x %08x %08x local ediv %08x %08x %04x",
            CO_COMBINE_UINT32_LE(record.peer_irk+12), CO_COMBINE_UINT32_LE(record.peer_irk+8),
            CO_COMBINE_UINT32_LE(record.peer_irk+4), CO_COMBINE_UINT32_LE(record.peer_irk),
            CO_COMBINE_UINT32_LE(record.local_rand+4), CO_COMBINE_UINT32_LE(record.local_rand),
            CO_COMBINE_UINT16_LE(record.local_ediv));
    }
}

static void bt_record_dump_test(void)
{
    btif_device_record_t record = {{{0}}, 0};
    bt_status_t status = BT_STS_SUCCESS;
    int count = 0;
    int i = 0;

    count = nv_record_get_paired_dev_count();
    for (; i < count; i += 1)
    {
        status = nv_record_enum_dev_records(i, &record);
        if (status != BT_STS_SUCCESS)
        {
            return;
        }
        TRACE(0, "bt record [%d/%d] %08x:%04x flags %08x", i, count,
            CO_COMBINE_UINT32_LE(record.bdAddr.address + 2), CO_COMBINE_UINT16_LE(record.bdAddr.address),
            (record.for_bt_source<<24)|(record.trusted<<16)|(record.keyType<<8)|(record.pinLen));
        TRACE(0, "bt record linkkey %08x %08x %08x %08x",
            CO_COMBINE_UINT32_LE(record.linkKey+12), CO_COMBINE_UINT32_LE(record.linkKey+8),
            CO_COMBINE_UINT32_LE(record.linkKey+4), CO_COMBINE_UINT32_LE(record.linkKey));
    }
}

static void bt_record_del_by_index_test(const char *Index, uint32_t str_len)
{
    const char *pend = Index + str_len;
    btif_device_record_t record = {{{0}}, 0};
    uint8_t index = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Index, pend, &index, 1);
    if (len != 1)
    {
        return;
    }
    if (nv_record_enum_dev_records(index, &record) == BT_STS_SUCCESS)
    {
        nv_record_ddbrec_delete(&record.bdAddr);
    }
}

static void bt_record_del_all_test(void)
{
    nv_record_ddbrec_clear();
}

static void bt_record_del_test(void)
{
    bt_addr_type_t peer_type;
    const bt_bdaddr_t *peer_addr = NULL;
    peer_addr = app_test_get_le_address(&peer_type);
    nv_record_ddbrec_delete(peer_addr);
}

static void bt_nv_flush_test(void)
{
    nv_record_flash_flush();
}

static void bt_shutdown_test(void)
{
    app_shutdown();
}

static void bt_trigger_assert_test(void)
{
    CO_ASSERT_0(0);
}

#ifndef BLE_ONLY_ENABLED

static void bt_test_start_inquiry(const char *LimitInquiry, uint32_t str_len)
{
    const char *pend = LimitInquiry + str_len;
    uint8_t limit_inquiry = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&LimitInquiry, pend, &limit_inquiry, 1);
    if (len != 1)
    {
        return;
    }
    btif_me_inquiry(limit_inquiry ? BTIF_BT_IAC_LIAC : BTIF_BT_IAC_GIAC, 10, 0);
}

static void bt_test_cancel_inquiry(void)
{
    btif_me_cancel_inquiry();
}

static void bt_test_start_scan(const char *ScanMode00NoScan01InqScan02PageScan03BothScan13Limited, uint32_t str_len)
{
    const char *pend = ScanMode00NoScan01InqScan02PageScan03BothScan13Limited + str_len;
    uint8_t scan_mode = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&ScanMode00NoScan01InqScan02PageScan03BothScan13Limited, pend, &scan_mode, 1);
    if (len != 1)
    {
        return;
    }
    app_bt_set_access_mode((btif_accessible_mode_t)scan_mode);
}

static void bt_test_general_bonding(const char *GeneralBonding, uint32_t str_len)
{
    const char *pend = GeneralBonding + str_len;
    uint8_t general_bonding = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&GeneralBonding, pend, &general_bonding, 1);
    if (len != 1)
    {
        return;
    }
    btif_sec_set_general_bonding(general_bonding);
}

static void bt_test_responder_trigger_auth(const char *RespTriggerAuth, uint32_t str_len)
{
    const char *pend = RespTriggerAuth + str_len;
    uint8_t responder_trigger_auth = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&RespTriggerAuth, pend, &responder_trigger_auth, 1);
    if (len != 1)
    {
        return;
    }
    btif_sec_allow_responder_trigger_auth(responder_trigger_auth);
}

static void bt_test_set_min_enc_key_size(const char *MinEncKeySize, uint32_t str_len)
{
    const char *pend = MinEncKeySize + str_len;
    uint8_t min_enc_key_size = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&MinEncKeySize, pend, &min_enc_key_size, 1);
    if (len != 1)
    {
        return;
    }
    btif_sec_set_min_enc_key_size(min_enc_key_size);
}

static void bt_set_both_scan_test(void)
{
    app_bt_set_access_mode((btif_accessible_mode_t)0x03);
}

static void bt_disable_scan_test(void)
{
    app_bt_set_access_mode((btif_accessible_mode_t)0x00);
}

static void bt_reconnect_hfp_test(void)
{
    app_bt_reconnect_hfp_profile((bt_bdaddr_t *)app_test_get_bt_address());
}

static void bt_reconnect_a2dp_test(void)
{
    app_bt_reconnect_a2dp_profile((bt_bdaddr_t *)app_test_get_bt_address());
}

static void bt_reconnect_test(void)
{
    bt_reconnect_a2dp_test();
    bt_reconnect_hfp_test();
}

static void bt_start_auth_test(void)
{
    struct hci_conn_item_t *conn = NULL;
    const bt_bdaddr_t *bdaddr = app_test_get_bt_address();
    conn = hci_find_conn_by_address(HCI_CONN_TYPE_BT_ACL, BT_ADDR_TYPE_PUBLIC, bdaddr);
    if (conn == NULL)
    {
        TRACE(0, "bt_start_auth_test: invalid address");
        return;
    }
    btif_me_auth_req(conn->connhdl);
}

static void bt_test_set_pts_mode(void)
{
    besbt_cfg.pts_test_dont_bt_role_switch = true;
}

static void bt_disconnect_test(void)
{
    struct hci_conn_item_t *conn = NULL;
    const bt_bdaddr_t *bdaddr = app_test_get_bt_address();
    conn = hci_find_conn_by_address(HCI_CONN_TYPE_BT_ACL, BT_ADDR_TYPE_PUBLIC, bdaddr);
    if (conn == NULL)
    {
        TRACE(0, "bt_start_auth_test: invalid address");
        return;
    }
    bt_defer_call_func_2(btlib_hcicmd_disconnect,
        bt_fixed_param(conn->connhdl),
        bt_fixed_param(BTIF_BEC_USER_TERMINATED));
}

static void bt_l2cap_close_or_send_data(const char *Conidx_Dcid_DataLen_SendEcho_Data, uint32_t str_len)
{
    const char *pend = Conidx_Dcid_DataLen_SendEcho_Data + str_len;
    uint8_t conidx = 0;
    uint8_t dcid_be[2] = {0};
    uint8_t data_len = 0;
    uint8_t is_echo_data = 0;
    uint8_t data[16] = {0};
    uint16_t dcid = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_Dcid_DataLen_SendEcho_Data, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_Dcid_DataLen_SendEcho_Data, pend, dcid_be, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_Dcid_DataLen_SendEcho_Data, pend, &data_len, 1);
    if (len != 1)
    {
        return;
    }
    if (data_len)
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_Dcid_DataLen_SendEcho_Data, pend, &is_echo_data, 1);
        if (len != 1)
        {
            return;
        }
        if (data_len <= 16)
        {
            len = app_ble_scan_hex_data_from_string(&Conidx_Dcid_DataLen_SendEcho_Data, pend, data, data_len);
            if (len != data_len)
            {
                return;
            }
        }
    }
    dcid = CO_COMBINE_UINT16_BE(dcid_be);
    gap_conn_item_t *conn = gap_get_conn_item(gap_conn_idx_as_hdl(conidx));
    if (conn == NULL)
    {
        TRACE(0, "invalid bt device_id d%x", conidx);
        return;
    }
    l2cap_conn_t *l2cap_conn = l2cap_conn_search_by_handle(conn->connhdl);
    if (l2cap_conn == NULL)
    {
        TRACE(0, "invalid bt connhdl %04x", conn->connhdl);
        return;
    }
    l2cap_channel_t *chan = l2cap_channel_search_dcid(l2cap_conn, dcid);
    if (chan == NULL)
    {
        TRACE(0, "invalid l2cap dcid %04x", dcid);
        return;
    }
    if (data_len)
    {
        if (is_echo_data)
        {
            l2cap_send_echo_req(l2cap_conn->device_id, l2cap_conn, data_len, data);
        }
        else
        {
            l2cap_send_data(chan->l2cap_handle, data, data_len, NULL);
        }
    }
    else
    {
        l2cap_close(chan->l2cap_handle);
    }
}

#endif /* BLE_ONLY_ENABLED */

static void ble_test_print_local_irk(void)
{
    const uint8_t *l_irk = gap_get_local_irk();
    TRACE(0, "local irk: %08x %08x %08x %08x",
        CO_COMBINE_UINT32_LE(l_irk+12), CO_COMBINE_UINT32_LE(l_irk+8),
        CO_COMBINE_UINT32_LE(l_irk+4), CO_COMBINE_UINT32_LE(l_irk));
}

static void ble_test_resolving_list_size(void)
{
    TRACE(0, "resolving list size: %d/%d enabled %d",
        gap_resolving_list_curr_size(),
        gap_resolving_list_max_size(),
        gap_address_reso_is_enabled());
}

static void ble_test_resolving_list_clear(void)
{
    gap_resolving_list_clear();
}

static void ble_test_resolving_list_dump(void)
{
    const gap_resolv_item_t *item = NULL;
    uint8_t size = gap_resolving_list_curr_size();
    int i = 0;
    for (; i < size; i += 1)
    {
        item = gap_resolving_list_get_item(i);
        if (item)
        {
            TRACE(0, "resolving list item: [%d] %02x %04x:%02x device mode %d",
                i, item->peer_type, CO_COMBINE_UINT32_LE(item->peer_addr.address+2),
                CO_COMBINE_UINT16_LE(item->peer_addr.address), item->device_privacy_mode);
            TRACE(0, "resolving list item: [%d] p_irk %08x %08x %08x %08x",
                i, CO_COMBINE_UINT32_LE(item->peer_irk+12), CO_COMBINE_UINT32_LE(item->peer_irk+8),
                CO_COMBINE_UINT32_LE(item->peer_irk+4), CO_COMBINE_UINT32_LE(item->peer_irk));
        }
    }
}

static void ble_test_resolving_list_set_network_mode(void)
{
    gap_set_all_network_privacy_mode();
}

static void ble_test_resolving_list_set_device_mode(void)
{
    gap_set_all_device_privacy_mode();
}

static void ble_test_resolving_list_set_bonded_devices(void)
{
    app_ble_add_devices_info_to_resolving();
}

static void ble_test_resolving_list_set_rpa_timeout(const char *Timeout, uint32_t str_len)
{
    const char *pend = Timeout + str_len;
    uint8_t timeout[2];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Timeout, pend, timeout, 2);
    if (len != 2)
    {
        return;
    }
    gap_set_rpa_timeout(CO_COMBINE_UINT16_BE(timeout));
}

static void ble_test_resolving_list_remove_by_index(const char *Index, uint32_t str_len)
{
    const char *pend = Index + str_len;
    uint8_t index = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Index, pend, &index, 1);
    if (len != 1)
    {
        return;
    }
    gap_resolving_list_remove_by_index(index, true, false);
}

static void ble_test_resolving_list_remove_by_address(const char *Type_BdaddrLsb2Msb, uint32_t str_len)
{
    const char *pend = Type_BdaddrLsb2Msb + str_len;
    uint8_t type = 0;
    uint8_t bdaddr[6];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, &type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, bdaddr, 6);
    if (len != 6)
    {
        return;
    }
    gap_resolving_list_remove((bt_addr_type_t)type, (bt_bdaddr_t *)bdaddr, true, false);
}

static void ble_test_resolving_list_add(const char *Type_BdAddr_DeviceMode, uint32_t str_len)
{
    const char *pend = Type_BdAddr_DeviceMode + str_len;
    uint8_t type = 0;
    uint8_t bdaddr[6];
    uint8_t device_privacy_mode = 0;
    uint8_t peer_irk[16];
    bool has_peer_irk = false;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Type_BdAddr_DeviceMode, pend, &type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdAddr_DeviceMode, pend, bdaddr, 6);
    if (len != 6)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdAddr_DeviceMode, pend, &device_privacy_mode, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdAddr_DeviceMode, pend, peer_irk, 16);
    has_peer_irk = (len == 16);
    gap_resolving_list_add((bt_addr_type_t)type, (bt_bdaddr_t *)bdaddr, has_peer_irk ? peer_irk : NULL, device_privacy_mode, true, false);
}

static void ble_test_ble_enable_addr_reso(const char *Enable, uint32_t str_len)
{
    const char *pend = Enable + str_len;
    uint8_t enable = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Enable, pend, &enable, 1);
    if (len != 1)
    {
        return;
    }
    gap_enable_address_resolution(enable);
}

static void ble_test_filter_list_size(void)
{
    TRACE(0, "filter list size: %d/%d",
        gap_filter_list_curr_size(),
        gap_filter_list_max_size());
}

static void ble_test_filter_list_clear(void)
{
    gap_filter_list_clear();
}

static void ble_test_filter_list_dump(void)
{
    const gap_filter_item_t *item = NULL;
    uint8_t size = gap_filter_list_curr_size();
    int i = 0;
    for (; i < size; i += 1)
    {
        item = gap_filter_list_get_item(i);
        if (item)
        {
            TRACE(0, "filter list item: [%d] %02x %04x:%02x",
                i, item->peer_type, CO_COMBINE_UINT32_LE(item->peer_addr.address+2),
                CO_COMBINE_UINT16_LE(item->peer_addr.address));
        }
    }
}

static void ble_test_filter_list_add_anonymous(void)
{
    gap_filter_list_add_anonymous();
}

static void ble_test_filter_list_remove_anonymous(void)
{
    gap_filter_list_remove_anonymous();
}

static void ble_test_filter_list_set_bonded_devices(void)
{
    gap_filter_list_set_bonded_devices();
}

static void ble_test_filter_list_remove_by_index(const char *Index, uint32_t str_len)
{
    const char *pend = Index + str_len;
    uint8_t index = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Index, pend, &index, 1);
    if (len != 1)
    {
        return;
    }
    gap_filter_list_remove_by_index(index);
}

static void ble_test_filter_list_remove_by_address(const char *Type_BdaddrLsb2Msb, uint32_t str_len)
{
    const char *pend = Type_BdaddrLsb2Msb + str_len;
    uint8_t type = 0;
    uint8_t bdaddr[6];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, &type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, bdaddr, 6);
    if (len != 6)
    {
        return;
    }
    gap_filter_list_remove((bt_addr_type_t)type, (bt_bdaddr_t *)bdaddr);
}

static void ble_test_filter_list_add(const char *Type_BdaddrLsb2Msb, uint32_t str_len)
{
    const char *pend = Type_BdaddrLsb2Msb + str_len;
    uint8_t type = 0;
    uint8_t bdaddr[6];
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, &type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Type_BdaddrLsb2Msb, pend, bdaddr, 6);
    if (len != 6)
    {
        return;
    }
    gap_filter_list_add((bt_addr_type_t)type, (bt_bdaddr_t *)bdaddr);
}

static void ble_test_disc_conidx(const char *Conidx, uint32_t str_len)
{
    const char *pend = Conidx + str_len;
    uint8_t conidx = 0;
    uint16_t len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    app_ble_start_disconnect(conidx);
}

static void ble_test_set_adv_data(const char *AdvHandle_AdvData00RspData01_DataLen_Data, uint32_t str_len)
{
    const char *pend = AdvHandle_AdvData00RspData01_DataLen_Data + str_len;
    uint8_t adv_handle = 0;
    uint8_t is_scan_rsp_data = 0;
    uint8_t adv_len = 0;
    uint8_t adv_data[32] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&AdvHandle_AdvData00RspData01_DataLen_Data, pend, &adv_handle, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&AdvHandle_AdvData00RspData01_DataLen_Data, pend, &is_scan_rsp_data, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&AdvHandle_AdvData00RspData01_DataLen_Data, pend, &adv_len, 1);
    if (len != 1)
    {
        return;
    }
    if (adv_len)
    {
        len = app_ble_scan_hex_data_from_string(&AdvHandle_AdvData00RspData01_DataLen_Data, pend, adv_data, adv_len);
        if (len != adv_len)
        {
            return;
        }
    }
    if (is_scan_rsp_data)
    {
        gap_set_adv_data(adv_handle, adv_data, adv_len);
    }
    else
    {
        gap_set_scan_rsp_data(adv_handle, adv_data, adv_len);
    }
}

static void ble_test_start_advertising(const char *Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, uint32_t str_len)
{
    const char *pend = Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr + str_len;
    uint8_t extended = 0;
    uint8_t own_addr_type = 0;
    uint8_t conectable = 0;
    uint8_t scannable = 0;
    uint8_t directed = 0;
    uint8_t has_peer_addr = 0;
    uint8_t peer_addr_type = BT_ADDR_TYPE_PUBLIC;
    uint8_t peer_addr[6] = {0};
    uint8_t *peer_ble_addr = g_test_le_pts_addr+1;
    uint8_t use_filter_list = 0;
    uint8_t adv_len = 0;
    uint8_t scan_rsp_len = 0;
    uint8_t has_custom_addr = 0;
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    uint8_t adv_data[32] = {0};
    uint8_t scan_rsp_data[32] = {0};
    uint8_t local_addr[6] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &extended, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &own_addr_type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &conectable, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &scannable, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &directed, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &has_peer_addr, 1);
    if (len != 1)
    {
        return;
    }
    if (directed && has_peer_addr)
    {
        len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &peer_addr_type, 1);
        if (len != 1)
        {
            return;
        }
        len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, peer_addr, 6);
        if (len != 6)
        {
            return;
        }
        peer_ble_addr = peer_addr;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &use_filter_list, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &adv_len, 1);
    if (len != 1)
    {
        return;
    }
    if (adv_len)
    {
        len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, adv_data, adv_len);
        if (len != adv_len)
        {
            return;
        }
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &scan_rsp_len, 1);
    if (len != 1)
    {
        return;
    }
    if (scan_rsp_len)
    {
        len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, scan_rsp_data, scan_rsp_len);
        if (len != scan_rsp_len)
        {
            return;
        }
    }
    len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, &has_custom_addr, 1);
    if (len != 1)
    {
        return;
    }
    if (has_custom_addr)
    {
        len = app_ble_scan_hex_data_from_string(&Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr, pend, local_addr, 6);
        if (len != 6)
        {
            return;
        }
    }
    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);
    if (directed)
    {
        if (extended)
        {
            adv_handle = gap_start_directed_advertising((bt_addr_type_t)own_addr_type, (bt_addr_type_t)peer_addr_type, (bt_bdaddr_t *)peer_ble_addr, NULL);
        }
        else
        {
            adv_handle = gap_legacy_directed_advertising((bt_addr_type_t)own_addr_type, (bt_addr_type_t)peer_addr_type, (bt_bdaddr_t *)peer_ble_addr, NULL);
        }
    }
    else
    {
        uint32_t cont_filter_legacy_conn_scan = 0;
        cont_filter_legacy_conn_scan |= scannable ? 0x01 : 0;
        cont_filter_legacy_conn_scan |= conectable ? 0x02 : 0;
        cont_filter_legacy_conn_scan |= extended ? 0 : 0x04;
        cont_filter_legacy_conn_scan |= use_filter_list ? 0x08 : 0;
        cont_filter_legacy_conn_scan |= 0x10; // continue advtising
        adv_handle = gap_impl_start_advertising((bt_addr_type_t)own_addr_type, NULL, cont_filter_legacy_conn_scan, has_custom_addr ? (bt_bdaddr_t *)local_addr : NULL);
    }
    if (adv_len)
    {
        gap_set_adv_data(adv_handle, adv_data, adv_len);
    }
    if (scan_rsp_len)
    {
        gap_set_scan_rsp_data(adv_handle, scan_rsp_data, scan_rsp_len);
    }
    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_legacy_non_discoverable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_legacy_non_discoverable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_NON_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_legacy_non_discoverable_connectable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_legacy_connectable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_NON_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_legacy_connectable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_legacy_connectable_advertising(BT_ADDR_TYPE_PUBLIC, false, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_LIMITED_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_legacy_scannable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_legacy_scannable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_GENERAL_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_non_discoverable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_start_non_discoverable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_NON_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_non_discoverable_connectable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_start_connectable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_NON_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_connectable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    gap_dt_buf_t adv_data_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_start_connectable_advertising(BT_ADDR_TYPE_PUBLIC, false, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_LIMITED_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_scannable_adv(void)
{
    uint8_t adv_handle = GAP_INVALID_ADV_HANDLE;
    uint16_t service_uuid[] = {GATT_UUID_GAP_SERVICE, GATT_UUID_GATT_SERVICE};
    gap_dt_buf_t adv_data_buf = {0};
    gap_dt_buf_t scan_rsp_buf = {0};

    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);

    adv_handle = gap_start_scannable_advertising(BT_ADDR_TYPE_PUBLIC, true, NULL);

    gap_dt_add_flags(&adv_data_buf, GAP_FLAGS_LE_GENERAL_DISCOVERABLE_MODE, false);
    gap_set_adv_data(adv_handle, ppb_get_data(adv_data_buf.ppb), adv_data_buf.ppb->len);
    gap_dt_buf_clear(&adv_data_buf);

    gap_set_le_service_16_uuid(&scan_rsp_buf, service_uuid, 2);
    gap_set_scan_rsp_data(adv_handle, ppb_get_data(scan_rsp_buf.ppb), scan_rsp_buf.ppb->len);
    gap_dt_buf_clear(&scan_rsp_buf);

    gap_enable_advertising(adv_handle, 0);
}

static void ble_test_close_all_adv(void)
{
    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);
}

#if BLE_PA_SUPPORT
static uint8_t bap_big_id = BAP_INVALID_BIG_ID;
static uint8_t pa_adv_handle = GAP_INVALID_ADV_HANDLE;
static uint16_t pa_sync_handle = GAP_INVALID_CONN_HANDLE;
extern "C" void app_bap_dp_itf_send_data_directly(uint16_t conhdl, uint16_t seq_num, uint8_t *payload, uint16_t payload_len, uint32_t ref_time);

static int bap_test_event_callback(uintptr_t group_id, bap_event_t event, bap_event_param_t param)
{
    if (event == BAP_EVENT_BIG_OPENED)
    {
        bap_big_opened_t *big_opened = param.big_opened;
        if (!big_opened->err_code)
        {
            TRACE(0, "bap_test_callback big opened: broadcaster %d big_id %d bis_count %d iso_handle %04x",
                big_opened->is_broadcaster, big_opened->big_id, big_opened->bis_count, big_opened->stream[0]->iso_handle);
            bap_big_id = big_opened->big_id;
            if (big_opened->is_broadcaster)
            {
                bap_iso_param_t iso_param = {0};
                iso_param.controller_delay_us = 0;
                iso_param.codec_cfg_len = 0;
                bap_setup_iso_tx_data_path(big_opened->stream[0]->iso_handle, &iso_param);
            }
        }
    }
    else if (event == BAP_EVENT_BIG_CLOSED)
    {
        TRACE(0, "bap_test_callback big closed");
        bap_big_id = BAP_INVALID_BIG_ID;
    }
    else if (event == BAP_EVENT_ISO_PATH_SETUP)
    {
        bap_iso_path_setup_t *iso_setup = param.iso_path_setup;
        TRACE(0, "bap_test_callback iso setup: %02x iso_handle %04x", iso_setup->error_code, iso_setup->stream->iso_handle);
        if (!iso_setup->error_code)
        {
#if BLE_AUDIO_ENABLED
            uint8_t payload[8] = {0};
            app_bap_dp_itf_send_data_directly(iso_setup->stream->iso_handle, 0, payload, sizeof(payload), 0);
#endif
        }
    }
    else if (event == BAP_EVENT_ISO_PATH_REMOVED)
    {
        TRACE(0, "bap_test_callback iso removed");
    }
    return 0;
}

static int ble_test_pa_callback(uintptr_t adv, gap_adv_event_t event, gap_adv_callback_param_t param)
{
    if (event == GAP_ADV_EVENT_PA_ENABLED)
    {
        gap_pa_enabled_t *pa_enabled = param.pa_enabled;
        bap_big_param_t big_param = {0};
        TRACE(0, "pa_test_callback pa enabled");
        big_param.bis_count = 0x01; // 0x01 to 0x1F
        big_param.phy_bits = 0x01; // bit 0: transmitter phy is LE 1M, bit 1: LE 2M, bit 2: LE Coded, Host shall set at least one bit
        big_param.max_sdu_size = 20; // 0x01 to 0x0FFFF, max octets of an SDU
        big_param.sdu_interval_us = 10000; // 0xFF to 0x0F_FFFF, the interval in us of periodic SDUs
        big_param.packing = 0; // 0x00 sequential, 0x01 interleaved, just recommendation
        big_param.framing = 0; // 0x00 unframed, 0x01 framed
        big_param.encryption = 0; // 0x00 unencrypted, 0x01 encrypted
        big_param.rtn = 2; // 0x00 to 0x1E, retrans num of every BIS data PDU, just recommend, ignored for test cmd
        big_param.max_transport_latency_us = 20; // 0x05 to 0x0FA0, max transport latency in us, includes pre-transmissions, ignored for test cmd
        bap_create_big(bap_test_event_callback, pa_enabled->adv_handle, &big_param, false);
    }
    else if (event == GAP_ADV_EVENT_PA_DISABLED)
    {
        TRACE(0, "pa_test_callback pa disabled");
    }
    else if (event == GAP_ADV_EVENT_PA_SUBEVENT_DATA_REQ)
    {
        gap_pa_subevent_data_t data = {0};
        uint8_t subevent_data[2] = {0xAB, 0xCD};
        TRACE(0, "pa_test_callback pa subevent data req");
        data.subevent = 0x00;
        data.response_slot_start = 0x00; // the first response slot to be used in the subevent
        data.response_slot_count = 0x01; // num of response slots to be used
        data.subevent_data_len = sizeof(subevent_data); // 0 to 251
        data.subevent_data = subevent_data;
        gap_set_pa_subevent_data(pa_adv_handle, 0x01, &data);
    }
    else if (event == GAP_ADV_EVENT_CONN_OPENED)
    {
        gap_conn_param_t *conn_opened = param.conn_opened;
        TRACE(0, "pa_test_callback pawr conn opened");
        gap_pa_set_info_transfer(conn_opened->con_idx, 0xAABB, pa_adv_handle);
    }
    return 0;
}

static void ble_test_start_pa(const char *Enable_OwnAddrType_PAWR_PaDataLen_PaData, uint32_t str_len)
{
    const char *pend = Enable_OwnAddrType_PAWR_PaDataLen_PaData + str_len;
    uint8_t enable_pa = 0;
    uint8_t own_addr_type = 0;
    uint8_t is_pa_with_rsp = 0;
    uint8_t data_len = 0;
    uint8_t pa_data[32] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Enable_OwnAddrType_PAWR_PaDataLen_PaData, pend, &enable_pa, 1);
    if (len != 1)
    {
        return;
    }
    if (enable_pa)
    {
        len = app_ble_scan_hex_data_from_string(&Enable_OwnAddrType_PAWR_PaDataLen_PaData, pend, &own_addr_type, 1);
        if (len != 1)
        {
            return;
        }
        len = app_ble_scan_hex_data_from_string(&Enable_OwnAddrType_PAWR_PaDataLen_PaData, pend, &is_pa_with_rsp, 1);
        if (len != 1)
        {
            return;
        }
        len = app_ble_scan_hex_data_from_string(&Enable_OwnAddrType_PAWR_PaDataLen_PaData, pend, &data_len, 1);
        if (len != 1)
        {
            return;
        }
        if (data_len)
        {
            if (data_len <= sizeof(pa_data))
            {
                len = app_ble_scan_hex_data_from_string(&Enable_OwnAddrType_PAWR_PaDataLen_PaData, pend, pa_data, data_len);
                if (len != data_len)
                {
                    return;
                }
            }
        }
    }
    if (bap_big_id != BAP_INVALID_BIG_ID)
    {
        bap_terminate_big(bap_big_id);
        bap_big_id = BAP_INVALID_BIG_ID;
    }
    if (pa_adv_handle != GAP_INVALID_ADV_HANDLE)
    {
        gap_set_pa_enable(pa_adv_handle, false, false);
        pa_adv_handle = GAP_INVALID_ADV_HANDLE;
    }
    gap_disable_all_advertising(GAP_ADV_DISABLE_ALL_BY_BLE_TEST);
    if (enable_pa)
    {
        uint32_t cont_filter_legacy_conn_scan = 0;
        bool scannable = false;
        bool conectable = false;
        bool extended = true;
        bool use_filter_list = false;
        uint16_t pa_interval_1_25ms = 10;
        gap_pa_subevent_param_t subevent_param = {0};
        cont_filter_legacy_conn_scan |= scannable ? 0x01 : 0;
        cont_filter_legacy_conn_scan |= conectable ? 0x02 : 0;
        cont_filter_legacy_conn_scan |= extended ? 0 : 0x04;
        cont_filter_legacy_conn_scan |= use_filter_list ? 0x08 : 0;
        cont_filter_legacy_conn_scan |= 0x10; // continue advtising
        pa_adv_handle = gap_impl_start_advertising((bt_addr_type_t)own_addr_type, ble_test_pa_callback, cont_filter_legacy_conn_scan, NULL);
        subevent_param.num_subevents = 1;
        subevent_param.subevent_interval_1_25ms = 0x06; // 0x06 to 0xFF * 1.25ms (7.5ms to 318.75ms)
        subevent_param.response_slot_delay_1_25ms = 0x01; // 0x00 no response slots, 0xXX 0x01 to 0xFE * 1.25ms (1.25ms to 317.5ms) time between the adv packet in a subevent and the 1st response slot
        subevent_param.response_slot_spacing_0_125ms = 10; // 0x00 no response slots, 0xXX 0x02 to 0xFF * 0x125ms (0x25ms to 31.875ms) time between response slots
        subevent_param.num_response_slots = 0x01; // 0x00 no response slots, 0xXX 0x01 to 0xFF num of subevent response slots
        gap_set_pa_parameters(pa_adv_handle, pa_interval_1_25ms, true, is_pa_with_rsp ? &subevent_param : NULL);
        if (data_len)
        {
            gap_set_pa_data(pa_adv_handle, GAP_ADV_DATA_COMPLETE, pa_data, data_len);
        }
        gap_set_pa_enable(pa_adv_handle, true, true);
        gap_enable_advertising(pa_adv_handle, 0);
    }
}

static void ble_test_adv_set_past(void)
{
    gap_advertiser_start_pawr_initiating(pa_adv_handle, 0x00, BT_ADDR_TYPE_PUBLIC, (bt_bdaddr_t *)(g_test_le_pts_addr+1));
}

static int ble_test_scan_callback(uintptr_t scan, gap_scan_event_t event, gap_scan_callback_param_t param)
{
    static uint8_t broadcast_code[16] = {0};
    static bool pa_sync_established = false;
    static bool create_big_sync_pending = false;
    if (event == GAP_SCAN_EVENT_ADV_REPORT)
    {
        const gap_adv_report_t *adv_report = param.adv_report;
        if (adv_report->adv_set_id != GAP_INVALID_ADV_SET_ID && adv_report->peer_type == g_test_le_pts_addr[0] &&
            memcmp(&adv_report->peer_addr, g_test_le_pts_addr+1, sizeof(bt_bdaddr_t)) == 0)
        {
            TRACE(0, "sync_test_callback adv report: adv_sid %d peer addr %d %04x bc %d %p", adv_report->adv_set_id, adv_report->peer_type,
                CO_COMBINE_UINT16_BE(adv_report->peer_addr.address), adv_report->parsed.big_info_data_len, adv_report->parsed.broadcast_code);
            if (!pa_sync_established)
            {
                gap_pa_sync_param_t param;
                param.options = HCI_LE_PA_DUPLICATE_FILTER_INITIAL_ENABLE;
                param.adv_set_id = adv_report->adv_set_id;
                param.adv_addr_type = adv_report->peer_type;
                param.adv_addr = adv_report->peer_addr;
                param.skip = 0x0000;
                param.sync_timeout_10ms = 1000;
                param.sync_cte_type = 0x00;
                if (adv_report->parsed.broadcast_code)
                {
                    memcpy(broadcast_code, adv_report->parsed.broadcast_code, sizeof(broadcast_code));
                }
                gap_pa_create_sync(&param, ble_test_scan_callback);
            }
        }
    }
    else if (event == GAP_SCAN_EVENT_PA_SYNC_ESTABLISHED)
    {
        const gap_pa_sync_establish_t *pa_sync = param.pa_sync_estb;
        pa_sync_established = true;
        create_big_sync_pending = false;
        TRACE(0, "sync_test_callback pa established: %02x sync_handle %04x service_data %04x num_subevents %d",
            pa_sync->error_code, pa_sync->pa_sync_hdl, pa_sync->service_data, pa_sync->num_subevents);
    }
    else if (event == GAP_SCAN_EVENT_PA_SYNC_TERMINATED)
    {
        TRACE(0, "sync_test_callback pa terminated");
        pa_sync_established = false;
    }
    else if (event == GAP_SCAN_EVENT_PA_REPORT)
    {
        const gap_adv_report_t *pa_report = param.per_adv_report;
        TRACE(0, "sync_test_callback pa report: sync_handle %04x adv_sid %d peer addr %d %04x bc %d %p",
            pa_report->sync_handle, pa_report->adv_set_id, pa_report->peer_type,
            CO_COMBINE_UINT16_BE(pa_report->peer_addr.address), pa_report->parsed.big_info_data_len, pa_report->parsed.broadcast_code);
        if (pa_report->parsed.broadcast_code)
        {
            memcpy(broadcast_code, pa_report->parsed.broadcast_code, sizeof(broadcast_code));
        }
    }
    else if (event == GAP_SCAN_EVENT_BIG_INFO_REPORT)
    {
        const gap_big_info_adv_report_t *big_info = param.big_info_report;
        TRACE(0, "sync_test_callback big info: sync_handle %04x num_bis %d nse %d encrypted %d",
            big_info->sync_handle, big_info->num_bis, big_info->nse, big_info->encrypted);
        if (big_info->num_bis && !create_big_sync_pending)
        {
            bap_big_sync_param_t big_param = {0};
            big_param.bis_count = 0x01; // 0x01 to 0x1F, total number of BISes to synchronize
            big_param.bis_index[0] = 0x01; // 0x01 to 0x1F, index of a BIS in the BIG
            big_param.big_sync_timeout = 1000; // 0x0A to 0x4000, per 10ms, 100ms to 163.84s, sync timeout for the BIG
            big_param.encryption = big_info->encrypted; // 0x00 unencrypted, 0x01 encrypted
            big_param.mse = 0x00; // 0x00 controler can schedule reception of any num of se up to NSE, 0x01 to 0x1F max num of se should be used
            if (big_info->encrypted)
            {
                memcpy(big_param.broadcast_code, broadcast_code, 16); // used for deriving the session key for decrypting payloads of BISes in the BIG
            }
            create_big_sync_pending = true;
            bap_create_big_sync(bap_test_event_callback, big_info->sync_handle, &big_param);
        }
    }
    return 0;
}

static void ble_test_create_big_sync(const char *CancelSync, uint32_t str_len)
{
    const char *pend = CancelSync + str_len;
    uint8_t cancel_sync = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&CancelSync, pend, &cancel_sync, 1);
    if (len != 1)
    {
        return;
    }
    if (bap_big_id != BAP_INVALID_BIG_ID)
    {
        bap_terminate_big(bap_big_id);
        bap_big_id = BAP_INVALID_BIG_ID;
    }
    if (pa_sync_handle != GAP_INVALID_CONN_HANDLE)
    {
        gap_pa_terminate_sync(pa_sync_handle);
        pa_sync_handle = GAP_INVALID_CONN_HANDLE;
    }
    gap_pa_create_sync_cancel();
    gap_disable_scanning();
    if (!cancel_sync)
    {
        gap_scan_param_t param;
        memset(&param, 0, sizeof(gap_scan_param_t));
        param.filter_duplicated = true;
        gap_start_scanning(&param, ble_test_scan_callback);
    }
}
#endif /* BLE_PA_SUPPORT */

#if BLE_GAP_CENTRAL_ROLE
static void ble_test_scanning(const char *OwnAddrType_ActiveScan_Filter_ContinueScan, uint32_t str_len)
{
    const char *pend = OwnAddrType_ActiveScan_Filter_ContinueScan + str_len;
    uint8_t own_addr_type = 0;
    uint8_t active_scan = 0;
    uint8_t use_filter_list = 0;
    uint8_t continue_scan = 0;
    gap_scan_param_t param = {(bt_addr_type_t)0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_ActiveScan_Filter_ContinueScan, pend, &own_addr_type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_ActiveScan_Filter_ContinueScan, pend, &active_scan, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_ActiveScan_Filter_ContinueScan, pend, &use_filter_list, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_ActiveScan_Filter_ContinueScan, pend, &continue_scan, 1);
    if (len != 1)
    {
        return;
    }
    param.own_addr_type = (bt_addr_type_t)own_addr_type;
    param.use_filter_list = use_filter_list ? true : false;
    param.active_scan = active_scan ? true : false;
    param.continue_scanning = continue_scan ? true : false;
    gap_start_scanning(&param, NULL);
}

static void ble_test_initiating(const char *OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, uint32_t str_len)
{
    const char *pend = OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr + str_len;
    gap_init_param_t param = {(bt_addr_type_t)0};
    uint8_t own_addr_type = 0;
    uint8_t direct = 0;
    uint8_t has_peer_addr = 0;
    uint8_t peer_type = 0;
    uint8_t peer_addr[6] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, pend, &own_addr_type, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, pend, &direct, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, pend, &has_peer_addr, 1);
    if (len != 1)
    {
        return;
    }
    if (has_peer_addr)
    {
        len = app_ble_scan_hex_data_from_string(&OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, pend, &peer_type, 1);
        if (len != 1)
        {
            return;
        }
        len = app_ble_scan_hex_data_from_string(&OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr, pend, peer_addr, 6);
        if (len != 6)
        {
            return;
        }
    }
    param.own_addr_type = (bt_addr_type_t)own_addr_type;
    param.use_filter_list = direct ? false : true;
    if (direct)
    {
        if (has_peer_addr)
        {
            param.peer_type = (bt_addr_type_t)peer_type;
            param.peer_addr = *((bt_bdaddr_t *)peer_addr);
        }
        else
        {
            param.peer_type = (bt_addr_type_t)g_test_le_pts_addr[0];
            param.peer_addr = *((bt_bdaddr_t *)(g_test_le_pts_addr+1));
        }
    }
    gap_start_initiating(&param, NULL);
}

static void ble_test_update_conn_parameters(const char *Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, uint32_t str_len)
{
    const char *pend = Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout + str_len;
    gap_update_params_t param = {0};
    uint8_t conidx = 0;
    uint8_t min_interval[2] = {0};
    uint8_t max_interval[2] = {0};
    uint8_t max_latency[2] = {0};
    uint8_t superv_timeout[2] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, pend, min_interval, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, pend, max_interval, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, pend, max_latency, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout, pend, superv_timeout, 2);
    if (len != 2)
    {
        return;
    }
    param.conn_interval_min_1_25ms = CO_COMBINE_UINT16_BE(min_interval);
    param.conn_interval_max_1_25ms = CO_COMBINE_UINT16_BE(max_interval);
    param.max_peripheral_latency = CO_COMBINE_UINT16_BE(max_latency);
    param.superv_timeout_ms = CO_COMBINE_UINT16_BE(superv_timeout) * 10;
    gap_update_le_conn_parameters(gap_zero_based_ble_conidx_as_hdl(conidx), &param);
}

static void ble_test_start_scanning(void)
{
    gap_start_scanning(NULL, NULL);
}

static void ble_test_stop_scanning(void)
{
    gap_disable_scanning();
}

static void ble_test_direct_connect(void)
{
    bt_addr_type_t peer_type;
    const bt_bdaddr_t *peer_addr = NULL;
    peer_addr = app_test_get_le_address(&peer_type);
    gap_start_direct_initiating(BT_ADDR_TYPE_PUBLIC, peer_type, peer_addr, NULL);
}
#endif

static void ble_test_disconnect_all(void)
{
    gap_terminate_all_ble_connection();
}

static void ble_test_set_pts_mode(void)
{
#ifndef BLE_ONLY_ENABLED
    bt_test_set_pts_mode();
#endif
    gap_pts_set_dont_start_smp(true);
    gap_set_send_sec_error_rsp_directly(true);
}

static void app_ble_set_l2cap_test(const char *L2capTest, uint32_t str_len)
{
    const char *pend = L2capTest + str_len;
    uint8_t l2cap_test = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&L2capTest, pend, &l2cap_test, 1);
    if (len != 1)
    {
        return;
    }
    gap_pts_set_ble_l2cap_test(l2cap_test);
}

static void ble_test_set_min_conn_interval(const char *MinConnInterval_1_25ms, uint32_t str_len)
{
    const char *pend = MinConnInterval_1_25ms + str_len;
    uint8_t min_conn_interval_1_25ms = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&MinConnInterval_1_25ms, pend, &min_conn_interval_1_25ms, 1);
    if (len != 1)
    {
        return;
    }
    gap_set_min_conn_interval(min_conn_interval_1_25ms);
}

#if BLE_GATT_CLIENT_SUPPORT
static void gattc_test_create_eatt(const char *Conidx, uint32_t str_len)
{
    const char *pend = Conidx + str_len;
    uint8_t conidx = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    gatt_create_eatt_bearer(gap_zero_based_ble_conidx_as_hdl(conidx));
}

static void gattc_test_disconnect_eatt(const char *Conidx, uint32_t str_len)
{
    const char *pend = Conidx + str_len;
    uint8_t conidx = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    gatt_disconnect_eatt_bearers(gap_zero_based_ble_conidx_as_hdl(conidx), true);
}

static void gattc_test_signed_write(const char *Conidx_AttrHandle_Len_Data, uint32_t str_len)
{
    const char *pend = Conidx_AttrHandle_Len_Data + str_len;
    gatt_prf_t prf = {0};
    uint8_t conidx = 0;
    uint8_t attr_handle[2] = {0};
    uint8_t data_len = 0;
    uint8_t data[32] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Len_Data, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Len_Data, pend, attr_handle, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Len_Data, pend, &data_len, 1);
    if (len != 1)
    {
        return;
    }
    if (data_len)
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Len_Data, pend, data, data_len);
        if (len != data_len)
        {
            return;
        }
        prf.prf_id = GATT_PRF_NONE;
        prf.con_idx = gap_zero_based_conidx_to_ble_conidx(conidx);
        prf.connhdl = gap_zero_based_ble_conidx_as_hdl(conidx);
        gattc_send_signed_write_cmd(&prf, CO_COMBINE_UINT16_BE(attr_handle), data, data_len);
    }
}

static void gattc_test_read_request(const char *Conidx_AttrHandle_Char00Desc01_EATT, uint32_t str_len)
{
    const char *pend = Conidx_AttrHandle_Char00Desc01_EATT + str_len;
    gatt_prf_t prf = {0};
    uint8_t conidx = 0;
    uint8_t attr_handle[2] = {0};
    uint8_t read_desc = 0;
    uint8_t use_eatt = 0;
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_EATT, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_EATT, pend, attr_handle, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_EATT, pend, &read_desc, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_EATT, pend, &use_eatt, 1);
    if (len != 1)
    {
        return;
    }
    prf.prf_id = use_eatt ? GATT_PRF_LAST_ID : GATT_PRF_NONE;
    prf.con_idx = gap_zero_based_conidx_to_ble_conidx(conidx);
    prf.connhdl = gap_zero_based_ble_conidx_as_hdl(conidx);
    if (read_desc)
    {
        gattc_read_descriptor_by_handle(&prf, CO_COMBINE_UINT16_BE(attr_handle), true);
    }
    else
    {
        gattc_read_character_by_handle(&prf, CO_COMBINE_UINT16_BE(attr_handle), true);
    }
}

static void gattc_test_multi_read(const char *Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, uint32_t str_len)
{
    const char *pend = Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens + str_len;
    uint8_t conidx = 0;
    uint8_t count = 0;
    uint8_t attr_handles[2*3] = {0};
    uint8_t is_multi_var = 0;
    uint8_t has_value_lens = 0;
    uint8_t value_lens[3] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, &count, 1);
    if (len != 1)
    {
        return;
    }
    if (count && count <= (sizeof(attr_handles) / 2))
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, attr_handles, count * 2);
        if (len != count * 2)
        {
            return;
        }
    }
    else
    {
        count = 0;
    }
    if (count)
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, &is_multi_var, 1);
        if (len != 1)
        {
            return;
        }
        len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, &has_value_lens, 1);
        if (len != 1)
        {
            return;
        }
    }
    if (has_value_lens)
    {
        len = app_ble_scan_hex_data_from_string(&Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens, pend, value_lens, count);
        if (len != count)
        {
            return;
        }
    }
    if (count)
    {
        gatt_prf_t prf = {0};
        uint16_t handles[3];
        uint16_t lens[3];
        prf.prf_id = GATT_PRF_NONE;
        prf.con_idx = gap_zero_based_conidx_to_ble_conidx(conidx);
        prf.connhdl = gap_zero_based_ble_conidx_as_hdl(conidx);
        for (int i = 0; i < count; i += 1)
        {
            handles[i] = CO_COMBINE_UINT16_BE(attr_handles + 2 * i);
            if (has_value_lens)
            {
                lens[i] = value_lens[i];
            }
        }
        gattc_read_multi_char_values(&prf, is_multi_var ? true : false, count, handles, has_value_lens ? lens : NULL);
    }
}

static void gattc_test_write_request(const char *Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, uint32_t str_len)
{
    const char *pend = Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data + str_len;
    gatt_prf_t prf = {0};
    uint8_t conidx = 0;
    uint8_t attr_handle[2] = {0};
    uint8_t write_desc = 0;
    uint8_t write_cmd = 0;
    uint8_t data_len = 0;
    uint8_t data[64] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, attr_handle, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, &write_desc, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, &write_cmd, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, &data_len, 1);
    if (len != 1)
    {
        return;
    }
    if (data_len)
    {
        if (data_len <= 16)
        {
            len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data, pend, data, data_len);
            if (len != data_len)
            {
                return;
            }
        }
    }
    prf.prf_id = GATT_PRF_NONE;
    prf.con_idx = gap_zero_based_conidx_to_ble_conidx(conidx);
    prf.connhdl = gap_zero_based_ble_conidx_as_hdl(conidx);
    if (write_desc)
    {
        gattc_write_descriptor_by_handle(&prf, CO_COMBINE_UINT16_BE(attr_handle), data, data_len);
    }
    else
    {
        gattc_write_character_by_handle(&prf, CO_COMBINE_UINT16_BE(attr_handle), data, data_len, write_cmd);
    }
}

static void gattc_test_prepare_write(const char *Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, uint32_t str_len)
{
    const char *pend = Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data + str_len;
    gatt_prf_t prf = {0};
    uint8_t conidx = 0;
    uint8_t attr_handle[2] = {0};
    uint8_t write_execute = 0;
    uint8_t cancel_or_offset = 0;
    uint8_t data_len = 0;
    uint8_t data[64] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, attr_handle, 2);
    if (len != 2)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, &write_execute, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, &cancel_or_offset, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, &data_len, 1);
    if (len != 1)
    {
        return;
    }
    if (data_len)
    {
        if (data_len <= 16)
        {
            len = app_ble_scan_hex_data_from_string(&Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data, pend, data, data_len);
            if (len != data_len)
            {
                return;
            }
        }
    }
    prf.prf_id = GATT_PRF_NONE;
    prf.con_idx = gap_zero_based_conidx_to_ble_conidx(conidx);
    prf.connhdl = gap_zero_based_ble_conidx_as_hdl(conidx);
    if (write_execute)
    {
        gattc_prepare_write_execute(&prf, cancel_or_offset ? false : true);
    }
    else
    {
        gattc_prepare_write_by_handle(&prf, CO_COMBINE_UINT16_BE(attr_handle), cancel_or_offset, data, data_len);
    }
}

static void gattc_test_send_data(const char *Conidx_Eatt_DataLen_Data, uint32_t str_len)
{
    const char *pend = Conidx_Eatt_DataLen_Data + str_len;
    uint8_t conidx = 0;
    uint8_t eatt_preferred = 0;
    uint8_t data_len = 0;
    uint8_t data[16] = {0};
    int len = 0;
    len = app_ble_scan_hex_data_from_string(&Conidx_Eatt_DataLen_Data, pend, &conidx, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_Eatt_DataLen_Data, pend, &eatt_preferred, 1);
    if (len != 1)
    {
        return;
    }
    len = app_ble_scan_hex_data_from_string(&Conidx_Eatt_DataLen_Data, pend, &data_len, 1);
    if (len != 1)
    {
        return;
    }
    if (data_len)
    {
        if (data_len <= sizeof(data))
        {
            len = app_ble_scan_hex_data_from_string(&Conidx_Eatt_DataLen_Data, pend, data, data_len);
            if (len != data_len)
            {
                return;
            }
        }
        gattc_send_l2cap_packet(gap_zero_based_ble_conidx_as_hdl(conidx), data, data_len, eatt_preferred);
    }
}
#endif

#if (BLE_AUDIO_ENABLED)
static void bap_test_start_advertising(void)
{
    aob_conn_start_adv(true, true);
}

static void bap_test_stop_advertising(void)
{
    aob_conn_stop_adv();
}

#ifdef AOB_MOBILE_ENABLED
static void aob_mobile_scan_start_handler(void)
{
    if (ble_audio_is_ux_mobile())
    {
        app_bap_set_activity_type(GAF_BAP_ACT_TYPE_CIS_AUDIO);
        BLE_SCAN_PARAM_T scan_param = {0};

        scan_param.scanFolicyType = BLE_DEFAULT_SCAN_POLICY;
        scan_param.scanWindowMs   = 20;
        scan_param.scanIntervalMs = 50;
        app_ble_start_scan(&scan_param);
    }
}

static void bap_test_start_reconnection(void)
{
    bt_addr_type_t peer_type;
    const bt_bdaddr_t *peer_addr = NULL;
    peer_addr = app_test_get_le_address(&peer_type);
    bap_start_direct_connection(BT_ADDR_TYPE_PUBLIC, peer_type, peer_addr);
}

static void aob_media_start_test_handler(uint16_t frame_octet)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);

    ble_audio_test_start_unidirectional_stream(APP_GAF_DIRECTION_SINK,
                            APP_GAF_BAP_SAMPLING_FREQ_48000HZ, frame_octet,
                            BES_BLE_GAF_CONTEXT_TYPE_MEDIA_BIT);
}

static void aob_call_start_test_handler(uint8_t sample_rate_sink, uint8_t sample_rate_src,
                                                    uint16_t frame_octet_sink, uint16_t frame_octet_src)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);

    ble_audio_test_start_bidirectional_stream(sample_rate_sink, sample_rate_src,
                                                    frame_octet_sink, frame_octet_src,
                                                    BES_BLE_GAF_CONTEXT_TYPE_CONVERSATIONAL_BIT);
}

static void aob_media_start_test(void)
{
    aob_media_start_test_handler(155);
}

static void aob_call_start_test(void)
{
    aob_call_start_test_handler(APP_GAF_BAP_SAMPLING_FREQ_48000HZ, APP_GAF_BAP_SAMPLING_FREQ_32000HZ, 120, 80);
}

/// company with aob_media_start_test
static void aob_media_rejoin_media_test(void)
{
    // Use dft cis timing @see dft_cis_timing
    ble_audio_test_update_cis_timing(dft_cis_timing);
#ifdef AOB_LOW_LATENCY_MODE
    ble_audio_test_update_sdu_intv(5000, 5000);
#else
    // Use 10ms sdu interval
    ble_audio_test_update_sdu_intv(10000, 10000);
#endif
    AOB_MEDIA_ASE_CFG_INFO_T ase_to_start =
    {
        APP_GAF_BAP_SAMPLING_FREQ_48000HZ, 155, APP_GAF_DIRECTION_SINK, AOB_CODEC_ID_LC3, APP_GAF_BAP_CONTEXT_TYPE_MEDIA
    };

    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        /// if can not get streaming ase, means conn is a rejoin dev conn
        if (GAF_INVALID_ANY_LID == aob_media_mobile_get_cur_streaming_ase_lid(i, AOB_MGR_DIRECTION_SINK))
        {
            aob_media_mobile_start_stream(&ase_to_start, i, false);
        }
    }
}

static void aob_media_release_test(void)
{
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_release_all_stream_for_specifc_direction(i, APP_GAF_DIRECTION_SINK);
        aob_media_mobile_release_all_stream_for_specifc_direction(i, APP_GAF_DIRECTION_SRC);
    }
}

static void aob_media_disable_test(void)
{
    for (uint8_t i = 0; i < BLE_AUDIO_CONNECTION_CNT; i++)
    {
        aob_media_mobile_disable_all_stream_for_specifc_direction(i, APP_GAF_DIRECTION_SINK);
        aob_media_mobile_disable_all_stream_for_specifc_direction(i, APP_GAF_DIRECTION_SRC);
    }
}
#endif /// AOB_MOBILE_ENABLED

static void aob_acc_mcc_discovery_test(void)
{
    aob_media_mcs_discovery(0);
}

static void aob_acc_tbc_discovery_test(void)
{
    aob_call_tbs_discovery(0);
}

static void aob_acc_mcc_play_test(void)
{
    aob_media_play(0);
}

static void aob_acc_mcc_pause_test(void)
{
    aob_media_pause(0);
}

static void aob_ble_audio_init_test(void)
{
    bes_ble_audio_common_init();
}

static void aob_ble_audio_deinit_test(void)
{
    bes_ble_audio_common_deinit();
}

static void mcp_test_mcc_control(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[6] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mcc_control(0, param[0], (AOB_MGR_MC_OPCODE_E)param[1], *(uint32_t *)&param[2]);
}

static void mcp_test_mcc_set_val(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[6] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mcc_set_val(0, param[0], param[1], *(uint32_t *)&param[2]);
}

static void mcp_test_mcc_set_obj_id(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[8] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_mcc_set_obj_id(0, param[0], param[1], &param[2]);
}

static void mcp_test_mcc_search_item(const char *cmdParam, uint32_t cmdParam_len)
{
    uint8_t param[20] = {0};

    _str_to_hex(cmdParam, param, sizeof(param));

    aob_media_search_player(0, param[0], param[1], &param[2]);
}
#endif /// (BLE_AUDIO_ENABLED)

static const app_ble_test_handler_t g_app_ble_test_cmd_table[] =
{
    // ble device
    {"ble_del_record",              ble_test_del_record},
    {"ble_del_all_record",          ble_test_del_all_record},
    {"ble_dump_record",             ble_test_dump_record},
    {"bt_del_record",               bt_record_del_test},
    {"bt_del_all_record",           bt_record_del_all_test},
    {"bt_dump_record",              bt_record_dump_test},
    {"bt_nv_flush",                 bt_nv_flush_test},
    {"bt_shutdown",                 bt_shutdown_test},
    {"bt_trigger_assert",           bt_trigger_assert_test},
    {"ble_disconnect",              ble_test_disconnect_all},
    {"ble_dump_status",             gap_dump_ble_status},
    {"ble_set_pts_mode",            ble_test_set_pts_mode},
#ifndef BLE_ONLY_ENABLED
    {"bt_cancel_inq",               bt_test_cancel_inquiry},
    {"bt_set_both_scan",            bt_set_both_scan_test},
    {"bt_disable_scan",             bt_disable_scan_test},
    {"bt_reconn_hfp",               bt_reconnect_hfp_test},
    {"bt_reconn_a2dp",              bt_reconnect_a2dp_test},
    {"bt_reconnect",                bt_reconnect_test},
    {"bt_disconnect",               bt_disconnect_test},
    {"bt_start_auth",               bt_start_auth_test},
    {"bt_set_pts_mode",             bt_test_set_pts_mode},
#endif
    {"ble_local_irk",               ble_test_print_local_irk},
    {"ble_resl_size",               ble_test_resolving_list_size},
    {"ble_resl_clear",              ble_test_resolving_list_clear},
    {"ble_resl_dump",               ble_test_resolving_list_dump},
    {"ble_resl_network_mode",       ble_test_resolving_list_set_network_mode},
    {"ble_resl_device_mode",        ble_test_resolving_list_set_device_mode},
    {"ble_resl_set_bonded_devices", ble_test_resolving_list_set_bonded_devices},
    {"ble_filter_size",             ble_test_filter_list_size},
    {"ble_filter_clear",            ble_test_filter_list_clear},
    {"ble_filter_dump",             ble_test_filter_list_dump},
    {"ble_filter_add_anonymous",    ble_test_filter_list_add_anonymous},
    {"ble_filter_del_anonymous",    ble_test_filter_list_remove_anonymous},
    {"ble_filter_set_bonded_devices", ble_test_filter_list_set_bonded_devices},

    // advertising
    {"ble_legacy_none_adv",         ble_test_legacy_non_discoverable_adv},
    {"ble_legacy_none_conn_adv",    ble_test_legacy_non_discoverable_connectable_adv},
    {"ble_legacy_conn_adv",         ble_test_legacy_connectable_adv},
    {"ble_legacy_scan_adv",         ble_test_legacy_scannable_adv},
    {"ble_none_adv",                ble_test_non_discoverable_adv},
    {"ble_none_conn_adv",           ble_test_non_discoverable_connectable_adv},
    {"ble_conn_adv",                ble_test_connectable_adv},
    {"ble_scan_adv",                ble_test_scannable_adv},
    {"ble_close_all_adv",           ble_test_close_all_adv},

#if BLE_GAP_CENTRAL_ROLE
    {"open_scan",                   ble_test_start_scanning},
    {"close_scan",                  ble_test_stop_scanning},
    {"ble_connect",                 ble_test_direct_connect},
#endif
#if BLE_PA_SUPPORT
    {"ble_adv_set_past",            ble_test_adv_set_past},
#endif

    // smp
    {"smp_cmac_0",                  smp_test_ase_cmac_0},
    {"smp_cmac_16",                 smp_test_ase_cmac_16},
    {"smp_cmac_40",                 smp_test_ase_cmac_40},
    {"smp_cmac_64",                 smp_test_ase_cmac_64},
    {"smp_test_f4",                 smp_test_f4},
    {"smp_test_f5",                 smp_test_f5},
    {"smp_test_f5_key_T",           smp_test_f5_key_T},
    {"smp_test_f5_gen_key",         smp_test_f5_gen_key},
    {"smp_test_f6",                 smp_test_f6},
    {"smp_test_g2",                 smp_test_g2},
    {"smp_test_ah",                 smp_test_ah},
    {"smp_test_h6",                 smp_test_h6},
    {"smp_test_h7",                 smp_test_h7},
    {"smp_test_lirk",               smp_test_lirk},
    {"smp_test_lcsrk",              smp_test_lcsrk},
    {"smp_test_ltk_to_lk",          smp_test_ltk_to_lk},
    {"smp_test_lk_to_ltk",          smp_test_lk_to_ltk},
    {"smp_set_use_passkey",         smp_test_set_use_passkey},
    {"smp_set_use_oob",             smp_test_set_use_oob},
    {"smp_set_no_mitm",             smp_test_set_no_mitm_auth},
    {"smp_set_display_only",        smp_test_set_display_only},
    {"smp_set_keyboard_only",       smp_test_set_keyboard_only},
    {"smp_set_no_bonding",          smp_test_set_no_bonding},
    {"smp_gen_link_key",            smp_test_gen_linkkey_from_ltk},
    {"smp_set_dist_irk_only",       smp_test_set_dist_irk_only},
    {"smp_set_dist_csrk",           smp_test_set_dist_csrk},
    {"smp_ctkd_set_ble_addr",       smp_test_ctkd_set_ble_public_address},
    {"smp_ctkd_set_bt_addr",        smp_test_ctkd_set_bt_public_address},

    // gatt
    {"gatts_change_service",        gatts_test_change_service},
#if BLE_GATT_CLIENT_SUPPORT
    {"gattc_add_test_profile",      gattc_test_add_profile},
#endif

#if (BLE_AUDIO_ENABLED)
#ifdef AOB_MOBILE_ENABLED
    {"uc_mobile_scan_start",        aob_mobile_scan_start_handler}, /// BAP Unicast client
    {"uc_mobile_start_reconnect",   bap_test_start_reconnection},
    // UC CLI
    {"uc_media_start",              aob_media_start_test},
    {"uc_media_rejoin",             aob_media_rejoin_media_test},
    {"uc_media_disable",            aob_media_disable_test},
    {"uc_media_release",            aob_media_release_test},
    {"uc_call_stream_start",        aob_call_start_test},
    {"uc_call_stream_disable",      aob_media_disable_test},
    // ASSISTANT
    {"bc_assist_remove_src",        bap_test_start_assist_remove_source},
    // MICP
    {"micp_read_mic_mute",          micp_test_read_mic_mute},

#ifdef APP_BLE_BIS_SRC_ENABLE
    {"bc_src_stop_grp",             bap_test_stop_bis_src_grp},
    {"bc_src_start_stream",         bap_test_start_bis_src_streaming},
    {"bc_src_stop_stream",          bap_test_stop_bis_src_streaming},
#endif /// APP_BLE_BIS_SRC_ENABLE

#endif /// AOB_MOBILE_ENABLED

    {"acc_mcc_discovery",           aob_acc_mcc_discovery_test},
    {"acc_tbc_discovery",           aob_acc_tbc_discovery_test},
    {"acc_mcc_play",                aob_acc_mcc_play_test},
    {"acc_mcc_pause",               aob_acc_mcc_pause_test},

    {"start_adv",                   bap_test_start_advertising},
    {"close_adv",                   bap_test_stop_advertising},

    {"aob_init",                    aob_ble_audio_init_test},
    {"aob_deinit",                  aob_ble_audio_deinit_test},
#endif /// (BLE_AUDIO_ENABLED)
};

static const app_ble_test_with_param_handler_t g_app_ble_test_cmd_with_param_table[] =
{
    // ble device
    {"ble_del_index_record",        ble_test_del_index_record},                 // [BLE,ble_del_index_record|01]
    {"bt_del_index_record",         bt_record_del_by_index_test},               // [BLE,bt_del_index_record|01]
    {"set_local_addr",              app_test_set_local_addr},                   // [BLE,set_local_addr|BT00LE01_BdAddrLsb2Msb]
    {"set_peer_le_addr",            app_test_set_le_address},                   // [BLE,set_peer_le_addr|Type_BdAddrLsb2Msb]
    {"set_peer_bt_addr",            app_test_set_bt_address},                   // [BLE,set_peer_bt_addr|BdAddrLsb2Msb]
    {"ble_set_rpa_timeout",         ble_test_resolving_list_set_rpa_timeout},   // [BLE,ble_set_rpa_timeout|0020]
    {"ble_resl_del_by_index",       ble_test_resolving_list_remove_by_index},   // [BLE,ble_resl_del_by_index|00]
    {"ble_resl_del_by_addr",        ble_test_resolving_list_remove_by_address}, // [BLE,ble_resl_del_by_addr|Type_BdAddrLsb2Msb]
    {"ble_resl_add",                ble_test_resolving_list_add},               // [BLE,ble_resl_add|Type_BdAddrLsb2Msb_DeviceMode]
    {"ble_enable_addr_reso",        ble_test_ble_enable_addr_reso},             // [BLE,ble_enable_addr_reso|01]
    {"ble_filter_del_by_index",     ble_test_filter_list_remove_by_index},      // [BLE,ble_filter_del_by_index|00]
    {"ble_filter_del_by_addr",      ble_test_filter_list_remove_by_address},    // [BLE,ble_filter_del_by_addr|Type_BdAddrLsb2Msb]
    {"ble_filter_add",              ble_test_filter_list_add},                  // [BLE,ble_filter_add|Type_BdAddrLsb2Msb]
    {"ble_disc_conidx",             ble_test_disc_conidx},                      // [BLE,ble_disc_conidx|00]
    {"ble_set_l2cap_test",          app_ble_set_l2cap_test},                    // [BLE,ble_set_l2cap_test|01]
    {"ble_min_conn_interval",       ble_test_set_min_conn_interval},            // [BLE,ble_min_conn_interval|0A]
#ifndef BLE_ONLY_ENABLED
    {"bt_start_inq",                bt_test_start_inquiry},                     // [BLE,bt_start_inq|00]
    {"bt_start_scan",               bt_test_start_scan},                        // [BLE,bt_start_scan|13]
    {"bt_general_bonding",          bt_test_general_bonding},                   // [BLE,bt_general_bonding|01]
    {"bt_resp_trigger_auth",        bt_test_responder_trigger_auth},            // [BLE,bt_resp_trigger_auth|01]
    {"bt_min_enc_key_size",         bt_test_set_min_enc_key_size},              // [BLE,bt_min_enc_key_size|07]
    {"l2cap_close_or_send",         bt_l2cap_close_or_send_data},               // [BLE,l2cap_close_or_send|00004000] Conidx_Dcid_DataLen_SendEcho_Data
#endif

    // advertising
    // adv data:1B0201040503001801180D095054532D4741502D303642380319000000
    // [AOB,ble_start_advertising|00000000000000000000] legacy non-discoverale adv
    // [AOB,ble_start_advertising|000000000000001B0201040503001801180D095054532D4741502D30364238031900000000] legacy non-disc with adv data
    // [AOB,ble_start_advertising|000100000000001B0201040503001801180D095054532D4741502D3036423803190000000001A1A2A3A4A536] legacy adv with non-rpa
    // [AOB,ble_start_advertising|000001010100001B0201040503001801180D095054532D4741502D30364238031900000000] legacy direct adv
    {"ble_start_advertising",       ble_test_start_advertising},        // Extended_OwnAddrType_Conn_Scan_Direct_HasPeerAddr_PeerType_PeerAddr_Filter_AdvLen_AdvData_RspLen_RspData_HasCustomAddr_LocalAddr
    {"ble_set_adv_data",            ble_test_set_adv_data},             // AdvHandle_AdvData00RspData01_DataLen_Data
#if BLE_GAP_CENTRAL_ROLE
    {"ble_start_scanning",          ble_test_scanning},                 // OwnAddrType_ActiveScan_Filter_ContinueScan
    {"ble_start_initiating",        ble_test_initiating},               // OwnAddrType_Direct_HasPeerAddr_PeerType_PeerAddr
    {"ble_update_conn_param",       ble_test_update_conn_parameters},   // Conidx_MinInterval_MaxInterval_MaxLatency_SupervTimeout
#endif
#if BLE_PA_SUPPORT
    {"ble_start_pa",                ble_test_start_pa},                 // Enable_OwnAddrType_PAWR_PaDataLen_PaData
    {"ble_create_big_sync",         ble_test_create_big_sync},          // [BLE,ble_create_big_sync|00] CancelSync
#endif

    // smp
    {"smp_test_e",                  smp_test_encrypt_data},             // [BLE,smp_test_e|Key_Plain]
    {"smp_test_xor",                smp_test_xor},                      // [BLE,smp_test_xor|Ka_Kb]
    {"smp_test_shift",              smp_test_shift},                    // [BLE,smp_test_shift|K]
    {"smp_test_cmac",               smp_test_cmac},                     // [BLE,smp_test_cmac|K_m]
    {"smp_f4_numeric",              smp_test_f4_numeric},               // [BLE,smp_f4_numeric|PKbx_PKax_Rb]
    {"smp_f5_gen_ltk",              smp_test_f5_gen_ltk},               // [BLE,smp_f5_gen_ltk|DHKey_Ra_Rb_A_B]
    {"smp_f5_step",                 smp_test_f5_step},                  // [BLE,smp_f5_step|M]
    {"smp_f6_dhkey_check",          smp_test_f6_dhkey_check},           // [BLE,smp_f6_dhkey_check|MacKey_Ra_Rb_rb_IOCAP_OOBFlag_AuthReq_A_B]
    {"smp_g2_user_check_value",     smp_test_g2_user_confirm_value},    // [BLE,smp_g2_user_check_value|PKax_PKbx_Ra_Rb]
    {"smp_ah_irk_prand",            smp_test_ah_irk_prand},             // [BLE,smp_ah_irk_prand|IRK_Prand]
    {"smp_lk_to_ltk",               smp_test_linkkey_to_ltk},           // [BLE,smp_lk_to_ltk|ct2_linkkey]
    {"smp_ltk_to_lk",               smp_test_ltk_to_linkkey},           // [BLE,smp_ltk_to_lk|ct2_ltk]
    {"smp_input_tk",                smp_test_input_oob_legacy_tk},      // [BLE,smp_input_tk|tk]
    {"smp_input_passkey",           smp_test_input_6_digit_passkey},    // [BLE,smp_input_passkey|6-digit-passkey]
    {"smp_start_auth",              smp_test_start_authentication},     // [BLE,smp_start_auth|00]
    {"smp_set_key_size",            smp_test_set_key_size},             // [BLE,smp_set_key_size|01001000] Default_Conidx_KeyLen_MITM
    {"smp_dont_auto_start_auth",    smp_test_dont_auto_start_auth},     // [BLE,smp_dont_auto_start_auth|01]

    // gatt
    {"gatts_send_notify",           gatts_test_send_notify},            // [BLE,gatts_send_notify|0000] Ntf00Ind01_HandleCount_CharValueHandls
#if BLE_GATT_CLIENT_SUPPORT
    {"gattc_crate_eatt",            gattc_test_create_eatt},            // [BLE,gattc_crate_eatt|00]
    {"gattc_disconnect_eatt",       gattc_test_disconnect_eatt},        // [BLE,gattc_disconnect_eatt|00]
    {"gattc_disc_all_services",     gattc_test_discover_all_services},  // [BLE,gattc_disc_all_services|00]
    {"gattc_disc_char_by_uuid",     gattc_test_discover_char_by_uuid},  // [BLE,gattc_disc_char_by_uuid|00A001] Conidx_CharUuid_Char128Uuid
    {"gattc_clear_cache",           gattc_test_clear_cache},            // [BLE,gattc_clear_cache|00]
    {"gattc_signed_write",          gattc_test_signed_write},           // [BLE,gattc_signed_write|00000702AABB] Conidx_AttrHandle_Len_Data
    {"gattc_read_req",              gattc_test_read_request},           // [BLE,gattc_read_req|0000070000] Conidx_AttrHandle_Char00Desc01_EATT
    {"gattc_multi_read",            gattc_test_multi_read},             // [BLE,gattc_multi_read|0002A001B0020000] Conidx_Count_Handles_IsMultiVar_HasValueLens_ValueLens
    {"gattc_write_req",             gattc_test_write_request},          // [BLE,gattc_write_req|000007000001AA] Conidx_AttrHandle_Char00Desc01_WriteCmd_DataLen_Data
    {"gattc_prep_write",            gattc_test_prepare_write},          // [BLE,gattc_prep_write|000007000001AA] Conidx_AttrHandle_Prepare00Execute01_CancelOrOffset_DataLen_Data
    {"gattc_send_data",             gattc_test_send_data},              // [BLE,gattc_send_data|000140] Conidx_Eatt_DataLen_Data
#endif

#if (BLE_AUDIO_ENABLED)
    {"config_aud_cfg",              ble_audio_set_audio_configuration_test},
    {"uc_srv_ase_op",               bap_test_start_uc_srv_ase_operation},

    {"aob_connect_devices",         bap_test_start_direct_connection},

    {"vocs_set_audio_location",     vocs_test_set_vocs_location},
    {"vocs_set_output_desc",        vocs_test_set_vocs_description},

    {"mics_set_mic_mute",           mics_test_set_mic_mute},

    {"mcp_mcc_control",             mcp_test_mcc_control},
    {"mcp_mcc_set_val",             mcp_test_mcc_set_val},
    {"mcp_mcc_set_obj_id",          mcp_test_mcc_set_obj_id},
    {"mcp_mcc_search",              mcp_test_mcc_search_item},

#ifdef APP_BLE_BIS_DELEG_ENABLE
    {"bc_deleg_add_src",            bap_test_start_deleg_add_source},
#endif /// APP_BLE_BIS_DELEG_ENABLE

#ifdef AOB_MOBILE_ENABLED
    {"bc_assist_add_src",           bap_test_start_assist_add_source},
    {"bc_assist_set_bcast_code",    bap_test_assist_set_bcast_code},

    {"pacc_set_aud_loc",            bap_test_start_pacc_set_audio_location},

    {"uc_cli_cfg_codec",            bap_test_start_uc_cli_cfg_codec},
    {"uc_cli_cfg_qos",              bap_test_start_uc_cli_cfg_qos},
    {"uc_cli_enable",               bap_test_start_uc_cli_enable},
    {"uc_cli_disable",              bap_test_start_uc_cli_disable},
    {"uc_cli_release",              bap_test_start_uc_cli_release},
    {"uc_cli_upd_md",               bap_test_start_uc_cli_upd_md},
    // Use ase stm manage
    {"uc_cli_uni_stream",           bap_test_start_unidirectionla_stream},
    {"uc_cli_bi_stream",            bap_test_start_bidirectionla_stream},

#ifdef APP_BLE_BIS_SRC_ENABLE
    {"bc_src_start_grp",            bap_test_start_bis_src_grp},
    {"bc_src_upd_md",               bap_test_update_subgrp_metadata},
#endif /// APP_BLE_BIS_SRC_ENABLE

    {"vcc_set_abs_vol",             vcp_test_set_absolute_volume},
    {"vcc_set_vol_mute",            vcp_test_set_volume_mute},
    // Include service: VOCS
    {"vcc_set_vol_offset",          vcp_test_set_volume_offset},

    {"micc_set_mic_mute",           micp_test_set_mic_mute},
    // Include service: AICS
    {"micc_set_input_gain",         micp_test_set_mic_input_gain},
    {"micc_set_input_mute",         micp_test_set_mic_input_mute},
    {"micc_set_input_gain_mode",    micp_test_set_mic_input_gain_mode},

    {"aicc_read_character",         aicp_test_read_character},

    // MCP
    {"mcp_set_player_name",         mcp_mcs_set_player_name},
    {"mcp_set_track_info",          mcp_mcs_set_curr_track_info},
    {"mcp_media_action",            mcp_mcs_media_action},

#endif /// AOB_MOBILE_ENABLED
#endif /// BLE_AUDIO_ENABLED

#if 0
    {"mobile_connect",              ble_audio_mobile_connect_test}, // [AOB,mobile_connect|214 204 251 165 185 226]
    {"mobile_disconnect",           ble_audio_mobile_disconnect_test},
    {"config_peer_addr",            ble_audio_test_config_peer_ble_addr},
    {"config_bis_src_addr",         ble_audio_test_config_bis_src_ble_addr},
    {"update_ble_addr",             ble_audio_test_update_ble_addr},
    {"config_aud_cfg",              ble_audio_set_audio_configuration_test},
#ifdef APP_BLE_BIS_SRC_ENABLE
    {"config_bis_code",             ble_audio_test_config_bis_code},
    {"config_bis_id",               ble_audio_test_config_bis_id},
#endif
#ifdef AOB_MOBILE_ENABLED
    {"update_timing",               ble_audio_update_timing_test_handler}, // [AOB,aob_connect_devices|11222333336600|9AED365D25D401...]
    {"aob_connect_devices",         ble_audio_mobile_connect_any_devices},
    {"aob_set_cis_num_in_cig",      ble_audio_mobile_set_cis_num_in_cig}, // [AOB,start_ble_pairing|0] to start pairing on the connection with index 0 the index can be updated accordingly
    {"start_ble_pairing",           ble_audio_test_start_pairing},
    {"audio_sharing_config_master_addr", ble_audio_test_audio_sharing_config_master_addr},
#endif
#if defined(APP_BLE_BIS_ASSIST_ENABLE) || defined(APP_BLE_BIS_DELEG_ENABLE) || defined(APP_BLE_BIS_SINK_ENABLE)
    {"bc_set_bcast_id",             aob_bc_scan_set_bcast_id},
    {"bc_set_bcast_code",           aob_bc_scan_set_bcast_code},
    {"bc_scan_channel_start",       aob_bc_scan_param_start_handler},
#endif
#ifdef AOB_UC_TEST
    {"aob_uc_test_set_param",       aob_mobile_uc_set_test_case_param_handler},
#endif
#if (BLE_APP_SEC)
    {"config_encrypt",              ble_audio_test_config_encrypt_request},
    {"ble_slave_initiate_secure_connection", ble_audio_test_slave_initiate_secure_connection},
#endif
    {"uc_srv_set_presdelay_test",   aob_uc_srv_set_presdelay_test},
#ifdef IS_BLE_AUDIO_PTS_TEST_ENABLED
    {"csisc_notify_lock",           aob_csisc_notify_lock_test},
    {"atc_csisc_set_mem_lock",      _mobile_csisc_set_mem_lock},
    {"tbs_call_action_test",        _mobile_pts_test_call_action},
#endif
#endif
};

static app_ble_test_cmd_with_param_handler_t app_ble_test_cmd_with_param_get_handler(char* buf)
{
    app_ble_test_cmd_with_param_handler_t p = NULL;
    for(uint32_t i = 0; i < ARRAY_SIZE(g_app_ble_test_cmd_with_param_table); i++)
    {
        if (strncmp((char*)buf, g_app_ble_test_cmd_with_param_table[i].string, strlen((char*)buf))==0 ||
            strstr(g_app_ble_test_cmd_with_param_table[i].string, (char*)buf))
        {
            TRACE(1, "ble cmd:%s", g_app_ble_test_cmd_with_param_table[i].string);
            p = g_app_ble_test_cmd_with_param_table[i].function;
            break;
        }
    }
    return p;
}

static int app_ble_test_cmd_with_param_handler(char* cmd, uint32_t cmdLen, char* cmdParam, uint32_t cmdParamLen)
{
    int ret = 0;

    app_ble_test_cmd_with_param_handler_t handl_function = app_ble_test_cmd_with_param_get_handler(cmd);
    if(handl_function)
    {
        handl_function(cmdParam, cmdParamLen);
    }
    else
    {
        ret = -1;
        TRACE(0,"can not find handle function");
    }
    return ret;
}

static app_ble_test_cmd_handler_t app_ble_test_cmd_get_handler(unsigned char* buf)
{
    app_ble_test_cmd_handler_t p = NULL;
    for(uint32_t i = 0; i < ARRAY_SIZE(g_app_ble_test_cmd_table); i++)
    {
        if (strncmp((char*)buf, g_app_ble_test_cmd_table[i].string, strlen((char*)buf))==0 ||
            strstr(g_app_ble_test_cmd_table[i].string, (char*)buf))
        {
            TRACE(1, "ble cmd:%s", g_app_ble_test_cmd_table[i].string);
            p = g_app_ble_test_cmd_table[i].function;
            break;
        }
    }
    return p;
}

static int app_ble_test_cmd_handler(unsigned char *buf, unsigned int length)
{
    int ret = 0;

    app_ble_test_cmd_handler_t handl_function = app_ble_test_cmd_get_handler(buf);
    if(handl_function)
    {
        //handl_function();
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)handl_function);
    }
    else
    {
        ret = -1;
        TRACE(0,"can not find handle function");
    }
    return ret;
}

static unsigned int app_ble_test_cmd_callback(unsigned char *cmd, unsigned int cmd_length)
{
    int param_len = 0;
    char* cmd_param = NULL;
    char* cmd_end = (char *)cmd + cmd_length;

    cmd_param = strstr((char*)cmd, (char*)"|");

    if (cmd_param)
    {
        *cmd_param = '\0';
        cmd_length = cmd_param - (char *)cmd;
        cmd_param += 1;

        param_len = cmd_end - cmd_param;

        app_ble_test_cmd_with_param_handler((char *)cmd, cmd_length, cmd_param, param_len);
    }
    else
    {
        app_ble_test_cmd_handler((unsigned char*)cmd, strlen((char*)cmd));
    }

    return 0;
}

#endif /* APP_TRACE_RX_ENABLE */

void app_ble_test_cmd_init(void)
{
#ifdef APP_TRACE_RX_ENABLE
    TRACE(0,"app_ble_test_cmd_init");
    app_trace_rx_register("BLE", app_ble_test_cmd_callback);
    app_trace_rx_register("AOB", app_ble_test_cmd_callback);
#endif
}

