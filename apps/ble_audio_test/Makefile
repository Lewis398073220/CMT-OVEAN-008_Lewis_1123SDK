cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c :=
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ble_audio_default_config.cpp))
obj_s :=

ifeq ($(BLE_AUDIO_ENABLED),1)
ifeq ($(BLE_STACK_NEW_DESIGN),0)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ble_audio_test.cpp))
endif
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y := $(src_obj)

ccflags-y += -DBLUETOOTH_BLE_IMPL

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -Iservices/bt_app \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/overlay \
    -Iservices/resources \
    -Iservices/lea_player/inc \
	-Iservices/ux/common/ble_audio_core/inc \
    -Imultimedia/inc/audio/process/resample/include \
    -Iservices/nv_section/factory_section \
    -Iservices/nv_section/fpga_section \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/cfifo \
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
    -Iapps/wireless_mic \
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
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/audio_manager \
    -Iservices/ota \
    -Iutils/list \
    -Iservices/aob_app/gaf_audio\
    -Iservices/tota_v2/ \
    -Iservices/ota/bes_ota/inc

ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/product/earbud_tws/inc
endif