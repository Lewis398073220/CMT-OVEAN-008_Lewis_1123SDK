#pragma once

#include "smf_api.h"
#include "smf_media_def.h"

enum h264_profile_e {
	H264_Baseline = 1,
	H264_Main,
	H264_High,
};

enum h264_format_e {
	IIII = 1,
	IPPP,
	IBBP,
};

typedef struct {
	uint32_t _bitrate;
	uint8_t _vbr;
	uint8_t _profile;//h264_profile_e
	uint8_t _format; //h264_format_e	
	uint8_t _gop;//group of picture
}smf_media_h264_def_t;

typedef struct {
	smf_media_def_t head;
	smf_media_video_def_t video;
	smf_media_h264_def_t h264;
}smf_media_h264_t;

typedef struct smf_h264_dec_open_param_t {
	smf_media_h264_t media;
}smf_h264_dec_open_param_t;

typedef struct smf_h264_enc_open_param_t {
	smf_media_h264_t* media;
}smf_h264_enc_open_param_t;
/**
 * register h264 decoder
 */
EXTERNC void smf_h264_decoder_register(void);
EXTERNC void smf_h264_encoder_register(void);
EXTERNC void smf_h264_demuxer_register(void);
