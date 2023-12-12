#pragma once
#include "stdint.h"
#include "stdbool.h"

#define SMF_MSG_VERSION 2023082800
///smf msg message header
typedef struct SmfMsg_t {
	uint16_t cmd;
	uint16_t size;
	uint32_t version;
    uint8_t cpuid;
    uint8_t svcid;//stream id
	uint8_t flags;
	uint8_t flagsExt;
} smf_msg_t, SmfMsg_t;
typedef smf_msg_t media_msg_t;
///smf msg response message header
typedef struct SmfMsgResponse_t {
    smf_msg_t head;
    union {
        uint32_t flags;
        struct {
            bool result : 1;
        };
    };
	union {
		uint64_t error;
		uint32_t err32[2];
	};
} smf_msg_response_t, SmfMsgResponse_t;
//

enum smf_cpu_id_e{
	SMF_CPU_ID_BEGIN = 0X00,
	SMF_CPU_DEFAULT = 0X00,
	SMF_CPU_M55 = 0X01,
	SMF_CPU_BTH = 0X02,
	SMF_CPU_SEN = 0X03,
	SMF_CPU_DSP = 0X04,
	SMF_CPU_ID_END,
};

typedef bool(*CbMsg)(smf_msg_t*);
typedef bool(*CbMsgPriv)(smf_msg_t*,void* priv);
typedef bool(*CbMsgSizePriv)(smf_msg_t*,uint32_t size,void* priv);

