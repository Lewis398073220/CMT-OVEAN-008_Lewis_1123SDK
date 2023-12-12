CHIP		?= best1600

DEBUG		?= 1

DEBUG_PORT	?= 2

FAULT_DUMP	?= 1

export FREEMAN_ENABLED_STERO ?= 0
ifeq ($(FREEMAN_ENABLED_STERO),1)
KBUILD_CPPFLAGS += -DFREEMAN_ENABLED_STERO
endif

NO_ISPI ?= 1
export NO_ISPI

ifneq ($(NOSTD),1)
RTOS		?= 1
endif

ifeq ($(RTOS),1)
USE_MEM_CFG ?= 1
else
NOAPP		?= 1
export NOAPP
endif

ifeq ($(PT_APP),1)
USE_MEM_CFG ?= 1
endif

AUDIO_RESAMPLE ?= 1

export DMA_APPLIED_ON_M55 ?= 1

export BTH_AS_MAIN_MCU ?= 1

export NO_CMU_INIT ?= 1

export OSC_26M_X4_AUD2BB ?= 1

export ULTRA_LOW_POWER ?= 1

ifneq ($(NOAPP),1)
ifeq ($(BTH_AS_MAIN_MCU),1)
export BTH_USE_SYS_FLASH        ?= 1
export BTH_USE_SYS_PERIPH       ?= 1
export BTH_CTRL_DSP_M55_SYSFREQ ?= 1
else
export BTH_CTRL_DSP_M55_SYSFREQ ?= 0
endif
endif

export DSP_LIB ?= 0
export CMSIS_M55_LIB ?= 1
export BES_MEMCPY_SRC_VERSION := 2

ifeq ($(PSAP_SW_APP),1)
export SPEECH_LIB ?= 1
export DMA_AUDIO_APP ?= 1
export DMA_AUDIO_APP_DYN_ON ?= 1
endif

export ANC_ASSIST_VPU  ?= 0
export ANC_ALGO_DSP ?= 0
export ANC_ASSIST_SAMPLE_RATE  ?= 16000
# ANC_ASSIST_SAMPLE_RATE must be 16000 or 32000 or 48000

ifeq ($(FIR_ADAPT_ANC_M55),1)
KBUILD_CPPFLAGS += -DFIR_ADAPT_ANC_M55
KBUILD_CPPFLAGS += -DASSIST_RESULT_FIFO_BUF
endif

export DMA_AUDIO_APP ?= 0
ifeq ($(DMA_AUDIO_APP),1)
export DMA_RPC_SVR ?= 1
-include $(srctree)/config/$(CHIP)/dma_aud_cfg.mk
endif

export SUBSYS_IMAGE_SEGMENT ?= 1

init-y      :=
core-y      := tests/dsp_m55/ platform/cmsis/ platform/hal/ utils/hwtimer_list/ utils/stream_mcps/
ifneq ($(NOAPP),1)
core-y      += utils/heap/ utils/cqueue/ utils/kfifo/ utils/list/ utils/cfifo/
core-y      += multimedia/
endif

