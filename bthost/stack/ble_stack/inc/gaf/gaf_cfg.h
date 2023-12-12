/**
 ****************************************************************************************
 *
 * @file gaf_cfg.h
 *
 * @brief Generic Audio Framework - Configuration
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef GAF_CFG_H_
#define GAF_CFG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"            // RW IP Configuration
#include <stdint.h>                 // Standard Integer Definitions
#include <stdbool.h>                // Standard Boolean definitions
#include "co_math.h"                // Common Math Functions Definitions

/*
 * COMPILATION FLAGS
 ****************************************************************************************
 */
#if BLE_AUDIO_ENABLED
#define CFG_GAF_API_MSG
#define CFG_GAF_DBG
#define CFG_GAF_ADV
#define CFG_GAF_SCAN
#define CFG_GAF_CLI

// IAP supported
#ifdef BLE_STACK_PORTING_CHANGES
#define CFG_IAP_INTF_NB (3)
#else
#define CFG_IAP_INTF_NB (2)
#endif

// ARC supported
#define CFG_ARC_VCS
#define CFG_ARC_MICS
#define CFG_ARC_VOCS
#define CFG_ARC_AICS

// ACC supported
#define CFG_ACC_MCS
#define CFG_ACC_TBS
#define CFG_ACC_OTS
#define CFG_ACC_DTS

// ATC supported
//#define CFG_ATC_RAAS
#define CFG_ATC_CSISM
#define CFG_CAP_CAC

// BAP supported
#define CFG_BAP_UC_SRV
#define CFG_BAP_CAPA_SRV

#if defined (CFG_BAP_BC)
#define CFG_BAP_BC_SRC
#define CFG_BAP_BC_SCAN
#define CFG_BAP_BC_DELEG
#define CFG_BAP_BC_ASSIST
#endif

// ARC supported
#define CFG_ARC_VCC
#define CFG_ARC_MICC
#define CFG_ARC_VOCC
#define CFG_ARC_AICC

// ACC supported
#define CFG_ACC_MCC
#define CFG_ACC_TBC
#define CFG_ACC_OTC
#define CFG_ACC_DTC

// ATC supported
//#define CFG_ATC_RAAC
#define CFG_ATC_CSISC
#define CFG_CAP_CAS

// BAP supported
#define CFG_BAP_UC_CLI
#define CFG_BAP_CAPA_CLI
#define CFG_BAP_BC_SINK

#define CFG_TMAP_TMAS
#define CFG_TMAC

#define CFG_HAP_HAS
#define CFG_HAP_HAC

#define CFG_GMAP_GMAS
#define CFG_GMAP_GMAC

#else
#define CFG_IAP_INTF_NB (0)
#endif


#if (BLE_EMB_PRESENT)
/// Number of streams that can be created using IAP
#define IAP_NB_STREAMS           (BLE_ISO_STREAM_MAX)
#else //(BLE_EMB_PRESENT)
/// Number of streams that can be created using IAP
#define IAP_NB_STREAMS           (4)
#endif //(BLE_EMB_PRESENT)

/// Unicast mode supported
#if defined(CFG_CIS)
#define GAF_UNICAST_SUPP         (1)
#else //(defined(CFG_CIS))
#define GAF_UNICAST_SUPP         (0)
#endif //(defined(CFG_CIS))

/// Broadcast mode supported
#if defined(CFG_BIS)
#define GAF_BROADCAST_SUPP       (1)
#if (BLE_OBSERVER)
#define GAF_BROADCAST_SLV_SUPP   (1)
#else
#define GAF_BROADCAST_SLV_SUPP   (0)
#endif //(BLE_OBSERVER)
#if (BLE_BROADCASTER)
#define GAF_BROADCAST_MST_SUPP   (1)
#else
#define GAF_BROADCAST_MST_SUPP   (0)
#endif //(BLE_OBSERVER)
#else //(defined(CFG_BIS))
#define GAF_BROADCAST_SUPP       (0)
#endif //(defined(CFG_BIS))

