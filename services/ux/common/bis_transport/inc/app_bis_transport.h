/**
 * @copyright Copyright (c) 2015-2022 BES Technic.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 */
#ifndef APP_BIS_TRANSPOND_H_
#define APP_BIS_TRANSPOND_H_

/***********************************header include********************************************/


/***********************************external function declaration*****************************/


/***********************************private macro defination***********************************/

/***********************************private type defination************************************/

/**
 ****************************************************************************************
 * @brief Bis transport event report callback type
 * @param[in]: event_type, event type, see@APP_BIS_TRAN_EVENT_E
 * @param[in]: param, event parameter
 ****************************************************************************************
 */
typedef void (*app_bis_tran_event_callback_t)(uint8_t event_type, void *param);
typedef uint32_t (*app_bis_tran_read_data_callback_t)(uint8_t stream_idx, uint8_t *data, uint16_t data_len);

typedef enum
{
    APP_BIS_TRAN_ROLE_UNKNOWN = 0,
    APP_BIS_TRAN_ROLE_SRC,
    APP_BIS_TRAN_ROLE_SINK,
} APP_BIS_TRAN_ROLE_E;

typedef enum
{
    APP_BIS_TRAN_STATE_CLOSE = 0,
    APP_BIS_TRAN_STATE_OPENED,
    APP_BIS_TRAN_STATE_STREAM_STARTED,
    APP_BIS_TRAN_STATE_UNKNOWN,
} APP_BIS_TRAN_STATE_E;

typedef enum
{
    APP_BIS_TRAN_INPUT_PLAY_MONO=0,
    APP_BIS_TRAN_INPUT_PLAY_STEREO,
    APP_BIS_TRAN_INPUT_PLAY_MONOMER_STEREO,
    APP_BIS_TRAN_INPUT_PLAY_MONOMER_STEREO_TWO_STREAM,
    APP_BIS_TRAN_INPUT_PLAY_UNKONW,
} APP_BIS_TRAN_INPUT_PLAY_MODE_E;

typedef enum
{
    APP_BIS_TRAN_USE_SEND=0,
    APP_BIS_TRAN_SVC_READ,
} APP_BIS_TRAN_DATA_TANS_MODE_E;

typedef enum
{
    BIS_SRC_PCM_INPUT_LINEIN = 0,
    BIS_SRC_PCM_INPUT_WIFI,
    BIS_SRC_PCM_INPUT_USB,
    BIS_SRC_PCM_INPUT_BT,
    BIS_SRC_PCM_INPUT_NUM,
} APP_BIS_TRAN_INPUT_TYPE_E;

/// function return error code
typedef enum
{
    APP_BIS_TRAN_NO_ERROR = 0,
    APP_BIS_TRAN_STATE,         //state error
    APP_BIS_TRAN_PARAM_INVALID, //parameter invalid
    APP_BIS_TRAN_OPER_PRO,      //Operation prohibition
} APP_BIS_TRAN_ERROR_CODE_E;

/// function return error code
typedef enum
{
    APP_BIS_TRAN_ONLY_LOCAL = 0,
    APP_BIS_TRAN_ONLY_REMOTE,
    APP_BIS_TRAN_LOCAL_AND_REMOTE,
    APP_BIS_TRAN_DEV_MAX,
} APP_BIS_TRAN_AUDIO_PLAYBACK_DEVICE_E;

typedef enum
{
    // param: NULL
    APP_BIS_TRAN_EVENT_STREAM_START,
    // param: NULL
    APP_BIS_TRAN_EVENT_STREAM_STOP,
    APP_BIS_TRAN_EVENT_UNKNOWN,
} APP_BIS_TRAN_EVENT_E;

typedef enum
{
    APP_BIS_TRAN_STREAM_0 = 0,
    APP_BIS_TRAN_STREAM_1,
    APP_BIS_TRAN_STREAM_2,
    APP_BIS_TRAN_STREAM_3,
    APP_BIS_TRAN_STREAM_4,
    APP_BIS_TRAN_STREAM_MAX,
} APP_BIS_TRAN_AUDIO_STREAM_IDX_E;

/// volume sync@TGT_VOLUME_LEVEL_T
typedef enum {
    APP_BIS_TRAN_VOLUME_LEVEL_MUTE = 0,
    APP_BIS_TRAN_VOLUME_LEVEL_1,
    APP_BIS_TRAN_VOLUME_LEVEL_2,
    APP_BIS_TRAN_VOLUME_LEVEL_3,
    APP_BIS_TRAN_VOLUME_LEVEL_4,
    APP_BIS_TRAN_VOLUME_LEVEL_5,
    APP_BIS_TRAN_VOLUME_LEVEL_6,
    APP_BIS_TRAN_VOLUME_LEVEL_7,
    APP_BIS_TRAN_VOLUME_LEVEL_8,
    APP_BIS_TRAN_VOLUME_LEVEL_9,
    APP_BIS_TRAN_VOLUME_LEVEL_10,
    APP_BIS_TRAN_VOLUME_LEVEL_11,
    APP_BIS_TRAN_VOLUME_LEVEL_12,
    APP_BIS_TRAN_VOLUME_LEVEL_13,
    APP_BIS_TRAN_VOLUME_LEVEL_14,
    APP_BIS_TRAN_VOLUME_LEVEL_15,
    APP_BIS_TRAN_VOLUME_LEVEL_MAX,

    APP_BIS_TRAN_VOLUME_LEVEL_QTY
}APP_BIS_TRAN_VOLUME_LEVEL_E;

