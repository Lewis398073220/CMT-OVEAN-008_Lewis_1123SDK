/*
 * lhdcUtil.h
 *
 *  Created on: 2001/1/3
 *      Author: eric.lee
 */

#ifndef LHDC_UTIL_H
#define LHDC_UTIL_H

#include <stdbool.h>
#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define for LHDC stream type.
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


bool chkLhdcHandleExist(void);
void lhdcInit(uint32_t bitPerSample, uint32_t sampleRate, uint32_t scaleTo16Bits, lhdc_ver_t version);
bool lhdcReadyForInput(void);
uint32_t lhdcPutData(uint8_t * pInpBuf, uint32_t NumBytes);
//uint32_t lhdcDecodeProcess(uint8_t * pOutBuf);
uint32_t lhdcDecodeProcess(uint8_t * pOutBuf, uint8_t * pInput, uint32_t len);
bool lhdcSetLicenseKeyTable(uint8_t * licTable, LHDC_GET_BT_INFO pFunc);
bool lhdcSetLicenseKeyTable_dsp(uint8_t* lhdc_data);
void lhdcSetLicenseKeyChkPeriod (uint8_t period);
bool larcIsEnabled();
char * getVersionCode();

void lhdcDestroy();

void lhdc_register_log_cb(print_log_fp cb);

uint32_t lhdcGetSampleSize( void);
bool lhdcFetchFrameInfo(uint8_t * frameData, lhdc_frame_Info_t * frameInfo);

uint32_t lhdcChannelSelsect(lhdc_channel_t channel_type);

#ifdef __cplusplus
}
#endif
#endif /* End of LHDC_UTIL_H */
