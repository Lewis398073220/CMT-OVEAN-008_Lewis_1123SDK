cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)protobuf/*.c))

all_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*.cpp))
all_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*.cpp))
all_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)protobuf/*.cpp))

filtout_cpp :=
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*_test.cpp))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*_test.cpp))
ifneq ($(TWS_SYSTEM_ENABLED), 1)
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*_tws.cpp))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*_tws.cpp))
endif

ifneq ($(GSOUND_HOTWORD_ENABLE), 1)
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*hotword*.cpp))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*hotword*.cpp))
else
ifeq ($(GSOUND_HOTWORD_EXTERNAL), 1)
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*hotword_internal.cpp))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*hotword_internal.cpp))
else
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_target/*hotword_external.cpp))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gsound_custom/src/*hotword_external.cpp))
endif
endif

obj_cpp := $(filter-out $(filtout_cpp),$(all_cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

LIB_BIN_DIR := lib/thirdparty/gsound
# include the libgsound
obj-y += libgsound.a

# include the dsp library if hotword enabled
ifeq ($(GSOUND_HOTWORD_ENABLE), 1)
#ifneq ($(VOICE_DETECTOR_SENS_EN),1)
obj-y += libcortexm4_google_hotword_dsp_multi_bank_api.a
#endif
endif

subdir-ccflags-y += \
    -Iapps/audioplayers/ \
    -Iapps/common/ \
    -Iapps/battery/ \
    -Iapps/key/ \
    -Iapps/main/ \
    -Iapps/factory \
    -Iapps/voice_detector \
    -Iplatform/hal \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/norflash \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/cp_accel \
    -Iservices/ai_voice/manager \
    -Iservices/ux/common/ibrt/ibrt_middleware/inc \
    -Iservices/ux/product/earbud_tws/inc \
    -Iservices/app_ai/inc \
    -Iservices/audio_bt \
    -Iservices/audio_dump/include \
    -Iservices/nv_section/customparam_section/ \
    -Iservices/nv_section/log_section/ \
    -Iservices/voicepath/gsound \
    -Iservices/voicepath/gsound/gsound_target/ \
    -Iservices/voicepath/gsound/gsound_target_api_read_only/ \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/protobuf \
    -Iservices/cp_accel \
    -Iservices/resources \
    -Iservices/bluedroid/adapter/src \
    -Iservices/audio_process \
    -Iservices/overlay \
    -Iservices/audioflinger \
    -Iservices/bridge/ \
    $(BLUETOOTH_ADAPTER_INCLUDES) \
    -Imultimedia/inc \
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
    -Imultimedia/inc/speech/application/Pcm8kCvsd/include \
    -Iservices/app_ai/voice_sbc \
    -Iservices/norflash_api \
    -Iservices/ota/ai_ota \
    -Iservices/ai_voice/audio \
    -Ithirdparty/userapi \
    -Iutils/cqueue \
    -Iutils/retention_ram \
    -Iutils/crc32 \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iutils/heap \
    -Iservices/audio_manager \
    -Iapps/earbuds/conn

ifeq ($(FLASH_SUSPEND),1)
CFLAGS_gsound_custom_ota.o += -DFLASH_SUSPEND
endif

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
subdir-ccflags-y += \
    -Iservices/ux/common/ibrt/app_ibrt/inc  \
    -Iservices/ux/common/ibrt/ibrt_core/inc \
    -Iservices/ux/common/ibrt/ibrt_ui/inc \
    -Iservices/ux/product/earbud_tws/inc
endif
endif

ifeq ($(OTA_ENABLE),1)
ccflags-y += -Iservices/ota
endif

ifeq ($(VOICE_DETECTOR_SENS_EN),1)
ccflags-y +=     -Itests/sensor_hub/inc \
                 -Iapps/sensorhub
endif