/// GAF Advertiser module supported
#if defined(CFG_GAF_ADV)
#define GAF_ADV                  (1)
#else
#define GAF_ADV                  (0)
#endif //(defined(CFG_GAF_ADV))
/// GAF Scanner module supported
#if defined(CFG_GAF_SCAN)
#define GAF_SCAN                 (1)
#else
#define GAF_SCAN                 (0)
#endif //(defined(CFG_GAF_SCAN))
/// GAF Client module supported
#if defined(CFG_GAF_CLI)
#define GAF_CLI                  (1)
#else
#define GAF_CLI                  (0)
#endif //(defined(CFG_GAF_CLI))

/// GAF Library modules supported
#if (GAF_ADV || GAF_SCAN || GAF_CLI)
#define GAF_LIB                  (1)
#else
#define GAF_LIB                  (0)
#endif //(GAF_ADV || GAF_SCAN || GAF_CLI)

/// IAP supported
#if (CFG_IAP_INTF_NB != 0)
#define GAF_IAP                  (1)
#else
#define GAF_IAP                  (0)
#endif //(CFG_IAP_INTF_NB != 0)

/// ARC supported
#if defined(CFG_ARC_VCS) || defined(CFG_ARC_VCC) || defined(CFG_ARC_MICS) || defined(CFG_ARC_MICC) || \
    defined(CFG_ARC_AICS) || defined(CFG_ARC_AICC) || defined(CFG_ARC_VOCS) || defined(CFG_ARC_VOCC)
#define GAF_ARC                  (1)

#if defined(CFG_ARC_VCS)
#define GAF_ARC_VCS              (1)
#else //(defined(CFG_ARC_VCS))
#define GAF_ARC_VCS              (0)
#endif //(defined(CFG_ARC_VCS))

#if defined(CFG_ARC_VCC)
#define GAF_ARC_VCC              (1)
#else //(defined(CFG_ARC_VCC))
#define GAF_ARC_VCC              (0)
#endif //(defined(CFG_ARC_VCC))

#if defined(CFG_ARC_MICS)
#define GAF_ARC_MICS             (1)
#else //(defined(CFG_ARC_MICS))
#define GAF_ARC_MICS             (0)
#endif //(defined(CFG_ARC_MICS))

#if defined(CFG_ARC_MICC)
#define GAF_ARC_MICC             (1)
#else //(defined(CFG_ARC_MICC))
#define GAF_ARC_MICC             (0)
#endif //(defined(CFG_ARC_MICC))

#if defined(CFG_ARC_AICS)
#define GAF_ARC_AICS             (1)
#else //(defined(CFG_ARC_AICS))
#define GAF_ARC_AICS             (0)
#endif //(defined(CFG_ARC_AICS))

#if defined(CFG_ARC_AICC)
#define GAF_ARC_AICC             (1)
#else //(defined(CFG_ARC_AICC))
#define GAF_ARC_AICC             (0)
#endif //(defined(CFG_ARC_AICC))

#if defined(CFG_ARC_VOCS)
#define GAF_ARC_VOCS             (1)
#else //(defined(CFG_ARC_VOCS))
#define GAF_ARC_VOCS             (0)
#endif //(defined(CFG_ARC_VOCS))

#if defined(CFG_ARC_VOCC)
#define GAF_ARC_VOCC             (1)
#else //(defined(CFG_ARC_VOCC))
#define GAF_ARC_VOCC             (0)
#endif //(defined(CFG_ARC_VOCC))

#if (GAF_ARC_VCS || GAF_ARC_MICS || GAF_ARC_AICS || GAF_ARC_VOCS)
#define GAF_ARC_SRV              (1)
#else //(GAF_ARC_VCS || GAF_ARC_MICS || GAF_ARC_AICS || GAF_ARC_VOCS)
#define GAF_ARC_SRV              (0)
#endif //(GAF_ARC_VCS || GAF_ARC_MICS || GAF_ARC_AICS || GAF_ARC_VOCS)

