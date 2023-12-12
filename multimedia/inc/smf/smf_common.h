#pragma once
#ifndef __SMF_COMMON_H__
#define __SMF_COMMON_H__
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//#include "smf_frame.h"
//#include "smf_api_param.h"

#ifndef EXTERNC
#ifndef __cplusplus
#define EXTERNC
#else
#define EXTERNC extern "C"
#endif
#endif

//#ifdef __cplusplus 
//#if __cplusplus < 201103L
//#define override
//#define final
//#endif
//#endif

typedef enum smf_direction_e {
	smf_direction_upward=0,
	smf_direction_downward,
	smf_direction_forward,
	smf_direction_backward,
}smf_direction_e;

typedef struct smf_message_t {
	uint64_t id;
	void* creater;
	uint8_t sub;
	uint8_t port;
	//8bits
	bool is_processed : 1;
	bool is_notify : 1;
	uint8_t direction : 2;//smf_direction_e
	uint32_t flags : 12;
	//	
	uint32_t data[4];
}smf_message_t;

typedef struct smf_progress_t {
	uint32_t start;
	uint32_t current;
	uint32_t end;
}smf_progress_t;
//
typedef struct {
	const char* url;
	const char* title;
	const char* artist;
	const char* album;
	uint32_t duration;
}smf_meta_info_t;
//
typedef struct {
	uint32_t playtime;
	uint32_t frameindex;
	uint32_t position;
}smf_stream_status_t;

typedef struct smf_buffer_t {	
	uint16_t max;
	uint16_t offset;
	uint16_t size;
	uint8_t falgs;
	uint8_t refs;
	void* buff;
}smf_buffer_t;
//
typedef struct {
	void* data;
	unsigned size;
}smf_pair_t;

typedef struct {
	uint16_t size;
	void* data;
}smf_pair16_t;
//
typedef struct {
	void* func;
	void* priv;
}smf_callback_t;
//
typedef struct {
	unsigned keys;
	void* data;
	unsigned size;
	void* priv;
	void (*uninit)(void*priv);
	void* other;
}smf_param_t;
//
typedef struct {
	unsigned keys;
	unsigned vals;
}smf_keys_value_t;
//
typedef union {
	int8_t i8[4];	
	int16_t i16[2];	
	int32_t i32;
	uint8_t u8[4];
	uint16_t u16[2];
	uint32_t u32;
}smf_int8x4_t;
//
typedef union {
	int8_t  i8[8];	
	int16_t i16[4];	
	int32_t i32[2];
	int64_t i64;
	uint8_t u8[8];
	uint16_t u16[4];
	uint32_t u32[2];
	uint64_t u64;
}smf_int8x8_t;

typedef struct smf_fifo_t {
	void* data;
	uint32_t max;
	uint64_t wi;
	uint64_t ri;
}smf_fifo_t;

typedef struct smf_ring_t {
	void* data;
	uint32_t max;
	uint64_t wi;
	//uint64_t ri;
}smf_ring_t;

typedef struct {
	uint8_t* begin;
	uint8_t* end;
	uint8_t* ptr;
	uint8_t* ptr_end;
}smf_shared_pool_t;

typedef struct {
	int* frameMax;
	int* frameMin;
	void** frame;
	void** media;
	smf_stream_status_t* status;
}smf_elememt_data_t;

typedef void (*smf_hook_t)(void* data, uint32_t size, void* priv);

typedef struct {
	int num;
	int den;
}smf_fraction_t;
#endif
