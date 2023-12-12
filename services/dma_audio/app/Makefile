cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../common/*.c $(cur_dir)../common/*.cpp $(cur_dir)../common/*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iutils/cqueue \
	-Iutils/heap \
	-Iutils/kfifo \
	-Iutils/hwtimer_list \
	-Iutils/signal_generator \
	-Iplatform/hal \
	-Iplatform/drivers/dsp_m55 \
	-Iplatform/drivers/hifi4 \
	-Iplatform/drivers/sensor_hub \
	-Iplatform/drivers/cp_subsys \
	-Iplatform/drivers/stream_dma_rpc \
	-Iplatform/drivers/ana \
	-Iservices/audioflinger \
	-Iservices/dma_audio/common \
	-Iservices/mcpp \
	-Itests/dsp_m55 \
	-Iapps/dsp_hifi4 \
	-Iapps/dsp_m55 \

ifeq ($(PLAYBACK2_SIGNAL_TEST),1)
SIGNAL_GEN_SRCS ?= 1
ccflags-y += -DPLAYBACK2_SIGNAL_TEST
endif

ifeq ($(PLAYBACK1_SIGNAL_TEST),1)
SIGNAL_GEN_SRCS ?= 1
ccflags-y += -DPLAYBACK1_SIGNAL_TEST
endif

ifeq ($(SIGNAL_GEN_SRCS),1)
obj-y += ../../../utils/signal_generator/
endif

ifeq ($(AUDIO_RESAMPLE),1)
ccflags-y += -D__AUDIO_RESAMPLE__
endif

ifeq ($(NOAPP),1)
ccflags-y += -DNOAPP
endif

ifeq ($(SENSOR_HUB),1)
ccflags-y += -DSENSOR_HUB
endif

ifeq ($(SLEEP_TEST),1)
ccflags-y += -DSLEEP_TEST
endif

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -DDMA_AUDIO_APP
endif

ifeq ($(DMA_AUDIO_TEST),1)
ccflags-y += -DDMA_AUDIO_TEST
endif

ifeq ($(DMA_AUDIO_COMPLEX_PSAP_SCO),1)
DMA_AUDIO_SCO_MODE ?= 1
ccflags-y += -DDMA_AUDIO_COMPLEX_PSAP_SCO
endif

ifeq ($(DMA_AUDIO_COMPLEX_PSAP_A2DP),1)
ccflags-y += -DDMA_AUDIO_COMPLEX_PSAP_A2DP
endif

ifeq ($(DMA_AUDIO_COMPLEX_PSAP_ANC),1)
ccflags-y += -DDMA_AUDIO_COMPLEX_PSAP_ANC
endif

ifeq ($(DMA_AUDIO_DELAY_TEST),1)
ccflags-y += -DDMA_AUDIO_DELAY_TEST
endif

DMA_AUDIO_MAX_DELAY_MS ?= 80
ifneq ($(DMA_AUDIO_MAX_DELAY_MS),)
ccflags-y += -DDMA_AUDIO_MAX_DELAY_MS=$(DMA_AUDIO_MAX_DELAY_MS)
endif

ifeq ($(DMA_AUDIO_ALWAYS_ON),1)
ccflags-y += -DDMA_AUDIO_ALWAYS_ON
endif

ifeq ($(DMA_AUDIO_INIT_ON),1)
ccflags-y += -DDMA_AUDIO_INIT_ON
endif

ifeq ($(DMA_AUDIO_KEY),1)
ccflags-y += -DDMA_AUDIO_KEY
endif

ifeq ($(DMA_RPC_CLI),1)
ccflags-y += -DDMA_RPC_CLI
endif

ifeq ($(CODEC_SW_SYNC),1)
ccflags-y += -DCODEC_SW_SYNC
endif

ifeq ($(I2S_SW_SYNC),1)
ccflags-y += -DI2S_SW_SYNC
endif

ifeq ($(DMA_RPC_SVR_CORE),HIFI)
ccflags-y += -DDMA_RPC_SVR_CORE_HIFI
else ifeq ($(DMA_RPC_SVR_CORE),CP)
ccflags-y += -DDMA_RPC_SVR_CORE_CP
else ifeq ($(DMA_RPC_SVR_CORE),SENS)
ccflags-y += -DDMA_RPC_SVR_CORE_SENS
else
ccflags-y += -DDMA_RPC_SVR_CORE_M55
endif

ifeq ($(APP_RPC_ENABLE),1)
ccflags-y += -DAPP_RPC_ENABLE
endif

ifeq ($(DSP_COMBINE_BIN),1)
ccflags-y += -DDSP_COMBINE_BIN
endif

ifeq ($(PSAP_SW_APP),1)
ccflags-y += -DPSAP_SW_APP
endif

ifeq ($(DMA_AUDIO_SCO_MODE),1)
ccflags-y += -DDMA_AUDIO_SCO_MODE
endif

ifeq ($(DMA_AUDIO_LOW_LATENCY),1)
ccflags-y += -DDMA_AUDIO_LOW_LATENCY
endif

ifeq ($(DMA_AUDIO_USE_DIGMIC),1)
ccflags-y += -DDMA_AUDIO_USE_DIGMIC
endif

ifeq ($(DMA_AUDIO_GPIO_TRIG_CODEC),1)
ccflags-y += -DDMA_AUDIO_GPIO_TRIG_CODEC
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
ifeq ($(DMA_AUDIO_USE_TDM0_SLAVE)-$(DMA_AUDIO_USE_TDM1_SLAVE),1-1)
$(error CONFIG ERROR3, both TDM0, TDM1 is slave device !)
endif
ifeq ($(DMA_AUDIO_USE_TDM1_MASTER)-$(DMA_AUDIO_USE_TDM1_SLAVE),1-1)
$(error CONFIG ERROR5, TDM1 master, TDM1 slave !)
endif
ifeq ($(DMA_AUDIO_USE_TDM0_MASTER)-$(DMA_AUDIO_USE_TDM0_SLAVE),1-1)
$(error CONFIG ERROR6, TDM0 master, TDM0 slave !)
endif

ifeq ($(DMA_AUDIO_USE_I2S0_MASTER)-$(DMA_AUDIO_USE_I2S1_MASTER),1-1)
$(error CONFIG ERROR1, both I2S0, I2S1 is master device !)
endif
ifeq ($(DMA_AUDIO_USE_I2S0_SLAVE)-$(DMA_AUDIO_USE_I2S1_SLAVE),1-1)
$(error CONFIG ERROR3, both I2S0, I2S1 is slave device !)
endif
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

