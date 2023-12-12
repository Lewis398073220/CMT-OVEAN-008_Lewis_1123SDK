
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -I$(BLE_STACK_DIR_PATH)/inc \
    -I$(BLE_STACK_DIR_PATH)/inc/l2cap \
    -I$(BLE_STACK_DIR_PATH)/inc/gap \
    -I$(BLE_STACK_DIR_PATH)/inc/gatt \
    -I$(BT_PROFILES_DIR_PATH)/inc \
    -I$(BT_APP_DIR_PATH)/inc \
    -I$(BT_APP_DIR_PATH)/inc/a2dp_codecs \
    -I$(BT_IF_DIR_PATH)/inc \
    -I$(BLE_APP_DIR_PATH)/inc \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/adapter_service \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/common \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/common \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gap \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gatt \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/aob \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gfps \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/dp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/ai \
    -Iservices/gfps/inc \
    -Iutils/cqueue \
    -Iutils/encrypt \
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iplatform/drivers/bt \
    -Iapps/key \
    -Iapps/main \
    -Iapps/earbuds/conn \
    -Iservices/audio_manager \
    -Iservices/audio_bt


