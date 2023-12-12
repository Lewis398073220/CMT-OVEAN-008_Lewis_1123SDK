#ifndef __APP_FP_RFCOMM_H__
#define __APP_FP_RFCOMM_H__
#if GFPS_ENABLED

#include "gfps.h"

typedef struct
{
    uint8_t    devId;
    uint8_t    isConnected;
    uint8_t    isRfcommInitialized;
    bt_spp_channel_t *spp_chan;
} fpRfcommEnv_t;

typedef struct
{
    fpRfcommEnv_t fp_rfcomm_service[BT_DEVICE_NUM];
    fp_event_cb cb;
} fpRfcommSrvEnv_t;


#ifdef __cplusplus
extern "C" {
#endif

bool app_fp_rfcomm_send(uint8_t device_id, uint8_t *ptrData, uint32_t length);

bt_status_t app_fp_rfcomm_init(void);

void app_fp_rfcomm_register_callback(fp_event_cb callback);

void app_fp_disconnect_rfcomm(uint8_t device_id);

uint16_t app_fp_rfcomm_get_data_len(uint8_t devId);

void app_fp_rfcomm_data_done(uint8_t devId, uint16_t consumeLen, uint8_t *buf, uint16_t *len);


#ifdef __cplusplus
}
#endif
#endif

#endif  // __APP_FP_RFCOMM_H__
