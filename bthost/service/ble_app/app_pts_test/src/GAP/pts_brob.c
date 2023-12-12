#include <stdio.h>
#include <cmsis_os.h>
#include "gap.h"
#include "gapm_le.h"
#include "gapc_msg.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static uint8_t bcst_adv_data[]={0x02, 0x01, 0x04, 0x05, 0x03, 0x00, 0x18, 0x01, 0x18,
                                 0x0D, 0x09, 0x50, 0x54, 0x53, 0x2D, 0x47, 0x41, 0x50,
                                 0x2D, 0x30, 0x36, 0x42, 0x38, 0x03, 0x19, 0x00, 0x00};

static void pts_brob_start_adv(uint8_t addr_type)
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.own_addr_type = addr_type;
    memcpy(adv_para->adv_data.data, bcst_adv_data, sizeof(bcst_adv_data));
    adv_para->adv_data.len = sizeof(bcst_adv_data);

    if(addr_type != GAPM_STATIC_ADDR)
    {
        memcpy(&adv_para->para.adv_param.peer_addr, &dongle_addr, sizeof(dongle_addr));
    }

    pts_adv_start();
}

static void pts_gap_brob_bcst_bv_01_02_04_c(char *para_buf, uint32_t str_len)
{
    pts_brob_start_adv(GAPM_STATIC_ADDR);
}


static void pts_gap_brob_bcst_bv_03_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        pts_sm_set_pairing_para(PTS_PAIRING_AUTH, 1);
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_brob_start_adv(GAPM_STATIC_ADDR);
    }
    else if(2 == steps){
        pts_disconnect();
        osDelay(10);
        pts_adv_disenable();
        osDelay(10);
        pts_adv_delete();
    }
    else if(3 == steps){
        pts_brob_start_adv(GAPM_GEN_RSLV_ADDR);
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}


static void pts_gap_brob_obsv_bv_01_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_brob_obsv_bv_02_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_brob_obsv_bv_05_c(char *para_buf, uint32_t str_len)
{

}


static void pts_gap_brob_obsv_bv_06_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

    if(1 == steps){
        //Connect the dongle
        pts_conn_start();
    }
    else if(2 == steps){
        //The dongle will connect to the test machine and
        //ask the test machine to actively disconnect
        pts_sm_set_pairing_para(PTS_PAIRING_SEC_REQ_LEVEL, 0);
        pts_sm_pair_req();
    }
    else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}



static pts_case_num_t brob_bcst_bv_num[] = {
{1, pts_gap_brob_bcst_bv_01_02_04_c},
{2, pts_gap_brob_bcst_bv_01_02_04_c},
{3, pts_gap_brob_bcst_bv_03_c},
{4, pts_gap_brob_bcst_bv_01_02_04_c},
{0xFF, NULL},
};

static pts_case_num_t brob_obsv_bv_num[] = {
{1, pts_gap_brob_obsv_bv_01_c},
{2, pts_gap_brob_obsv_bv_02_c},
{5, pts_gap_brob_obsv_bv_05_c},
{6, pts_gap_brob_obsv_bv_06_c},
{0xFF, NULL},
};

pts_module_case_t pts_brob_case[] = {
{"BCST", brob_bcst_bv_num, NULL},
{"OBSV", brob_obsv_bv_num, NULL},
{NULL, NULL, NULL},
};

