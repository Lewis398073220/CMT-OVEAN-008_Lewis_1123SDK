#include <stdio.h>
#include "gap.h"
#include "gapm_le.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

#define TSPX_LIM_ADV_TIMEOUT 30720

static void pts_disc_start_adv(uint8_t adv_type, uint8_t *adv_data, uint8_t data_len)
{
    pts_adv_para_t * adv_para = NULL;

    adv_para = pts_adv_get_para_index();
    adv_para->para.adv_param.prop = adv_type;
    memcpy(adv_para->adv_data.data, adv_data, data_len);
    adv_para->adv_data.len = data_len;

    pts_adv_start();  
}

static void gap_disc_add_adv_data(send_adv_data_t *adv_data, uint8_t adv_type_flags)
{
    uint8_t data[5] = {0};

    pts_adv_data_add_element(adv_data, GAP_AD_TYPE_FLAGS, 0, &adv_type_flags, 1);

    data[0] = PTS_TX_POWER_LEVEL;
    pts_adv_data_add_element(adv_data, GAP_AD_TYPE_TRANSMIT_POWER, 0, data, 1);

    //host will add the name element by itself
    //adv_data_add(adv_data, GAP_AD_TYPE_COMPLETE_NAME, 0, (uint8_t *)"bes_ble", strlen("bes_ble"));

    data[0] = GATT_CHAR_DEVICE_NAME & 0x00FF;
    data[1] = (GATT_CHAR_DEVICE_NAME & 0xFF00) >> 8;
    pts_adv_data_add_element(adv_data, GAP_AD_TYPE_MORE_16_BIT_UUID, 0, data, 2);

    data[0] = PTS_SLAVE_CONN_INTERVAL_MIN & 0x00FF;
    data[1] = (PTS_SLAVE_CONN_INTERVAL_MIN & 0xFF00) >> 8;
    data[2] = PTS_SLAVE_CONN_INTERVAL_MAX & 0x00FF;
    data[3] = (PTS_SLAVE_CONN_INTERVAL_MAX & 0xFF00) >> 8;
    pts_adv_data_add_element(adv_data, GAP_AD_TYPE_SLAVE_CONN_INT_RANGE, 0, data, 4);
}


static void pts_gap_disc_nonm_bv_01_c(char *para_buf, uint32_t str_len)
{
    uint8_t adv_data[]={0x02, 0x01, 0x00};

    pts_disc_start_adv(GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK, adv_data, sizeof(adv_data));
}

static void pts_gap_disc_nonm_bv_02_c(char *para_buf, uint32_t str_len)
{
    uint8_t adv_data[]={0x02, 0x01, 0x00};

    pts_disc_start_adv(GAPM_ADV_PROP_UNDIR_CONN_MASK, adv_data, sizeof(adv_data));
}

static void pts_gap_disc_limm_bv_01_c(char *para_buf, uint32_t str_len)
{
    osTimerId TimerID;
    osTimerDef (DISC_TIMER, pts_adv_disenable);
    TimerID = osTimerCreate(osTimer(DISC_TIMER), osTimerOnce, NULL);
    send_adv_data_t adv_data;

    adv_data.len=0;
    gap_disc_add_adv_data(&adv_data, 0x01);
    pts_disc_start_adv(GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK, adv_data.data, adv_data.len);

    osTimerStart(TimerID, TSPX_LIM_ADV_TIMEOUT);

}

static void pts_gap_disc_limm_bv_02_c(char *para_buf, uint32_t str_len)
{
    osTimerId TimerID;
    osTimerDef (DISC_TIMER, pts_adv_disenable);
    TimerID = osTimerCreate(osTimer(DISC_TIMER), osTimerOnce, NULL);
    send_adv_data_t adv_data;

    adv_data.len=0;
    gap_disc_add_adv_data(&adv_data, 0x01);
    pts_disc_start_adv(GAPM_ADV_PROP_UNDIR_CONN_MASK, adv_data.data, adv_data.len);

    osTimerStart(TimerID, TSPX_LIM_ADV_TIMEOUT);
}

static void pts_gap_disc_genm_bv_01_c(char *para_buf, uint32_t str_len)
{
    send_adv_data_t adv_data;

    adv_data.len=0;
    gap_disc_add_adv_data(&adv_data, 0x02);
    pts_disc_start_adv(GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK, adv_data.data, adv_data.len);

}

static void pts_gap_disc_genm_bv_02_c(char *para_buf, uint32_t str_len)
{
    send_adv_data_t adv_data;

    adv_data.len=0;
    gap_disc_add_adv_data(&adv_data, 0x02);
    pts_disc_start_adv(GAPM_ADV_PROP_UNDIR_CONN_MASK, adv_data.data, adv_data.len);
}

static void pts_gap_disc_limp_bv_01_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_limp_bv_02_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_limp_bv_03_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_limp_bv_04_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_limp_bv_05_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_genp_bv_01_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_genp_bv_02_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_genp_bv_03_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_genp_bv_04_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_genp_bv_05_c(char *para_buf, uint32_t str_len)
{

}

static void pts_gap_disc_rpa_bv_01_c(char *para_buf, uint32_t str_len)
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




static pts_case_num_t disc_nonm_bv_num[] = {
{1,  pts_gap_disc_nonm_bv_01_c},
{2,  pts_gap_disc_nonm_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t disc_limm_bv_num[] = {
{1,  pts_gap_disc_limm_bv_01_c},
{2,  pts_gap_disc_limm_bv_02_c},
{0xFF, NULL},
};

static pts_case_num_t disc_genm_bv_num[] = {
{1,  pts_gap_disc_genm_bv_01_c},
{2,  pts_gap_disc_genm_bv_02_c},

{0xFF, NULL},
};

static pts_case_num_t disc_limp_bv_num[] = {
{1,  pts_gap_disc_limp_bv_01_c},
{2,  pts_gap_disc_limp_bv_02_c},
{3,  pts_gap_disc_limp_bv_03_c},
{4,  pts_gap_disc_limp_bv_04_c},
{5,  pts_gap_disc_limp_bv_05_c},
{0xFF, NULL},
};

static pts_case_num_t disc_genp_bv_num[] = {
{1,  pts_gap_disc_genp_bv_01_c},
{2,  pts_gap_disc_genp_bv_02_c},
{3,  pts_gap_disc_genp_bv_03_c},
{4,  pts_gap_disc_genp_bv_04_c},
{5,  pts_gap_disc_genp_bv_05_c},
{0xFF, NULL},
};

static pts_case_num_t disc_rpa_bv_num[] = {
{1,  pts_gap_disc_rpa_bv_01_c},
{0xFF, NULL},
};


pts_module_case_t pts_disc_case[] = {
{"NONM",  disc_nonm_bv_num, NULL},
{"LIMM",  disc_limm_bv_num, NULL},
{"GENM",  disc_genm_bv_num, NULL},
{"LIMP",  disc_limp_bv_num, NULL},
{"GENP",  disc_genp_bv_num, NULL},
{"RPA" ,  disc_rpa_bv_num,  NULL},
{NULL, NULL, NULL},
};


