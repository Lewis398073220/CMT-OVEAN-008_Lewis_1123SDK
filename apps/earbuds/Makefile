
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)conn/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)conn/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)conn/*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iservices/osif \
    -Iservices/nv_section/factory_section \
    -Iservices/app_ai/inc \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/sensor_hub \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iutils/list \
    -Iapps \
    -Iapps/key \
    -Iapps/main \
    -Iapps/common \
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Iapps/factory \
    -Iapps/ble_audio_test \
    -Iservices/nv_section/log_section \
    -Iservices/audio_bt/ \
    -Iservices/voicepath \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ai_voice/protocol/gma/gma_manager \
    -Iservices/ai_voice/protocol/gma/gma_crypto \
    -Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
    -Iservices/ai_voice/protocol/bixbyvoice \
    -Iservices/ai_voice/protocol/bixbyvoice/bixbyvoice_manager \
    -Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
    -Iservices/ai_voice/codec/compression \
    -Iservices/ai_voice/codec/opus121/include \
    -Iservices/norflash_api \
    -Ibthost/service/bt_app/inc \
    -Iservices/audio_manager \
    -Iservices/audio_manager/test \
    -Iapps/bt_sync \
    -Iutils/hsm \
    -Iservices/ux/common/ibrt/custom_api \
    -Iservices/ux/common/ibrt/app_ibrt/inc \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iapps/earbuds/conn/ \
    -Iservices/gfps/inc \
    -Ibthost/stack/bt_if/inc

ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iapps/earbuds/conn
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
subdir-ccflags-y += \
    $(BLE_STACK_INCLUDES) \
    -Iservices/log_manager \
    -Iservices/audio_manager \
    -Iservices/ux/common/ble_audio_core/inc
endif

ifeq ($(BES_OTA), 1)
subdir-ccflags-y += \
    -Iservices/ota/bes_ota/inc
endif

ifeq ($(A2DP_AUDIO_STEREO_MIX_CTRL),1)
CFLAGS_app_ibrt_customif_ui.o += -DA2DP_AUDIO_STEREO_MIX_CTRL
endif

ifeq ($(SPA_AUDIO_ENABLE),1)
subdir-ccflags-y += -Ithirdparty/userapi/spa_dummy_app/inc
ifeq ($(SPA_AUDIO_SEC),1)
ccflags-y += -Ithirdparty/userapi/spa_dummy_app/sec
endif
endif

ifeq ($(AI_VOICE),1)
subdir-ccflags-y += \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/ai_voice/transport
endif

GPIO_WAKEUP_ENABLE ?= 0
ifeq ($(GPIO_WAKEUP_ENABLE),1)
subdir-ccflags-y += -DGPIO_WAKEUP_ENABLE
endif

ifeq ($(CORE_BRIDGE_DEMO_MSG),1)
subdir-ccflags-y += -DCORE_BRIDGE_DEMO_MSG \
-Iapps/sensorhub
endif