typedef struct
{
    /// bis transport module state @APP_BIS_TRAN_STATE_E
    uint8_t state;
    /// bis transport module state @APP_BIS_TRAN_ROLE_E
    uint8_t role;
} app_bis_tran_get_info_t;

typedef struct
{
    uint32_t ch_bf;
    uint8_t broadcast_id[3];
    uint8_t encrypt_key[16];
    app_bis_tran_event_callback_t event_callback;
} app_bis_tran_sink_param_t;

typedef struct
{
    uint8_t input_type;
    uint8_t input_mode;
    uint8_t broadcast_id[3];
    uint8_t encrypt_enable;
    uint8_t encrypt_key[16];
    app_bis_tran_event_callback_t event_callback;
} app_bis_tran_param_t;

typedef struct
{
    // stream idx see@APP_BIS_TRAN_AUDIO_STREAM_IDX_E
    uint8_t stream_idx;
    /// select playback device, see@APP_BIS_TRAN_AUDIO_PLAYBACK_DEVICE_E
    uint8_t playback_dev;
    // pcm sampling param
    uint8_t channel_num;
    uint32_t channel_bf;
    uint8_t  bits_depth;
    uint32_t sample_rate;
    uint32_t frame_samples;

} app_bis_tran_stream_param_t;

typedef struct
{
    // see@APP_BIS_TRAN_DATA_TANS_MODE_E
    uint8_t data_tran_mode;
    uint8_t stream_num;
    app_bis_tran_stream_param_t *stream_param;
    /// data_tran_mode=1
    app_bis_tran_read_data_callback_t read_data_cb;
} app_bis_tran_start_param_t;

/****************************private function declaration************************************/

/****************************private variable defination*************************************/

/**
 ****************************************************************************************
 * @brief Bis transport module debug command initialize api
 ****************************************************************************************
 */
void app_bis_tran_cmd_init(void);

/**
 ****************************************************************************************
 * @brief Open bis source transport function
 * @param[in]: input_type, bis transport input source type, see@APP_BIS_TRAN_INPUT_TYPE_E
 * @param[in]: input_mode, bis transport input source play mode, see@APP_BIS_TRAN_INPUT_PLAY_MODE_E
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_src_open(app_bis_tran_param_t *param_info);

/**
 ****************************************************************************************
 * @brief Close bis source transport function
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_src_close(void);

/**
 ****************************************************************************************
 * @brief Bis source transport stream start
 * @param[in]: param, bis transport stream start param
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_stream_start(app_bis_tran_start_param_t *param);

/**
 ****************************************************************************************
 * @brief Bis source transport stream stop
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_stream_stop(void);

/**
 ****************************************************************************************
 * @brief Bis source transport write pcm data, only app_bis_tran_stream_start
          function param.data_tran_mode = APP_BIS_TRAN_USE_SEND use this function
 * @param[in]: stream_idx, audio channels index, see@APP_BIS_TRAN_AUDIO_CHAN_IDX_E
 * @param[in]: data, write data buffer pointer
 * @param[in]: data_len, write data length
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_write_pcm_data(uint8_t stream_idx, uint8_t *data, uint16_t data_len);

/**
 ****************************************************************************************
 * @brief User notifies new data can be read , only app_bis_tran_stream_start
          function param.data_tran_mode = APP_BIS_TRAN_SVC_READ use this function
 * @param[in]: stream_idx, audio channels index, see@APP_BIS_TRAN_AUDIO_CHAN_IDX_E
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_produce_data_ntf(uint8_t stream_idx);

/**
 ****************************************************************************************
 * @brief Set local and remote device paly volume
 * @param[in]: volume_level, volume level, see@APP_BIS_TRAN_VOLUME_LEVEL_E
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t  app_bis_tran_set_volume(uint8_t volume_level);

/**
 ****************************************************************************************
 * @brief Open bis sink transport function
 * @param[in]: sink_param, bis transport sink open param
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_sink_open(app_bis_tran_sink_param_t *sink_param);

/**
 ****************************************************************************************
 * @brief Close bis sink transport function
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_sink_close(void);

/**
 ****************************************************************************************
 * @brief Get bis transport module info function
 * @param[in]: info, bis transport info ptr
 * @param[out]: error code, see@APP_BIS_TRAN_ERROR_CODE_E
 ****************************************************************************************
 */
uint8_t app_bis_tran_get_info(app_bis_tran_get_info_t *info);

#endif // APP_BIS_TRANSPOND_H_
