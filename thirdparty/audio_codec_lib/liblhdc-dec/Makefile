#lhdc_obj := src/
subdir-ccflags-y += \
		-Ithirdparty/audio_codec_lib/liblhdc-dec/inc  \
		-Iservices/multimedia/speech/process/common/include \
		-Iservices/bt_app

subdir-ccflags-y += -Wall -Wno-long-long



cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))
# -DHAVE_TEST_KEY

ifeq ($(A2DP_LHDC_LARC), 1)
auth_type += -DHAVE_LARC
endif

ifneq ($(GEN_LIB), 1)
auth_type += -DHAVE_TEST_KEY
endif

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dec_src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dec_src/lhdc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dec_src/llac/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifneq (,$(findstring dsp_, $(T)))
PLATFORM_NAME := m55
else
PLATFORM_NAME := m33
endif

APP_LHDC_LIB_NAME := liblhdc-dec_$(PLATFORM_NAME)
$(APP_LHDC_LIB_NAME)-y := $(lhdc_obj)
LIB_BIN_DIR := lib/thirdparty/audio_codec/liblhdc-dec
obj-y += $(APP_LHDC_LIB_NAME).a


#CFLAGS_dec.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_cirbuf.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_lhdcUtil.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_encdec.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_lic_check.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_licdecrypt.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
ccflags-y +=  -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant

subdir-ccflags-y += \
    -Iservices/tws/inc \
    -Iutils/heap \
    -Iutils/cqueue \
    -Iapps/audioplayers \
    -Iservices/bt_app \
    -Iplatform \
    -Iplatform/hal \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/dec_src \
    -Ithirdparty/audio_codec_lib/liblhdc-dec \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iconfig \
    -Iinclude/cmsis_dsp
