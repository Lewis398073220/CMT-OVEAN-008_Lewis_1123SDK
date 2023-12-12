#ifndef _RWAPP_CONFIG_H_
#define _RWAPP_CONFIG_H_
/**
 ****************************************************************************************
 * @addtogroup app
 * @brief Application configuration definition
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/******************************************************************************************/
/* -------------------------   BLE APPLICATION SETTINGS      -----------------------------*/
/******************************************************************************************/
#define FAST_PAIR_REV_2_0   1
#define BLE_APP_GFPS_VER    FAST_PAIR_REV_2_0

/// Mesh Application
#if defined(CFG_APP_MESH)
#define BLE_APP_MESH         1
#else
#define BLE_APP_MESH         0
#endif

/// Application Profile
#if defined(CFG_APP_PRF)
#define BLE_APP_PRF          1
#else
#define BLE_APP_PRF          0
#endif

#ifndef CFG_APP_SEC
#define CFG_APP_SEC
#endif

/// GFPS enabled
#ifdef GFPS_ENABLED
#if BLE_APP_GFPS_VER==FAST_PAIR_REV_2_0
    #define CFG_APP_GFPS
    #ifndef CFG_APP_SEC
    #define CFG_APP_SEC
    #endif
#else
    #undef CFG_APP_GFPS
#endif
#endif

#if defined(BISTO_ENABLED)||defined(VOICE_DATAPATH)|| \
    defined(CTKD_ENABLE)||defined(__AI_VOICE_BLE_ENABLE__)
#ifndef CFG_APP_SEC
#define CFG_APP_SEC
#endif
#endif

#if defined(BISTO_ENABLED)||defined(__AI_VOICE_BLE_ENABLE__)|| \
    defined(CTKD_ENABLE)||defined(GFPS_ENABLED)||(BLE_AUDIO_ENABLED)
#ifndef BLE_USB_AUDIO_USE_LE_LAGACY_NO_SECURITY_CON
#ifndef CFG_SEC_CON
#define CFG_SEC_CON
#endif
#endif
#endif

/// TILE Data Path Application
#ifdef TILE_DATAPATH
#define BLE_APP_TILE           1
#else
#define BLE_APP_TILE           0
#endif

/// OTA Application
#ifdef BES_OTA
#if !defined(OTA_OVER_TOTA_ENABLED)
#define BLE_APP_OTA           1
#else
#define BLE_APP_OTA           0
#endif
#else
#define BLE_APP_OTA           0
#endif

/// TOTA Application
#ifdef BLE_TOTA_ENABLED
#define BLE_APP_TOTA           1
#ifdef OTA_OVER_TOTA_ENABLED
#define BLE_APP_OTA_OVER_TOTA  1
#else
#define BLE_APP_OTA_OVER_TOTA  0
#endif
#else
#define BLE_APP_OTA_OVER_TOTA  0
#define BLE_APP_TOTA           0
#endif

/// AI_VOICE
#ifdef __AI_VOICE_BLE_ENABLE__
#define CFG_APP_AI_VOICE
#endif

/// HID Application
#if defined(BLE_HID_ENABLE)
#define BLE_APP_HID          1
#ifndef CFG_APP_SEC
#define CFG_APP_SEC
#endif
#else
#define BLE_APP_HID          0
#endif

/// Battery Service Application
#if defined(BLE_BATT_ENABLE)
#define BLE_APP_BATT          1
#else
#define BLE_APP_BATT          0
#endif

/// DIS Application
#if defined(BLE_DISS_ENABLE)
#define BLE_APP_DIS          1
#else
#define BLE_APP_DIS          0
#endif

/// CHIP_FPGA1000 Application
#ifdef CHIP_FPGA1000
#ifndef CFG_APP_SEC
#ifdef ENABLE_BUD_TO_BUD_COMMUNICATION
#define CFG_APP_SECx
#else
#define CFG_APP_SEC
#endif
#endif
#endif

/// Health Thermometer Application
#if defined(CFG_APP_HT)
#define BLE_APP_HT           1
#else
#define BLE_APP_HT           0
#endif

/// HR Application
#if defined(CFG_APP_HR)
#define BLE_APP_HR           1
#else
#define BLE_APP_HR           0
#endif

/// Data Path Server Application
#if defined(CFG_APP_DATAPATH_SERVER)
#define BLE_APP_DATAPATH_SERVER           1
#else
#define BLE_APP_DATAPATH_SERVER           0
#endif

