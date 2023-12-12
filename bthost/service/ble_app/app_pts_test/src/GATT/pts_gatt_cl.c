#include <stdio.h>
#include "gatt.h"
#include "co_endian.h"
#include "prf_types.h"
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"

uint8_t gatt_dongle_addr[GAP_BD_ADDR_LEN]={0x01,0xd5,0x1d,0xe8,0x07,0xc0};//dongle address in pts 

void pts_gatt_init_and_start(void){
    pts_gatt_reg(GATT_ROLE_CLIENT);
    pts_conn_para_t *gatt_con_info = pts_conn_get_para_index();
    memcpy(gatt_con_info->para.peer_addr.addr,gatt_dongle_addr,GAP_BD_ADDR_LEN);
    pts_conn_start(0);
}

static void pts_gatt_cl_gac_bv_01_c(char *para_buf, uint32_t str_len){
    int test_step, hdl, data_size;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x,%d", &test_step, &hdl, &data_size);
    }

    switch(test_step){
    case 1:
        pts_gatt_init_and_start();
        break;
    case 2:
        pts_gatt_cli_mtu_updata();
        break;
    case 3:
        pts_gatt_cli_write(0, hdl, 0, data_size, NULL);
        break;
    case 4:
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
        break;
    default:
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
        break;
    }
}

void le_host_pts_gatt_cl_gad_read_buf(char *para_buf, uint32_t str_len,int *test_step,int *start_hdl,int *end_hdl,int *uuid_type,uint32_t *uuid){
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x,%x,%d,%x-%x-%x-%x-%x-%x-%x-%x", test_step, start_hdl, end_hdl,
                uuid_type, uuid, uuid+1, uuid+2, uuid+3, uuid+4, uuid+5, uuid+6, uuid+7);
    }
}

void le_uuid_change(int uuid_type,uint32_t *uuid){//sscanf save 16bit in uint32,use 2 uint16 to get uint32
    if(uuid_type == GATT_UUID_32){
        *uuid = (*(uuid+1) & 0xFFFF) | ((uuid[0] & 0xFFFF) << 16);
    }
    if(uuid_type == GATT_UUID_128){
        uint32_t temp_data[4] = {0};
        temp_data[0] = (*(uuid+7) & 0xFFFF) | ((*(uuid+6)  & 0xFFFF) << 16);
        temp_data[1] = (*(uuid+5) & 0xFFFF) | ((*(uuid+4)  & 0xFFFF) << 16);
        temp_data[2] = (*(uuid+3)  & 0xFFFF) | ((*(uuid+2)  & 0xFFFF) << 16);
        temp_data[3] = (*(uuid+1)  & 0xFFFF) | ((*uuid  & 0xFFFF) << 16);
        memcpy((uint8_t *)uuid, (uint8_t *)&temp_data[0], 16);
    }
}

void le_host_pts_gatt_cl_gaw_read_buf(char *para_buf, uint32_t str_len,int *test_step,int *hdl, int *offset,int *data_len){
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x,%x,%d", test_step, hdl, offset, data_len);
    }
}


