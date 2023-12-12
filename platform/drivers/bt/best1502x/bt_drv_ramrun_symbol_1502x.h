
/*******************************************************************************************************************************/
/*********************************************bt controller symbol**************************************************************/

#ifndef  __BT_CONTROLLER_SYMBOL_H__
#define  __BT_CONTROLLER_SYMBOL_H__


#define HCI_FC_ENV_ADDR                                               0XC000724C
#define LD_ACL_ENV_ADDR                                               0XC0005108
#define BT_UTIL_BUF_ENV_ADDR                                          0XC0004600
#define BLE_UTIL_BUF_ENV_ADDR                                         0XC000533C
#define LD_BES_BT_ENV_ADDR                                            0XC00052A4
#define DBG_STATE_ADDR                                                0XC0007340
#define LC_STATE_ADDR                                                 0XC0004C70
#define LD_SCO_ENV_ADDR                                               0XC0005124
#define RX_MONITOR_ADDR                                               0XC0007B5C
#define LC_ENV_ADDR                                                   0XC0004C5C
#define LM_NB_SYNC_ACTIVE_ADDR                                        0XC0004C56
#define LM_ENV_ADDR                                                   0XC0004904
#define LM_KEY_ENV_ADDR                                               0XC0004A74
#define HCI_ENV_ADDR                                                  0XC000716C
#define LC_SCO_ENV_ADDR                                               0XC0004C38
#define LLM_ENV_ADDR                                                  0XC0005860
#define LD_ENV_ADDR                                                   0XC0004D7C
#define RWIP_ENV_ADDR                                                 0XC0007CD0
#define BLE_RX_MONITOR_ADDR                                           0XC0007B04
#define LLC_ENV_ADDR                                                  0XC0006600
#define RWIP_RF_ADDR                                                  0XC0007C68
#define LD_ACL_METRICS_ADDR                                           0XC0004DC0
#define RF_RX_HWGAIN_TBL_ADDR                                         0XC00043A6
#define RF_RX_GAIN_FIXED_TBL_ADDR                                     0XC0007ADE
#define HCI_DBG_EBQ_TEST_MODE_ADDR                                    0XC0007438
#define DBG_BT_COMMON_SETTING_ADDR                                    0XC000739C
#define DBG_BT_SCHE_SETTING_ADDR                                      0XC0007468
#define DBG_BT_IBRT_SETTING_ADDR                                      0XC00072C6
#define DBG_BT_HW_FEAT_SETTING_ADDR                                   0XC0007370
#define HCI_DBG_SET_SW_RSSI_ADDR                                      0XC0007490
#define LP_CLK_ADDR                                                   0XC0007CCC
#define RWIP_PROG_DELAY_ADDR                                          0XC0007CC9
#define DATA_BACKUP_CNT_ADDR                                          0XC0004080
#define DATA_BACKUP_ADDR_PTR_ADDR                                     0XC0004084
#define DATA_BACKUP_VAL_PTR_ADDR                                      0XC0004088
#define SCH_MULTI_IBRT_ADJUST_ENV_ADDR                                0XC0006FA0
#define RF_RX_GAIN_THS_TBL_LE_ADDR                                    0XC000436A
#define RF_RX_GAIN_THS_TBL_LE_2M_ADDR                                 0XC0004388
#define RF_RPL_TX_PW_CONV_TBL_ADDR                                    0XC00042D4
#define REPLACE_MOBILE_ADDR_ADDR                                      0XC0004CD0
#define REPLACE_ADDR_VALID_ADDR                                       0XC0004586
#define PCM_NEED_START_FLAG_ADDR                                      0XC00045C0
#define RT_SLEEP_FLAG_CLEAR_ADDR                                      0XC00045F0
#define RF_RX_GAIN_THS_TBL_BT_3M_ADDR                                 0XC0007AC0
#define NORMAL_IQTAB_ADDR                                             0XC0007B2C
#define NORMAL_IQTAB_EN_ADDR                                          0XC00045D8
#define POWER_ADJUST_EN_ADDR                                          0XC00045DC
#define LD_IBRT_ENV_ADDR                                              0XC00052B8
#define LLM_LOCAL_LE_FEATS_ADDR                                       0XC0004290
#define ISOOHCI_ENV_ADDR                                              0XC0006FE4
#define RF_RX_GAIN_THS_TBL_BT_ADDR                                    0XC0004310
#define DBG_BT_COMMON_SETTING_T2_ADDR                                 0XC00073EA
#define LLD_CON_ENV_ADDR                                              0XC0006A44
#define POWER_ADJUST_EN_ADDR                                          0XC00045DC
#define LLD_ISO_ENV_ADDR                                              0XC0006A84
#define ECC_RX_MONITOR_ADDR                                           0XC0007AB8
#define __STACKLIMIT_ADDR                                             0XC00088B8
#define PER_MONITOR_PARAMS_ADDR                                       0XC0004282
#define TX_POWER_VAL_BKUP_ADDR                                        0XC0007C18
#define DBG_ENV_ADDR                                                  0XC0007500
#define RF_RX_GAIN_THS_TBL_ECC_ADDR                                   0XC000434C
#define MASTER_CON_SUPPORT_LE_AUDIO_ADDR                              0XC0004584
#define DBG_BT_COMMON_SETTING_T2_ADDR                                 0XC00073EA
#define RX_RECORD_ADDR                                                0XC0005168
#define SCH_PROG_DBG_ENV_ADDR                                         0XC0006CC4
#define LLD_PER_ADV_ENV_ADDR                                          0XC00069AC
#define HCI_DBG_BLE_ADDR                                              0XC000732C
#define LLD_ENV_ADDR                                                  0XC0006898
#define RWIP_RST_STATE_ADDR                                           0XC0007CC8
#define SENS2BT_EN_ADDR                                               0XC00045ED

//commit 9eb84944744a725e12e5a262efef16eba5b720bb
//Author: TianxuWu <tianxuwu@bestechnic.com>
//Date:   Fri Nov 17 13:53:12 2023 +0800
//    Fix lld con duration modified cause arb time mismatch issue.
//    
//    Change-Id: I16f6610087ba0647446d1f83056d2280b4ce7459

#ifndef  BT_CONTROLLER_COMMIT_ID
#define  BT_CONTROLLER_COMMIT_ID                            "commit 9eb84944744a725e12e5a262efef16eba5b720bb"
#endif
#ifndef  BT_CONTROLLER_COMMIT_AUTHOR
#define  BT_CONTROLLER_COMMIT_AUTHOR                        "Author: TianxuWu <tianxuwu@bestechnic.com>"
#endif
#ifndef  BT_CONTROLLER_COMMIT_DATE
#define  BT_CONTROLLER_COMMIT_DATE                          "Date:   Fri Nov 17 13:53:12 2023 +0800"
#endif
#ifndef  BT_CONTROLLER_COMMIT_MESSAGE
#define  BT_CONTROLLER_COMMIT_MESSAGE                       "Fix lld con duration modified cause arb time mismatch issue.  Change-Id: I16f6610087ba0647446d1f83056d2280b4ce7459 "
#endif
#ifndef  BT_CONTROLLER_BUILD_TIME
#define  BT_CONTROLLER_BUILD_TIME                           "2023-11-17 14:06:55"
#endif
#ifndef  BT_CONTROLLER_BUILD_OWNER
#define  BT_CONTROLLER_BUILD_OWNER                          "TianxuWu"
#endif

#endif