/// Advanced Headphone Server Application
#if defined(CFG_APP_AHP_SERVER)
#define BLE_APP_AHP_SERVER    1
#else
#define BLE_APP_AHP_SERVER    0
#endif

/// Switching Ambient Service Appliaction
#if defined(CFG_APP_SAS_SERVER)
#define BLE_APP_SAS_SERVER           1
#else
#define BLE_APP_SAS_SERVER           0
#endif

/// Data Path client Application
#if defined(CFG_APP_DATAPATH_CLIENT)
#define BLE_APP_DATAPATH_CLIENT           1
#else
#define BLE_APP_DATAPATH_CLIENT           0
#endif

/// Battery Service Application
#if defined(CFG_APP_BAS)
#define BLE_APP_BATT          1
#else
#define BLE_APP_BATT          0
#endif

/// DIS Application
#if defined(CFG_APP_DIS)
#define BLE_APP_DIS          1
#else // defined(CFG_APP_DIS)
#define BLE_APP_DIS          0
#endif // defined(CFG_APP_DIS)

/// Time Application
#if defined(CFG_APP_TIME)
#define BLE_APP_TIME         1
#else
#define BLE_APP_TIME         0
#endif

/// Security Application
#if (defined(CFG_APP_SEC) || BLE_APP_HID || defined(BLE_APP_AM0))
#define BLE_APP_SEC          1
#else
#define BLE_APP_SEC          0
#endif

/// A2DP Demo
#if defined(CFG_APP_A2DP)
#define APP_A2DP      1
#else
#define APP_A2DP      0
#endif

/// ANCC Application
#if defined(ANCC_ENABLED)
#define BLE_APP_ANCC    1
#else
#define BLE_APP_ANCC    0
#endif

/// ANCS Application
#if defined(ANCS_ENABLED)
#define BLE_APP_ANCS    1
#else
#define BLE_APP_ANCS    0
#endif

/// AMS Application
#if defined(AMS_ENABLED)
#define BLE_APP_AMS    1
#else
#define BLE_APP_AMS    0
#endif

/// AMSC Application
#if defined(AMSC_ENABLED)
#define BLE_APP_AMSC   1
#else
#define BLE_APP_AMSC   0
#endif

/// BMS Application
#if defined(BMS_ENABLED)
#define BLE_APP_BMS    1
#else
#define BLE_APP_BMS    0
#endif

/// GFPS Application
#if defined(CFG_APP_GFPS)
#define BLE_APP_GFPS          1
#else
#define BLE_APP_GFPS          0
#endif

#ifdef SWIFT_ENABLED
#define BLE_APP_SWIFT         1
#else
#define BLE_APP_SWIFT         0
#endif

/// AMA Voice Application
#if defined(CFG_APP_AI_VOICE)
#if defined(__AMA_VOICE__)
#define BLE_APP_AMA_VOICE    1
#endif
#if defined(__DMA_VOICE__)
#define BLE_APP_DMA_VOICE    1
#endif
#if defined(__GMA_VOICE__)
#define BLE_APP_GMA_VOICE    1
#endif
#if defined(__SMART_VOICE__)
#define BLE_APP_SMART_VOICE    1
#endif
#if defined(__TENCENT_VOICE__)
#define BLE_APP_TENCENT_VOICE    1
#endif
#if defined(__CUSTOMIZE_VOICE__)
#define BLE_APP_CUSTOMIZE_VOICE    1
#endif
#define BLE_APP_AI_VOICE           1
#else // defined(CFG_APP_AMA)
#if defined(__AMA_VOICE__)
#define BLE_APP_AMA_VOICE    0
#endif
#if defined(__DMA_VOICE__)
#define BLE_APP_DMA_VOICE    0
#endif
#if defined(__GMA_VOICE__)
#define BLE_APP_GMA_VOICE    0
#endif
#if defined(__SMART_VOICE__)
#define BLE_APP_SMART_VOICE    0
#endif
#if defined(__TENCENT_VOICE__)
#define BLE_APP_TENCENT_VOICE    0
#endif
#if defined(__CUSTOMIZE_VOICE__)
#define BLE_APP_CUSTOMIZE_VOICE    0
#endif
#define BLE_APP_AI_VOICE           0
#endif // defined(CFG_APP_AMA)

/// @} rwapp_config

#endif /* _RWAPP_CONFIG_H_ */