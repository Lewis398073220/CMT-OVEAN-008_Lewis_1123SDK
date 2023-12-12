ifeq ($(DMA_AUDIO_APP),1)

#-----------------------------------------
# DMA AUDIO COMMON CONFIG
#-----------------------------------------
#export DMA_RPC_SVR_CORE              ?=M55
#export DMA_RPC_SVR_CORE              ?=HIFI
export DMA_RPC_SVR_CORE              ?=CP
#export DMA_RPC_SVR_CORE              ?=SENS

#export CP_SUBSYS_TRC_TO_MCU            ?= 1
export CHIP_DMA_CFG_IDX             := 1
export DMA_RPC_DEBUG                ?= 0
export DMA_AUDIO_LOW_LATENCY        ?= 1
export DMA_AUDIO_USE_ADC1           ?= 1
export DMA_AUDIO_USE_DAC2           ?= 1
export DMA_AUDIO_USE_TDM0_SLAVE     ?= 0
export DMA_AUDIO_USE_TDM1_SLAVE     ?= 0
export DMA_AUDIO_USE_TDM0_MASTER    ?= 0
export DMA_AUDIO_USE_TDM1_MASTER    ?= 0
export DMA_RPC_STREAM_API           ?= 1
export DMA_AUDIO_SW_RESAMPLE        ?= 1
export DMA_AUDIO_APP_DYN_ON         ?= 1

ifneq ($(DMA_AUDIO_USE_TDM0_SLAVE)-$(DMA_AUDIO_USE_TDM1_SLAVE),0-0)
export DMA_AUDIO_USE_ADC1            := 0
export DMA_AUDIO_SYNC_CLOCK          ?= 1
export AUDIO_RESAMPLE                ?= 1
ifeq ($(DMA_AUDIO_TDM_CFG),1)
export DMA_AUDIO_48K                 ?= 1
export DMA_AUDIO_16BIT               ?= 1
export DMA_AUDIO_8CH                 ?= 1
else
export DMA_AUDIO_96K                 ?= 1
export DMA_AUDIO_24BIT               ?= 1
export DMA_AUDIO_2CH                 ?= 1
endif
else ifneq ($(DMA_AUDIO_USE_TDM0_MASTER)-$(DMA_AUDIO_USE_TDM1_MASTER),0-0)
export DMA_AUDIO_96K                 ?= 1
export DMA_AUDIO_24BIT               ?= 1
export DMA_AUDIO_2CH                 ?= 1
else # !TDM

# DMA AUDIO SAMPLE RATE OPTION #
#export DMA_AUDIO_8K                 ?= 1
#export DMA_AUDIO_16K                ?= 1
#export DMA_AUDIO_32K                ?= 1
export DMA_AUDIO_96K                ?= 1
#export DMA_AUDIO_8P463K             ?= 1
#export DMA_AUDIO_16P927K            ?= 1
#export DMA_AUDIO_50P781K            ?= 1
#export DMA_AUDIO_101P562K           ?= 1
#export DMA_AUDIO_48K                ?= 1

# DMA AUDIO SAMPLE BIT OPTION #
#export DMA_AUDIO_16BIT                ?= 1
#export DMA_AUDIO_32BIT                ?= 1
export DMA_AUDIO_24BIT                ?= 1

# DMA AUDIO ADC SAMPLE CHANNEL OPTION #
#export DMA_AUDIO_1CH                ?= 1
#export DMA_AUDIO_3CH                ?= 1
#export DMA_AUDIO_4CH                ?= 1
export DMA_AUDIO_2CH                ?= 1
endif

ifeq ($(DMA_AUDIO_USE_ADC1),1)
KBUILD_CPPFLAGS += -DCP_SUBSYS_ADC1_DMA_ENABLE
endif

ifeq ($(DMA_RPC_SVR),1)
#-----------------------------------------
# DMA AUDIO RPC SERVER CONFIG
#-----------------------------------------
ifeq ($(DMA_AUDIO_SW_RESAMPLE),1)
export RESAMPLE_ANY_SAMPLE_RATE      ?= 1
endif
export DMA_RPC_SVR                   ?= 1
export DAUD_DEBUG                    ?= 1
export DAUD_WARNING                  ?= 1
ifeq ($(AUDIO_RESAMPLE),1)
export DMA_AUDIO_SYNC_CLOCK          ?= 1
endif
# DMA AUDIO DEBUG OPTION #
export DMA_AUDIO_MIPS_STAT           ?= 0
export DMA_AUDIO_MIPS_STAT_IMM       ?= 0
export DMA_AUDIO_SYNC_DEBUG_VERBOSE  ?= 1
else
#-----------------------------------------
# DMA AUDIO RPC CLIENT CONFIG
#-----------------------------------------
ifeq ($(NOAPP),1)
export HWTEST                    ?= 1
export CP_SUBSYS_TEST            ?= 1
export DMA_AUDIO_TEST            ?= 1
endif
export CP_AS_SUBSYS              ?= 1
export AUDIO_OUTPUT_DAC2         := 1
export CODEC_SW_SYNC             ?= 1
export DMA_RPC_CLI               ?= 1
ifneq ($(DMA_AUDIO_USE_TDM0_SLAVE)-$(DMA_AUDIO_USE_TDM1_SLAVE),0-0)
export AF_DEVICE_TDM            ?= 1
export CODEC_TIMER              := 1
endif # TDM
endif
endif # DMA_AUDIO_APP

