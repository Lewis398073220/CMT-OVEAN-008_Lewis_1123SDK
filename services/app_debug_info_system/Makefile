
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

APP_DBG_INFO_SYSTEM := libapp_dbg_system

ifeq ($(USE_TRACE_ID), 1)
APP_DBG_INFO_SYSTEM := $(APP_DBG_INFO_SYSTEM)_log_compressed
endif

$(APP_DBG_INFO_SYSTEM)-y := $(src_obj)

obj-y :=  $(APP_DBG_INFO_SYSTEM).a

subdir-ccflags-y += \
    $(BT_PROFILES_INCLUDES) \
    $(BT_IF_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -Iapps/key \
    -Iutils/cqueue \
    -Iutils/list \
    -Iapps/common \
    -Iapps/ble_audio_test \
    -Iplatform/drivers/bt \
    -Iservices/bt_app \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/bt_if_enhanced/inc \
    -Iservices/lea_player/inc \
    -Iservices/ibrt/ibrt_core/inc \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/app_debug_info_system/inc \
    -Iservices/ux/common/ble_audio_core/inc

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc
endif
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_ui/inc
endif
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
subdir-ccflags-y += \
    -Iservices/audio_manager \
    -Iservices/audio_manager/test
endif
