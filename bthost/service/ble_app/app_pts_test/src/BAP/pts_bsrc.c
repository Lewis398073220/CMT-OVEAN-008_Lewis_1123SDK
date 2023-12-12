#include <stdio.h>
#include "pts_ble_host_app.h"
#include "pts_ble_host_interface.h"
#include "app_bap_bc_src_msg.h"
#include "app_gaf_custom_api.h"

#define PTS_BSRC_LOCATION_BF            (APP_GAF_LOC_SIDE_LEFT_BIT | APP_GAF_LOC_SIDE_RIGHT_BIT)
#define PTS_BSRC_PHY_BF                 APP_PHY_2MBPS_BIT
#define PTS_BSRC_PACKING_TYPE           APP_ISO_PACKING_SEQUENTIAL

/*BAP.TS.p2 A.6 */
/*{uint32_t location_bf ,uint16_t frame_octet ,uint8_t sampling_freq ,uint8_t frame_dur , uint8_t frames_sdu}*/
app_bap_cfg_param_t lc3_codec_spec_config[16]={{PTS_BSRC_LOCATION_BF,26 ,1,0,0}, //LC3 8_1
                                               {PTS_BSRC_LOCATION_BF,30 ,1,1,0}, //LC3 8_2
                                               {PTS_BSRC_LOCATION_BF,30 ,3,0,0}, //LC3 16_1
                                               {PTS_BSRC_LOCATION_BF,40 ,3,1,0}, //LC3 16_2
                                               {PTS_BSRC_LOCATION_BF,45 ,5,0,0}, //LC3 24_1
                                               {PTS_BSRC_LOCATION_BF,60 ,5,1,0}, //LC3 24_2
                                               {PTS_BSRC_LOCATION_BF,60 ,6,0,0}, //LC3 32_1
                                               {PTS_BSRC_LOCATION_BF,80 ,6,1,0}, //LC3 32_2
                                               {PTS_BSRC_LOCATION_BF,97 ,7,0,0}, //LC3 441_1
                                               {PTS_BSRC_LOCATION_BF,130,7,1,0}, //LC3 441_2
                                               {PTS_BSRC_LOCATION_BF,75 ,8,0,0}, //LC3 48_1
                                               {PTS_BSRC_LOCATION_BF,100,8,1,0}, //LC3 48_2
                                               {PTS_BSRC_LOCATION_BF,90 ,8,0,0}, //LC3 48_3
                                               {PTS_BSRC_LOCATION_BF,120,8,1,0}, //LC3 48_4
                                               {PTS_BSRC_LOCATION_BF,117,8,0,0}, //LC3 48_5
                                               {PTS_BSRC_LOCATION_BF,155,8,1,0}}; //LC3 48_6

