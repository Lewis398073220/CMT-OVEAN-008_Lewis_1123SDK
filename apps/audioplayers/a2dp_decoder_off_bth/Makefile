cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := a2dp_decoder_cc_common.o
obj-y += a2dp_decoder_cc_off_bth.o

obj-y += a2dp_decoder_cc_sbc.o
CFLAGS_a2dp_decoder_cc_sbc.o += -O3

ifeq ($(A2DP_AAC_ON),1)
obj-y += a2dp_decoder_cc_aac.o
CFLAGS_a2dp_decoder_cc_aac.o += -O3
endif

ifeq ($(A2DP_AUDIO_STEREO_MIX_CTRL),1)
CFLAGS_a2dp_decoder_cc_aac.o += -DA2DP_AUDIO_STEREO_MIX_CTRL
endif

ifeq ($(A2DP_SCALABLE_ON),1)
obj-y += a2dp_decoder_cc_scalable.o
CFLAGS_a2dp_decoder_cc_scalable.o += -O3
SCALABLE_INCLUDES =  \
	-Ithirdparty/audio_codec_lib/scalable
else
SCALABLE_INCLUDES =
endif

ifeq ($(A2DP_LDAC_ON),1)
obj-y += a2dp_decoder_cc_ldac.o
CFLAGS_a2dp_decoder_cc_ldac.o += -O3
LDAC_INCLUDES =  \
	-Ithirdparty/audio_codec_lib/ldac/inc
else
LDAC_INCLUDES =
endif

ifeq ($(A2DP_LHDC_ON),1)
obj-y += a2dp_decoder_cc_lhdc.o
CFLAGS_a2dp_decoder_cc_lhdc.o += -O3
LHDC_INCLUDES =  \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/inc
else
LHDC_INCLUDES =
endif

ifeq ($(A2DP_LHDCV5_ON),1)
obj-y += a2dp_decoder_cc_lhdcv5.o
CFLAGS_a2dp_decoder_cc_lhdcv5.o += -O3
LHDC_INCLUDES =  \
    -Ithirdparty/audio_codec_lib/liblhdcv5-dec/inc
else
LHDC_INCLUDES =
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
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Imultimedia/inc/audio/process/sbcplc/include \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ota \
    -Iservices/audioflinger \
    -Iutils/list \
    -Iapps/bt_sync \
    -Iservices/audio_manager \
    -Iapps/app_rpc	\
    $(SCALABLE_INCLUDES) \
    $(LDAC_INCLUDES) \
    $(LHDC_INCLUDES)

ccflags-y += -Imultimedia/inc/

ifeq ($(A2DP_AAC_ON),1)
AAC_INCLUDES = -Imultimedia/inc/
else
AAC_INCLUDES =
endif

ccflags-y += -Iapps/dsp_m55

ccflags-y += \
	$(AAC_INCLUDES) \
	$(SCALABLE_INCLUDES) \
    $(LDAC_INCLUDES) \
    $(LHDC_INCLUDES)
