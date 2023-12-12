#pragma once
#ifndef __SMF_MEDIA_DEF_H__
#define __SMF_MEDIA_DEF_H__
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
	uint32_t* vtable;
	uint32_t _size;
	uint64_t _codec;
	uint32_t _mflags;
	void* _ext;
	uint32_t _extraSize;
	uint8_t* _extraData;
}smf_media_def_t;
//
typedef struct {
	uint32_t _size;
	uint32_t _codec;//not used uint64_t
	uint32_t _codec_high;//not used uint64_t
	uint32_t _mflags;
	void* _ext;
	uint32_t _extraSize;
	uint8_t* _extraData;
}smf_media32_def_t;
//
typedef struct {
	uint32_t _rate; 
	uint8_t _channels; 
	uint8_t _rev; 
	uint16_t _frameSamples; 
}smf_media_audio_def_t;
//
typedef struct {
	uint16_t _width;
	uint16_t _height;
	uint16_t _frameRate;
	uint16_t _frameRateDenum;
	uint16_t _roiX, _roiY, _roiW, _roiH;//Region of Interest
}smf_media_video_def_t;
#endif