#if (GAF_ARC_VCC || GAF_ARC_MICC || GAF_ARC_AICC || GAF_ARC_VOCC)
#define GAF_ARC_CLI              (1)
#else //(GAF_ARC_VCC || GAF_ARC_MICC || GAF_ARC_AICC || GAF_ARC_VOCC)
#define GAF_ARC_CLI              (0)
#endif //(GAF_ARC_VCC || GAF_ARC_MICC || GAF_ARC_AICC || GAF_ARC_VOCC)
#else //(defined(CFG_ARC_VCS) || defined(CFG_ARC_VCC) ...)
#define GAF_ARC                  (0)
#define GAF_ARC_SRV              (0)
#define GAF_ARC_CLI              (0)
#define GAF_ARC_VCS              (0)
#define GAF_ARC_VCC              (0)
#define GAF_ARC_MICS             (0)
#define GAF_ARC_MICC             (0)
#define GAF_ARC_AICS             (0)
#define GAF_ARC_AICC             (0)
#define GAF_ARC_VOCS             (0)
#define GAF_ARC_VOCC             (0)
#endif //(defined(CFG_ARC_VCS) || defined(CFG_ARC_VCC) ...)

/// ACC supported
#if defined(CFG_ACC_TBS) || defined(CFG_ACC_TBC) || defined(CFG_ACC_MCS) || defined(CFG_ACC_MCC)
#define GAF_ACC                  (1)

#if defined(CFG_ACC_TBS)
#define GAF_ACC_TBS              (1)
#else //(defined(CFG_ACC_TBS))
#define GAF_ACC_TBS              (0)
#endif //(defined(CFG_ACC_TBS))

#if defined(CFG_ACC_TBC)
#define GAF_ACC_TBC              (1)
#else //(defined(CFG_ACC_TBC))
#define GAF_ACC_TBC              (0)
#endif //(defined(CFG_ACC_TBC))

#if defined(CFG_ACC_MCS)
#define GAF_ACC_MCS              (1)
#else //(defined(CFG_ACC_MCS))
#define GAF_ACC_MCS              (0)
#endif //(defined(CFG_ACC_MCS))

#if defined(CFG_ACC_MCC)
#define GAF_ACC_MCC              (1)
#else //(defined(CFG_ACC_MCC))
#define GAF_ACC_MCC              (0)
#endif //(defined(CFG_ACC_MCC))

#if (GAF_ACC_MCS)
#if defined(CFG_ACC_OTS)
#define GAF_ACC_OTS              (1)
#else //(defined(CFG_ACC_OTS))
#define GAF_ACC_OTS              (0)
#endif //(defined(CFG_ACC_OTS))
#else //(GAF_ACC_MCS)
#define GAF_ACC_OTS              (0)
#endif //(GAF_ACC_MCS)

#if (GAF_ACC_MCC)
#if defined(CFG_ACC_OTC)
#define GAF_ACC_OTC              (1)
#else //(defined(CFG_ACC_OTC))
#define GAF_ACC_OTC              (0)
#endif //(defined(CFG_ACC_OTC))
#else //(GAF_ACC_MCC)
#define GAF_ACC_OTC              (0)
#endif //(GAF_ACC_MCC)

#if (GAF_ACC_MCS)
#if defined(CFG_ACC_DTS)
#define GAF_ACC_DTS              (1)
#else //(defined(CFG_ACC_DTS))
#define GAF_ACC_DTS              (0)
#endif //(defined(CFG_ACC_DTS))
#else //(GAF_ACC_MCC)
#define GAF_ACC_DTS              (0)
#endif //(GAF_ACC_MCC)

#if (GAF_ACC_MCC)
#if defined(CFG_ACC_DTC)
#define GAF_ACC_DTC              (1)
#else //(defined(CFG_ACC_DTC))
#define GAF_ACC_DTC              (0)
#endif //(defined(CFG_ACC_DTC))
#else //(GAF_ACC_MCC)
#define GAF_ACC_DTC              (0)
#endif //(GAF_ACC_MCC)

#if (GAF_ACC_TBS || GAF_ACC_MCS || GAF_ACC_OTS || GAF_ACC_DTS)
#define GAF_ACC_SRV              (1)
#else //(GAF_ACC_TBS || GAF_ACC_MCS || GAF_ACC_OTS || GAF_ACC_DTS)
#define GAF_ACC_SRV              (0)
#endif //(GAF_ACC_TBS || GAF_ACC_MCS || GAF_ACC_OTS || GAF_ACC_DTS)

