cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s :=
obj_c :=
obj_cpp :=

ifeq ($(IBRT),1)
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/custom_api/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/custom_api/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/custom_api/*.cpp))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_middleware/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_middleware/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_middleware/src/*.cpp))

ifeq ($(IBRT_UI),1)
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_core/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_core/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_core/src/*.cpp))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_ui/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_ui/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ibrt/ibrt_ui/src/*.cpp))
endif

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)product/earbud_tws/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)product/earbud_tws/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)product/earbud_tws/src/*.cpp))
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ble_audio_core/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ble_audio_core/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/ble_audio_core/src/*.cpp))
endif

ifeq ($(APP_SOUND_ENABLE),1)
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/sound/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/sound/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/sound/*.cpp))
endif

ifeq ($(BLE_BIS_TRANSPORT),1)
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/bis_transport/src/*.s))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/bis_transport/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)common/bis_transport/src/*.cpp))
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

UX_COMMON_NAME := libux

ifeq ($(IBRT_UI),1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_multipoint
endif

ifeq ($(AI_VOICE), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_ai
endif

ifeq ($(BLE), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_ble
endif

ifeq ($(USE_TRACE_ID), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_log_compressed
endif

ifeq ($(SASS_ENABLE), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_sass
endif

ifeq ($(GFPS_ENABLE), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_gfps
endif

ifeq ($(BLE_AUDIO_ENABLED), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_aob
endif

ifeq ($(SUPPORT_REMOTE_COD), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_cod
endif

ifeq ($(GATT_OVER_BR_EDR), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_gatt
endif

ifeq ($(FREEMAN_ENABLED_STERO), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_stero
endif

ifeq ($(BLE_STACK_NEW_DESIGN), 1)
UX_COMMON_NAME := $(UX_COMMON_NAME)_ble_new_stack
endif

ifeq ($(AOB_MOBILE_ENABLED), 1)
ifeq ($(BLE_USB_AUDIO_SUPPORT), 1)
CUSTOM_API_LIB_NAME := $(CUSTOM_API_LIB_NAME)_usb_dongle
endif
endif

$(UX_COMMON_NAME)-y := $(src_obj)
obj-y :=  $(UX_COMMON_NAME).a

CFLAGS_app_ui_mgr.o += -DBESLIB_INFO=$(BESLIB_INFO)
CFLAGS_app_tws_ibrt.o += -DBESLIB_INFO=$(BESLIB_INFO)
CFLAGS_app_custom_api.o += -DBESLIB_INFO=$(BESLIB_INFO)
CFLAGS_ble_audio_core.o += -DBESLIB_INFO=$(BESLIB_INFO)

ifneq ($(APP_SOUND_THREAD_SIZE),)
KBUILD_CPPFLAGS += -DAPP_SOUND_THREAD_SIZE=$(APP_SOUND_THREAD_SIZE)
endif

ifeq ($(BLE_BIS_TRANSPORT),1)
ifeq ($(BLE_BIS_TRAN_INPUT_BT),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRAN_INPUT_BT
endif
ifeq ($(BLE_BIS_TRAN_INPUT_USB),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRAN_INPUT_USB
endif
ifeq ($(BLE_BIS_TRAN_INPUT_WIFI),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRAN_INPUT_WIFI
endif
ifeq ($(BLE_BIS_TRAN_INPUT_LINEIN),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRAN_INPUT_LINEIN
endif
export BLE_BIS_TRAN_CARRY_PRIVATE_DATA ?= 1
ifeq ($(BLE_BIS_TRAN_INPUT_LINEIN),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRAN_CARRY_PRIVATE_DATA
endif
endif


subdir-ccflags-y += \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/osif/ \
    -Iservices/overlay \
    -Iservices/resources \
    -Iservices/ux/common/ble_audio_core/inc \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -I$(BLE_GAF_CORE_DIR_PATH)/inc \
    -I$(BLE_AOB_APP_DIR_PATH)/inc \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/usb/usb_dev/inc \
    -Iplatform/hal \
    -Iutils/cqueue \
    -Iutils/eshell \
    -Iutils/heap \
    -Iutils/hwtimer_list \
    -Iutils/lockcqueue \
    -Iutils/list \
    -Iutils/intersyshci \
    -Iutils/crc16 \
    -Iutils/crc32 \
    -Iutils/rom_utils \
    -Iutils/cppobject \
    -Iutils/cfifo \
    -Iapps \
    -Iapps/key \
    -Iapps/main \
    -Iapps/battery \
    -Iapps/bt_sync \
    -Iapps/common \
    -Iapps/anc/inc	\
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Iapps/audioplayers/audio_trigger/version1 \
    -Iapps/ble_audio_test \
    -Iapps/factory \
    -Iapps/psap/inc \
    -Iapps/sensorhub \
    -Iapps/ble_audio_central_app/ble_audio_usb \
    -Ibthost/service/bt_app \
    -Imultimedia/inc/audio/process/resample/include \
    -Iservices/audio_bt \
    -Iservices/audioflinger \
    -Iservices/audio_manager \
    -Iservices/audio_manager/test \
    -Iservices/ai_voice/manager \
    -Iservices/ai_voice/transport \
    -Iservices/ai_voice/codec/compression \
    -Iservices/ai_voice/codec/opus121/include \
    -Iservices/ai_voice/protocol/gma/gma_manager \
    -Iservices/ai_voice/protocol/gma/gma_crypto \
    -Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
    -Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
    -Iservices/ai_voice/protocol/bixbyvoice \
    -Iservices/ai_voice/protocol/bixbyvoice/bixbyvoice_manager \
    -Iservices/app_debug_info_system/inc \
    -Iservices/app_ai/inc \
    -Iservices/bt_app \
    -Iservices/bis_transport/inc \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/nv_section/factory_section \
    -Iservices/nv_section/log_section \
    -Iservices/norflash_api \
    -Iservices/ota \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/interconnection/green \
    -Iservices/voicepath \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target \
    -Iservices/lea_player/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/tota_v2 \
    -Ithirdparty/userapi \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
    -Iapps/earbuds/conn

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/bt_app \
    -Iservices/ux/common/ibrt/custom_api \
    -Iservices/ux/common/ibrt/app_ibrt/inc \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ux/common/config
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc\
    -Iservices/ux/product/earbud_tws/inc \
    -Iutils/fsm \
    -Iservices/osif \
    -Iutils/hsm \
    -Iutils/kfifo
endif
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
subdir-ccflags-y += \
    $(BLE_STACK_INCLUDES) \
    -Iservices/log_manager \
    -Iservices/audio_manager \
    -Iservices/ux/common/ble_audio_core/inc
endif

ifeq ($(BLE_BIS_TRANSPORT),1)
subdir-ccflags-y += \
    -Iservices/ux/common/bis_transport/inc \
    -Iservices/multimedia/audio/process/filters/include \
    -Iservices/multimedia/audio/codec/lc3/inc/ \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/audioflinger \
    -Iservices/audio_bt     \
    -Iservices/audio_process    \
    -Imultimedia/inc/audio/process/filters/include \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Itests/anc_usb    \
    -Iapps/common \
    -Iapps/key
endif

subdir-ccflags-y += -Wno-date-time

export APP_UI_MONITOR ?= 0
ifeq ($(APP_UI_MONITOR),1)
KBUILD_CPPFLAGS += -DAPP_UI_MONITOR

export APP_UI_MONITOR_GUARD_TIMEOUT ?= 45000
KBUILD_CPPFLAGS += -DAPP_UI_MONITOR_GUARD_TIMEOUT=$(APP_UI_MONITOR_GUARD_TIMEOUT)

# /* record buff size(power of 2!) */
export APP_UI_MONITOR_RECORD_BUF_SIZE ?= 1024
KBUILD_CPPFLAGS += -DAPP_UI_MONITOR_RECORD_BUF_SIZE=$(APP_UI_MONITOR_RECORD_BUF_SIZE)
endif

