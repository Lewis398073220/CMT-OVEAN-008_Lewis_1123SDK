#pragma once
#ifndef __SMF_CODEC_YUV_H__
#define __SMF_CODEC_YUV_H__

#include "smf_api.h"
#include "smf_media_def.h"
enum yuv_format_e {
	I420 = 1,//y8-uuvv
	yv12,//y8-vvuu
	nv12,//y8-uvuv
	nv21,//y8-vuvu
	I422,//y4-uuvv
	yv16,//y4-vvuu
	nv61,//y4-uvuv
	nv16,//y4-vuvu
	yuvy,
	vyuy,
	uyvy,
	I444,//yy-uuvv
	yv24,//yy-vvuu
	nv24,//yy-uvuv
	nv42,//yy-vuvu
	yuv,
};

typedef struct {
	uint8_t _format;//yuv_format_e
	uint8_t _pixelBits;//8,16,24,32
}smf_media_yuv_def_t;

typedef struct {
	smf_media_def_t head;
	smf_media_video_def_t video;
	smf_media_yuv_def_t yuv;
}smf_media_yuv_t;

typedef struct smf_yuv_enc_open_param_t {
	smf_media_yuv_t media;
}smf_yuv_enc_open_param_t;

typedef struct smf_yuv_dec_open_param_t {
	smf_media_yuv_t* media;
}smf_yuv_dec_open_param_t;

/**
 * register yuv encoder
 */
EXTERNC void smf_yuv_encoder_register(void);

/**
 * register yuv decoder
 */
EXTERNC void smf_yuv_decoder_register(void);
#endif
