/*
 * lhdcUtil.h
 *
 *  Created on: 2022/3/19
 *      Author: eric.lee
 */

#ifndef LHDC_BES_CC_UTIL_H
#define LHDC_BES_CC_UTIL_H
#ifdef DSP_M55
#include <stdbool.h>
#include "bluetooth.h"

typedef enum {
    LHDC_STRM_TYPE_COMBINE,
    LHDC_STRM_TYPE_SPLIT
}LHDC_STRM_TYPE;

typedef enum {
  VERSION_2 = 200,
  VERSION_3 = 300,
  VERSION_4 = 400,
  VERSION_LLAC = 500
}lhdc_ver_t;

typedef enum _lhdc_dec_type_t{
  DEC_TYPE_LHDC,
  DEC_TYPE_LLAC,
} lhdc_dec_type_t;

typedef enum {
  LHDCV2_BLOCK_SIZE = 512,
  LHDCV3_BLOCK_SIZE = 256,
}lhdc_block_size_t;

typedef struct bes_bt_local_info_t{
    uint8_t bt_addr[BTIF_BD_ADDR_SIZE];
    const char *bt_name;
    uint8_t bt_len;
    uint8_t ble_addr[BTIF_BD_ADDR_SIZE];
    const char *ble_name;
    uint8_t ble_len;
}bes_bt_local_info;

typedef struct _lhdc_frame_Info
{
    uint32_t frame_len;
    uint32_t isSplit;
    uint32_t isLeft;

} lhdc_frame_Info_t;


typedef enum {
    LHDC_OUTPUT_STEREO = 0,
    LHDC_OUTPUT_LEFT_CAHNNEL,
    LHDC_OUTPUT_RIGHT_CAHNNEL,
} lhdc_channel_t;

typedef int LHDCSample;

typedef void (*print_log_fp)(char*  msg);
typedef int (*LHDC_GET_BT_INFO)(bes_bt_local_info * bt_info);



#define A2DP_LHDC_HDR_LATENCY_LOW   0x00
#define A2DP_LHDC_HDR_LATENCY_MID   0x01
#define A2DP_LHDC_HDR_LATENCY_HIGH  0x02
#define A2DP_LHDC_HDR_LATENCY_MASK  (A2DP_LHDC_HDR_LATENCY_MID | A2DP_LHDC_HDR_LATENCY_HIGH)

#define A2DP_LHDC_HDR_FRAME_NO_MASK 0xfc

 // LHDC Heap Information
#define LHDC_HEAP_SIZE 1024 * 2
#define LLAC_HEAP_SIZE 1024 * 12
#define LHDC_DEC_HEAP_SIZE 1024 * 6
#define LHDC_HEAP_SIZE_TOTAL LHDC_HEAP_SIZE + LLAC_HEAP_SIZE + LHDC_DEC_HEAP_SIZE


#ifdef __cplusplus
extern "C" {
#endif



    uint8_t getLhdcData_dsp(void);
    uint8_t genLhdcData_dsp(void);
    void lhdcInit_mcu(uint32_t bitPerSample, uint32_t sampleRate, uint32_t scaleTo16Bits, lhdc_ver_t version);
    bool lhdcFetchFrameInfo_mcu(uint8_t* frameData, lhdc_frame_Info_t* frameInfo);
    uint32_t lhdcGetSampleSize_mcu(void);
    bool lhdcSetData_mcu(uint8_t* plhdc_data, int in_len, uint8_t* data_addr, LHDC_GET_BT_INFO pf_bt_info_get, uint8_t blhdc_data);
    bool lhdcSetLicenseKeyTable_dsp(uint8_t* lhdc_data);


#ifdef __cplusplus
}
#endif
#endif
#endif /* End of LHDC_UTIL_H */
