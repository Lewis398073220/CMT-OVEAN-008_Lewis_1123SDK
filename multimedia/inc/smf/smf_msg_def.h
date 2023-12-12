#pragma once

enum smf_msg_e {
	SMF_MSG_SHAOPIAN = 0x000,//000~03f
	SMF_MSG_USER = 0x040,//040~0ff
	//
	SMF_MSG_POOL = 0x210,
	SMF_MSG_CMD = 0x220,
	SMF_MSG_FS = 0x230,
	SMF_MSG_VOIP = 0x240,
	SMF_MSG_SHM = 0x250,
	SMF_MSG_PROXY = 0x260,
	SMF_MSG_FRAME = 0x270,
	//
	SMF_MSG_LocalPlayer = 0x840,
	SMF_MSG_LocalRecord = 0x850,
	SMF_MSG_LocalSco = 0x860,
	SMF_MSG_APPlayer = 0x870,
	SMF_MSG_APRecord = 0x878,
	//SMF_MSG_APSco = 0x880,
	SMF_MSG_ESIMSco = 0x8e0,
};