#if (GAF_ACC_TBC || GAF_ACC_MCC || GAF_ACC_OTC || GAF_ACC_DTC)
#define GAF_ACC_CLI              (1)
#else //(GAF_ACC_TBC || GAF_ACC_MCC || GAF_ACC_OTC || GAF_ACC_DTC)
#define GAF_ACC_CLI              (0)
#endif //(GAF_ACC_TBC || GAF_ACC_MCC || GAF_ACC_OTC || GAF_ACC_DTC)
#else //(defined(CFG_ACC_TBS) || defined(CFG_ACC_TBC) ...)
#define GAF_ACC                  (0)
#define GAF_ACC_SRV              (0)
#define GAF_ACC_CLI              (0)
#define GAF_ACC_TBS              (0)
#define GAF_ACC_TBC              (0)
#define GAF_ACC_MCS              (0)
#define GAF_ACC_MCC              (0)
#define GAF_ACC_OTS              (0)
#define GAF_ACC_OTC              (0)
#define GAF_ACC_DTS              (0)
#define GAF_ACC_DTC              (0)
#endif //(defined(CFG_ACC_TBS) || defined(CFG_ACC_TBC) ...)

/// ATC supported
#if defined(CFG_ATC_CSISM) || defined(CFG_ATC_CSISC)
#define GAF_ATC                  (1)

#if defined(CFG_ATC_CSISM)
#define GAF_ATC_CSISM            (1)
#else //(defined(CFG_ATC_CSISM))
#define GAF_ATC_CSISM            (0)

#if defined(CFG_CAP_CAS)
#undef CFG_CAP_CAS
#endif //(defined(CFG_CAP_CAS))
#endif //(defined(CFG_ATC_CSISM))

#if defined(CFG_ATC_CSISC)
#define GAF_ATC_CSISC            (1)
#else //(defined(CFG_ATC_CSISC))
#define GAF_ATC_CSISC            (0)

#if defined(CFG_CAP_CAC)
#undef CFG_CAP_CAC
#endif //(defined(CFG_CAP_CAC))
#endif //(defined(CFG_ATC_CSISC))

#if (GAF_ATC_CSISM)
#define GAF_ATC_SRV              (1)
#else //(GAF_ATC_CSISM)
#define GAF_ATC_SRV              (0)
#endif //(GAF_ATC_CSISM)

#if (GAF_ATC_CSISC)
#define GAF_ATC_CLI              (1)
#else //( GAF_ATC_CSISC)
#define GAF_ATC_CLI              (0)
#endif //(GAF_ATC_CSISC)
#else //(GAF_ATC_CSISC)
#define GAF_ATC                  (0)
#define GAF_ATC_SRV              (0)
#define GAF_ATC_CLI              (0)
#define GAF_ATC_CSISM            (0)
#define GAF_ATC_CSISC            (0)
#endif //(GAF_ATC_CSISC)

#if defined(CFG_CAP_CAS) || defined(CFG_CAP_CAC)
#define GAF_CAP                  (1)

#if defined(CFG_CAP_CAS)
#define GAF_CAP_CAS              (1)
#else
#define GAF_CAP_CAS              (0)
#endif //(defined(CFG_CAP_CAS))

#if defined(CFG_CAP_CAC)
#define GAF_CAP_CAC              (1)
#else
#define GAF_CAP_CAC              (0)
#endif //(defined(CFG_CAP_CAC))

#else
#define GAF_CAP                  (0)
#define GAF_CAP_CAS              (0)
#define GAF_CAP_CAC              (0)
#endif //(defined(CFG_CAP_CAS) || defined(CFG_CAP_CAC))

#if defined(CFG_TMAP_TMAS) || defined(CFG_TMAP_TMAC)
#define GAF_TMAP                 (1)

#if defined(CFG_TMAP_TMAS)
#define GAF_TMAP_TMAS                 (1)
#else
#define GAF_TMAP_TMAS                 (0)
#endif //(defined(CFG_TMAP_TMAS))

