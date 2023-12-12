cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := a2dp_encoder_cc_common.o
obj-y += a2dp_encoder_cc_off_bth.o

ifeq ($(A2DP_LHDC_ON),1)
obj-y += a2dp_encoder_cc_lhdcv5.o
CFLAGS_a2dp_encoder_cc_lhdcv5.o += -O3
LHDC_INCLUDES =  \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc
else
LHDC_INCLUDES =
endif

ifeq ($(A2DP_AAC_ON),1)
obj-y += a2dp_encoder_cc_aac.o
CFLAGS_a2dp_encoder_cc_aac.o += -O3
endif

ifeq ($(A2DP_LDAC_ON),1)
obj-y += a2dp_encoder_cc_ldac.o
CFLAGS_a2dp_encoder_cc_ldac.o += -O3
endif

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iservices/bt_app \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/overlay \
    -Iservices/resources \
    -Imultimedia/inc/audio/process/resample/include \
    -Iservices/nv_section/factory_section \
    -Iservices/nv_section/fpga_section \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/audioflinger \
    -Iservices/audio_dump/include \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iapps/key \
    -Iapps/main \
    -Iapps/common \
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Iapps/audioplayers/a2dp_encoder_cc \
    -Iapps/anc/inc \
    -Iapps/factory \
    -Iservices/interconnection/green \
    -Iutils/hwtimer_list \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
    -Iservices/nv_section/log_section \
    -Iservices/ai_voice/manager \
    -Iservices/app_ai/inc \
    -Iapps/battery/ \
    -Iutils/crc16 \
    -Iutils/crc32 \
    -Iutils/rom_utils \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ota \
    -Iservices/audioflinger \
    -Iutils/list \
    -Iapps/bt_sync \
    -Iservices/audio_manager \
    -Iapps/app_rpc	\
    -Iapps/app_rpc/rpc_bth_dsp \
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Imultimedia/audio/codec/sbc/src/inc \
    -Ithirdparty/audio_codec_lib/liblhdcv5-enc/inc \
    -Imultimedia/inc \
    -Ithirdparty/audio_codec_lib/ldac/inc

ccflags-y += -Iapps/dsp_m55 \
             -Wno-unused-function