cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../gaf_codec/*.cpp))

lib_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
lib_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
lib_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../gaf_cc_stream/cc_stream_common.cpp))
lib_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../gaf_cc_stream/gaf_cc_stream_process.cpp))

src_obj := $(lib_obj:.cpp=.o)

BLE_AUDIO_LIB_NAME := lib_gaf_m55_stream

$(BLE_AUDIO_LIB_NAME)-y := $(src_obj)

obj-y := $(BLE_AUDIO_LIB_NAME).a
obj-y += $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    $(BLE_APP_INCLUDES) \
    -Iservices/overlay \
    -Iservices/mcpp \
    -Iservices/mcpp/thirdparty \
    -Iservices/audio_dump/include \
    -Iservices/lea_player/inc \
    -Iutils/kfifo \
    -Iutils/cfifo \
    -Iutils/heap \
    -Iapps/common \
    -Iapps/anc/inc \
    -Imultimedia/inc/speech/inc \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/inc/audio/process/sbcplc/include \
    -Ibthost/service/ble_audio/aob_app/gaf_audio

ccflags-y += -Iapps/dsp_m55
subdir-ccflags-y += -Imultimedia/inc/
subdir-ccflags-y += -Imultimedia/inc/smf