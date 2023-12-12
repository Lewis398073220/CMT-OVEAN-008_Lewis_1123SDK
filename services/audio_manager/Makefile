cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

AM_LIB_NAME := libaudio_manager

ifeq ($(BLE_AUDIO_ENABLED), 1)
AM_LIB_NAME := $(AM_LIB_NAME)_aob
endif

$(AM_LIB_NAME)-y := $(src_obj)

obj-y :=  $(AM_LIB_NAME).a

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)test/*.cpp))

obj-y +=$(obj_cpp:.cpp=.o)

CFLAGS_app_audio_active_device_manager.o += -DBESLIB_INFO=$(BESLIB_INFO)

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -I$(BLE_GAF_CORE_DIR_PATH)/inc \
    -I$(BLE_AOB_APP_DIR_PATH)/inc \
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
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ota \
    -Iservices/audioflinger \
    -Iservices/ux/common/ble_audio_core/inc \
    -Iutils/list \
    -Iapps/audio_manager \
    -Iservices/audio_manager/test

ccflags-y += -Imultimedia/inc

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/common/ibrt/ibrt_core/inc
endif
endif
