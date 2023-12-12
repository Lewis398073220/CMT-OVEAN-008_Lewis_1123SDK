/**
 ****************************************************************************************
 *
 * @file bap_codec.h
 *
 * @brief Basic Audio Profile - Codec API
 *
 * Copyright (C) RivieraWaves 2019-2021
 *
 ****************************************************************************************
 */

#ifndef _BAP_CODEC_H_
#define _BAP_CODEC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gaf_cfg.h"    // GAF Configuration

#if (GAF_BAP)

#include "gaf.h"        // GAF Definitions
#include "bap.h"        // BAP Definitions

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if a list of values in LTV format is properly formatted
 *
 * @param[in] p_ltv_data      Pointer to LTV data structure
 *
 * @return An error status
 *  - GAF_ERR_NO_ERROR if data is properly formatted
 *  - GAF_ERR_INVALID_PARAM if data is not properly formatted
 ****************************************************************************************
 */
uint16_t bap_codec_ltv_check(const gaf_ltv_t* p_ltv_data);

/**
 ****************************************************************************************
 * @brief Check if a list of values in LTV format is properly formatted
 *
 * @param[in] p_ltv_data      Pointer to LTV data structure
 *
 * @return An error status
 *  - GAF_ERR_NO_ERROR if data is properly formatted
 *  - GAF_ERR_INVALID_PARAM if data is not properly formatted
 ****************************************************************************************
 */

#if (GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC || GAF_BAP_BC_SCAN)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_cfg_check(const gaf_codec_id_t* p_codec_id, const bap_cfg_t* p_cfg,
                             const bap_cfg_metadata_t* p_metadata, uint8_t* p_cfg_ltv_len,
                             uint8_t* p_metadata_ltv_len);
#endif //(GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC || GAF_BAP_BC_SCAN)

#if (GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint8_t bap_codec_cfg_pack(uint8_t* p_data, const bap_cfg_t* p_cfg, bool with_add_cfg);
#endif //(GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC)

#if (GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC || GAF_BAP_BC_DELEG || GAF_BAP_BC_ASSIST)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint8_t bap_codec_cfg_pack_metadata(uint8_t* p_data, const bap_cfg_metadata_t* p_metadata);
#endif //(GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SRC || GAF_BAP_BC_DELEG || GAF_BAP_BC_ASSIST)

#if (GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SCAN)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_cfg_unpack(const gaf_codec_id_t* p_codec_id, bap_cfg_ptr_t* p_capa, gaf_ltv_t *p_capa_ltv);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_cfg_unpack_metadata(const gaf_codec_id_t* p_codec_id, bap_cfg_metadata_ptr_t* p_metadata,
                                       gaf_ltv_t *p_metadata_ltv);
#endif //(GAF_BAP_UC_SRV || GAF_BAP_UC_CLI || GAF_BAP_BC_SCAN)

#if (GAF_BAP_CAPA_SRV || GAF_BAP_CAPA_CLI)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_capa_check(const gaf_codec_id_t* p_codec_id, const bap_capa_t* p_capa,
                              const bap_capa_metadata_t* p_metadata, uint16_t* p_ltv_len);
#endif //(GAF_BAP_CAPA_SRV || GAF_BAP_CAPA_CLI)

#if (GAF_BAP_CAPA_SRV)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint8_t bap_codec_capa_pack(uint8_t* p_data, const bap_capa_t* p_capa);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint8_t bap_codec_capa_pack_metadata(uint8_t* p_data, const bap_capa_metadata_t* p_metadata);
#endif //(GAF_BAP_CAPA_SRV)

#if (GAF_BAP_CAPA_CLI)
/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_capa_unpack(const gaf_codec_id_t* p_codec_id, bap_capa_ptr_t* p_capa, gaf_ltv_t *p_capa_ltv);

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
uint16_t bap_codec_capa_unpack_metadata(const gaf_codec_id_t* p_codec_id, bap_capa_metadata_ptr_t* p_metadata,
                                        gaf_ltv_t *p_metadata_ltv);
#endif //(GAF_BAP_CAPA_CLI)

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for LC3
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is for LC3, else false
 ****************************************************************************************
 */
bool bap_codec_is_lc3(const gaf_codec_id_t* p_codec_id);

#ifdef NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for vendor specific data
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is for vendor, else false
 ****************************************************************************************
 */
bool bap_codec_is_vendor(const gaf_codec_id_t* p_codec_id);
#endif

#if defined(HID_ULL_ENABLE)
/**
****************************************************************************************
* @brief Check if a Codec ID is Codec ID for ull
* 
* @param[in] p_codec_id        Pointer to Codec ID
* 
* @return True if Codec ID is for ull, else false
****************************************************************************************
*/
bool bap_codec_is_ull(const gaf_codec_id_t* p_codec_id);
#endif // HID_ULL_ENABLE

#if defined(LC3PLUS_SUPPORT)
/**
 ****************************************************************************************
 * @brief Check if a Codec ID is Codec ID for LC3plus
 * 
 * @param[in] p_codec_id        Pointer to Codec ID
 * 
 * @return True if Codec ID is for LC3plus, else false
 ****************************************************************************************
 */
bool bap_codec_is_lc3plus(const gaf_codec_id_t* p_codec_id);
#endif

/**
 ****************************************************************************************
 * @brief Check if a Codec ID is valid
 *
 * @param[in] p_codec_id        Pointer to Codec ID
 *
 * @return True if Codec ID is valid else false
 ****************************************************************************************
 */
bool bap_codec_is_id_valid(const gaf_codec_id_t* p_codec_id);

#endif // (GAF_BAP)

#endif // BAP_CODEC_H_
