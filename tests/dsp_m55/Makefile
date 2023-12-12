cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifneq ($(CMSIS_M55_LIB),1)
obj-y := $(filter-out dsp_m55_armcc_func.o,$(obj-y))
endif

ccflags-y += \
	$(BLUETOOTH_ADAPTER_INCLUDES) \
    -Iplatform/drivers/dsp_m55 \
    -Imultimedia/inc/speech/inc \
    -Iservices/communication \
    -Iservices/lea_player/inc \
    -Iutils/hwtimer_list \
    -Iapps/dsp_m55 \
    -Iapps/common \
    -Iutils/list \
    -Iapps/anc/inc \
    -Iapps/anc_assist/inc \
    -Iapps/anc_assist/src \
    -Iapps/anc_assist/dsp_src \
    -Iapps/anc_assist/src/assist \
    -Iapps/anc/src/assist \
    -Iapps/voice_assist/src \
    -Iapps/voice_assist/inc \
    -Iapps/audioplayers/a2dp_decoder_off_bth \
    -Iutils/heap
COMPILE_APP_SRCS ?= 1

ifeq ($(NOAPP),1)
ccflags-y += -DNOAPP
COMPILE_APP_SRCS := 0
endif

ifeq ($(COMPILE_APP_SRCS),1)
obj-y += \
	../../apps/common/ \
	../../apps/dsp_m55/
endif

ifeq ($(DSP_M55_TRC_TO_MCU),1)
obj-y += ../../platform/drivers/rmt_trace/
TRC_FLAGS := -Iplatform/drivers/rmt_trace -DDSP_M55_TRC_TO_MCU
ifeq ($(RMT_TRC_IN_MSG_CHAN),1)
TRC_FLAGS += -DRMT_TRC_IN_MSG_CHAN
endif
CFLAGS_dsp_m55_main.o += $(TRC_FLAGS)
CFLAGS_dsp_m55_trc_to_mcu.o += $(TRC_FLAGS)
endif

ifeq ($(MCU2DSP_M55_MSG_TEST),1)
ccflags-y += -DMCU2DSP_M55_MSG_TEST
endif

ifeq ($(TEST_SINGLE_WIRE_COM),1)
obj-y += ../../services/communication/
ccflags-y += -Iservices/communication
ccflags-y += -DTEST_SINGLE_WIRE_COM
endif

ifeq ($(DMA_AUDIO_APP),1)
ccflags-y += -DDMA_AUDIO_APP -Iservices/audioflinger -Iservices/dma_audio/dsp -Iservices/dma_audio/common
ifeq ($(DMA_AUDIO_APP_DYN_ON),1)
ccflags-y += -DDMA_AUDIO_APP_DYN_ON
endif
endif

ifeq ($(SLEEP_TEST),1)
CFLAGS_dsp_m55_main.o += -DSLEEP_TEST
ifeq ($(NO_TIMER),1)
CFLAGS_dsp_m55_main.o += -DNO_TIMER
endif
endif

ifneq ($(DEBUG_PORT),)
CFLAGS_dsp_m55_main.o += -DDEBUG_PORT=$(DEBUG_PORT)
endif

ifeq ($(SUBSYS_FLASH_BOOT),1)
CFLAGS_dsp_m55_main.o += -DSUBSYS_FLASH_BOOT
endif

ifeq ($(NO_FLASH_INIT),1)
CFLAGS_dsp_m55_main.o += -DNO_FLASH_INIT
endif

ifeq ($(SMF_RPC_M55),1)
CFLAGS_dsp_m55_main.o += -DSMF_RPC_M55
endif

ifneq ($(APP_MCPP_SRV),)
CFLAGS_dsp_m55_main.o += -DAPP_MCPP_SRV
endif

ifeq ($(DMA_APPLIED_ON_M55),1)
CFLAGS_dsp_m55_main.o += -DDMA_APPLIED_ON_M55
endif

ifeq ($(CORE_BRIDGE_NO_PING_MSG),1)
CFLAGS_dsp_m55_core_app.o += -DCORE_BRIDGE_NO_PING_MSG
endif

ifeq ($(CORE_BRIDGE_DEMO_MSG),1)
CFLAGS_dsp_m55_core_app.o += -DCORE_BRIDGE_DEMO_MSG
endif

ifeq ($(AUDIO_DEBUG),1)
obj-y += ../../services/audio_dump/
endif
