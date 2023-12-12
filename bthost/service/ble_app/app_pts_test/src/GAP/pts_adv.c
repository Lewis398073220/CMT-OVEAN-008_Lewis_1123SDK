#include <stdio.h>
#include "gap.h"
#include "gatt.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "tgt_hardware.h"
static void pts_gap_adv_start(uint8_t *data)
{
    static uint8_t enable_flag = 0;
    pts_adv_para_t * adv_para = NULL;

    if (!enable_flag)
    {
        adv_para = pts_adv_get_para_index();
        adv_para->adv_data.len = data[0] + 1;
        memcpy(adv_para->adv_data.data, data, data[0] + 1);

        pts_adv_start();

        enable_flag++;
    } else {
        pts_adv_set_data(GAPM_SET_ADV_DATA, data, data[0] + 1);
    }
}
static void pts_gap_adv_bv_01_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_MORE_16_BIT_UUID;
    adv_data.data[0] = GATT_CHAR_DEVICE_NAME&0x00FF;
    adv_data.data[1] = (GATT_CHAR_DEVICE_NAME&0xFF00) >> 8;
    adv_data.len = sizeof(adv_data.type) + 2;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_02_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_COMPLETE_NAME;
    strcpy((char *)adv_data.data, "bes_ble");
    adv_data.len = sizeof(adv_data.type) + strlen("bes_ble");

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_03_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_FLAGS;
    adv_data.data[0] = 0x1A;
    adv_data.len = sizeof(adv_data.type) + 1;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_04_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
    adv_data.data[0] = 0x11;
    adv_data.len = sizeof(adv_data.type) + 1;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_05_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_TRANSMIT_POWER;
    adv_data.data[0] = PTS_TX_POWER_LEVEL;
    adv_data.len = sizeof(adv_data.type) + 1;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_08_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_SLAVE_CONN_INT_RANGE;
    adv_data.data[0] = PTS_SLAVE_CONN_INTERVAL_MIN & 0x00FF;
    adv_data.data[1] = (PTS_SLAVE_CONN_INTERVAL_MIN & 0xFF00) >> 8;
    adv_data.data[2] = PTS_SLAVE_CONN_INTERVAL_MAX & 0x00FF;
    adv_data.data[3] = (PTS_SLAVE_CONN_INTERVAL_MAX & 0xFF00) >> 8;
    adv_data.len = sizeof(adv_data.type) + 4;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_09_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_RQRD_16_BIT_SVC_UUID;
    adv_data.data[0] = GATT_CHAR_DEVICE_NAME & 0x00FF;
    adv_data.data[1] = (GATT_CHAR_DEVICE_NAME & 0xFF00) >> 8;
    adv_data.len = sizeof(adv_data.type) + 2;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_10_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_SERVICE_16_BIT_DATA;
    adv_data.data[0] = GATT_CHAR_DEVICE_NAME & 0x00FF;
    adv_data.data[1] = (GATT_CHAR_DEVICE_NAME & 0xFF00) >> 8;
    strcpy((char *)&adv_data.data[2], "bes_ble");
    adv_data.len = sizeof(adv_data.type) + 2 + strlen("bes_ble");

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_11_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_APPEARANCE;
    adv_data.data[0] = PTS_APPEARANCE_TYPE & 0x00FF;
    adv_data.data[1] = (PTS_APPEARANCE_TYPE & 0xFF00) >> 8;
    adv_data.len = sizeof(adv_data.type) + 2;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static void pts_gap_adv_bv_12_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_PUB_TGT_ADDR;
    memcpy(adv_data.data, ble_global_addr, GAP_BD_ADDR_LEN);
    adv_data.len = sizeof(adv_data.type) + GAP_BD_ADDR_LEN;

    pts_gap_adv_start((uint8_t *)&adv_data);
}


static void pts_gap_adv_bv_13_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_RAND_TGT_ADDR;
    memcpy(adv_data.data, ble_global_addr, GAP_BD_ADDR_LEN);
    adv_data.len = sizeof(adv_data.type) + GAP_BD_ADDR_LEN;

    pts_gap_adv_start((uint8_t *)&adv_data);
}


static void pts_gap_adv_bv_14_c(char *para_buf, uint32_t str_len)
{
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_ADV_INTV;
    adv_data.data[0] = PTS_BLE_ADVERTISING_INTERVAL & 0x00FF;
    adv_data.data[1] = (PTS_BLE_ADVERTISING_INTERVAL & 0xFF00) >> 8;
    adv_data.len = sizeof(adv_data.type) + 2;

    pts_gap_adv_start((uint8_t *)&adv_data);
}


static void pts_gap_adv_bv_17_c(char *para_buf, uint32_t str_len)
{
    // URI ADV TYPE
    // see: http://www.iana.org/assignments/uri-schemes/uri-schemes.xhtml
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_URI;
    adv_data.data[0] = 0X00;
    adv_data.data[1] = 0X01;
    strcpy((char *)&adv_data.data[2], "https://www.bluetooth.com");
    adv_data.len = sizeof(adv_data.type) + strlen("https://www.bluetooth.com") +2;

    pts_gap_adv_start((uint8_t *)&adv_data);
}


static void pts_gap_adv_bv_18_c(char *para_buf, uint32_t str_len)
{
    // Advertising Interval long
    adv_data_elem_t adv_data = {0};
    
    adv_data.type = GAP_AD_TYPE_ADV_INTV_LONG;
    adv_data.data[0] = 0x01;
    adv_data.data[1] = 0x12;
    adv_data.data[2] = 0x02;
    adv_data.len = sizeof(adv_data.type) + 3;

    pts_gap_adv_start((uint8_t *)&adv_data);
}

static pts_case_num_t adv_bv_num[] = {
{1,  pts_gap_adv_bv_01_c},
{2,  pts_gap_adv_bv_02_c},
{3,  pts_gap_adv_bv_03_c},
{4,  pts_gap_adv_bv_04_c},
{5,  pts_gap_adv_bv_05_c},
{8,  pts_gap_adv_bv_08_c},
{9,  pts_gap_adv_bv_09_c},
{10, pts_gap_adv_bv_10_c},
{11, pts_gap_adv_bv_11_c},
{12, pts_gap_adv_bv_12_c},
{13, pts_gap_adv_bv_13_c},
{14, pts_gap_adv_bv_14_c},
{17, pts_gap_adv_bv_17_c},
{18, pts_gap_adv_bv_18_c},
{0xFF, NULL},
};

pts_module_case_t pts_adv_case[] = {
{BLE_HOST_TEST_NO_CASE, adv_bv_num, NULL},
{NULL, NULL, NULL},
};