/*BAP.TS.p2 A.7 */
/*{uint32_t sdu_intv_us ,uint16_t max_sdu ,uint16_t max_tlatency_ms ,uint8_t packing ,uint8_t framing , uint8_t phy_bf ,uint8_t rtn}*/
app_bap_bc_grp_param_t lc3_audio_stream_config[32]={{7500 ,26 ,8 ,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //8_1_1
                                                    {10000,30 ,10,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //8_2_1
                                                    {7500 ,30 ,8 ,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //16_1_1
                                                    {10000,40 ,10,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //16_2_1
                                                    {7500 ,45 ,8 ,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //24_1_1
                                                    {10000,60 ,10,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //24_2_1
                                                    {7500 ,60 ,8 ,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //32_1_1
                                                    {10000,80 ,10,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,2}, //32_2_1
                                                    {8163 ,97 ,24,PTS_BSRC_PACKING_TYPE,1,PTS_BSRC_PHY_BF,4}, //441_1_1
                                                    {10884,130,31,PTS_BSRC_PACKING_TYPE,1,PTS_BSRC_PHY_BF,4}, //441_2_1
                                                    {7500 ,75 ,15,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_1_1
                                                    {10000,100,20,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_2_1
                                                    {7500 ,90 ,15,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_3_1
                                                    {10000,120,20,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_4_1
                                                    {7500 ,117,15,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_5_1
                                                    {10000,155,20,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_6_1
                                                    {7500 ,26 ,45,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //8_1_2
                                                    {10000,30 ,60,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //8_2_2
                                                    {7500 ,30 ,45,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //16_1_2
                                                    {10000,40 ,60,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //16_2_2
                                                    {7500 ,45 ,45,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //24_1_2
                                                    {10000,60 ,60,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //24_2_2
                                                    {7500 ,60 ,45,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //32_1_2
                                                    {10000,80 ,60,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //32_2_2
                                                    {8163 ,97 ,54,PTS_BSRC_PACKING_TYPE,1,PTS_BSRC_PHY_BF,4}, //441_1_2
                                                    {10884,130,60,PTS_BSRC_PACKING_TYPE,1,PTS_BSRC_PHY_BF,4}, //441_2_2
                                                    {7500 ,75 ,50,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_1_2
                                                    {10000,100,65,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_2_2
                                                    {7500 ,90 ,50,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_3_2
                                                    {10000,120,65,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_4_2
                                                    {7500 ,117,50,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}, //48_5_2
                                                    {10000,155,65,PTS_BSRC_PACKING_TYPE,0,PTS_BSRC_PHY_BF,4}}; //48_6_2

static void pts_bap_bsrc_scc_bv(uint8_t case_id, int step)
{
    uint8_t codec_idx = 0;
    uint8_t stream_idx = 0;

    if(case_id<17){
         codec_idx = case_id-1;
         stream_idx = case_id-1;
    }else if(case_id<33){
         codec_idx = case_id-17;
         stream_idx = case_id-1;
    }

    TRACE(0,"[PTS_TEST] codec_idx=%d ,stream_idx=%d \n",codec_idx,stream_idx);
    app_bap_bc_src_grp_info_t *p_grp_t = app_bap_bc_src_get_big_info_by_big_idx(0);
    app_bap_bc_src_sgrp_t *subgrp = &p_grp_t->p_sgrp_infos[0];
    app_bap_bc_src_stream_t *stream = &p_grp_t->stream_info[0];
    memcpy(&subgrp->p_cfg->param,&lc3_codec_spec_config[codec_idx],sizeof(app_bap_cfg_param_t));
    memcpy(&p_grp_t->grp_param,&lc3_audio_stream_config[stream_idx],sizeof(app_bap_bc_grp_param_t));
    memcpy(&stream->p_cfg->param,&lc3_codec_spec_config[codec_idx],sizeof(app_bap_cfg_param_t));

    if(step == 1) {
        app_bap_bc_src_disable(p_grp_t->grp_lid);
    }else if(step == 2){
        app_bap_bc_src_remove_group_cmd(p_grp_t->grp_lid);
    }
}

static void pts_bap_bsrc_scc_bv_01_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 1;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_02_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 2;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_03_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 3;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_04_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 4;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_05_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 5;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_06_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 6;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_07_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 7;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_08_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 8;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_09_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 9;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_10_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 10;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_11_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 11;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_12_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 12;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_13_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 13;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_14_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 14;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_15_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 15;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_16_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 16;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_17_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 17;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_18_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 18;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_19_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 19;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_20_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 20;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_21_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 21;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_22_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 22;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_23_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 23;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_24_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 24;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_25_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 25;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_26_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 26;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_27_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 27;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_28_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 28;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_29_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 29;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_30_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 30;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_31_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 31;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_32_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 32;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_33_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 33;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_34_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 34;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_35_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 35;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_scc_bv_36_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 36;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_scc_bv(case_id,steps);
}

static void pts_bap_bsrc_str_multibis_bv(uint8_t case_id, int step)
{
    uint8_t codec_idx = 0;
    uint8_t stream_idx = 0;

    codec_idx = case_id-18;

    TRACE(0,"[PTS_TEST] codec_idx=%d ,stream_idx=%d \n",codec_idx,stream_idx);
    app_bap_bc_src_grp_info_t *p_grp_t = app_bap_bc_src_get_big_info_by_big_idx(0);
    app_bap_bc_src_sgrp_t *subgrp = &p_grp_t->p_sgrp_infos[0];
    memcpy(&subgrp->p_cfg->param,&lc3_codec_spec_config[codec_idx],sizeof(app_bap_cfg_param_t));
    memcpy(&p_grp_t->grp_param,&lc3_audio_stream_config[stream_idx],sizeof(app_bap_bc_grp_param_t));
    for (uint8_t stream_lid = 0; stream_lid < p_grp_t->nb_streams; stream_lid++)
    {
        app_bap_bc_src_stream_t *stream = &p_grp_t->stream_info[stream_lid];
        memcpy(&stream->p_cfg->param,&lc3_codec_spec_config[codec_idx],sizeof(app_bap_cfg_param_t));
    }

    if(step == 1) {
        app_bap_bc_src_disable(p_grp_t->grp_lid);
    }else if(step == 2){
        app_bap_bc_src_remove_group_cmd(p_grp_t->grp_lid);
    }
}

static void pts_bap_bsrc_str_bv_17_c(char *para_buf, uint32_t str_len)
{
    return ;/* NO test case*/
}

static void pts_bap_bsrc_str_bv_18_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 18;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_19_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 19;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_20_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 20;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_21_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 21;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_22_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 22;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_23_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 23;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_24_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 24;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_25_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 25;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_26_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 26;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_27_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 27;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_28_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 28;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_29_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 29;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_30_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 30;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_31_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 31;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_32_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 32;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static void pts_bap_bsrc_str_bv_33_c(char *para_buf, uint32_t str_len)
{
    uint8_t case_id = 33;
    int steps = 0;

    if((para_buf) && (str_len)){
        TRACE(0, "[%s][%d]para: %s", __FUNCTION__, __LINE__, (char *)para_buf);
        sscanf((char *)para_buf, "%d", &steps);
    }
    pts_bap_bsrc_str_multibis_bv(case_id,steps);
}

static pts_case_num_t bis_scc_bv_num[] = {
{1, pts_bap_bsrc_scc_bv_01_c},
{2, pts_bap_bsrc_scc_bv_02_c},
{3, pts_bap_bsrc_scc_bv_03_c},
{4, pts_bap_bsrc_scc_bv_04_c},
{5, pts_bap_bsrc_scc_bv_05_c},
{6, pts_bap_bsrc_scc_bv_06_c},
{7, pts_bap_bsrc_scc_bv_07_c},
{8, pts_bap_bsrc_scc_bv_08_c},
{9, pts_bap_bsrc_scc_bv_09_c},
{10, pts_bap_bsrc_scc_bv_10_c},
{11, pts_bap_bsrc_scc_bv_11_c},
{12, pts_bap_bsrc_scc_bv_12_c},
{13, pts_bap_bsrc_scc_bv_13_c},
{14, pts_bap_bsrc_scc_bv_14_c},
{15, pts_bap_bsrc_scc_bv_15_c},
{16, pts_bap_bsrc_scc_bv_16_c},
{17, pts_bap_bsrc_scc_bv_17_c},
{18, pts_bap_bsrc_scc_bv_18_c},
{19, pts_bap_bsrc_scc_bv_19_c},
{20, pts_bap_bsrc_scc_bv_20_c},
{21, pts_bap_bsrc_scc_bv_21_c},
{22, pts_bap_bsrc_scc_bv_22_c},
{23, pts_bap_bsrc_scc_bv_23_c},
{24, pts_bap_bsrc_scc_bv_24_c},
{25, pts_bap_bsrc_scc_bv_25_c},
{26, pts_bap_bsrc_scc_bv_26_c},
{27, pts_bap_bsrc_scc_bv_27_c},
{28, pts_bap_bsrc_scc_bv_28_c},
{29, pts_bap_bsrc_scc_bv_29_c},
{30, pts_bap_bsrc_scc_bv_30_c},
{31, pts_bap_bsrc_scc_bv_31_c},
{32, pts_bap_bsrc_scc_bv_32_c},
{33, pts_bap_bsrc_scc_bv_33_c},
{34, pts_bap_bsrc_scc_bv_34_c},
{35, pts_bap_bsrc_scc_bv_35_c},
{36, pts_bap_bsrc_scc_bv_36_c},
{36, pts_bap_bsrc_scc_bv_36_c},
{0xFF, NULL},
};

static pts_case_num_t bis_str_bv_num[] = {
{1, pts_bap_bsrc_scc_bv_01_c},
{2, pts_bap_bsrc_scc_bv_02_c},
{3, pts_bap_bsrc_scc_bv_03_c},
{4, pts_bap_bsrc_scc_bv_04_c},
{5, pts_bap_bsrc_scc_bv_05_c},
{6, pts_bap_bsrc_scc_bv_06_c},
{7, pts_bap_bsrc_scc_bv_07_c},
{8, pts_bap_bsrc_scc_bv_08_c},
{9, pts_bap_bsrc_scc_bv_09_c},
{10, pts_bap_bsrc_scc_bv_10_c},
{11, pts_bap_bsrc_scc_bv_11_c},
{12, pts_bap_bsrc_scc_bv_12_c},
{13, pts_bap_bsrc_scc_bv_13_c},
{14, pts_bap_bsrc_scc_bv_14_c},
{15, pts_bap_bsrc_scc_bv_15_c},
{16, pts_bap_bsrc_scc_bv_16_c},
{17, pts_bap_bsrc_str_bv_17_c},
{18, pts_bap_bsrc_str_bv_18_c},
{19, pts_bap_bsrc_str_bv_19_c},
{20, pts_bap_bsrc_str_bv_20_c},
{21, pts_bap_bsrc_str_bv_21_c},
{22, pts_bap_bsrc_str_bv_22_c},
{23, pts_bap_bsrc_str_bv_23_c},
{24, pts_bap_bsrc_str_bv_24_c},
{25, pts_bap_bsrc_str_bv_25_c},
{26, pts_bap_bsrc_str_bv_26_c},
{27, pts_bap_bsrc_str_bv_27_c},
{28, pts_bap_bsrc_str_bv_28_c},
{29, pts_bap_bsrc_str_bv_29_c},
{30, pts_bap_bsrc_str_bv_30_c},
{31, pts_bap_bsrc_str_bv_31_c},
{32, pts_bap_bsrc_str_bv_32_c},
{33, pts_bap_bsrc_str_bv_33_c},
{0xFF, NULL},
};

pts_module_case_t pts_bsrc_case[] = {
{"SCC", bis_scc_bv_num, NULL},
{"STR", bis_str_bv_num, NULL},
{NULL, NULL, NULL},
};


