cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_bt_cmd.cpp))

ifeq ($(BLE_STACK_NEW_DESIGN),1)
ifeq ($(BLE_HOST_SUPPORT),1)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_ble_test.cpp))
endif
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y := $(src_obj)

ccflags-y += -DBLUETOOTH_BLE_IMPL

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    $(BLE_STACK_INC_INTERNAL) \
    -I$(BLE_AOB_APP_DIR_PATH)/aob_bes_ux/inc \
    -I$(BLE_AOB_APP_DIR_PATH)/inc \
    -Iservices/bt_app \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/overlay \
    -Iservices/resources \
    -Iservices/bt_service/inc \
    -Iservices/lea_player/inc \
    -Iservices/ux/common/ble_audio_core/inc \
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
    -Iutils/cfifo \
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
    -Iservices/osif \
    -Iservices/ibrt/custom_api \
    -Iservices/ibrt/ibrt_middleware/inc \
    -Iservices/audio_manager \
    -Iservices/ota \
    -Iutils/list \
    -Iservices/aob_app/gaf_audio\
    -Iservices/aob_app/gaf_codec_off_bth

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iapps/earbuds/conn \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/app_ibrt/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc
endif
endif
