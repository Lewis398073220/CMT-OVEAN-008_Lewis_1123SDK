
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/resources \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/uarthci \
    -Iplatform/hal \
    -Iutils/cqueue \
    -Iutils/list \
    -Iutils/heap \
    -Iutils/crc16 \
    -Iutils/crc32 \
    -Iutils/rom_utils \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iutils/hwtimer_list \
    -Iapps \
    -Iapps/key \
    -Iapps/main \
    -Iapps/battery \
    -Iapps/bt_sync \
    -Iapps/common \
    -Iapps/factory \
    -Iapps/anc/inc	\
    -Iapps/btspeaker \
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Iapps/audioplayers/audio_trigger/version1 \
    -Iapps/sensorhub \
    -Iservices/audio_bt \
    -Iservices/ai_voice/manager \
    -Iservices/ai_voice/transport \
    -Iservices/nv_section/factory_section \
    -Iservices/nv_section/log_section \
    -Iservices/norflash_api \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/multimedia/audio/process/resample/include \
    -Iservices/interconnection/green \
    -Iservices/voicepath	\
    -Iservices/app_ai/inc \
    -Iservices/audio_manager \
    -Iservices/bt_sync \
    -Iservices/overlay \
    -Iservices/ota \
    -Iservices/tota_v2 \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/ai_voice/protocol/bixbyvoice \
    -Iservices/ai_voice/protocol/bixbyvoice/bixbyvoice_manager \
    -Iservices/ai_voice/protocol/gma/gma_manager \
    -Iservices/ai_voice/protocol/gma/gma_crypto \
    -Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
    -Iservices/ai_voice/codec/compression \
    -Iservices/ai_voice/codec/opus121/include \
    -Ithirdparty/userapi \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/inc

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/bt_app \
    -Iservices/ux/common/ibrt/custom_api \
    -Iservices/ux/common/ibrt/app_ibrt/inc \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iutils/fsm \
    -Iservices/osif \
    -Iservices/ux/common/ibrt/ibrt_core/inc  \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iapps/earbuds/conn \
    -Iutils/hsm \
    -Iutils/kfifo
endif
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
subdir-ccflags-y += \
    $(BLE_STACK_INCLUDES) \
    -Iservices/ux/common/ble_audio_core/inc
endif


ifeq ($(GFPS_ENABLE),1)
ccflags-y += \
   -Iservices/ble_app/app_gfps \
   -Iservices/gfps/inc
endif

ifeq ($(BES_OTA), 1)
subdir-ccflags-y += \
    -Iservices/ota/bes_ota/inc
endif

ifeq ($(AI_OTA), 1)
subdir-ccflags-y += \
    -Iservices/ota/ai_ota
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
subdir-ccflags-y += \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target_api_read_only
endif

ifeq ($(CUSTOMER_APP_BOAT),1)
subdir-ccflags-y += -Iservices/tota_v2
endif

ifeq ($(SPA_AUDIO_ENABLE),1)
subdir-ccflags-y += -Ithirdparty/userapi/spa_dummy_app/inc
ifeq ($(SPA_AUDIO_SEC),1)
ccflags-y += -Ithirdparty/userapi/spa_dummy_app/sec
endif
endif