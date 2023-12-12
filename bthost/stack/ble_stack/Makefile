cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/ahi/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gap/gapc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gap/gapm/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gatt/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/hl/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/l2cap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/hci/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/mhdt/*.c))

ifeq ($(BLE_ISO_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/iso/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/iso/isogen/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/iso/isoohci/*.c))
endif

ifeq ($(BLE_AUDIO_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/acc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/acc/mc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/acc/ot/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/acc/tb/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/al/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/arc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/arc/aic/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/arc/mic/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/arc/vc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/arc/voc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/atc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/atc/csi/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/bap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/bap/bc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/bap/capa/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/bap/codec/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/bap/uc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/cap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/hap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/iap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/tb/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/tmap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/gaf/gmap/*.c))
endif

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/datapath/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/hrp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/hogp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/bas/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/dis/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/prf/*.c))

ifeq ($(BLE_AUDIO_ENABLED),1)
ble_profile_src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/csip/*.c))
ble_profile_src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/otp/*.c))
ble_profile_src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/dtp/*.c))
endif

ifeq ($(BES_AHP),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/ahp/ahps/*.c))
endif

ifeq ($(AI_VOICE),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/ai/*.c))
endif

ifeq ($(BMS),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/bms/*.c))
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/bms/bmsp/api \
        -I$(BLE_PROFILES_DIR_PATH)/bms/bmsp/src
endif

ifeq ($(BMSC),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/bms/*.c))
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/bms/bmsc/api \
        -I$(BLE_PROFILES_DIR_PATH)/bms/bmsc/src
endif

ifeq ($(ANCC), 1)
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/anc/ancc/api \
        -I$(BLE_PROFILES_DIR_PATH)/anc/ancc/src
endif

ifeq ($(ANCS), 1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/anc/*.c))
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/anc/ancs/api \
        -I$(BLE_PROFILES_DIR_PATH)/anc/ancs/src
endif

ifeq ($(AMS), 1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/ams/*.c))
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/ams/amsp/api \
        -I$(BLE_PROFILES_DIR_PATH)/ams/amsp/src
endif

ifeq ($(AMSC),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/ams/*.c))
ccflags-y += \
        -I$(BLE_PROFILES_DIR_PATH)/ams/amsc/api \
        -I$(BLE_PROFILES_DIR_PATH)/ams/amsc/src
endif

ifeq ($(TILE_DATAPATH_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/tile/*.c))
endif

ifeq ($(BES_SAS),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/sas/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/sas/sasc/src/*.c))
endif

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/ota/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/gfps/gfps_provider/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/buds/*.c))

ifeq ($(TEST_OVER_THE_AIR),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/tota/*.c))
subdir-ccflags-y +=-Iservices/tota
endif

ifeq ($(TEST_OVER_THE_AIR_v2),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ip/profiles/tota/*.c))
subdir-ccflags-y +=-Iservices/tota_v2
endif

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)modules/aes/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)modules/common/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)modules/ke/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)modules/rwip/*.c))

BTBLE_LIB_NAME := $(CHIP)_libble_stack

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
$(BTBLE_LIB_NAME)-y += $(ble_profile_src_obj:.c=.o)

obj-y := $(BTBLE_LIB_NAME).a

CFLAGS_besble.o += -DBESLIB_INFO=$(BESLIB_INFO)

ifeq ($(GFPS_ENABLE),1)
obj-y += ip/profiles/gfps/
subdir-ccflags-y += -Iservices/gfps/inc
endif

ccflags-y += -DBLUETOOTH_BLE_IMPL

subdir-ccflags-y += \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    $(BLE_STACK_INC_INTERNAL) \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    $(BES_BT_IMPL_INCLUDES) \
    -Iutils/cqueue \
    -Iservices/ota/bes_ota/inc \
    -Iservices/ota

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Iservices/ux/common/ibrt/custom_api \
    -Iservices/audio_manager \
    $(BLUETOOTH_ADAPTER_INCLUDES)
endif

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc

ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/bt_app \
    -Iapps/key \
    -Iservices/bt_app/a2dp_codecs/include
endif
ifeq ($(IBRT_UI),1)
ccflags-y += \
    -Iservices/ux/common/ibrt/ibrt_ui/inc
endif
endif

ifeq ($(mHDT_LE_SUPPORT),1)
ccflags-y += \
    -I$(BLE_STACK_DIR_PATH)/inc/mhdt
endif