void pts_gatt_cl_gad_bv_01_c(char *para_buf, uint32_t str_len){
    bool full = true;
    uint8_t disc_type = GATT_DISCOVER_SVC_PRIMARY_ALL;
    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    uint32_t uuid[8] = {0};
    int uuid_type = GATT_UUID_16;
    uuid[0] = GATT_DECL_PRIMARY_SERVICE;

    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);

    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_svc(disc_type, full, start_hdl, end_hdl, (uint8_t)uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gad_bv_02_c(char *para_buf, uint32_t str_len){

    bool full = false;
    uint8_t disc_type = GATT_DISCOVER_SVC_PRIMARY_BY_UUID;
    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    uint32_t uuid[8] = {0};
    int uuid_type = GATT_UUID_16;

    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);
    le_uuid_change(uuid_type,(uint32_t *)&uuid);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_svc(disc_type, full, start_hdl, end_hdl, (uint8_t)uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

void pts_gatt_cl_gad_bv_03_c(char *para_buf, uint32_t str_len){
    bool full = true;
    uint8_t disc_type = GATT_DISCOVER_SVC_PRIMARY_ALL;
    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    uint32_t uuid[8] = {0};
    int uuid_type = GATT_UUID_16;

    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);
    uuid[0] = GATT_DECL_INCLUDE;
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_svc(disc_type, full, start_hdl, end_hdl, (uint8_t)uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gad_bv_04_c(char *para_buf, uint32_t str_len){
    uint8_t disc_type = GATT_DISCOVER_CHAR_ALL;
    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    uint32_t uuid[8] = {0};
    int uuid_type = GATT_UUID_16;

    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);
    le_uuid_change(uuid_type,(uint32_t *)&uuid);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_char(disc_type, start_hdl, end_hdl, (uint8_t)uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

void pts_gatt_cl_gad_bv_05_c(char *para_buf, uint32_t str_len){
    uint8_t disc_type = GATT_DISCOVER_CHAR_BY_UUID;
    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    uint32_t uuid[8] = {0};
    int uuid_type = GATT_UUID_16;

    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);
    le_uuid_change(uuid_type,(uint32_t *)&uuid);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_char(disc_type, start_hdl, end_hdl, (uint8_t)uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }

}

void pts_gatt_cl_gad_bv_06_c(char *para_buf, uint32_t str_len){

    int start_hdl = 0x0001, end_hdl = 0xFFFF;
    int test_step=0;
    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,NULL,NULL);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_discover_desc(start_hdl, end_hdl);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gar_cli_read(char *para_buf, uint32_t str_len){//GATT CLI READ
    int test_step, hdl;
    int length = 0;int offset = 0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x,%x,%d", &test_step, &hdl, &offset, &length);
    }

    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_read(hdl, offset, length);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gar_cli_multi_read(char *para_buf, uint32_t str_len){//GATT CLI READ MULTIPLE
    int test_step;
    uint8_t multi_att_count = 0;
    uint32_t data[10] = {0};

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        multi_att_count = sscanf((char *)para_buf, "%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &test_step, &data[0],&data[1],
                                  &data[2], &data[3], &data[4], &data[5], &data[6], &data[7], &data[8], &data[9]);
    }
    for(int i = 0; i < (multi_att_count-1)/2; ++i){
        data[i] = (data[i*2] & 0xFFFF) | ((data[i*2+1] & 0xFFFF) << 16);
    }

    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_read_multiple((multi_att_count-1)/2, (gatt_att_t *)data);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gar_cli_read_by_uuid(char *para_buf, uint32_t str_len){//GATT CLI READ BY UUID
    int test_step=0;
    uint32_t uuid[8] = {0}; int start_hdl = 0x0001, end_hdl = 0xFFFF; int uuid_type = GATT_UUID_16;
    le_host_pts_gatt_cl_gad_read_buf(para_buf,str_len,&test_step,&start_hdl,&end_hdl,&uuid_type,(uint32_t *)&uuid);
    le_uuid_change(uuid_type,(uint32_t *)&uuid);

    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_read_by_uuid(start_hdl, end_hdl, uuid_type, (uint8_t *)&uuid);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gaw_no_rsp(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0, offset=0, data_len=0;
    le_host_pts_gatt_cl_gaw_read_buf(para_buf,str_len,&test_step,&hdl,&offset,&data_len);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_write(GATT_WRITE_NO_RESP, hdl, offset, data_len, NULL);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gaw_signed(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0, offset=0, data_len=0;
    le_host_pts_gatt_cl_gaw_read_buf(para_buf,str_len,&test_step,&hdl,&offset,&data_len);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_write(GATT_WRITE_SIGNED, hdl, offset, data_len, NULL);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gaw_write(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0, offset=0, data_len=0;
    le_host_pts_gatt_cl_gaw_read_buf(para_buf,str_len,&test_step,&hdl,&offset,&data_len);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_write(GATT_WRITE, hdl, offset, data_len, NULL);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gaw_reliable(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0, offset=0, data_len=0;
    le_host_pts_gatt_cl_gaw_read_buf(para_buf,str_len,&test_step,&hdl,&offset,&data_len);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
        pts_gatt_cli_write_reliable(GATT_WRITE, hdl, offset, data_len, NULL);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gan(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x", &test_step, &hdl);
    }
    uint16_t cli_cfg = co_htobs(PRF_CLI_START_NTF);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_write(GATT_WRITE, hdl, 0, sizeof(uint16_t), (uint8_t *)&cli_cfg);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

void pts_gatt_cl_gai(char *para_buf, uint32_t str_len){
    int test_step=0, hdl=0;
    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]data: %s", __FUNCTION__, __LINE__, para_buf);
        sscanf(para_buf, "%d,%x", &test_step, &hdl);
    }
    uint16_t cli_cfg = co_htobs(PRF_CLI_START_IND);
    if(test_step == 1){
        pts_gatt_init_and_start();
    }else if(test_step == 2){
         pts_gatt_cli_write(GATT_WRITE, hdl, 0, sizeof(uint16_t), (uint8_t *)&cli_cfg);
    }else if(test_step == 3){
        pts_gatt_unreg(PTS_GATT_ROLE_CLIENT);
        pts_disconnect();
    }else{
        TRACE(0, "[%s][%d]: ERROR without operation steps", __FUNCTION__, __LINE__);
    }
}

static pts_case_num_t le_gatt_cl_gac_bv_num[] = {
{1, pts_gatt_cl_gac_bv_01_c},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gad_bv_num[] = {
{1, pts_gatt_cl_gad_bv_01_c},
{2, pts_gatt_cl_gad_bv_02_c},
{3, pts_gatt_cl_gad_bv_03_c},
{4, pts_gatt_cl_gad_bv_04_c},
{5, pts_gatt_cl_gad_bv_05_c},
{6, pts_gatt_cl_gad_bv_06_c},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gar_bv_num[] = {
{1, pts_gatt_cl_gar_cli_read},
{3, pts_gatt_cl_gar_cli_read_by_uuid},
{4, pts_gatt_cl_gar_cli_read},
{5, pts_gatt_cl_gar_cli_multi_read},
{6, pts_gatt_cl_gar_cli_read},
{7, pts_gatt_cl_gar_cli_read},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gar_bi_num[] = {
{1, pts_gatt_cl_gar_cli_read},
{2, pts_gatt_cl_gar_cli_read},
{3, pts_gatt_cl_gar_cli_read},
{4, pts_gatt_cl_gar_cli_read},
{5, pts_gatt_cl_gar_cli_read},
{6, pts_gatt_cl_gar_cli_read_by_uuid},
{7, pts_gatt_cl_gar_cli_read_by_uuid},
{9, pts_gatt_cl_gar_cli_read_by_uuid},
{10, pts_gatt_cl_gar_cli_read_by_uuid},
{11, pts_gatt_cl_gar_cli_read_by_uuid},
{12, pts_gatt_cl_gar_cli_read},
{13, pts_gatt_cl_gar_cli_read},
{14, pts_gatt_cl_gar_cli_read},
{15, pts_gatt_cl_gar_cli_read},
{16, pts_gatt_cl_gar_cli_read},
{17, pts_gatt_cl_gar_cli_read},
{18, pts_gatt_cl_gar_cli_multi_read},
{19, pts_gatt_cl_gar_cli_multi_read},
{20, pts_gatt_cl_gar_cli_multi_read},
{21, pts_gatt_cl_gar_cli_multi_read},
{22, pts_gatt_cl_gar_cli_multi_read},
{35, pts_gatt_cl_gar_cli_read},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gaw_bv_num[] = {
{1, pts_gatt_cl_gaw_no_rsp},
{2, pts_gatt_cl_gaw_signed},
{3, pts_gatt_cl_gaw_write},
{5, pts_gatt_cl_gaw_write},
{6, pts_gatt_cl_gaw_reliable},
{8, pts_gatt_cl_gaw_write},
{9, pts_gatt_cl_gaw_write},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gaw_bi_num[] = {
{2, pts_gatt_cl_gaw_write},
{3, pts_gatt_cl_gaw_write},
{4, pts_gatt_cl_gaw_write},
{5, pts_gatt_cl_gaw_write},
{6, pts_gatt_cl_gaw_write},
{7, pts_gatt_cl_gaw_write},
{8, pts_gatt_cl_gaw_write},
{9, pts_gatt_cl_gaw_write},
{11, pts_gatt_cl_gaw_write},
{12, pts_gatt_cl_gaw_write},
{13, pts_gatt_cl_gaw_write},
{32, pts_gatt_cl_gaw_reliable},
{33, pts_gatt_cl_gaw_write},
{34, pts_gatt_cl_gaw_write},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gan_bv_num[] = {
{1, pts_gatt_cl_gan},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gai_bv_num[] = {
{1, pts_gatt_cl_gai},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gai_bi_num[] = {
{1, pts_gatt_cl_gai},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gas_bv_num[] = {
{1, pts_gatt_cl_gai},
{0xFF, NULL},
};

static pts_case_num_t le_gatt_cl_gat_bv_num[] = {
{1, pts_gatt_cl_gar_cli_read},
{2, pts_gatt_cl_gaw_write},
{0xFF, NULL},
};

pts_module_case_t pts_gatt_cl_case[] = {
{"GAC", le_gatt_cl_gac_bv_num, NULL},
{"GAD", le_gatt_cl_gad_bv_num, NULL},
{"GAR", le_gatt_cl_gar_bv_num, le_gatt_cl_gar_bi_num},
{"GAW", le_gatt_cl_gaw_bv_num, le_gatt_cl_gaw_bi_num},
{"GAN", le_gatt_cl_gan_bv_num, NULL},
{"GAI", le_gatt_cl_gai_bv_num, le_gatt_cl_gai_bi_num},
{"GAS", le_gatt_cl_gas_bv_num, NULL},
{"GAT", le_gatt_cl_gat_bv_num, NULL},
{NULL, NULL, NULL},
};

extern pts_module_case_t pts_gatt_sr_case[];
pts_sub_module_t pts_gatt_sub_modele[] = {
{"CL",  pts_gatt_cl_case},
{"SR",  pts_gatt_sr_case},
{NULL, NULL}
};

void pts_gatt_module_reg()
{
    TRACE(0,"%s",__func__);
    ble_host_pts_module_reg("GATT", pts_gatt_sub_modele);
}

