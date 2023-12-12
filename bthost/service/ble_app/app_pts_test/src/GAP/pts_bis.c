#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

static void pts_gap_bis_bsm_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

}

static void pts_gap_bis_bbm_bv_01_c(char *para_buf, uint32_t str_len)
{
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }

       switch(steps){
        case 1:
            {
                pts_bis_para_t *big_info = pts_big_get_para_index();
                struct gapm_activity_create_adv_cmd *adv_info = big_info->adv_para;

                adv_info->type = GAPM_ADV_TYPE_PERIODIC;
                adv_info->adv_param.prop = GAPM_EXT_ADV_PROP_NON_CONN_NON_SCAN_MASK;
                adv_info->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
                adv_info->adv_param.with_flags = false;
                adv_info->adv_param.max_tx_pwr = 0xBA;
                adv_info->adv_param.filter_pol = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
                adv_info->adv_param.prim_cfg.adv_intv_min = 0x100;
                adv_info->adv_param.prim_cfg.adv_intv_max = 0x100;
                adv_info->adv_param.prim_cfg.chnl_map = 7;
                adv_info->adv_param.prim_cfg.phy = 1;
                adv_info->second_cfg.max_skip = 1;
                adv_info->second_cfg.phy = 1;
                adv_info->second_cfg.adv_sid = 1;
                adv_info->randomAddrRenewIntervalInSecond = 0;
                adv_info->period_cfg.interval_min = 0x0a;
                adv_info->period_cfg.interval_max = 0x0a;

                pts_big_start();
                break;
            }
        case 2:
            {
                int i;
                static uint32_t ref_time = 0xbc3210;
                uint8_t bis_iso_data[]={0x01, 0x01, 0x04, 0x05, 0x03, 0x00, 0x18, 0x01, 0x18};

                pts_send_iso_start(2, 4, 15);
                for(i = 0; i < 5; i++)
                {
                    osDelay(500);
                    pts_send_iso_data(bis_iso_data[0], bis_iso_data, sizeof(bis_iso_data), ref_time);
                    bis_iso_data[0]++;
                    ref_time += 0x500;
                }
                break;
            }
        default:
            {
                TRACE(0, "[%s][%d]ERROR:No steps are entered!", __FUNCTION__, __LINE__);
                break;
            }
    }

}

static pts_case_num_t bis_bsm_bv_num[] = {
{1, pts_gap_bis_bsm_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t bis_bbm_bv_num[] = {
{1, pts_gap_bis_bbm_bv_01_c},
{0xFF, NULL},
};

pts_module_case_t pts_bis_case[] = {
{"BSM", bis_bsm_bv_num, NULL},
{"BBM", bis_bbm_bv_num, NULL},
{NULL, NULL, NULL},
};