ifeq ($(ANC_ASSIST_VPU),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_VPU
endif

export LC3PLUS_HR_ENABLE ?= 0
ifeq ($(LC3PLUS_HR_ENABLE),1)
KBUILD_CPPFLAGS += -DLC3PLUS_HR_ENABLE
endif

ifeq ($(ANC_ALGO_DSP),1)
KBUILD_CPPFLAGS += -DANC_ALGO_DSP
export SPEECH_LIB := 1
export ANC_ASSIST_DSP := 1
endif

ifeq ($(ANC_ASSIST_DSP),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_DSP
KBUILD_CPPFLAGS += -DANC_ASSIST_ON_DSP_M55
core-y += apps/voice_assist/
core-y += apps/anc_assist/dsp_src/
export APP_RPC_BTH_M55_EN := 1
export APP_RPC := 1
endif

ifeq ($(AUDIO_DSP_ACCEL_USE_M55),1)
KBUILD_CPPFLAGS += -DAUDIO_DSP_ACCEL_USE_M55
export APP_MCPP_CLI := M55
export APP_MCPP_SRV ?= thirdparty
export APP_MCPP_BTH_M55 := 1
endif

ifneq ($(SPEECH_ALGO_DSP),)
export APP_MCPP_CLI := M55
export APP_MCPP_SRV := $(SPEECH_ALGO_DSP)
export APP_MCPP_BTH_M55 := 1
endif

ifeq ($(APP_MCPP_CLI),M55)
core-y += services/mcpp/
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI_M55
export APP_RPC := 1
export SPEECH_LIB := 1
export APP_RPC_BTH_M55_EN := 1
endif

ifneq ($(APP_MCPP_SRV),)
KBUILD_CPPFLAGS += -DAPP_MCPP_SRV
endif

ifeq ($(AUDIO_ALGO_DSP),1)
core-y += services/audio_algo_dsp/
KBUILD_CPPFLAGS += -DAUDIO_ALGO_DSP
export AUDIO_ALGO_DSP_DEBUG ?= 0
ifeq ($(AUDIO_ALGO_DSP_DEBUG),1)
KBUILD_CPPFLAGS += -DAUDIO_ALGO_DSP_DEBUG
export M55_AUDIO_DUMP ?= 1
endif
endif

export M55_AUDIO_DUMP ?= 0
ifeq ($(M55_AUDIO_DUMP),1)
KBUILD_CPPFLAGS += -DM55_AUDIO_DUMP
export DSP_M55_TRC_TO_MCU := 0
export DEBUG_PORT := 1
export AUDIO_DEBUG := 1
export TRACE_BAUD_RATE := 3000000
endif

ifeq ($(DMA_AUDIO_APP),1)
core-y += platform/drivers/stream_dma_rpc/ services/dma_audio/dsp/
endif

ifeq ($(PSAP_SW_APP),1)
core-y += apps/psap_sw/dsp/
endif

ifeq ($(AUDIO_DOWN_MIXER),1)
core-y += apps/audio_down_mixer/
endif

ifeq ($(SPEECH_TUNING_CALL_VIA_CMD),1)
KBUILD_CPPFLAGS += -DSPEECH_TUNING_CALL_VIA_CMD
endif

ifeq ($(SMF_RPC_M55),1)
ifneq ($(APP_RPC),1)
core-y += apps/app_rpc/
endif
endif

ifeq ($(APP_RPC),1)
core-y += apps/app_rpc/
endif

ifeq ($(USE_MEM_CFG),1)
-include $(srctree)/config/$(CHIP)/mem_cfg.mk
endif

ifeq ($(SUBSYS_FLASH_BOOT),1)
core-y		+= platform/drivers/norflash/
LDS_FILE	:= best1000.lds
else
LDS_FILE	:= subsys.lds
endif

export DSP_M55_TRC_TO_MCU ?= 1

ifeq ($(DSP_M55_TRC_TO_MCU),1)
ifneq ($(filter best1600 best1603,$(CHIP)),)
export RMT_TRC_IN_MSG_CHAN ?= 1
endif
export RMT_TRC_BUF_SIZE ?= 4096
export TRACE_BUF_SIZE ?= 0
export TRACE_GLOBAL_TAG ?= 1
export DUMP_LOG_ENABLE ?= 1
export LOG_DUMP_SECTION_SIZE ?= 0
endif

KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iplatform/hal -Iservices/audio_dump/include

ifneq ($(NOAPP),1)

export GAF_CODEC_CROSS_CORE ?=0
ifeq ($(GAF_CODEC_CROSS_CORE),1)
KBUILD_CPPFLAGS += -DGAF_CODEC_CROSS_CORE
export LC3_CODEC_ON ?= 1
ifeq ($(LC3_CODEC_ON),1)
KBUILD_CPPFLAGS += -DLC3_CODEC_ON
endif

export GAF_DECODER_CROSS_CORE_USE_M55 ?=0
ifeq ($(GAF_DECODER_CROSS_CORE_USE_M55),1)
KBUILD_CPPFLAGS += -DGAF_DECODER_CROSS_CORE_USE_M55
endif

export GAF_ENCODER_CROSS_CORE_USE_M55 ?=0
ifeq ($(GAF_ENCODER_CROSS_CORE_USE_M55),1)
KBUILD_CPPFLAGS += -DGAF_ENCODER_CROSS_CORE_USE_M55
endif
core-y          += services/overlay/
core-y          += services/lea_player/gaf_m55/
endif

export BLE_AUDIO_ENABLED ?=0
ifeq ($(BLE_AUDIO_ENABLED),1)
KBUILD_CPPFLAGS += DBLE_AUDIO_ENABLED
endif

export A2DP_DECODER_CROSS_CORE ?= 0
ifeq ($(A2DP_DECODER_CROSS_CORE),1)
core-y		+= apps/audioplayers/a2dp_decoder_off_bth/
KBUILD_CPPFLAGS += -DA2DP_DECODER_CROSS_CORE
export A2DP_DECODER_CROSS_CORE_USE_M55 ?= 1
ifeq ($(A2DP_DECODER_CROSS_CORE_USE_M55),1)
KBUILD_CPPFLAGS += -DA2DP_DECODER_CROSS_CORE_USE_M55
endif
export A2DP_AAC_ON ?= 1
ifeq ($(A2DP_AAC_ON),1)
export FDKAAC_VERSION ?= 2
KBUILD_CPPFLAGS += -DA2DP_AAC_ON
endif

export A2DP_SCALABLE_ON ?= 0
ifeq ($(A2DP_SCALABLE_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SCALABLE_ON
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
core-y += thirdparty/audio_codec_lib/scalable/
endif

export A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_ON
KBUILD_CPPFLAGS += -DA2DP_LHDC_V3
KBUILD_CFLAGS += -DA2DP_LHDC_ON
KBUILD_CFLAGS += -DA2DP_LHDC_V3
core-y += thirdparty/audio_codec_lib/liblhdc-dec/
endif

export A2DP_LHDCV5_ON ?= 0
ifeq ($(A2DP_LHDCV5_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDCV5_ON
core-y += thirdparty/audio_codec_lib/liblhdcv5-dec/
endif

export A2DP_LDAC_ON ?= 0
ifeq ($(A2DP_LDAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LDAC_ON
core-y += thirdparty/audio_codec_lib/ldac/
endif
export A2DP_SBC_PLC_ENABLED ?= 1
ifeq ($(A2DP_SBC_PLC_ENABLED),1)
KBUILD_CPPFLAGS += -DA2DP_SBC_PLC_ENABLED
endif

export A2DP_AAC_PLC_ENABLED ?= 1
ifeq ($(A2DP_AAC_PLC_ENABLED),1)
KBUILD_CPPFLAGS += -DA2DP_AAC_PLC_ENABLED
endif

#KBUILD_CPPFLAGS += -DA2DP_SCALABLE_UHQ_SUPPORT
endif


export A2DP_ENCODER_CROSS_CORE_USE_M55  ?= 0
ifeq ($(A2DP_ENCODER_CROSS_CORE_USE_M55),1)
core-y          += apps/audioplayers/a2dp_encoder_off_bth/
KBUILD_CPPFLAGS += -DA2DP_ENCODER_CROSS_CORE
KBUILD_CPPFLAGS += -DA2DP_ENCODER_CROSS_CORE_USE_M55

export A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_ON
KBUILD_CPPFLAGS += -DA2DP_LHDC_V3
KBUILD_CFLAGS += -DA2DP_LHDC_ON
KBUILD_CFLAGS += -DA2DP_LHDC_V3
ifeq ($(A2DP_LHDCV5_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDCV5_ON
KBUILD_CFLAGS += -DA2DP_LHDCV5_ON
core-y += thirdparty/audio_codec_lib/liblhdcv5-enc/
else
core-y += thirdparty/audio_codec_lib/liblhdc-enc/
endif # A2DP_LHDCV5_ON
endif # A2DP_LHDC_ON

export A2DP_AAC_ON ?= 0
ifeq ($(A2DP_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_AAC_ON
KBUILD_CFLAGS += -DA2DP_AAC_ON
endif # A2DP_AAC_ON

export A2DP_LDAC_ON ?= 0
ifeq ($(A2DP_LDAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LDAC_ON
KBUILD_CFLAGS += -DA2DP_LDAC_ON
core-y += thirdparty/audio_codec_lib/ldac/
endif # A2DP_LDAC_ON
endif # A2DP_ENCODER_CROSS_CORE_USE_M55

endif # NOAPP != 1

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

CFLAGS_IMAGE +=

LDFLAGS_IMAGE +=

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

# AUDIO_DEBUG
ifeq ($(AUDIO_DEBUG_CMD),1)
export AUDIO_DEBUG_CMD
endif

ifeq ($(AUDIO_DEBUG),1)
export AUDIO_DEBUG
export AUDIO_DEBUG_CMD ?= 1
KBUILD_CPPFLAGS += -DAUDIO_DEBUG
endif
# AUDIO_DEBUG END

export BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT ?= 0
ifneq ($(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT),0)
KBUILD_CPPFLAGS += -DBLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT=$(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
endif

export GAF_LC3_BES_PLC_ON ?= 0
ifeq ($(GAF_LC3_BES_PLC_ON),1)
KBUILD_CPPFLAGS += -DGAF_LC3_BES_PLC_ON
endif

export NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR ?= 0
ifeq ($(NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR),1)
KBUILD_CPPFLAGS += -DNO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
endif

export AAC_REDUCE_SIZE ?= 0
ifeq ($(AAC_REDUCE_SIZE),1)
KBUILD_CPPFLAGS += -DAAC_REDUCE_SIZE
endif

export SBC_REDUCE_SIZE ?= 0
ifeq ($(SBC_REDUCE_SIZE),1)
KBUILD_CPPFLAGS += -DSBC_REDUCE_SIZE
endif

export LEA_FOUR_CIS_ENABLED ?=0
ifeq ($(LEA_FOUR_CIS_ENABLED),1)
KBUILD_CPPFLAGS += -DLEA_FOUR_CIS_ENABLED
endif