cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/ai_voice/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/ai_voice/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/ai_voice/*.cpp))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/common/*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/common/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/common/*.cpp))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_cpp:.cpp=.o)

ccflags-y += -DBLUETOOTH_BLE_IMPL

ifeq ($(IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED),1)
obj-y += ../../../services/app_debug_info_system/
endif

obj-y += ../../../utils/retention_ram/

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -I$(BLE_APP_DIR_PATH)/app_dis/ \
    -I$(BLE_APP_DIR_PATH)/app_ahp \
    -I$(BLE_APP_DIR_PATH)/app_sas \
    -Ibthost/stack/dbg/ \
    -Ibthost/stack/patch/ \
    -Ibthost/stack/ble_if/inc \
    -Ibthost/porting/ \
    -Iservices/voicepath/$(VOICE_DATAPATH_TYPE) \
    -Iservices/audio_process \
    -Iapps/audioplayers/a2dp_decoder \
    -Iservices/lea_player/inc \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/overlay \
    -Iservices/nvrecord \
    -Iservices/resources \
    -Iservices/voicepath/ \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/retention_ram \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iutils/heap \
    -Iutils/hsm \
    -Iutils/crc16 \
    -Iutils/hwtimer_list \
    -Iutils/aes_cmac \
    -Iutils/ \
    -Iapps/battery \
    -Iapps/key \
    -Iapps/main \
    -Iapps/common \
    -Iapps/bt_sync \
    -Iapps/audioplayers \
    -Iapps/factory \
    -Iservices/audio_dump/include \
    -Iservices/bridge/ \
    -Imultimedia/inc/speech/inc \
    -Iservices/audio_bt \
    -Iservices/bt_sync \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_custom/src \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/ai_voice/manager \
    -Iservices/ai_voice/transport \
    -Iservices/app_ai/inc \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/voicepath/gsound/gsound_target \
    -Ithirdparty/tile \
    -Ithirdparty/tile/tile_target \
    -Ithirdparty/tile/tile_common/tile_assert \
    -Ithirdparty/tile/tile_common/tile_features \
    -Ithirdparty/tile/tile_common/tile_gatt_db \
    -Ithirdparty/tile/tile_common/tile_player \
    -Ithirdparty/tile/tile_common/tile_service \
    -Ithirdparty/tile/tile_common/tile_storage \
    -Iservices/ota\
    -Iservices/audio_manager \
    -Iutils/list \
    -Iservices/app_debug_info_system/inc \
    -Iutils/encrypt \
    -Iapps/anc/inc \
    -Iservices/osif

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -DSW_PLAYBACK_RESAMPLE
endif
ifeq ($(SW_CAPTURE_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -DSW_CAPTURE_RESAMPLE
endif
ifeq ($(BES_OTA_BASIC),1)
ifeq ($(IBRT_OTA),1)
ccflags-y += -Iservices/ibrt_ota/inc
else
ccflags-y += -Iservices/ota
endif
endif

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_core/inc  \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/common/ibrt/app_ibrt/inc \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iapps/earbuds/conn \
    -Iservices/ux/product/earbud_tws/inc
endif
endif

ifeq ($(BES_OTA),1)
ccflags-y += -Iservices/ota/bes_ota/inc
endif

ifeq ($(BISTO_ENABLE),1)
ccflags-y += \
    -I$(BLE_APP_DIR_PATH)/app_gfps
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
ccflags-y += \
    -Iservices/audio_manager \
    -Iservices/ux/common/ble_audio_core/inc
endif
