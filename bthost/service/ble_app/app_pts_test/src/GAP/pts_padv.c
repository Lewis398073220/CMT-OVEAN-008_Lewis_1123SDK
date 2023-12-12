#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

#define TX_POWER_LEVEL 12

#define PERIODIC_INTERVAL_MIN 60      //Periodic for PTS tests only
#define PERIODIC_INTERVAL_MAX 120     //Periodic for PTS tests only

uint8_t pAdv_data[]={0x02, 0x01, 0x04};

static void pts_gap_padv_start(uint8_t *data)
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.type = GAPM_ADV_TYPE_PERIODIC;
    adv_para->para.period_cfg.interval_max = PERIODIC_INTERVAL_MAX;
    adv_para->para.period_cfg.interval_min = PERIODIC_INTERVAL_MIN;
    adv_para->adv_data.len = data[0] + 1;
    memcpy(adv_para->adv_data.data, data, data[0] + 1);

    pts_adv_start(); 
}

static void pts_gap_padv_pasm_bv_01_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_COMPLETE_NAME;
    strcpy((char *)adv_data.data, "bes_ble");
    adv_data.len = sizeof(adv_data.type) + strlen("bes_ble");

    pts_gap_padv_start((uint8_t *)&adv_data);
}

static pts_case_num_t padv_pasm_bv_num[] = {
{1,  pts_gap_padv_pasm_bv_01_c},
{0xFF, NULL},
};

pts_module_case_t pts_padv_case[] = {
{"PASM", padv_pasm_bv_num, NULL},
{NULL, NULL, NULL},
};


