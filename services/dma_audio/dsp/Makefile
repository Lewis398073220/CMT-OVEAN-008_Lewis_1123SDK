cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../common/*.c $(cur_dir)../common/*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iplatform/hal \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/dsp_m55 \
	-Iplatform/drivers/cp_subsys \
	-Iplatform/drivers/sensor_hub \
	-Iplatform/drivers/stream_dma_rpc \
	-Iservices/dma_audio/dsp \
	-Iservices/dma_audio/common \
	-Iservices/communication \
	-Iservices/audioflinger \
	-Iutils/hwtimer_list \
	-Iutils/heap \
	-Iapps/dsp_m55 \
	-Iapps/psap_sw \
	-Iapps/common \
	-Itests/dsp_m55 \
	-Itests/sensor_hub/core \
	-Itests/cp_subsys \
	-Imultimedia/inc/audio/process/resample/include \
	-Imultimedia/inc/audio/process/limiter/include \
	-Imultimedia/inc/speech/inc

ifeq ($(DMA_AUDIO_COMPLEX_PSAP_SCO),1)
DMA_AUDIO_SCO_MODE ?= 1
endif

ifeq ($(AUDIO_DOWN_MIXER),1)
ccflags-y += -DAUDIO_DOWN_MIXER
DMA_AUDIO_ALGO      ?= 1
DMA_AUDIO_NO_MEMCPY ?= 1
DAUD_USE_HEAP       ?= 1
DAUD_HEAP_SIZE      ?= 0x6000
endif

ifeq ($(PSAP_SW_APP),1)
ccflags-y += -DPSAP_SW_APP
DMA_AUDIO_ALGO      ?= 1
DMA_AUDIO_NO_MEMCPY ?= 1
DAUD_USE_HEAP       ?= 1
DAUD_HEAP_SIZE      ?= 0x6000
endif

ifeq ($(DMA_AUDIO_SW_RESAMPLE),1)
DAUD_USE_HEAP ?= 1
ccflags-y += -DDMA_AUDIO_SW_RESAMPLE
DMA_AUDIO_RESAMPLE_EX ?= 1
ifeq ($(DMA_AUDIO_RESAMPLE_EX),1)
ccflags-y += -DDMA_AUDIO_RESAMPLE_EX
endif
endif

ifeq ($(DMA_AUDIO_DELAY_TEST), 1)
DAUD_HEAP_SIZE      ?= 0x80000
ccflags-y += -DDMA_AUDIO_DELAY_TEST
endif

DAUD_USE_HEAP       ?= 1
ifeq ($(DAUD_USE_HEAP),1)
ccflags-y += -DDAUD_USE_HEAP
ifneq ($(DAUD_HEAP_SIZE),)
ccflags-y += -DDAUD_HEAP_SIZE=$(DAUD_HEAP_SIZE)
endif
endif

ifeq ($(NOAPP)-$(DAUD_USE_HEAP),1-1)
ifneq ($(CHIP_SUBSYS),sensor_hub)
obj-y += ../../../utils/heap/
endif # CHIP_SUBSYS
endif # NOAPP

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -DDMA_AUDIO_APP
endif

ifeq ($(DMA_AUDIO_LOW_LATENCY),1)
ccflags-y += -DDMA_AUDIO_LOW_LATENCY
endif

ifeq ($(DMA_AUDIO_INIT_LOOP_ON),1)
ccflags-y += -DDMA_AUDIO_INIT_LOOP_ON
endif

ifeq ($(DMA_AUDIO_STREAM_DATA_COPY1),1)
ccflags-y += -DDMA_AUDIO_STREAM_DATA_COPY1
endif

ifeq ($(DMA_AUDIO_MIPS_STAT),1)
ccflags-y += -DDMA_AUDIO_MIPS_STAT
endif

ifeq ($(DMA_AUDIO_MIPS_STAT_IMM),1)
ccflags-y += -DDMA_AUDIO_MIPS_STAT_IMM
endif

ifneq ($(AUD_DATA_MEMCPY_SAMP_INC),)
ccflags-y += -DAUD_DATA_MEMCPY_SAMP_INC=$(AUD_DATA_MEMCPY_SAMP_INC)
endif

ifeq ($(AUDIO_ANC_FB_MC),1)
ccflags-y += -DAUDIO_ANC_FB_MC
endif

ifeq ($(ANC_APP),1)
ccflags-y += -DANC_APP
endif

ifeq ($(AUDIO_RESAMPLE),1)
ccflags-y += -D__AUDIO_RESAMPLE__
endif

ifeq ($(CODEC_SW_SYNC),1)
ccflags-y += -DCODEC_SW_SYNC
endif

ifeq ($(DAUD_DEBUG),1)
ccflags-y += -DDAUD_DEBUG
endif

ifneq ($(DAUD_DEBUG_VERBOSE),)
ccflags-y += -DDAUD_DEBUG_VERBOSE=$(DAUD_DEBUG_VERBOSE)
endif

ifeq ($(DAUD_WARNING),1)
ccflags-y += -DDAUD_WARNING
endif

ifeq ($(DMA_AUDIO_USE_DIGMIC),1)
ccflags-y += -DDMA_AUDIO_USE_DIGMIC
endif

ifeq ($(DMA_AUDIO_8K),1)
ccflags-y += -DDMA_AUDIO_8K
else ifeq ($(DMA_AUDIO_8P463K),1)
ccflags-y += -DDMA_AUDIO_8P463K
else ifeq ($(DMA_AUDIO_16K),1)
ccflags-y += -DDMA_AUDIO_16K
else ifeq ($(DMA_AUDIO_16P927K),1)
ccflags-y += -DDMA_AUDIO_16P927K
else ifeq ($(DMA_AUDIO_32K),1)
ccflags-y += -DDMA_AUDIO_32K
else ifeq ($(DMA_AUDIO_48K),1)
ccflags-y += -DDMA_AUDIO_48K
else ifeq ($(DMA_AUDIO_50P781K),1)
ccflags-y += -DDMA_AUDIO_50P781K
else ifeq ($(DMA_AUDIO_96K),1)
ccflags-y += -DDMA_AUDIO_96K
else ifeq ($(DMA_AUDIO_101P562K),1)
ccflags-y += -DDMA_AUDIO_101P562K
else ifeq ($(DMA_AUDIO_192K),1)
ccflags-y += -DDMA_AUDIO_192K
else ifeq ($(DMA_AUDIO_384K),1)
ccflags-y += -DDMA_AUDIO_384K
endif

ifeq ($(DMA_AUDIO_8BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_8BIT
else ifeq ($(DMA_AUDIO_16BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_16BIT
else ifeq ($(DMA_AUDIO_24BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_24BIT
else ifeq ($(DMA_AUDIO_32BIT),1)
subdir-ccflags-y += -DDMA_AUDIO_32BIT
endif

ifeq ($(DMA_AUDIO_1CH),1)
ccflags-y += -DDMA_AUDIO_1CH
else ifeq ($(DMA_AUDIO_2CH),1)
ccflags-y += -DDMA_AUDIO_2CH
else ifeq ($(DMA_AUDIO_3CH),1)
ccflags-y += -DDMA_AUDIO_3CH
else ifeq ($(DMA_AUDIO_4CH),1)
ccflags-y += -DDMA_AUDIO_4CH
else ifeq ($(DMA_AUDIO_8CH),1)
ccflags-y += -DDMA_AUDIO_8CH
endif

ifneq ($(DMA_AUD_CHAN_NUM_PLAY),)
ccflags-y += -DDMA_AUD_CHAN_NUM_PLAY=$(DMA_AUD_CHAN_NUM_PLAY)
endif

ifeq ($(DMA_AUDIO_USE_DAC3),1)
ccflags-y += -DDMA_AUDIO_USE_DAC3
else ifeq ($(DMA_AUDIO_USE_DAC2),1)
ccflags-y += -DDMA_AUDIO_USE_DAC2
else ifeq ($(DMA_AUDIO_USE_DAC1),1)
ccflags-y += -DDMA_AUDIO_USE_DAC1
else ifeq ($(DMA_AUDIO_USE_I2S0_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_I2S0_MASTER
else ifeq ($(DMA_AUDIO_USE_I2S1_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_I2S1_MASTER
else ifeq ($(DMA_AUDIO_USE_TDM0_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_TDM0_MASTER
else ifeq ($(DMA_AUDIO_USE_TDM1_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_TDM1_MASTER
endif

ifeq ($(DMA_AUDIO_USE_TDM0_MASTER)-$(DMA_AUDIO_USE_TDM1_MASTER),1-1)
$(error CONFIG ERROR1, both TDM0, TDM1 is master device !)
endif
# ifeq ($(DMA_AUDIO_USE_TDM0_MASTER)-$(DMA_AUDIO_USE_TDM1_SLAVE),1-1)
# $(error CONFIG ERROR2, TDM0 master and TDM1 slave !)
# endif
ifeq ($(DMA_AUDIO_USE_TDM0_SLAVE)-$(DMA_AUDIO_USE_TDM1_SLAVE),1-1)
$(error CONFIG ERROR3, both TDM0, TDM1 is slave device !)
endif
# ifeq ($(DMA_AUDIO_USE_TDM0_SLAVE)-$(DMA_AUDIO_USE_TDM1_MASTER),1-1)
# $(error CONFIG ERROR4, TDM0 slave and TDM1 master !)
# endif
ifeq ($(DMA_AUDIO_USE_TDM1_MASTER)-$(DMA_AUDIO_USE_TDM1_SLAVE),1-1)
$(error CONFIG ERROR5, TDM1 master, TDM1 slave !)
endif
ifeq ($(DMA_AUDIO_USE_TDM0_MASTER)-$(DMA_AUDIO_USE_TDM0_SLAVE),1-1)
$(error CONFIG ERROR6, TDM0 master, TDM0 slave !)
endif

ifeq ($(DMA_AUDIO_USE_I2S0_MASTER)-$(DMA_AUDIO_USE_I2S1_MASTER),1-1)
$(error CONFIG ERROR1, both I2S0, I2S1 is master device !)
endif
# ifeq ($(DMA_AUDIO_USE_I2S0_MASTER)-$(DMA_AUDIO_USE_I2S1_SLAVE),1-1)
# $(error CONFIG ERROR2, I2S0 master and I2S1 slave !)
# endif
ifeq ($(DMA_AUDIO_USE_I2S0_SLAVE)-$(DMA_AUDIO_USE_I2S1_SLAVE),1-1)
$(error CONFIG ERROR3, both I2S0, I2S1 is slave device !)
endif
# ifeq ($(DMA_AUDIO_USE_I2S0_SLAVE)-$(DMA_AUDIO_USE_I2S1_MASTER),1-1)
# $(error CONFIG ERROR4, I2S0 slave and I2S1 master !)
# endif
ifeq ($(DMA_AUDIO_USE_I2S1_MASTER)-$(DMA_AUDIO_USE_I2S1_SLAVE),1-1)
$(error CONFIG ERROR5, I2S1 master, I2S1 slave !)
endif
ifeq ($(DMA_AUDIO_USE_I2S0_MASTER)-$(DMA_AUDIO_USE_I2S0_SLAVE),1-1)
$(error CONFIG ERROR6, I2S0 master, I2S0 slave !)
endif

ifeq ($(DMA_AUDIO_USE_ADC3),1)
ccflags-y += -DDMA_AUDIO_USE_ADC3
else ifeq ($(DMA_AUDIO_USE_ADC2),1)
ccflags-y += -DDMA_AUDIO_USE_ADC2
else ifeq ($(DMA_AUDIO_USE_ADC1),1)
ccflags-y += -DDMA_AUDIO_USE_ADC1
else ifeq ($(DMA_AUDIO_USE_TDM0_SLAVE),1)
ccflags-y += -DDMA_AUDIO_USE_TDM0_SLAVE
else ifeq ($(DMA_AUDIO_USE_TDM1_SLAVE),1)
ccflags-y += -DDMA_AUDIO_USE_TDM1_SLAVE
else ifeq ($(DMA_AUDIO_USE_TDM0_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_TDM0_MASTER
else ifeq ($(DMA_AUDIO_USE_TDM1_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_TDM1_MASTER
else ifeq ($(DMA_AUDIO_USE_I2S0_SLAVE),1)
ccflags-y += -DDMA_AUDIO_USE_I2S0_SLAVE
else ifeq ($(DMA_AUDIO_USE_I2S1_SLAVE),1)
ccflags-y += -DDMA_AUDIO_USE_I2S1_SLAVE
else ifeq ($(DMA_AUDIO_USE_I2S0_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_I2S0_MASTER
else ifeq ($(DMA_AUDIO_USE_I2S1_MASTER),1)
ccflags-y += -DDMA_AUDIO_USE_I2S1_MASTER
endif

ifeq ($(DMA_AUDIO_SYNC_CLOCK),1)
ccflags-y += -DDMA_AUDIO_SYNC_CLOCK
endif

ifneq ($(DMA_AUDIO_SYNC_DEBUG_VERBOSE),)
ccflags-y += -DDMA_AUDIO_SYNC_DEBUG_VERBOSE=$(DMA_AUDIO_SYNC_DEBUG_VERBOSE)
endif

ifeq ($(DMA_AUDIO_NO_MEMCPY),1)
ccflags-y += -DDMA_AUDIO_NO_MEMCPY
endif

ifeq ($(DMA_AUDIO_SCO_MODE),1)
ccflags-y += -DDMA_AUDIO_SCO_MODE
endif

DMA_AUDIO_ALGO ?= 1
ifeq ($(DMA_AUDIO_ALGO),1)
ccflags-y += -DDMA_AUDIO_ALGO
endif # DMA_AUDIO_ALGO

# PSAP_SW_APP == bes
ifeq ($(PSAP_SW_APP),1)
ccflags-y += -DPSAP_SW_APP
ifeq ($(PSAP_SW_ALGO),)
PSAP_SW_ALGO := bes
endif
ccflags-y += -Iapps/psap_sw/dsp/$(PSAP_SW_ALGO)
endif

ifeq ($(AUDIO_DOWN_MIXER),1)
ccflags-y += -DAUDIO_DOWN_MIXER
ccflags-y += -Iapps/audio_down_mixer/dsp
endif

ifeq ($(DMA_AUDIO_APP_DYN_ON),1)
ccflags-y += -DDMA_AUDIO_APP_DYN_ON
endif

ifeq ($(DMA_AUDIO_DATA_SW_MERGE),1)
ccflags-y += -DDMA_AUDIO_DATA_SW_MERGE
endif