#if defined(CFG_TMAP_TMAC)
#define GAF_TMAP_TMAC                 (1)
#else
#define GAF_TMAP_TMAC                 (0)
#endif //(defined(CFG_TMAP_TMAC))
#else
#define GAF_TMAP                      (0)
#define GAF_TMAP_TMAS                 (0)
#define GAF_TMAP_TMAC                 (0)
#endif //(defined(CFG_TMAP_TMAS) || defined(CFG_TMAP_TMAC))

#if defined(CFG_HAP_HAS) || defined(CFG_HAP_HAC)
#define GAF_HAP                  (1)

#if defined(CFG_HAP_HAS)
#define GAF_HAP_HAS              (1)
#else
#define GAF_HAP_HAS              (0)
#endif //(defined(CFG_HAP_HAS))

#if defined(CFG_HAP_HAC)
#define GAF_HAP_HAC              (1)
#else
#define GAF_HAP_HAC              (0)
#endif //(defined(CFG_HAP_HAC))

#else
#define GAF_HAP                  (0)
#define GAF_HAP_HAS              (0)
#define GAF_HAP_HAC              (0)
#endif //(defined(CFG_HAP_HAS) || defined(CFG_HAP_HAC))

#if defined(CFG_GMAP_GMAS) || defined(CFG_GMAP_GMAC)
#define GAF_GMAP                      (1)

#if defined(CFG_GMAP_GMAS)
#define GAF_GMAP_GMAS                 (1)
#else
#define GAF_GMAP_GMAS                 (0)
#endif //(defined(CFG_GMAP_GMAS))

#if defined(CFG_GMAP_GMAC)
#define GAF_GMAP_GMAC                 (1)
#else
#define GAF_GMAP_GMAC                 (0)
#endif //(defined(CFG_GMAP_GMAC))
#else
#define GAF_GMAP                      (0)
#define GAF_GMAP_GMAS                 (0)
#define GAF_GMAP_GMAC                 (0)
#endif //(defined(CFG_GMAP_GMAS) || defined(CFG_GMAP_GMAC))

#if (defined(CFG_BAP_UC_SRV) || defined(CFG_BAP_UC_CLI) || defined(CFG_BAP_CAPA_SRV) || defined(CFG_BAP_CAPA_CLI) || \
     defined(CFG_BAP_BC_SRC) || defined(CFG_BAP_BC_SINK) || defined(CFG_BAP_BC_SCAN) || defined(CFG_BAP_BC_DELEG) || \
     defined(CFG_BAP_BC_ASSIST))
#define GAF_BAP                  (1)

#if defined(CFG_BAP_UC_SRV)
#define GAF_BAP_UC_SRV           (1)
#else
#define GAF_BAP_UC_SRV           (0)
#endif //(defined(CFG_BAP_UC_SRV))

#if defined(CFG_BAP_UC_CLI)
#define GAF_BAP_UC_CLI           (1)
#else
#define GAF_BAP_UC_CLI           (0)
#endif //(defined(CFG_BAP_UC_CLI))

#if defined(CFG_BAP_CAPA_SRV)
#define GAF_BAP_CAPA_SRV         (1)
#else
#define GAF_BAP_CAPA_SRV         (0)
#endif //(defined(CFG_BAP_CAPA_SRV))

#if defined(CFG_BAP_CAPA_CLI)
#define GAF_BAP_CAPA_CLI         (1)
#else
#define GAF_BAP_CAPA_CLI         (0)
#endif //(defined(CFG_BAP_CAPA_CLI))

#if (defined(CFG_BAP_BC_SRC) && GAF_BROADCAST_SUPP)
#define GAF_BAP_BC_SRC           (1)
#else
#define GAF_BAP_BC_SRC           (0)
#endif //(defined(CFG_BAP_BC_SRC) && GAF_BROADCAST_SUPP)

#if (defined(CFG_BAP_BC_ASSIST) && GAF_BROADCAST_SUPP)
#define GAF_BAP_BC_ASSIST         (1)
#else
#define GAF_BAP_BC_ASSIST         (0)
#endif //(defined(GAF_BAP_BC_ASSIST) && GAF_BROADCAST_SUPP)

