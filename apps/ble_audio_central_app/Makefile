cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_lea_usb := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ble_audio_usb/*.cpp))
obj_lea_i2s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ble_audio_i2s/*.cpp))

ifeq ($(BLE_I2S_AUDIO_SUPPORT),1)
obj_cpp += $(obj_lea_i2s)
else ifeq ($(BLE_USB_AUDIO_SUPPORT),1)
obj_cpp += $(obj_lea_usb)
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iservices/overlay \
    -Iservices/resources \
    -Iservices/audio_bt \
    -Iservices/audioflinger \
    -Iservices/audio_process \
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/inc/audio/process/filters/include \
    -Imultimedia/inc/audio/codec/sbc/inc \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -I$(BLE_GAF_CORE_DIR_PATH)/inc \
    -I$(BLE_AOB_APP_DIR_PATH)/inc \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/cfifo \
    -Iutils/heap \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iapps/main \
    -Iapps/key \
    -Iapps/common \
    -Iapps/battery \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/app_bt_source/inc \
    -Iservices/app_bt_source/src/a2dp_encoder_cp \
    -Iservices/bt_app/ \
    -Iservices/osif/ \
    -Iservices/tota/ \
    -Iservices/lea_player/inc \
    -Ithirdparty/audio_codec_lib/liblhdc-enc/inc \
    $(AAC_INCLUDES) \
    -Iplatform/drivers/cp_accel \
    -Iservices/norflash_api \
    -Itests/anc_usb \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -Iutils/list \
    -Iutils/hsm \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Iapps/bt_sync \
    -Iapps/main \
    -Iservices/audio_dump/include \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/common/ble_audio_core/inc \
    -Iservices/audio_manager \
    -Iutils/hwtimer_list

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -DDMA_AUDIO_APP
subdir-ccflags-y += \
    -Iservices/dma_audio/app \
    -Iservices/dma_audio/common
endif

ifeq ($(APP_UART_MODULE),1)
ccflags-y += -Iapps/common/uart_dma
endif

ifeq ($(APP_CHIP_BRIDGE_MODULE),1)
ccflags-y += -Iapps/common/chip_bridge
endif