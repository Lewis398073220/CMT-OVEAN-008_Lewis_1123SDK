CHIP		?= best1502x

CHIP_ROLE_CP := 1

DEBUG		?= 1

DEBUG_PORT	?= 2

FAULT_DUMP	?= 1

NOSTD		?= 1

ifneq ($(NOSTD),1)
RTOS 		?= 1
#KERNEL 		:=  FREERTOS
endif

ifeq ($(RTOS),1)
USE_MEM_CFG ?= 1
else
NOAPP		?= 1
export NOAPP
endif

export LIBC_OVERRIDE ?= 1

AUDIO_RESAMPLE ?= 1

export OSC_26M_X4_AUD2BB ?= 1

export ULTRA_LOW_POWER ?= 1

export DSP_LIB ?= 1
ifeq ($(DSP_LIB), 1)
NOSTD        := 0
endif

ifeq ($(PSAP_SW_APP),1)
export SPEECH_LIB ?= 1
export DMA_AUDIO_APP ?= 1
endif

export DMA_AUDIO_APP ?= 0
ifeq ($(DMA_AUDIO_APP),1)
export DMA_RPC_SVR ?=1
ifeq ($(CHIP),best1600)
DMA_AUD_CFG_PATH := best1600
else ifeq ($(CHIP),best1502x)
DMA_AUD_CFG_PATH := best1502x_ibrt
else
DMA_AUD_CFG_PATH := $(CHIP)
endif
include $(srctree)/config/$(DMA_AUD_CFG_PATH)/dma_aud_cfg.mk
endif

export NO_OVERLAY := 1

init-y		:=
core-y		:= tests/cp_subsys/ platform/cmsis/ platform/hal/ utils/hwtimer_list/
ifneq ($(NOAPP),1)
core-y		+= utils/heap/ utils/cqueue/ utils/cfifo/
endif

ifeq ($(SUBSYS_FLASH_BOOT),1)
core-y		+= platform/drivers/norflash/
LDS_FILE	:= best1000.lds
else
LDS_FILE	:= subsys.lds
endif

ifeq ($(AUDIO_DSP_ACCEL_USE_CP_SUBSYS),1)
KBUILD_CPPFLAGS += -DAUDIO_DSP_ACCEL_USE_CP_SUBSYS
export APP_MCPP_CLI := CP_SUBSYS
export APP_MCPP_SRV ?= thirdparty
export APP_MCPP_BTH_CP := 1
endif

ifeq ($(APP_MCPP_CLI),CP_SUBSYS)
export APP_MCPP_BTH_CP_SUBSYS := 1
core-y		+= utils/kfifo/ services/mcpp/ utils/stream_mcps/
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI_CP_SUBSYS
endif

ifneq ($(APP_MCPP_SRV),)
KBUILD_CPPFLAGS += -DAPP_MCPP_SRV
endif

export CP_SUBSYS_TRC_TO_MCU ?= 1
ifeq ($(CP_SUBSYS_TRC_TO_MCU),1)
export TRACE_BUF_SIZE ?= 0
export TRACE_GLOBAL_TAG ?= 1
export DUMP_LOG_ENABLE ?= 1
export LOG_DUMP_SECTION_SIZE ?= 0
endif

ifeq ($(PSAP_SW_APP),1)
core-y += apps/psap_sw/dsp/
endif

#note: default open, because bth default open bt_ramrun, if close bt_ramrun, there BT_RAM_DISTRIBUTION = 0
export BT_RAM_DISTRIBUTION ?= 1
ifeq ($(BT_RAM_DISTRIBUTION),1)
KBUILD_CPPFLAGS += -D__BT_RAM_DISTRIBUTION__
endif

KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iplatform/hal

ifeq ($(USE_MEM_CFG),1)
-include $(srctree)/config/$(CHIP)/mem_cfg.mk
endif

KBUILD_CFLAGS +=

LIB_LDFLAGS +=

CFLAGS_IMAGE +=

LDFLAGS_IMAGE +=

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float
