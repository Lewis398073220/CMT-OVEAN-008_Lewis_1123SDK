cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)umm_malloc/*.c))

ifeq ($(INTERACTION),1)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)green/*.cpp))
endif

ifeq ($(INTERCONNECTION),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)red/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)red/*.cpp))
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += -Iplatform/drivers/ana \
					-Iplatform/hal \
					-Iservices/bluedroid/adapter/src \
					-Iapps/key \
					-Iservices/audio_process \
					$(BLUETOOTH_ADAPTER_INCLUDES) \
					-Iservices/overlay \
					-Iservices/resources \
					-Iplatform/drivers/uarthci \
					-Iplatform/drivers/ana \
					-Iplatform/drivers/bt \
					-Iutils/cqueue \
					-Iutils/retention_ram \
					-Iservices/audioflinger \
					-Iutils/lockcqueue \
					-Iutils/intersyshci \
					-Iapps/main \
					-Iapps/common \
					-Iapps/audioplayers \
					-Iapps/factory \
					-Iservices/audio_bt \
					-Iservices/voicepath/gsound/gsound_target/ \
					-Iservices/bridge/ \
					-Iservices/interconnection/red \
					-Iservices/interconnection/green \
					-Iservices/interconnection/umm_malloc \
					-Iservices/ota \
					-Iservices/norflash_api \
					-Imultimedia/inc/opus121/include \
					-Imultimedia/inc/speech/application/Pcm8kCvsd/include \
					-Imultimedia/inc/speech/codec/codecs/adpcm \
					-Imultimedia/inc/speech/process/lc_mmse_ns/include \
					-Imultimedia/inc/speech/process/ns_and_ec/include \
					-Imultimedia/inc/speech/process/speech_2mic_ns2_denoise/include \
					-Imultimedia/inc/speech/process/dual_mic_denoise/include \
					-Imultimedia/inc/speech/process/far_field_speech_enhancement/include \
					-Imultimedia/inc/speech/process/plc/include \
					-Imultimedia/inc/speech/process/common/include \
					-Iservices/voicepath \
					-Iservices/nv_section/factory_section \
					-Iservices/voicepath/smartvoice  \
					-Iapps/battery   \
					-Ithirdparty/userapi \
					-Iutils/crc16 \
					-Iutils/crc32 \
					-Iutils/sha256	\
					-Iutils/aes256 \
					-Iutils/heap \
					-Imultimedia/inc/audio/codec/sbc/inc \
					-Iservices/norflash_api \
					-Iservices/audio_manager

ifeq ($(VOICE_PROMPT),1)
ccflags-y += -DMEDIA_PLAYER_SUPPORT
endif

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc
endif
endif
