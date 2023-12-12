/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
 *
 ****************************************************************************/
#include "voice_sbc.h"
#include "hal_trace.h"

static sbc_encoder_t voice_sbc_encoder;
static uint32_t voice_sbc_frame_len = 0;

static VOICE_SBC_CONFIG_T	voice_sbc_config =
{
    VOICE_SBC_CHANNEL_COUNT     ,
    VOICE_SBC_CHANNEL_MODE 	    ,
    VOICE_SBC_BIT_POOL	 	    ,
    VOICE_SBC_SIZE_PER_SAMPLE   ,
    VOICE_SBC_SAMPLE_RATE	    ,
    VOICE_SBC_NUM_BLOCKS	    ,
    VOICE_SBC_NUM_SUB_BANDS	    ,
    VOICE_SBC_MSBC_FLAG		    ,
    VOICE_SBC_ALLOC_METHOD	    ,
};

static int voice_sbc_init_encoder(void)
{
    sbc_stream_info_t info;
    info.flags = voice_sbc_config.mSbcFlag;
    info.num_channels = voice_sbc_config.channelCnt;
    info.channel_mode = voice_sbc_config.channelMode;
    info.bit_pool     = voice_sbc_config.bitPool;
    info.sample_rate  = voice_sbc_config.sampleRate;
    info.alloc_method = voice_sbc_config.allocMethod;
    info.num_blocks   = voice_sbc_config.numBlocks;
    info.num_subbands = voice_sbc_config.numSubBands;
    sbc_encoder_open(&voice_sbc_encoder, info);

    voice_sbc_frame_len = sbc_get_frame_len(info);
    TRACE(1,"frame len is %d", voice_sbc_frame_len);
    return 0;
}

uint32_t voice_sbc_get_frame_len(void)
{
    return voice_sbc_frame_len;
}

uint32_t voice_sbc_encode(uint8_t *input, uint32_t inputBytes, uint32_t* purchasedBytes, uint8_t *output, uint8_t isReset)
{
    sbc_stream_info_t info;
    pcm_frame_t PcmEncData;
    sbc_frame_t sbc_data;
    uint16_t outputSbcBytes = 0, bytes_encoded = 0;

	if (isReset)
	{
		voice_sbc_init_encoder();
	}

    sbc_encoder_get_stream_info(&voice_sbc_encoder, &info);

    if (info.flags == SBC_FLAGS_MSBC)
    {
        uint16_t outputOffset = 0;

        for (uint32_t index = 0;index < inputBytes;index += (uint32_t)VOICE_SBC_PCM_DATA_SIZE_PER_FRAME)
        {
            PcmEncData.pcm_data = (int16_t *)(input+index);
            PcmEncData.buffer_size = VOICE_SBC_PCM_DATA_SIZE_PER_FRAME;
            PcmEncData.valid_size = VOICE_SBC_PCM_DATA_SIZE_PER_FRAME;

            sbc_data.sbc_data = output+outputOffset;
            sbc_data.valid_size = 0;

            //output[outputOffset++] = 0xAD;
            //output[outputOffset++] = 0x02;
            sbc_encoder_process_frame(&voice_sbc_encoder, &PcmEncData, &sbc_data);
            outputOffset += sbc_data.valid_size;
            //output[outputOffset++] = 0x00;

            bytes_encoded += VOICE_SBC_PCM_DATA_SIZE_PER_FRAME;
        }

        outputSbcBytes = outputOffset;
    }
    else
    {
        for (uint32_t index = 0;index < inputBytes;index += (uint32_t)(info.pcm_samples * voice_sbc_config.sizePerSample))
        {
            PcmEncData.pcm_data = (int16_t *)(input + index);
            PcmEncData.buffer_size = inputBytes - index;
            PcmEncData.valid_size = inputBytes - index;
            sbc_data.sbc_data = output + outputSbcBytes;
            sbc_data.valid_size = 0;
            if (!sbc_encoder_process_frame(&voice_sbc_encoder, &PcmEncData, &sbc_data))
            {
                if ((info.pcm_samples * voice_sbc_config.sizePerSample) > (inputBytes - index))
                {
                    bytes_encoded += inputBytes - index;
                }
                else
                {
                    bytes_encoded += info.pcm_samples * voice_sbc_config.sizePerSample;
                }

                outputSbcBytes += sbc_data.valid_size;
            }
        }
    }

    //TRACE(0,"Encode %d bytes PCM data into %d bytes SBC data. flag %d sample %d", inputBytes, outputSbcBytes, info.flags, info.pcm_samples);
    //TRACE(1,"Consumed PCM data %d bytes", bytes_encoded);

    *purchasedBytes = bytes_encoded;

    return outputSbcBytes;
}

int voice_sbc_init(VOICE_SBC_CONFIG_T* pConfig)
{
    voice_sbc_config = *pConfig;
    return 0;
}
