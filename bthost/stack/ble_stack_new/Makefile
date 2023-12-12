cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(BLE_HOST_SUPPORT),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)smp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gatt/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)iso/bap.c))

ifeq ($(BLE_AUDIO_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/bap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/acc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/arc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/cap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/atc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/tmap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/hap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gaf/gmap/*.c))
endif
endif

ifeq ($(BLE_ISO_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)iso/iso.c))
endif

BTBLE_LIB_NAME := $(CHIP)_libble_stack_new

ifeq ($(HFP_1_6_ENABLE), 1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_sbc_enc
endif

ifeq ($(USE_TRACE_ID), 1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_log_compressed
endif

ifeq ($(BLE_AUDIO_ENABLED), 1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_aob
endif

ifeq ($(LC3PLUS_SUPPORT),1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_lc3plus
endif

ifeq ($(AOB_MOBILE_ENABLED), 1)
ifeq ($(BLE_USB_AUDIO_SUPPORT), 1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_usb_dongle
endif
endif

ifeq ($(GATT_OVER_BR_EDR), 1)
BTBLE_LIB_NAME := $(BTBLE_LIB_NAME)_btgatt
endif

$(BTBLE_LIB_NAME)-y := $(obj_c:.c=.o)
obj-y := $(BTBLE_LIB_NAME).a

CFLAGS_besble.o += -DBESLIB_INFO=$(BESLIB_INFO)

ccflags-y += -DBLUETOOTH_BLE_IMPL

subdir-ccflags-y += \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    $(BES_BT_IMPL_INCLUDES) \
    $(BES_STACK_ADAPTER_INCLUDES) \
    -Iplatform/drivers/bt \
    -Iutils/cqueue

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/custom_api \
    -Iservices/audio_manager \
    $(BLUETOOTH_ADAPTER_INCLUDES)
endif

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/ibrt_middleware/inc

ifeq ($(IBRT_CORE_V2),1)
subdir-ccflags-y += \
    -Iservices/app_ibrt_v2/inc \
    -Iservices/ibrt_core_v2/inc \
    -Iservices/bt_app \
    -Iapps/key \
    -Iservices/bt_app/a2dp_codecs/include
endif
ifeq ($(IBRT_UI_V2),1)
ccflags-y += \
    -Iservices/ibrt_ui_v2/inc
endif
endif
