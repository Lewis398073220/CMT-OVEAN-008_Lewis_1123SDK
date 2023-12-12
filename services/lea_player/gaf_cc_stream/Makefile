cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

src_obj := $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

BLE_AUDIO_LIB_NAME := lib_gaf_cc_stream

$(BLE_AUDIO_LIB_NAME)-y := $(src_obj)

obj-y :=  $(BLE_AUDIO_LIB_NAME).a

subdir-ccflags-y += \
    $(BLE_APP_INCLUDES) \
    -Iservices/overlay \
    -Iservices/mcpp \
    -Iservices/mcpp/thirdparty \
    -Iservices/audio_dump/include \
    -Iutils/kfifo \
    -Iutils/cfifo \
    -Iutils/heap \
    -Iapps/common \
    -Iapps/anc/inc \
    -Imultimedia/inc/speech/inc \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/inc/audio/codec/sbc/inc \
    -Imultimedia/inc/audio/codec/lc3/inc \
    -Imultimedia/inc/audio/smf/core/inc \
    -Imultimedia/inc/audio/smf/core/include \
    -Imultimedia/inc/audio/process/sbcplc/include \
    -Ibthost/service/ble_audio/aob_app/gaf_audio
