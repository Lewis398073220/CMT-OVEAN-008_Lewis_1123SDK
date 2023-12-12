#ifndef __PTS_SM_H__
#define __PTS_SM_H__
#include "pts_ble_host_app.h"

typedef struct pts_smp_pair_info_auth {
    uint8_t Bonding_Flags : 2;
    uint8_t MIMT          : 1;
    uint8_t SC            : 1;
    uint8_t Keypress      : 1;
    uint8_t CT2           : 1;
    uint8_t RFU           : 2;
} pts_smp_pair_info_auth_t;

extern pts_module_case_t pts_cen_case[];
extern pts_module_case_t pts_per_case[];


void pts_sm_module_reg();


#endif
