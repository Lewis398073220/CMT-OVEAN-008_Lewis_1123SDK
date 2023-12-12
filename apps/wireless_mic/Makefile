cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)test/*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iservices/overlay \
    -Iservices/resources \
    -Iservices/audio_bt \
    -Iservices/audioflinger \
    -Iservices/audio_process \
    -Iapps/audioplayers \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/intersyshci \
    -Iapps/main \
    -Iapps/key \
    -Iapps/common \
    -Iservices/bt_app/ \
    -Iservices/osif/ \
    $(AAC_INCLUDES) \
    -Iplatform/drivers/cp_accel \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -Iutils/list \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Iservices/ux/common/ble_audio_core/inc
