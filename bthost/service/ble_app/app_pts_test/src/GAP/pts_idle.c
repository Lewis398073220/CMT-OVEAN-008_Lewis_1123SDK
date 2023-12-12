#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static void pts_gap_idle_start(uint8_t *data)
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->adv_data.len = data[0] + 1;
    memcpy(adv_para->adv_data.data, data, data[0] + 1);

    pts_adv_start(); 
}


static void pts_gap_idle_namp_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        adv_data_elem_t adv_data = {0};

        adv_data.type = GAP_AD_TYPE_FLAGS;
        adv_data.data[0] = 0x1A;
        adv_data.len = sizeof(adv_data.type) + 1;
        //start ADV
        pts_gap_idle_start((uint8_t *)&adv_data);
    }
    else if(steps == 2){
        pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_idle_namp_bv_02_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(steps == 1){
        adv_data_elem_t adv_data = {0};

        adv_data.type = GAP_AD_TYPE_FLAGS;
        adv_data.data[0] = 0x18;
        adv_data.len = sizeof(adv_data.type) + 1;
        //start ADV
        pts_gap_idle_start((uint8_t *)&adv_data);
    }
    else if(steps == 2){
        pts_disconnect();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static void pts_gap_idle_gin_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
}


static pts_case_num_t idle_namp_bv_num[] = {
{1, pts_gap_idle_namp_bv_01_c},
{2, pts_gap_idle_namp_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t idle_gin_bv_num[] = {
{1, pts_gap_idle_gin_bv_01_c},
{0xFF, NULL},
};

pts_module_case_t pts_idle_case[] = {
{"NAMP", idle_namp_bv_num, NULL},
{"GIN",  idle_gin_bv_num, NULL},
{NULL, NULL, NULL},
};


