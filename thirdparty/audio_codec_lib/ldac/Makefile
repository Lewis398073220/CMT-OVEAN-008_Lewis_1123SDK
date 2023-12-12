#ldac_obj := src/

ifneq (,$(findstring dsp_, $(T)))
PLATFORM_NAME := m55
else ifneq (,$(findstring best1600_, $(T)))
PLATFORM_NAME := m33
endif

APP_LDAC_LIB_NAME := ldac_lib_$(PLATFORM_NAME)
$(APP_LDAC_LIB_NAME)-y := $(ldac_obj)
LIB_BIN_DIR := lib/thirdparty/audio_codec/ldac
obj-y += $(APP_LDAC_LIB_NAME).a
subdir-ccflags-y += \
		-Ithirdparty/audio_codec_lib/ldac/inc  \
		-Iservices/multimedia/speech/process/common/include \
		-Iservices/bt_app \
		-Iutils/heap \
		-Iapps/common \
		-Iplatform/hal

subdir-ccflags-y += -Wall -Wno-long-long


		
	