#if (defined(CFG_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)
#define GAF_BAP_BC_DELEG          (1)
#else
#define GAF_BAP_BC_DELEG          (0)
#endif //(defined(GAF_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)

#if ((defined(CFG_BAP_BC_SINK) || GAF_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)
#define GAF_BAP_BC_SINK           (1)
#else
#define GAF_BAP_BC_SINK           (0)
#endif //((defined(CFG_BAP_BC_SINK) || CFG_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)

#if ((defined(CFG_BAP_BC_SCAN) || GAF_BAP_BC_SINK || GAF_BAP_BC_ASSIST || GAF_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)
#define GAF_BAP_BC_SCAN           (1)
#else
#define GAF_BAP_BC_SCAN           (0)
#endif //((defined(CFG_BAP_BC_SCAN) || GAF_BAP_BC_SINK || CFG_BAP_BC_ASSIST || CFG_BAP_BC_DELEG) && GAF_BROADCAST_SUPP)
#else
#define GAF_BAP                  (0)
#define GAF_BAP_UC_SRV           (0)
#define GAF_BAP_UC_CLI           (0)
#define GAF_BAP_CAPA_SRV         (0)
#define GAF_BAP_CAPA_CLI         (0)
#define GAF_BAP_BC_SRC           (0)
#define GAF_BAP_BC_SINK          (0)
#define GAF_BAP_BC_SCAN          (0)
#define GAF_BAP_BC_ASSIST        (0)
#define GAF_BAP_BC_DELEG         (0)
#endif //(defined(CFG_BAP_UC_SRV) || defined(CFG_BAP_UC_CLI) || defined(CFG_BAP_CAPA_SRV) ...)

#if defined(CFG_GAF_API_MSG)
#define GAF_API_MSG              (1)
#else
#define GAF_API_MSG              (0)
#endif //(defined(CFG_GAF_API_MSG))

// Support of Debug functions
#if defined(CFG_GAF_DBG)
#define GAF_DBG                  (1)
#else
#define GAF_DBG                  (0)
#endif //(defined(CFG_GAF_DBG))

// GAF Adaptation layer
#if (GAF_BAP_BC_SRC)
#define GAF_AL_PER_ADV           (1)
#else
#define GAF_AL_PER_ADV           (0)
#endif //(GAF_BAP_BC_SRC)

#if (GAF_BAP_BC_SCAN || GAF_BAP_BC_ASSIST)
#define GAF_AL_PER_SYNC          (1)
#else
#define GAF_AL_PER_SYNC          (0)
#endif //(GAF_BAP_BC_SCAN || GAF_BAP_BC_ASSIST)

#if (GAF_AL_PER_SYNC && (GAF_BAP_BC_ASSIST || GAF_BAP_BC_DELEG))
#define GAF_AL_PAST              (1)
#else
#define GAF_AL_PAST              (0)
#endif //(GAF_AL_PER_SYNC && (GAF_BAP_BC_ASSIST || GAF_BAP_BC_DELEG))

#if (GAF_BAP_BC_SINK || GAF_BAP_BC_SCAN || GAF_BAP_BC_ASSIST || GAF_AL_PER_SYNC || GAF_SCAN)
#define GAF_AL_SCAN              (1)
#else
#define GAF_AL_SCAN              (0)
#endif //(GAF_BAP_BC_SINK || GAF_BAP_BC_SCAN || GAF_BAP_BC_ASSIST || GAF_AL_PER_SYNC || GAF_SCAN)

#if (GAF_BAP_BC_DELEG || GAF_ADV)
#define GAF_AL_ADV               (1)
#else
#define GAF_AL_ADV               (0)
#endif //(GAF_BAP_BC_DELEG || GAF_ADV)


#if (GAF_AL_PER_ADV || GAF_AL_PER_SYNC || GAF_AL_SCAN || GAF_AL_ADV)
#define GAF_AL                   (1)
#define GAF_AL_ACT_MAX           (HOST_ACTIVITY_MAX)
#else
#define GAF_AL                   (0)
#endif //(GAF_AL_PER_ADV)

#endif // GAF_CFG_H_