export APP_UI_TEST_MODE_ENABLE ?= 0
ifeq ($(APP_UI_TEST_MODE_ENABLE),1)
KBUILD_CPPFLAGS += -DAPP_UI_TEST_MODE_ENABLE
endif

ifneq ($(APP_UI_THREAD_SIZE),)
KBUILD_CPPFLAGS += -DAPP_UI_THREAD_SIZE=$(APP_UI_THREAD_SIZE)
endif

ifeq ($(BES_OTA), 1)
subdir-ccflags-y += \
    -Iservices/ota/bes_ota/inc
endif

ifeq ($(AI_OTA), 1)
subdir-ccflags-y += \
    -Iservices/ota/ai_ota
endif

ifeq ($(GFPS_ENABLE),1)
ccflags-y += \
   -Iservices/ble_app/app_gfps \
   -Iservices/gfps/inc
endif

ifeq ($(TWS_RS_WITHOUT_MOBILE),1)
subdir-ccflags-y += \
    -Iapps/audioplayers/audio_trigger/version1
endif

ifeq ($(ANC_APP),1)
CFLAGS_app_ibrt_ui_test.o += -DANC_APP
endif

ifeq ($(A2DP_SBC_PLC_ENABLED),1)
CFLAGS_app_tws_ibrt_ui_test.o += -DA2DP_SBC_PLC_ENABLED
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

ifeq ($(APP_UART_MODULE),1)
ccflags-y += -Iapps/common/uart_dma
endif

ifeq ($(APP_CHIP_BRIDGE_MODULE),1)
ccflags-y += -Iapps/common/chip_bridge
endif

ifeq ($(BT_SOURCE),1)
ccflags-y += \
	-Iservices/ux/common/ibrt/ibrt_core/inc
endif
