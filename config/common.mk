
add_if_exists = $(foreach d,$(1),$(if $(wildcard $(srctree)/$(d)),$(d) ,))

# -------------------------------------------
# CHIP selection
# -------------------------------------------

export CHIP

ifneq (,)
else ifeq ($(CHIP),best1000)
KBUILD_CPPFLAGS += -DCHIP_BEST1000
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 1
export CHIP_FLASH_CTRL_VER := 1
export CHIP_PSRAM_CTRL_VER := 1
export CHIP_SPI_VER := 1
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export NO_LPU_26M ?= 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
else ifeq ($(CHIP),best1305)
KBUILD_CPPFLAGS += -DCHIP_BEST1305
CPU := m33
SHA256_ROM := 0
CHIP_HAS_DAC_FIFO := 2
export CHIP_HAS_SECURE_BOOT := 0
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 0
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2S := 1
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_SPDIF := 0
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_CP := 1
export CHIP_FLASH_CTRL_VER := 4
export CHIP_CACHE_VER := 3
export CHIP_SPI_VER := 4
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
ifeq ($(OSC_26M_X4_AUD2BB),1)
$(error Invalid Config! $(CHIP) can not open X4!)
endif
else ifeq ($(CHIP),best1306)
KBUILD_CPPFLAGS += -DCHIP_BEST1306
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
export CHIP_HAS_PSRAM := 0
CPU := m33
CHIP_HAS_DAC_FIFO := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 6
export CHIP_PSRAM_CTRL_VER := 3
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
export PMU_IRQ_UNIFIED ?= 1
else ifeq ($(CHIP),best1400)
ifeq ($(CHIP_SUBTYPE),best1402)
SUBTYPE_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST1402
export CHIP_FLASH_CTRL_VER := 3
else
KBUILD_CPPFLAGS += -DCHIP_BEST1400
export CHIP_FLASH_CTRL_VER := 2
endif
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_SPDIF := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_SPI_VER := 3
export BTDUMP_ENABLE := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export NO_LPU_26M ?= 1
export FLASH_CLK_DIV_1 ?= 1
else ifeq ($(CHIP),best1501)
KBUILD_CPPFLAGS += -DCHIP_BEST1501
ifeq ($(MCU_USE_SENS_PERIPH),1)
KBUILD_CPPFLAGS += -DMCU_USE_SENS_PERIPH
endif
ifeq ($(CHIP_SUBSYS),sensor_hub)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_SENS
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_I2S := 1
else
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
endif
CPU := m33
CHIP_HAS_DAC_FIFO := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_FLASH_CTRL_VER := 4
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
else ifeq ($(CHIP),best1501p)
KBUILD_CPPFLAGS += -DCHIP_BEST1501P
ifeq ($(CHIP_SUBSYS),sensor_hub)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_SENS
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_I2S := 1
else
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
endif
CPU := m33
CHIP_HAS_DAC_FIFO := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 6
export FLASH_LOCK_MEM_READ := 1
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
export PMU_IRQ_UNIFIED ?= 1
else ifeq ($(CHIP),best1502x)
KBUILD_CPPFLAGS += -DCHIP_BEST1502X
ifeq ($(MCU_USE_SENS_PERIPH),1)
KBUILD_CPPFLAGS += -DMCU_USE_SENS_PERIPH
endif
ifeq ($(CHIP_SUBSYS),sensor_hub)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_SENS
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_I2S := 1
else
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
export CHIP_HAS_PSRAM := 1
endif
CPU := m33
CHIP_HAS_DAC_FIFO := 2
export CHIP_HAS_FPU := 1
ifneq ($(CHIP_ROLE_CP),1)
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
endif
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 6
export FLASH_LOCK_MEM_READ := 1
export CHIP_PSRAM_CTRL_VER := 5
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
export PMU_IRQ_UNIFIED ?= 1
else ifeq ($(CHIP),best1600)
KBUILD_CPPFLAGS += -DCHIP_BEST1600
ifeq ($(BTH_AS_MAIN_MCU),1)
KBUILD_CPPFLAGS += -DBTH_AS_MAIN_MCU
ifeq ($(BTH_USE_SYS_FLASH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_FLASH
ifneq ($(FORCE_FLASH1_SLEEP),1)
BTH_USE_SYS_PERIPH := 1
endif
endif
else # !BTH_AS_MAIN_MCU
ifeq ($(SYS_USE_BTH_FLASH),1)
KBUILD_CPPFLAGS += -DSYS_USE_BTH_FLASH
endif
endif # !BTH_AS_MAIN_MCU
ifneq ($(filter 1 2 3,$(BTH_USE_SYS_GPIO)),)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_GPIO=$(BTH_USE_SYS_GPIO)
BTH_USE_SYS_PERIPH := 1
endif
ifeq ($(BTH_USE_SYS_PERIPH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_PERIPH
endif
ifeq ($(BTH_USE_SENS_CAP),1)
KBUILD_CPPFLAGS += -DBTH_USE_SENS_CAP
endif
ifeq ($(SYS_USE_SENS_CAP),1)
KBUILD_CPPFLAGS += -DSYS_USE_SENS_CAP
endif
export LIBC_OVERRIDE ?= 1
ifeq ($(CHIP_SUBSYS),bth)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_BTH
CPU := m33
LIBC_ROM := 0
CRC32_ROM := 0
SHA256_ROM := 0
ifeq ($(BTH_AS_MAIN_MCU),1)
# To switch sp and/or cpu
export BOOT_LOADER_ENTRY_HOOK := 1
endif
ifneq ($(filter 1 2,$(CHIP_DMA_CFG_IDX)),)
ifneq ($(BTH_USE_SYS_PERIPH),1)
$(error BTH_USE_SYS_PERIPH must be 1 when CHIP_DMA_CFG_IDX=1 or 2)
endif
else
endif
ifeq ($(BTH_AS_MAIN_MCU)-$(BTH_USE_SYS_PERIPH),1-1)
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
ifeq ($(BTH_USE_SYS_SENS_PERIPH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_SENS_PERIPH
endif
endif
else ifeq ($(CHIP_SUBSYS),sensor_hub)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_SENS
CPU := m33
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
else
CPU := m55
ifneq ($(BTH_AS_MAIN_MCU),1)
# To switch sp
export BOOT_LOADER_ENTRY_HOOK := 1
endif
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
ifneq ($(filter 1 2,$(CHIP_DMA_CFG_IDX)),)
ifneq ($(BTH_USE_SYS_PERIPH),1)
$(error BTH_USE_SYS_PERIPH must be 1 when CHIP_DMA_CFG_IDX=1 or 2)
endif
else
endif
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
ifeq ($(ROM_BUILD),1)
export BOOT_NANDFLASH_SUPPORT ?= 1
endif
endif
CHIP_HAS_DAC_FIFO := 3
CHIP_HAS_ADC_FIFO := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_HIFI4 := 1
ifeq ($(GRAPH_MODULE),1)
export CHIP_HAS_DMA2D := 1
export CHIP_HAS_GPU := 1
export CHIP_HAS_LCDC := 1
endif
export CHIP_CACHE_VER := 4
export CHIP_FLASH_CTRL_VER := 6
export NANDFLASH_SUPPORT ?= 1
export FLASH_LOCK_MEM_READ := 1
export CHIP_PSRAM_CTRL_VER := 4
export CHIP_SPI_VER := 6
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 0
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
ifeq ($(filter 1,$(ROM_BUILD) $(PROGRAMMER) $(SIMU) $(ARM_CMSE)),)
export BT_RCOSC_CAL ?= 0
endif
else ifeq ($(CHIP),best1603)
KBUILD_CPPFLAGS += -DCHIP_BEST1603
ifeq ($(BTH_AS_MAIN_MCU),1)
KBUILD_CPPFLAGS += -DBTH_AS_MAIN_MCU
ifeq ($(BTH_USE_SYS_FLASH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_FLASH
ifneq ($(FORCE_FLASH1_SLEEP),1)
BTH_USE_SYS_PERIPH := 1
endif
endif
else # !BTH_AS_MAIN_MCU
ifeq ($(SYS_USE_BTH_FLASH),1)
KBUILD_CPPFLAGS += -DSYS_USE_BTH_FLASH
endif
endif # !BTH_AS_MAIN_MCU
ifneq ($(filter 1 2 3,$(BTH_USE_SYS_GPIO)),)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_GPIO=$(BTH_USE_SYS_GPIO)
BTH_USE_SYS_PERIPH := 1
endif
ifeq ($(BTH_USE_SYS_PERIPH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_PERIPH
endif
export LIBC_OVERRIDE ?= 1
ifeq ($(CHIP_SUBSYS),bth)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_BTH
CPU := m33
LIBC_ROM := 0
CRC32_ROM := 0
SHA256_ROM := 0
ifeq ($(BTH_AS_MAIN_MCU),1)
# To switch sp and/or cpu
export BOOT_LOADER_ENTRY_HOOK := 1
endif
ifneq ($(filter 1 2,$(CHIP_DMA_CFG_IDX)),)
ifneq ($(BTH_USE_SYS_PERIPH),1)
$(error BTH_USE_SYS_PERIPH must be 1 when CHIP_DMA_CFG_IDX=1 or 2)
endif
else
endif
ifeq ($(BTH_AS_MAIN_MCU)-$(BTH_USE_SYS_PERIPH),1-1)
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_I2S_TDM_TRIGGER := 1
ifeq ($(BTH_USE_SYS_SENS_PERIPH),1)
KBUILD_CPPFLAGS += -DBTH_USE_SYS_SENS_PERIPH
endif
endif
else ifeq ($(CHIP_SUBSYS),sensor_hub)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_SUBSYS_SENS
CPU := m33
export CHIP_HAS_I2S_TDM_TRIGGER := 1
else
CPU := m55
ifneq ($(BTH_AS_MAIN_MCU),1)
# To switch sp
export BOOT_LOADER_ENTRY_HOOK := 1
endif
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_PSRAM := 1
ifneq ($(filter 1 2,$(CHIP_DMA_CFG_IDX)),)
ifneq ($(BTH_USE_SYS_PERIPH),1)
$(error BTH_USE_SYS_PERIPH must be 1 when CHIP_DMA_CFG_IDX=1 or 2)
endif
else
endif
export CHIP_HAS_I2S_TDM_TRIGGER := 1
ifeq ($(ROM_BUILD),1)
export BOOT_NANDFLASH_SUPPORT ?= 1
endif
endif
CHIP_HAS_DAC_FIFO := 3
CHIP_HAS_ADC_FIFO := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_HIFI4 := 1
ifeq ($(GRAPH_MODULE),1)
export CHIP_HAS_DMA2D := 1
export CHIP_HAS_GPU := 1
export CHIP_HAS_LCDC := 1
endif
export CHIP_CACHE_VER := 4
export CHIP_FLASH_CTRL_VER := 6
export NANDFLASH_SUPPORT ?= 1
export FLASH_LOCK_MEM_READ := 1
export CHIP_PSRAM_CTRL_VER := 4
export CHIP_SPI_VER := 6
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 0
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
ifeq ($(filter 1,$(ROM_BUILD) $(PROGRAMMER) $(SIMU) $(ARM_CMSE)),)
export BT_RCOSC_CAL ?= 1
endif
export PMU_LOW_POWER_CLOCK := 1
else ifeq ($(CHIP),best2000)
KBUILD_CPPFLAGS += -DCHIP_BEST2000
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_SPDIF := 2
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 1
export CHIP_PSRAM_CTRL_VER := 1
export CHIP_SPI_VER := 1
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2001)
KBUILD_CPPFLAGS += -DCHIP_BEST2001
ifeq ($(CHIP_SUBSYS),dsp)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST2001_DSP
CPU := a7
DSP_ENABLE ?= 1
else
CPU := m33
export CHIP_HAS_CP := 1
endif
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_PSRAMUHS := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_SPIDPD := 1
ifeq ($(DSP_ENABLE), 1)
export DSP_USE_GPDMA ?= 1
ifeq ($(LARGE_RAM), 1)
$(error LARGE_RAM conflicts with DSP_ENABLE)
endif
endif
ifeq ($(DSP_USE_GPDMA), 1)
KBUILD_CPPFLAGS += -DDSP_USE_GPDMA
else
ifeq ($(CHIP_SUBSYS),dsp)
else
endif
endif
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 3
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 4
export CHIP_CACHE_VER := 2
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export NO_LPU_26M ?= 1
export FLASH_SIZE ?= 0x1000000
export OSC_26M_X4_AUD2BB := 0
export USB_USE_USBPLL := 1
export CHIP_HAS_A7_DSP := 1
export A7_DSP_SPEED ?= 1100
#780:780M, 1000:1G, 1100:1.1G
else ifeq ($(CHIP),best2002)
KBUILD_CPPFLAGS += -DCHIP_BEST2002
CPU := m33
export CHIP_HAS_CP := 1
export CHIP_INTERSYS_VER := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SPI := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 3
export CHIP_HAS_I2S := 1
export CHIP_HAS_TRANSQ := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 5
export CHIP_SPI_VER := 4
export CHIP_HAS_SPIPHY := 1
export CHIP_CACHE_VER := 4
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export NO_LPU_26M ?= 0
export FLASH_SIZE ?= 0x100000
export OSC_26M_X4_AUD2BB := 0
export USB_USE_USBPLL := 0
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 0
else ifeq ($(CHIP),best2003)
KBUILD_CPPFLAGS += -DCHIP_BEST2003
ifeq ($(CHIP_SUBSYS),dsp)
SUBSYS_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST2003_DSP
CPU := a7
DSP_ENABLE ?= 1
else
CPU := m33
export CHIP_HAS_CP := 1
ifeq ($(ROM_BUILD),1)
#export SECURE_BOOT_ED25519_DISABLE ?= 1
export BOOT_NANDFLASH_SUPPORT ?= 1
OPT_LEVEL ?= s
endif
endif
export CHIP_INTERSYS_VER := 2
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_PSRAMUHS := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_SPIDPD := 0
ifeq ($(DSP_ENABLE), 1)
export DSP_USE_GPDMA ?= 1
ifeq ($(LARGE_RAM), 1)
$(error LARGE_RAM conflicts with DSP_ENABLE)
endif
endif
ifeq ($(DSP_USE_GPDMA), 1)
KBUILD_CPPFLAGS += -DDSP_USE_GPDMA
else
ifeq ($(CHIP_SUBSYS),dsp)
else
endif
endif
export CHIP_HAS_I2S := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 5
export NANDFLASH_SUPPORT ?= 1
export CHIP_PSRAM_CTRL_VER := 3
export CHIP_PSRAM_SQPI_MODE := 0
export CHIP_SPI_VER := 4
export CHIP_CACHE_VER := 4
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export NO_LPU_26M ?= 0
export FLASH_SIZE ?= 0x1000000
export OSC_26M_X4_AUD2BB := 0
export USB_USE_USBPLL := 0
export CHIP_HAS_A7_DSP := 1
#780:780M, 1000:1G, 1100:1.1G
export A7_DSP_SPEED ?= 1100
export PSRAMUHS_SPEED ?= 1066
export PSRAM_SPEED ?= 240
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 1
else ifeq ($(CHIP),best2300)
KBUILD_CPPFLAGS += -DCHIP_BEST2300
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2S := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 2
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2300a)
KBUILD_CPPFLAGS += -DCHIP_BEST2300A
CPU := m33
LIBC_ROM := 0
CRC32_ROM := 0
SHA256_ROM := 0
CHIP_HAS_DAC_FIFO := 2
export LIBC_OVERRIDE ?= 1
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2S := 2
export CHIP_HAS_TDM := 2
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_CP := 1
export CHIP_FLASH_CTRL_VER := 3
export CHIP_SPI_VER := 4
export CHIP_CACHE_VER := 2
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export NO_LPU_26M ?= 1
export BOOT_LOADER_ENTRY_HOOK ?= 1
else ifeq ($(CHIP),best2300p)
KBUILD_CPPFLAGS += -DCHIP_BEST2300P
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2S := 2
export CHIP_HAS_TDM := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_CP := 1
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 3
export CHIP_CACHE_VER := 2
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best3001)
ifeq ($(CHIP_SUBTYPE),best3005)
SUBTYPE_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST3005
export CHIP_CACHE_VER := 2
export CHIP_FLASH_CTRL_VER := 2
else
KBUILD_CPPFLAGS += -DCHIP_BEST3001
export CHIP_FLASH_CTRL_VER := 1
endif
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_SPI_VER := 3
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best3003)
KBUILD_CPPFLAGS += -DCHIP_BEST3003
CPU := m33
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_CACHE_VER := 2
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 4
export CHIP_HAS_DCO ?= 1
export CHIP_HAS_SECURE_BOOT := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),fpga1000)
KBUILD_CPPFLAGS += -DCHIP_FPGA1000
KBUILD_CPPFLAGS += -DCHIP_BEST1000
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_FLASH_CTRL_VER := 1
export CHIP_PSRAM_CTRL_VER := 1
export CHIP_SPI_VER := 1
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export CHIP_HAS_SECURE_BOOT := 1
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 0
export CHIP_BT_CLK_UNIT_IN_HSLOT := 0
export CHIP_HAS_MULTI_DMA_TC := 0
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 0
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best1501simu)
KBUILD_CPPFLAGS += -DCHIP_BEST1501SIMU
export CHIP_HAS_USB := 0
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 2
CPU ?= m4
ifeq ($(CPU),m4)
KBUILD_CPPFLAGS += -DCPU_M4
endif
ifeq ($(CHIP_SUB),best1501p)
KBUILD_CPPFLAGS += -D__FPGA_1501P__
KBUILD_CPPFLAGS += -DFLASH_ULTRA_LOW_SPEED
endif
export CHIP_HAS_FPU := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 0
export CHIP_HAS_EXT_PMU := 1
export CHIP_FLASH_CTRL_VER := 4
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 0
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
ifeq ($(RAMSTART),1)
KBUILD_CPPFLAGS += -DRAMSTART
endif
ifeq ($(ASIC_SIMU),1)
KBUILD_CPPFLAGS += -DASIC_SIMU
endif
else ifeq ($(CHIP),best1600simu)
KBUILD_CPPFLAGS += -DCHIP_BEST1600SIMU
export CHIP_HAS_USB := 0
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_CP := 0
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_I2S := 1
CPU ?= m4
ifeq ($(CPU),m4)
KBUILD_CPPFLAGS += -DCPU_M4
endif
export CHIP_HAS_FPU := 1
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_TDM := 1
export CHIP_HAS_I2S_TDM_TRIGGER := 1
export CHIP_HAS_PSC := 0
export CHIP_HAS_EXT_PMU := 1
export CHIP_FLASH_CTRL_VER := 4
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 5
export CHIP_CACHE_VER := 3
export CHIP_INTERSYS_VER := 2
export CHIP_GPIO_VER := 2
export PSC_GPIO_IRQ_CTRL := 1
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 2
export CHIP_HAS_SECURE_BOOT := 0
export CHIP_HAS_ANC_HW_GAIN_SMOOTHING := 1
export CHIP_BT_CLK_UNIT_IN_HSLOT := 1
export CHIP_HAS_MULTI_DMA_TC := 1
export CHIP_HAS_SUPPORT_ME_MEDIATOR := 1
ifeq ($(RAMSTART),1)
KBUILD_CPPFLAGS += -DRAMSTART
endif
else
$(error Invalid CHIP: $(CHIP))
endif

ifneq ($(CHIP_SUBTYPE),)
ifneq ($(SUBTYPE_VALID),1)
$(error Invalid CHIP_SUBTYPE=$(CHIP_SUBTYPE) for CHIP=$(CHIP))
endif
export CHIP_SUBTYPE
endif

ifneq ($(CHIP_SUBSYS),)
ifneq ($(SUBSYS_VALID),1)
$(error Invalid CHIP_SUBSYS=$(CHIP_SUBSYS) for CHIP=$(CHIP))
endif
export CHIP_SUBSYS
endif

ifeq ($(CPU),)
CPU := m33
endif
export CPU

ifneq ($(filter a%,$(CPU)),)
# Override lds file
LDS_FILE := armca.lds

ifeq ($(GEN_BOOT_SECTION),1)
LDS_CPPFLAGS += -DGEN_BOOT_SECTION
endif

ifeq ($(EXEC_IN_RAM),1)
LDS_CPPFLAGS += -DEXEC_IN_RAM
else ifeq ($(EXEC_IN_PSRAM),1)
LDS_CPPFLAGS += -DEXEC_IN_PSRAM
endif
endif

ifeq ($(CHIP_HAS_USB),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_USB
endif

ifeq ($(NO_TRUSTZONE),1)
KBUILD_CPPFLAGS += -DNO_TRUSTZONE
endif

ifneq ($(filter 1,$(NO_CP) $(ARM_CMSE)),)
export CHIP_HAS_CP := 0
export CP_IN_SAME_EE := 0
export CP_AS_SUBSYS := 0
export CHIP_ROLE_CP := 0
endif
ifeq ($(CHIP_HAS_CP),1)
ifneq ($(CP_AS_SUBSYS),1)
ifneq ($(CHIP_ROLE_CP),1)
export CP_IN_SAME_EE ?= 1
endif
endif
endif
ifneq ($(filter 1, $(CP_IN_SAME_EE) $(CP_AS_SUBSYS) $(CHIP_ROLE_CP)),)
ifneq ($(CHIP_HAS_CP),1)
$(error CP_IN_SAME_EE/CP_AS_SUBSYS/CHIP_ROLE_CP is invalid without CHIP_HAS_CP)
endif
ifneq ($(words $(filter 1, $(CP_IN_SAME_EE) $(CP_AS_SUBSYS) $(CHIP_ROLE_CP))),1)
$(error CP_IN_SAME_EE/CP_AS_SUBSYS/CHIP_ROLE_CP can NOT be enabled at the same time)
endif
ifeq ($(CP_IN_SAME_EE),1)
# Main CPU and CP CPU are running in the same execution environment (sharing the same code and data)
KBUILD_CPPFLAGS += -DCP_IN_SAME_EE
endif
ifeq ($(CP_AS_SUBSYS),1)
# This is main CPU and CP CPU will run as subsys
KBUILD_CPPFLAGS += -DCP_AS_SUBSYS
endif
ifeq ($(CHIP_ROLE_CP),1)
# This is CP CPU, running as subsys
KBUILD_CPPFLAGS += -DCHIP_ROLE_CP
NO_ISPI ?= 1
NO_CMU_INIT ?= 1
endif
endif

ifeq ($(NO_CMU_INIT),1)
KBUILD_CPPFLAGS += -DNO_CMU_INIT
endif

ifeq ($(CHIP_HAS_AUDIO_CONST_ROM),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_AUDIO_CONST_ROM
endif

ifeq ($(CHIP_HAS_ANC_HW_GAIN_SMOOTHING),1)
KBUILD_CPPFLAGS += -DANC_HW_GAIN_SMOOTHING
endif

ifeq ($(CORE_SLEEP_POWER_DOWN),1)
KBUILD_CPPFLAGS += -DCORE_SLEEP_POWER_DOWN
endif

ifeq ($(SUBSYS_FLASH_BOOT),1)
KBUILD_CPPFLAGS += -DSUBSYS_FLASH_BOOT
endif

ifneq ($(SECURE_BOOT_VER),)
KBUILD_CPPFLAGS += -DSECURE_BOOT_VER=$(SECURE_BOOT_VER)
endif

ifeq ($(CHIP_HAS_EXT_PMU),1)
export PMU_IRQ_UNIFIED ?= 1
endif

ifeq ($(MAGIC_NUM),1)
KBUILD_CPPFLAGS += -DMAGIC_NUM_AUTO
endif

export IS_USE_DEFAULT_XTAL_FCAP ?= 0
ifeq ($(IS_USE_DEFAULT_XTAL_FCAP),1)
KBUILD_CPPFLAGS += -DIS_USE_DEFAULT_XTAL_FCAP
export DEFAULT_XTAL_FCAP ?= 0x128F
KBUILD_CPPFLAGS += -DDEFAULT_XTAL_FCAP=$(DEFAULT_XTAL_FCAP)
endif

# -------------------------------------------
# Standard C library
# -------------------------------------------
export NUTTX_BUILD ?= 0

export NOSTD
export LIBC_ROM

ifeq ($(NOSTD),1)

ifeq ($(MBED),1)
$(error Invalid configuration: MBED needs standard C library support)
endif
ifeq ($(RTOS),1)
ifneq ($(NO_LIBC),1)
$(error Invalid configuration: RTOS needs standard C library support)
endif
endif

ifneq ($(NO_LIBC),1)
core-y += utils/libc/
KBUILD_CPPFLAGS += -Iutils/libc/inc
endif

SPECS_CFLAGS :=

LIB_LDFLAGS := $(filter-out -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys,$(LIB_LDFLAGS))

KBUILD_CPPFLAGS += -ffreestanding
ifeq ($(TOOLCHAIN),armclang)
# 1) Avoid -nostdinc
#    CMSIS header files need arm_compat.h, which is one of toolchain's standard header files
# 2) Always -nostdlib for compiling C/C++ files
#    Never convert standard API calls to non-standard library calls, but just emit standard API calls
# 3) Avoid -nostdlib for linking final image
#    Some 64-bit calculations and math functions need toolchain's standard library
KBUILD_CPPFLAGS += -nostdlib
else
KBUILD_CPPFLAGS += -nostdinc
CFLAGS_IMAGE += -nostdlib
endif

KBUILD_CPPFLAGS += -DNOSTD

else # NOSTD != 1

ifneq ($(filter 1,$(LIBC_ROM) $(LIBC_OVERRIDE)),)
core-y += utils/libc/
endif

ifeq ($(TOOLCHAIN),armclang)
LIB_LDFLAGS := $(filter-out -lsupc++,$(LIB_LDFLAGS))
else
SPECS_CFLAGS := --specs=nano.specs
LIB_LDFLAGS += -lm -lc -lgcc -lnosys
endif

endif # NOSTD != 1

# -------------------------------------------
# RTOS library
# -------------------------------------------

export RTOS

ifeq ($(RTOS),1)

ifeq ($(CPU),m4)
KERNEL ?= RTX
else
KERNEL ?= RTX5
ifeq ($(KERNEL),RTX)
$(error RTX doesn't support $(CPU))
endif
endif

export KERNEL

VALID_KERNEL_LIST := RTX RTX5 FREERTOS NUTTX RTTHREAD LITEOS_M

ifeq ($(filter $(VALID_KERNEL_LIST),$(KERNEL)),)
$(error Bad KERNEL=$(KERNEL). Valid values are: $(VALID_KERNEL_LIST))
endif

core-y += rtos/

KBUILD_CPPFLAGS += -DRTOS
KBUILD_CPPFLAGS += -DKERNEL_$(KERNEL)

ifeq ($(KERNEL),LITEOS_M)
KBUILD_CPPFLAGS += -DPOSIX_SUPPORT
ifeq ($(GEN_LIB),1)
KBUILD_CPPFLAGS += -DOHOS_LIB
endif
ifeq ($(SDK),1)
KBUILD_CPPFLAGS += -DOHOS_SDK
endif
endif

ifeq ($(KERNEL),NUTTX)
LIB_LDFLAGS := $(filter-out -lstdc++ -lc -lnosys,$(LIB_LDFLAGS)) -nodefaultlibs -nostdlib
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/nuttx/
else ifeq ($(KERNEL),RTX)
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/rtx/
KBUILD_CPPFLAGS += -D__RTX_CPU_STATISTICS__=1
#KBUILD_CPPFLAGS += -DTASK_HUNG_CHECK_ENABLED=1
else ifeq ($(KERNEL),RTX5)
OS_IDLESTKSIZE ?= 1024
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/rtx5/
KBUILD_CPPFLAGS += -D__RTX_CPU_STATISTICS__=1
#KBUILD_CPPFLAGS += -DTASK_HUNG_CHECK_ENABLED=1
else ifeq ($(KERNEL),RTTHREAD)
export OSTICK_USE_FAST_TIMER ?= 1
KBUILD_CPPFLAGS += -DRTT_APP_SUPPORT
KBUILD_CPPFLAGS +=  -Irtos/rt_thread/CMSIS/RTOS2/RT_Thread/board/   \
    				-Irtos/rt_thread/CMSIS/RTOS2/RT_Thread/src/		\
					-Irtos/rt_thread/CMSIS/CMSIS_5/CMSIS/Core/Include/ \
					-Irtos/rt_thread/CMSIS/CMSIS_5/CMSIS/RTOS2/Include/ \
					-Irtos/rt_thread/CMSIS/CMSIS_5/CMSIS/RTOS2/Template/ \
					-Irtos/rt_thread/include/ \
					-Irtos/rt_thread/

KBUILD_CPPFLAGS += -D__NO_STARTFILES__ -D__STARTUP_CLEAR_BSS
LIB_LDFLAGS += -nostartfiles
else #!rtx
ifeq ($(KERNEL),FREERTOS)
KBUILD_CPPFLAGS += \
    -Iinclude/rtos/freertos/

ifeq ($(CPU),m33)
KBUILD_CPPFLAGS += -Iinclude/rtos/freertos/ARM_CM33/
else
KBUILD_CPPFLAGS += -Iinclude/rtos/freertos/ARM_CM4F/
endif

endif #freertos
ifeq ($(KERNEL),LITEOS_M)
	export OSTICK_USE_FAST_TIMER ?= 1
    KBUILD_CPPFLAGS += -DCMSIS_OS_VER=2
    RTOS_NAME = $(shell echo $(KERNEL) | tr A-Z a-z)
    RTOS_INC_DIR += kal/cmsis kernel/include utils ./
    KBUILD_CPPFLAGS += $(addprefix -Irtos/liteos/$(RTOS_NAME)/,${RTOS_INC_DIR}) \
                        -Irtos/liteos/liteos_m/kal/cmsis
endif #LITEOS
endif #rtx

ifeq ($(TWS),1)
OS_TASKCNT ?= 12
OS_SCHEDULERSTKSIZE ?= 768
OS_IDLESTKSIZE ?= 512
else
OS_TASKCNT ?= 20
OS_SCHEDULERSTKSIZE ?= 512
OS_IDLESTKSIZE ?= 256
endif

ifeq ($(CPU),m33)
OS_CLOCK_NOMINAL ?= 16000
else
OS_CLOCK_NOMINAL ?= 32000
endif
OS_FIFOSZ ?= 24

export OS_TASKCNT
export OS_SCHEDULERSTKSIZE
export OS_IDLESTKSIZE
export OS_CLOCK_NOMINAL
export OS_FIFOSZ
export OS_TIMER_NUM
export OS_DYNAMIC_MEM_SIZE

ifeq ($(OS_THREAD_TIMING_STATISTICS_ENABLE),1)
export OS_THREAD_TIMING_STATISTICS_ENABLE
KBUILD_CPPFLAGS += -DOS_THREAD_TIMING_STATISTICS_ENABLE
KBUILD_CPPFLAGS += -DOS_THREAD_TIMING_STATISTICS_PEROID_MS=6000
endif

# shall set WATCHER_DOG ?= 0 when enable this feature
ifeq ($(SPECIFIC_FREQ_POWER_CONSUMPTION_MEASUREMENT_ENABLE),1)
export SPECIFIC_FREQ_POWER_CONSUMPTION_MEASUREMENT_ENABLE
KBUILD_CPPFLAGS += -DSPECIFIC_FREQ_POWER_CONSUMPTION_MEASUREMENT_ENABLE
IGNORE_POWER_ON_KEY_DURING_BOOT_UP ?= 1
endif

ifneq ($(OS_TIMER_NUM), )
KBUILD_CPPFLAGS += -DOS_TIMER_NUM=$(OS_TIMER_NUM)
endif

ifneq ($(OS_DYNAMIC_MEM_SIZE), )
KBUILD_CPPFLAGS += -DOS_DYNAMIC_MEM_SIZE=$(OS_DYNAMIC_MEM_SIZE)
endif

endif # RTOS

export EVR_TRACING ?= 0
ifeq ($(EVR_TRACING), 1)
KBUILD_CPPFLAGS += \
	-Iutils/sysview/SEGGER/ \
	-Iutils/sysview/Config

ifeq ($(KERNEL), FREERTOS)
KBUILD_CPPFLAGS += \
	-Iutils/sysview/Sample/FreeRTOSV10/
endif

KBUILD_CPPFLAGS += -DEVR_TRACING

core-y += utils/sysview/
endif

# -------------------------------------------
# MBED library
# -------------------------------------------

export MBED

ifeq ($(MBED),1)

core-y += mbed/

KBUILD_CPPFLAGS += -DMBED

KBUILD_CPPFLAGS += \
	-Imbed/api \
	-Imbed/common \

endif

# -------------------------------------------
# DEBUG functions
# -------------------------------------------

export DEBUG

ifeq ($(CHIP),best1400)
OPT_LEVEL ?= s
endif

ifneq ($(OPT_LEVEL),)
KBUILD_CFLAGS	+= -O$(OPT_LEVEL)
else
KBUILD_CFLAGS	+= -O2
endif

ifneq ($(filter 1,$(NOSTD) $(SIMU)),)
STACK_PROTECTOR := 0
endif

ifeq ($(DEBUG),1)

KBUILD_CPPFLAGS	+= -DDEBUG

STACK_PROTECTOR ?= 1
ifeq ($(STACK_PROTECTOR),1)
KBUILD_CFLAGS  	+= -fstack-protector-strong
endif

else # !DEBUG

KBUILD_CPPFLAGS	+= -DNDEBUG

REL_TRACE_ENABLE ?= 1
ifeq ($(REL_TRACE_ENABLE),1)
KBUILD_CPPFLAGS	+= -DREL_TRACE_ENABLE
endif

endif # !DEBUG

ifeq ($(NO_CHK_TRC_FMT),1)
KBUILD_CPPFLAGS	+= -DNO_CHK_TRC_FMT
else
# Typedef int32_t to int, and typedef uint32_t to unsigned int
KBUILD_CPPFLAGS	+= -U__INT32_TYPE__ -D__INT32_TYPE__=int -U__UINT32_TYPE__
endif

ifeq ($(MERGE_CONST),1)
ifeq ($(TOOLCHAIN),armclang)
$(error MERGE_CONST is not supported in $(TOOLCHAIN))
else
KBUILD_CPPFLAGS += -fmerge-constants -fmerge-all-constants
endif
endif

ifeq ($(CORE_DUMP),1)
export CORE_DUMP
core-y += utils/crash_catcher/ utils/xyzmodem/
endif

# -------------------------------------------
# SIMU functions
# -------------------------------------------

export SIMU

ifeq ($(SIMU),1)

KBUILD_CPPFLAGS += -DSIMU

ifeq ($(ROM_SRAM_TEXT_SIMU),1)
KBUILD_CPPFLAGS += -DROM_SRAM_TEXT_SIMU
endif

endif

# -------------------------------------------
# FPGA functions
# -------------------------------------------

export FPGA

ifeq ($(FPGA),1)

KBUILD_CPPFLAGS += -DFPGA

endif

# -------------------------------------------
# ROM_BUILD functions
# -------------------------------------------

export ROM_BUILD

ifeq ($(ROM_BUILD),1)

KBUILD_CPPFLAGS += -DROM_BUILD

endif

# -------------------------------------------
# NANDFLASH_BUILD functions
# -------------------------------------------

export NANDFLASH_BUILD

ifeq ($(NANDFLASH_BUILD),1)

KBUILD_CPPFLAGS += -DNANDFLASH_BUILD

endif

# Limit the length of REVISION_INFO if ROM_BUILD or using rom.lds
ifneq ($(filter 1,$(ROM_BUILD))$(filter rom.lds,$(LDS_FILE)),)
ifeq ($(CHIP),best1000)
REVISION_INFO := x
else
REVISION_INFO := $(GIT_REVISION)
endif
endif

# -------------------------------------------
# PROGRAMMER functions
# -------------------------------------------

export PROGRAMMER

ifeq ($(PROGRAMMER),1)

KBUILD_CPPFLAGS += -DPROGRAMMER

ifeq ($(NO_SIMPLE_TASK_SWITCH),1)
KBUILD_CPPFLAGS += -DNO_SIMPLE_TASK_SWITCH
endif

endif

# -------------------------------------------
# ROM_UTILS functions
# -------------------------------------------

export ROM_UTILS_ON ?= 0
ifeq ($(ROM_UTILS_ON),1)
KBUILD_CPPFLAGS += -DROM_UTILS_ON
core-y += utils/rom_utils/
endif

# -------------------------------------------
# Predefined common features
# -------------------------------------------

ifeq ($(OSC_26M_X4_AUD2BB),1)
export OSC_26M_X4_AUD2BB
export ANA_26M_X4_ENABLE ?= 1
export FLASH_LOW_SPEED ?= 0
endif

ifeq ($(USB_AUDIO_APP),1)
export CODEC_HIGH_QUALITY ?= 1
endif

ifeq ($(BT_ANC),1)
export ANC_APP := 1
endif
ifeq ($(ANC_APP),1)
export CODEC_HIGH_QUALITY ?= 1
endif

ifeq ($(CHIP),best1000)
export AUDIO_OUTPUT_DIFF ?= 1
AUDIO_OUTPUT_DC_CALIB ?= $(AUDIO_OUTPUT_DIFF)
export AUDIO_OUTPUT_SMALL_GAIN_ATTN ?= 1
export AUDIO_OUTPUT_SW_GAIN ?= 1
export ANC_L_R_MISALIGN_WORKAROUND ?= 1
else ifeq ($(CHIP),best2000)
ifeq ($(CODEC_HIGH_QUALITY),1)
export VCODEC_VOLT ?= 2.5V
else
export VCODEC_VOLT ?= 1.6V
endif
ifeq ($(VCODEC_VOLT),2.5V)
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else
AUDIO_OUTPUT_DC_CALIB ?= 1
AUDIO_OUTPUT_DC_CALIB_ANA ?= 0
endif
ifneq ($(AUDIO_OUTPUT_DIFF),1)
# Class-G module still needs improving
#DAC_CLASSG_ENABLE ?= 1
endif
else ifeq ($(CHIP),best2001)
export VCODEC_VOLT ?= 1.8V
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else ifneq ($(filter best3001 best3003 best3005,$(CHIP)),)
export VCODEC_VOLT ?= 2.5V
AUDIO_OUTPUT_DC_CALIB ?= 1
AUDIO_OUTPUT_DC_CALIB_ANA ?= 0
else
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
endif

ifeq ($(AUDIO_OUTPUT_DC_CALIB)-$(AUDIO_OUTPUT_DC_CALIB_ANA),1-1)
$(error AUDIO_OUTPUT_DC_CALIB and AUDIO_OUTPUT_DC_CALIB_ANA cannot be enabled at the same time)
endif
export AUDIO_OUTPUT_DC_CALIB
export AUDIO_OUTPUT_DC_CALIB_ANA

ifeq ($(PLAYBACK_USE_I2S),1)
export PLAYBACK_USE_I2S
KBUILD_CPPFLAGS += -DPLAYBACK_USE_I2S
export AF_DEVICE_I2S := 1
ifeq ($(CHIP_HAS_I2S_TDM_TRIGGER),1)
KBUILD_CPPFLAGS += -DHW_I2S_TDM_TRIGGER
endif
endif

ifeq ($(CHIP),best1400)
export AUDIO_RESAMPLE ?= 1
export PMU_IRQ_UNIFIED ?= 1
else ifeq ($(CHIP),best2001)
export AUDIO_RESAMPLE ?= 1
else ifeq ($(CHIP),best2003)
export AUDIO_RESAMPLE := 1
else
export AUDIO_RESAMPLE ?= 0
endif

ifeq ($(AUDIO_RESAMPLE),1)
ifeq ($(CHIP),best1000)
export SW_PLAYBACK_RESAMPLE ?= 1
export SW_CAPTURE_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 1
endif # CHIP is best1000
ifeq ($(CHIP),best2000)
export SW_CAPTURE_RESAMPLE ?= 1
export SW_SCO_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 0
endif # CHIP is best2000
ifeq ($(BT_ANC),1)
ifeq ($(NO_SCO_RESAMPLE),1)
$(error BT_ANC and NO_SCO_RESAMPLE cannot be enabled at the same time)
endif
endif # BT_ANC
endif # AUDIO_RESAMPLE

ifeq ($(SW_IIR_EQ_PROCESS),1)
export SW_IIR_EQ_PROCESS
export A2DP_EQ_24BIT = 1
endif

ifeq ($(HW_DC_FILTER_WITH_IIR),1)
export HW_DC_FILTER_WITH_IIR
KBUILD_CPPFLAGS += -DHW_DC_FILTER_WITH_IIR
export HW_FILTER_CODEC_IIR ?= 1
endif

ifeq ($(USB_AUDIO_APP),1)
export ANDROID_ACCESSORY_SPEC ?= 1
export FIXED_CODEC_ADC_VOL ?= 0

ifneq ($(BTUSB_AUDIO_MODE),1)
NO_PWRKEY ?= 1
NO_GROUPKEY ?= 1
endif
endif

ifneq ($(CHIP),best1000)
ifneq ($(CHIP)-$(TWS),best2000-1)
# For bt
export A2DP_EQ_24BIT ?= 1
# For usb audio
ifeq ($(APP_USB_A2DP_SOURCE),1)
export AUDIO_PLAYBACK_24BIT ?= 0
else
export AUDIO_PLAYBACK_24BIT ?= 1
endif
endif
endif

ifeq ($(PLAYBACK_FORCE_48K),1)
export PLAYBACK_FORCE_48K
endif

ifeq ($(ANC_ASSIST_ENABLED),1)
ifeq ($(ANC_ASSIST_LE_AUDIO_SUPPORT),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_LE_AUDIO_SUPPORT
export ANC_ASSIST_RESAMPLE_UES_EXT_BUF ?= 1
endif

export ANC_ASSIST_RESAMPLE_UES_EXT_BUF ?= 0
ifeq ($(ANC_ASSIST_RESAMPLE_UES_EXT_BUF),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_RESAMPLE_UES_EXT_BUF
endif

KBUILD_CPPFLAGS += -DANC_ASSIST_ENABLED
KBUILD_CPPFLAGS += -DANC_ASSIST_PLAYBACK
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT

ifneq ($(IBRT),1)
KBUILD_CPPFLAGS += -DANC_DUAL_CHANNEL
endif

export SPEECH_TX_24BIT := 1
export AUDIO_OUTPUT_SW_GAIN := 1

ifneq ($(ANC_ASSIST_ON_DSP),)
export APP_RPC_ENABLE := 1
KBUILD_CPPFLAGS += -DANC_ASSIST_ON_DSP
ifeq ($(ANC_ASSIST_ON_DSP),M55)
export APP_RPC_BTH_M55_EN := 1
KBUILD_CPPFLAGS += -DANC_ASSIST_ON_DSP_M55
else ifeq ($(ANC_ASSIST_ON_DSP),HIFI)
export APP_RPC_BTH_DSP_EN := 1
KBUILD_CPPFLAGS += -DANC_ASSIST_ON_DSP_HIFI
else ifeq ($(ANC_ASSIST_ON_DSP),SENS)
export APP_RPC_MCU_SENSOR_EN := 1
KBUILD_CPPFLAGS += -DANC_ASSIST_ON_DSP_SENS
else
$(error ANC_ASSIST_ON_DSP is invalid, expected M55 for HIFI)
endif
endif

ifeq ($(ANC_ASSIST_VPU),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_VPU
endif
ifeq ($(ANC_ASSIST_ALGO_ON_DSP),1)
KBUILD_CPPFLAGS += -DANC_ASSIST_ALGO_ON_DSP
endif
ifeq ($(CHIP),best2300p)
export ANC_ASSIST_USE_INT_CODEC := 1
export PLAYBACK_FORCE_48K := 1
export SPEECH_RX_24BIT := 1
export AF_STREAM_ID_0_PLAYBACK_FADEOUT := 1
endif

ifeq ($(CHIP),best1305)
KBUILD_CPPFLAGS += -DASSIST_LOW_RAM_MOD
endif

ifeq ($(VOICE_ASSIST_WD_ENABLED),1)
export VOICE_ASSIST_WD_ENABLED
export ANC_ASSIST_PILOT_TONE_ALWAYS_ON := 1
endif

ifeq ($(OPEN_EAR_PROMPT_LEAK),1)
KBUILD_CPPFLAGS += -DOPEN_EAR_PROMPT_LEAK
endif

ifeq ($(VOICE_ASSIST_PILOT_ANC_ENABLED),1)
export VOICE_ASSIST_PILOT_ANC_ENABLED
export ANC_ASSIST_PILOT_TONE_ALWAYS_ON := 1
endif

endif #ANC_ASSIST_ENABLED

ifeq ($(AUDIO_ANC_FB_ADJ_MC),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_ADJ_MC
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(AUDIO_ADJ_EQ),1)
export AUDIO_LIMITER := 1
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
KBUILD_CPPFLAGS += -DAUDIO_ADJ_EQ
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_F32_32
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_RFFT_F32_64
KBUILD_CPPFLAGS += -DARM_TABLE_BITREVIDX_FLT_32
endif

ifeq ($(VOICE_ASSIST_FF_FIR_LMS),1)
KBUILD_CPPFLAGS += -DVOICE_ASSIST_FF_FIR_LMS
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
endif

export FIR_ADAPT_ANC_M55 ?= 0
ifeq ($(FIR_ADAPT_ANC_M55),1)
KBUILD_CPPFLAGS += -DFIR_ADAPT_ANC_M55
KBUILD_CPPFLAGS += -DASSIST_RESULT_FIFO_BUF
endif

export ULTRA_LOW_POWER ?= 0
ifeq ($(ULTRA_LOW_POWER),1)
export FLASH_LOW_SPEED ?= 1
export PSRAM_LOW_SPEED ?= 1
endif

ifeq ($(CHIP),best2000)
ifeq ($(USB_HIGH_SPEED),1)
export AUDIO_USE_BBPLL ?= 1
endif
ifeq ($(AUDIO_USE_BBPLL),1)
ifeq ($(MCU_HIGH_PERFORMANCE_MODE),1)
$(error MCU_HIGH_PERFORMANCE_MODE conflicts with AUDIO_USE_BBPLL)
endif
else # !AUDIO_USE_BBPLL
ifeq ($(USB_HIGH_SPEED),1)
$(error AUDIO_USE_BBPLL must be used with USB_HIGH_SPEED)
endif
endif # !AUDIO_USE_BBPLL
endif # best2000

ifeq ($(FAST_TIMER_COMPENSATE),1)
export CALIB_SLOW_TIMER := 1
export TIMER_USE_FPU := 1
endif

### voice compression feature switch
export VOC_ENCODE_ENABLE ?= 0

ifeq ($(MAGIC_NUM),1)
KBUILD_CPPFLAGS += -DMAGIC_NUM_AUTO
endif

ifeq ($(IS_USE_DEFAULT_XTAL_FCAP),1)
export IS_USE_DEFAULT_XTAL_FCAP
KBUILD_CPPFLAGS += -DIS_USE_DEFAULT_XTAL_FCAP
export DEFAULT_XTAL_FCAP ?= 0x128F
KBUILD_CPPFLAGS += -DDEFAULT_XTAL_FCAP=$(DEFAULT_XTAL_FCAP)
endif

ifeq ($(HOST_SIMU),1)
export HOST_SIMU
KBUILD_CPPFLAGS += -DHOST_SIMU
endif

ifneq ($(NV_EXTENSION_MIRROR_RAM_SIZE),)
KBUILD_CPPFLAGS += -DNV_EXTENSION_MIRROR_RAM_SIZE=$(NV_EXTENSION_MIRROR_RAM_SIZE)
endif

export BLE_ONLY_ENABLED ?= 0
ifeq ($(BLE_ONLY_ENABLED),1)
export override BT_A2DP_SUPPORT := 0
export override BT_AVRCP_SUPPORT := 0
export override BT_HFP_SUPPORT := 0
export override BT_SPP_SUPPORT := 0
export override BT_DIP_SUPPORT := 0
export override BT_HID_DEVICE := 0
export override BT_PBAP_SUPPORT := 0
export override BT_OPP_SUPPORT := 0
export override BT_MAP_SUPPORT := 0
export override BT_PAN_SUPPORT := 0
export override SUPPORT_SIRI := 0
export override IBRT := 0
export override IBRT_UI := 0
export override IBRT_CORE_V2 := 0
export override IBRT_UI_V1 := 0
export override IBRT_UI_V2 := 0
export override IBRT_V2_MULTIPOINT := 0
export override BES_AUD := 0
export override BT_WATCH_APP := 0
export override BT_SOURCE := 0
export override BT_HFP_AG_ROLE := 0
export override BT_A2DP_SRC_ROLE := 0
export override BT_AVRCP_TG_ROLE := 0
export override GATT_OVER_BR_EDR := 0
export override CTKD_ENABLE := 0
export override IS_CTKD_OVER_BR_EDR_ENABLED := 0
KBUILD_CPPFLAGS += -UIBRT
KBUILD_CPPFLAGS += -UBES_AUD
KBUILD_CPPFLAGS += -UIBRT_UI
KBUILD_CPPFLAGS += -UIBRT_CORE_V2_ENABLE
KBUILD_CPPFLAGS += -UIBRT_UI_V1
KBUILD_CPPFLAGS += -UIBRT_UI_V2
KBUILD_CPPFLAGS += -UIBRT_V2_MULTIPOINT
KBUILD_CPPFLAGS += -UBT_WATCH_APP
KBUILD_CPPFLAGS += -UBT_SOURCE
KBUILD_CPPFLAGS += -U__GATT_OVER_BR_EDR__
KBUILD_CPPFLAGS += -UCTKD_ENABLE
KBUILD_CPPFLAGS += -UIS_CTKD_OVER_BR_EDR_ENABLED
KBUILD_CPPFLAGS += -DBLE_ONLY_ENABLED
KBUILD_CPPFLAGS += -DBLE_POWER_LEVEL_0
endif

export EP_1306_SMALL_CODE ?= 0
ifeq ($(EP_1306_SMALL_CODE),1)
KBUILD_CPPFLAGS += -DEP_1306_SMALL_CODE
export override BLE_GATT_CLIENT_CACHE := 0
endif

export BLE_GATT_CLIENT_CACHE ?= 1
ifeq ($(BLE_GATT_CLIENT_CACHE)$(BLE_STACK_NEW_DESIGN),11)
KBUILD_CPPFLAGS += -DBLE_GATT_CLIENT_CACHE
endif

export BLE_PERIPHERAL_ONLY ?= 0
ifeq ($(BLE_PERIPHERAL_ONLY),1)
KBUILD_CPPFLAGS += -DBLE_PERIPHERAL_ONLY
endif

export BLE_GATT_CLIENT_DISABLE ?= 0
ifeq ($(BLE_GATT_CLIENT_DISABLE),1)
KBUILD_CPPFLAGS += -DBLE_GATT_CLIENT_DISABLE
endif

export BLE_PERIODIC_ADV_DISABLE ?= 0
ifeq ($(BLE_PERIODIC_ADV_DISABLE),1)
KBUILD_CPPFLAGS += -DBLE_PERIODIC_ADV_DISABLE
endif

# -------------------------------------------
# BT features
# -------------------------------------------

ifneq ($(filter apps/ tests/speech_test/ tests/ota_boot/, $(core-y)),)
export BT_APP ?= 1
FULL_APP_PROJECT ?= 1
endif

export BT_HFP_SUPPORT ?= 1
ifeq ($(BT_HFP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_HFP_SUPPORT
endif

export BT_A2DP_SUPPORT ?= 1
ifeq ($(BT_A2DP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_A2DP_SUPPORT
endif

export BT_AVRCP_SUPPORT ?= 1
ifeq ($(BT_AVRCP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_AVRCP_SUPPORT
endif

export mHDT_SUPPORT ?= 0
ifeq ($(mHDT_SUPPORT),1)
KBUILD_CPPFLAGS += -DmHDT_SUPPORT=1
endif

export BT_SPP_SUPPORT ?= 1
ifeq ($(BT_SPP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_SPP_SUPPORT
endif

export BT_HID_DEVICE ?= 0
ifeq ($(BT_HID_DEVICE),1)
KBUILD_CPPFLAGS += -DBT_HID_DEVICE
endif

export BT_HID_HEAD_TRACKER_PROTOCOL ?= 0
ifeq ($(BT_HID_HEAD_TRACKER_PROTOCOL),1)
KBUILD_CPPFLAGS += -DBT_HID_HEAD_TRACKER_PROTOCOL
endif

export HOGP_HEAD_TRACKER_PROTOCOL ?= 0
ifeq ($(HOGP_HEAD_TRACKER_PROTOCOL),1)
KBUILD_CPPFLAGS += -DHOGP_HEAD_TRACKER_PROTOCOL
endif

export BT_PBAP_SUPPORT ?= 0
ifeq ($(BT_PBAP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_PBAP_SUPPORT
KBUILD_CPPFLAGS += -DBT_OBEX_SUPPORT
endif

export BT_OPP_SUPPORT ?= 0
ifeq ($(BT_OPP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_OPP_SUPPORT
KBUILD_CPPFLAGS += -DBT_OBEX_SUPPORT
endif

export BT_MAP_SUPPORT ?= 0
ifeq ($(BT_MAP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_MAP_SUPPORT
KBUILD_CPPFLAGS += -DBT_OBEX_SUPPORT
endif

export BT_PAN_SUPPORT ?= 0
ifeq ($(BT_PAN_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_PAN_SUPPORT
KBUILD_CPPFLAGS += -DBT_BNEP_SUPPORT
endif

export BLUETOOTH_ADAPTER_DIR_PATH = bthost/adapter
export BT_APP_DIR_PATH = bthost/service/bt_app
export BT_SOURCE_APP_DIR_PATH = bthost/service/bt_source
export BT_IF_DIR_PATH = bthost/stack/bt_if
export BT_PROFILES_DIR_PATH = bthost/stack/bt_profiles

export BLE_AUDIO_CENTRAL_DIR_PATH = apps/ble_audio_central_app
export BLE_AUDIO_USB_DIR_PATH = apps/ble_audio_central_app/ble_audio_usb
export BLE_AUDIO_I2S_DIR_PATH = apps/ble_audio_central_app/ble_audio_i2s
export BLE_IF_DIR_PATH = bthost/stack/ble_if

export BES_BT_IMPL_INCLUDES = \
    -I$(BT_APP_DIR_PATH)/inc \
    -I$(BT_APP_DIR_PATH)/inc/a2dp_codecs \
    -I$(BT_SOURCE_APP_DIR_PATH)/inc \
    -I$(BT_IF_DIR_PATH)/inc \
    -I$(BT_PROFILES_DIR_PATH)/inc

export BLE_STACK_INCLUDES = \
    -I$(BLE_STACK_DIR_PATH)/inc/hci \
    -I$(BLE_STACK_DIR_PATH)/inc/l2cap \
    -I$(BLE_STACK_DIR_PATH)/inc/gap \
    -I$(BLE_STACK_DIR_PATH)/inc/gatt \
    -I$(BLE_STACK_DIR_PATH)/inc/gaf \
    -I$(BLE_STACK_DIR_PATH)/inc/iso \
    -I$(BLE_STACK_DIR_PATH)/inc/profiles \
    -I$(BLE_STACK_DIR_PATH)/inc

export BLUETOOTH_ADAPTER_INCLUDES = \
    $(BES_BT_IMPL_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/adapter_service \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/common \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/source \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/a2dp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/avrcp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/hfp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/hci \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/l2cap \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/sdp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/me \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/ibrt \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/spp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/dip \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/hid \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt/map \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/bt \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/common \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/aob \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gap \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gatt \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/gfps \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/dp \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/ai \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/tile \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble/walkie_talkie \
    -I$(BLUETOOTH_ADAPTER_DIR_PATH)/inc/ble

export BT_BUILD_ONLY_USING_ADAPTER ?= 0
ifneq ($(BT_BUILD_ONLY_USING_ADAPTER),0)
    KBUILD_CPPFLAGS += -DBT_BUILD_ONLY_USING_ADAPTER
endif

export BLE_BUILD_ONLY_USING_ADAPTER ?= 0
ifneq ($(BLE_BUILD_ONLY_USING_ADAPTER),0)
    KBUILD_CPPFLAGS += -DBLE_BUILD_ONLY_USING_ADAPTER
endif

ifeq ($(BT_APP),1)

export INTERSYS_NO_THREAD ?= 0

export INTERSYS_DEBUG ?= 0
ifeq ($(INTERSYS_DEBUG),1)
	KBUILD_CPPFLAGS += -DINTERSYS_DEBUG=1
endif

export BT_DEBUG_TPORTS ?= 0
ifneq ($(BT_DEBUG_TPORTS),0)
	KBUILD_CPPFLAGS += -D__BT_DEBUG_TPORTS__
endif

export SNOOP_DATA_EXCHANGE_VIA_BLE ?= 0
ifeq ($(SNOOP_DATA_EXCHANGE_VIA_BLE),1)
	KBUILD_CPPFLAGS += -DSNOOP_DATA_EXCHANGE_VIA_BLE
endif

export SYNC_BT_CTLR_PROFILE ?= 0
ifeq ($(SYNC_BT_CTLR_PROFILE),1)
	KBUILD_CPPFLAGS += -DSYNC_BT_CTLR_PROFILE
endif

export PROFILE_DEBUG ?= 0
ifeq ($(PROFILE_DEBUG),1)
	KBUILD_CPPFLAGS += -DXA_DEBUG=1
endif

#KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__ -D__A2DP_AVDTP_DR__
#KBUILD_CPPFLAGS += -D__A2DP_AVDTP_DR__

ifeq ($(A2DP_AVDTP_CP),1)
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifneq ($(filter-out 2M 3M,$(BT_RF_PREFER)),)
$(error Invalid BT_RF_PREFER=$(BT_RF_PREFER))
endif
ifneq ($(BT_RF_PREFER),)
RF_PREFER := $(subst .,P,$(BT_RF_PREFER))
KBUILD_CPPFLAGS += -D__$(RF_PREFER)_PACK__
endif

export HIGH_EFFICIENCY_TX_PWR_CTRL ?= 0
ifeq ($(HIGH_EFFICIENCY_TX_PWR_CTRL),1)
KBUILD_CPPFLAGS += -DHIGH_EFFICIENCY_TX_PWR_CTRL
endif # ifeq ($(HIGH_EFFICIENCY_TX_PWR_CTRL),1)

export IS_CUSTOM_MAX_BLE_TX_PWR_VAL ?= 0
ifeq ($(IS_CUSTOM_MAX_BLE_TX_PWR_VAL),1)
export MAX_BLE_TX_PWR_VAL ?= 0x34
KBUILD_CPPFLAGS += -DMAX_BLE_TX_PWR_VAL=$(MAX_BLE_TX_PWR_VAL)
endif

export IS_USE_MIXING_FRQ_FOR_A2DP_STREAMING ?= 0
ifeq ($(IS_USE_MIXING_FRQ_FOR_A2DP_STREAMING),1)
KBUILD_CPPFLAGS += -DIS_USE_MIXING_FRQ_FOR_A2DP_STREAMING
endif

export AUDIO_SCO_BTPCM_CHANNEL ?= 1
ifeq ($(AUDIO_SCO_BTPCM_CHANNEL),1)
KBUILD_CPPFLAGS += -D_SCO_BTPCM_CHANNEL_
endif

export BT_ONE_BRING_TWO ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
KBUILD_CPPFLAGS += -D__BT_ONE_BRING_TWO__
endif

export BT_USE_COHEAP_ALLOC ?= 1
ifeq ($(BT_USE_COHEAP_ALLOC),1)
KBUILD_CPPFLAGS += -DBT_USE_COHEAP_ALLOC
endif

export BT_APP_USE_COHEAP ?= 0
ifeq ($(BT_APP_USE_COHEAP),1)
KBUILD_CPPFLAGS += -DBT_APP_USE_COHEAP
endif

export BT_DONT_AUTO_PLAY_BG_A2DP ?= 0
ifeq ($(BT_DONT_AUTO_PLAY_BG_A2DP),1)
KBUILD_CPPFLAGS += -DBT_DONT_AUTO_PLAY_BG_A2DP
endif

export AUTO_ACCEPT_SECOND_SCO ?= 0
ifeq ($(AUTO_ACCEPT_SECOND_SCO),1)
KBUILD_CPPFLAGS += -DAUTO_ACCEPT_SECOND_SCO
endif

export BT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL ?= 1
ifeq ($(BT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL),1)
KBUILD_CPPFLAGS += -DBT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL
endif

export BT_HOST_REJECT_UNEXCEPT_SCO_PACKET ?= 1
ifeq ($(BT_HOST_REJECT_UNEXCEPT_SCO_PACKET),1)
KBUILD_CPPFLAGS += -DBT_HOST_REJECT_UNEXCEPT_SCO_PACKET
endif

export A2DP_PROMPT_PLAY_ONLY_AVRCP_PLAY_RECEIVED ?= 1
ifeq ($(A2DP_PROMPT_PLAY_ONLY_AVRCP_PLAY_RECEIVED),1)
KBUILD_CPPFLAGS += -DA2DP_PROMPT_PLAY_ONLY_AVRCP_PLAY_RECEIVED
endif

export A2DP_DELAY_PROMPT_PLAY ?= 0
ifeq ($(A2DP_DELAY_PROMPT_PLAY),1)
KBUILD_CPPFLAGS += -DA2DP_DELAY_PROMPT_PLAY
endif

export BT_MUTE_A2DP ?= 0
ifeq ($(BT_MUTE_A2DP),1)
KBUILD_CPPFLAGS += -DBT_MUTE_A2DP
endif

export LOCAL_MP3_MODE_SUPPORT ?= 0
ifeq ($(LOCAL_MP3_MODE_SUPPORT),1)
KBUILD_CPPFLAGS += -DLOCAL_MP3_MODE_SUPPORT
endif

export BT_PAUSE_A2DP ?= 1
ifeq ($(BT_PAUSE_A2DP),1)
KBUILD_CPPFLAGS += -DBT_PAUSE_A2DP
endif

export BT_CLOSE_A2DP ?= 0
ifeq ($(BT_CLOSE_A2DP),1)
KBUILD_CPPFLAGS += -DBT_CLOSE_A2DP
endif

export BT_KEEP_ONE_STREAM_CLOSE_CONNECTED_A2DP ?= 0
ifeq ($(BT_KEEP_ONE_STREAM_CLOSE_CONNECTED_A2DP),1)
KBUILD_CPPFLAGS += -DBT_KEEP_ONE_STREAM_CLOSE_CONNECTED_A2DP
endif

export BT_PAUSE_A2DP_WHEN_CALL_EXIST ?= 0
ifeq ($(BT_PAUSE_A2DP_WHEN_CALL_EXIST),1)
KBUILD_CPPFLAGS += -DBT_PAUSE_A2DP_WHEN_CALL_EXIST
endif

export BT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL ?= 0
ifeq ($(BT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL),1)
KBUILD_CPPFLAGS += -DBT_UPDATE_ACTIVE_DEVICE_WHEN_INCOMING_CALL
endif

export ONLY_REPORT_VOLUME_WHEN_STREAMING ?= 0
ifeq ($(ONLY_REPORT_VOLUME_WHEN_STREAMING),1)
KBUILD_CPPFLAGS += -DONLY_REPORT_VOLUME_WHEN_STREAMING
endif

export BT_DONT_AUTO_REPORT_DELAY_REPORT ?= 0
ifeq ($(BT_DONT_AUTO_REPORT_DELAY_REPORT),1)
KBUILD_CPPFLAGS += -DBT_DONT_AUTO_REPORT_DELAY_REPORT
endif

export BT_HFP_DONT_SUPPORT_CLI_FEATURE ?= 0
ifeq ($(BT_HFP_DONT_SUPPORT_CLI_FEATURE),1)
KBUILD_CPPFLAGS += -DBT_HFP_DONT_SUPPORT_CLI_FEATURE
endif

export BT_HFP_DONT_SUPPORT_ENHANCED_CALL_FEATURE ?= 0
ifeq ($(BT_HFP_DONT_SUPPORT_ENHANCED_CALL_FEATURE),1)
KBUILD_CPPFLAGS += -DBT_HFP_DONT_SUPPORT_ENHANCED_CALL_FEATURE
endif

export BT_HFP_SUPPORT_HF_INDICATORS_FEATURE ?= 0
ifeq ($(BT_HFP_SUPPORT_HF_INDICATORS_FEATURE),1)
KBUILD_CPPFLAGS += -DBT_HFP_SUPPORT_HF_INDICATORS_FEATURE
endif

export BT_HFP_DONT_SUPPORT_APPLE_HF_AT_COMMAND ?= 0
ifeq ($(BT_HFP_DONT_SUPPORT_APPLE_HF_AT_COMMAND),1)
KBUILD_CPPFLAGS += -DBT_HFP_DONT_SUPPORT_APPLE_HF_AT_COMMAND
endif

export APP_RX_API_ENABLE ?= 0
ifeq ($(APP_RX_API_ENABLE),1)
KBUILD_CPPFLAGS += -DAPP_RX_API_ENABLE
endif

#stero headset not need this function. default close it
ifeq ($(FREEMAN_ENABLED_STERO),1)
export BT_DONT_PLAY_MUTE_WHEN_A2DP_STUCK_PATCH := 0
else
export BT_DONT_PLAY_MUTE_WHEN_A2DP_STUCK_PATCH ?= 0
endif
ifeq ($(BT_DONT_PLAY_MUTE_WHEN_A2DP_STUCK_PATCH),1)
KBUILD_CPPFLAGS += -DBT_DONT_PLAY_MUTE_WHEN_A2DP_STUCK_PATCH
endif

export IS_ALWAYS_KEEP_A2DP_TARGET_BUFFER_COUNT ?= 1
ifeq ($(IS_ALWAYS_KEEP_A2DP_TARGET_BUFFER_COUNT),1)
KBUILD_CPPFLAGS += -DIS_ALWAYS_KEEP_A2DP_TARGET_BUFFER_COUNT
endif

export SPP_SERVICE_NUM ?= 5
KBUILD_CPPFLAGS += -DSPP_SERVICE_NUM=$(SPP_SERVICE_NUM)

export IS_BES_OTA_USE_CUSTOM_RFCOMM_UUID ?= 1
ifeq ($(IS_BES_OTA_USE_CUSTOM_RFCOMM_UUID),1)
KBUILD_CPPFLAGS += -DIS_BES_OTA_USE_CUSTOM_RFCOMM_UUID
endif

export IS_TOTA_RFCOMM_UUID_CUSTOMIZED ?= 0
ifeq ($(IS_TOTA_RFCOMM_UUID_CUSTOMIZED),1)
KBUILD_CPPFLAGS += -DIS_TOTA_RFCOMM_UUID_CUSTOMIZED
endif

export BT_DISC_ACL_AFTER_AUTH_KEY_MISSING ?= 0
ifeq ($(BT_DISC_ACL_AFTER_AUTH_KEY_MISSING),1)
KBUILD_CPPFLAGS += -DBT_DISC_ACL_AFTER_AUTH_KEY_MISSING
endif

export A2DP_SNK_AVDTP_ERR_CODE_PTS_TEST ?= 0
ifeq ($(A2DP_SNK_AVDTP_ERR_CODE_PTS_TEST),1)
KBUILD_CPPFLAGS += -DA2DP_SNK_AVDTP_ERR_CODE_PTS_TEST
endif

export BT_DISABLE_INITIAL_ROLE_SWITCH ?= 0
ifeq ($(BT_DISABLE_INITIAL_ROLE_SWITCH),1)
KBUILD_CPPFLAGS += -DBT_DISABLE_INITIAL_ROLE_SWITCH
endif

export USE_PAGE_SCAN_REPETITION_MODE_R1 ?= 0
ifeq ($(USE_PAGE_SCAN_REPETITION_MODE_R1),1)
KBUILD_CPPFLAGS += -DUSE_PAGE_SCAN_REPETITION_MODE_R1
endif

export A2DP_PLAYER_USE_BT_TRIGGER ?= 1
ifeq ($(A2DP_PLAYER_USE_BT_TRIGGER),1)
KBUILD_CPPFLAGS += -D__A2DP_PLAYER_USE_BT_TRIGGER__
endif

export AUDIO_OUTPUT_ROUTE_SELECT ?= 0
ifeq ($(AUDIO_OUTPUT_ROUTE_SELECT),1)
KBUILD_CPPFLAGS += -DAUDIO_OUTPUT_ROUTE_SELECT
endif

export BT_SELECT_PROF_DEVICE_ID ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
ifeq ($(BT_SELECT_PROF_DEVICE_ID),1)
KBUILD_CPPFLAGS += -D__BT_SELECT_PROF_DEVICE_ID__
endif
endif

export SBC_SELECT_CHANNEL_SUPPORT ?= 1
ifeq ($(SBC_SELECT_CHANNEL_SUPPORT),1)
KBUILD_CPPFLAGS += -DSBC_SELECT_CHANNEL_SUPPORT
endif

export SBC_FUNC_IN_ROM ?= 0
ifeq ($(SBC_FUNC_IN_ROM),1)

KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM__

ifeq ($(CHIP),best2000)
UNALIGNED_ACCESS ?= 1
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000_ONLYSBC__
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000__
endif
endif

export BT_ALWAYS_IN_DISCOVERABLE_MODE ?= 0
ifeq ($(BT_ALWAYS_IN_DISCOVERABLE_MODE),1)
KBUILD_CPPFLAGS += -DBT_ALWAYS_IN_DISCOVERABLE_MODE
endif

export HFP_1_6_ENABLE ?= 1
ifeq ($(HFP_1_6_ENABLE),1)
KBUILD_CPPFLAGS += -DHFP_1_6_ENABLE
endif

# Fix codec and vqe sample rate
ifeq ($(SPEECH_BONE_SENSOR),1)
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 16000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
endif

ifeq ($(ANC_ASSIST_ENABLED),1)
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 16000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
endif

ifeq ($(VOICE_ASSIST_FF_FIR_LMS),1)
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 32000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
endif

ifeq ($(AUDIO_ADAPTIVE_IIR_EQ),1)
export AUDIO_ADAPTIVE_IIR_EQ := 1
export HW_DAC_IIR_EQ_PROCESS := 1
endif

ifeq ($(AUDIO_ADAPTIVE_FIR_EQ),1)
export AUDIO_ADAPTIVE_FIR_EQ := 1
export HW_FIR_EQ_PROCESS := 1
endif

export SPEECH_CODEC_FIXED_SAMPLE_RATE ?= 0
ifneq ($(filter 8000 16000 32000 48000,$(SPEECH_CODEC_FIXED_SAMPLE_RATE)),)
KBUILD_CPPFLAGS += -DSPEECH_CODEC_FIXED_SAMPLE_RATE=$(SPEECH_CODEC_FIXED_SAMPLE_RATE)
#export DSP_LIB ?= 1
endif

export SPEECH_VQE_FIXED_SAMPLE_RATE ?= 0
ifneq ($(filter 8000 16000,$(SPEECH_VQE_FIXED_SAMPLE_RATE)),)
#export DSP_LIB ?= 1
endif

export BLE_AUDIO_SPEECH_ALGORITHM_SUPPORT ?= 1
ifeq ($(BLE_AUDIO_SPEECH_ALGORITHM_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_SPEECH_ALGORITHM_SUPPORT
export LEA_CALL_FIX_ADC_SAMPLE_RATE ?= 1
endif

ifeq ($(LEA_CALL_FIX_ADC_SAMPLE_RATE),1)
KBUILD_CPPFLAGS += -DLEA_CALL_FIX_ADC_SAMPLE_RATE
endif

export A2DP_AAC_ON ?= 0
ifeq ($(A2DP_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_AAC_ON
KBUILD_CPPFLAGS += -D__ACC_FRAGMENT_COMPATIBLE__
ifneq ($(MAX_AAC_BITRATE),)
KBUILD_CPPFLAGS += -DMAX_AAC_BITRATE=$(MAX_AAC_BITRATE)
endif
endif

export FDKAAC_VERSION ?= 2

ifneq ($(FDKAAC_VERSION),)
KBUILD_CPPFLAGS += -DFDKAAC_VERSION=$(FDKAAC_VERSION)
endif

export AAC_REDUCE_SIZE ?= 0
ifeq ($(AAC_REDUCE_SIZE),1)
KBUILD_CPPFLAGS += -DAAC_REDUCE_SIZE
endif

export SBC_REDUCE_SIZE ?= 0
ifeq ($(SBC_REDUCE_SIZE),1)
KBUILD_CPPFLAGS += -DSBC_REDUCE_SIZE
endif

export GAF_DSP ?= 0
ifeq ($(GAF_DSP),1)
KBUILD_CPPFLAGS += -DGAF_DSP
endif

export A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_ON
export A2DP_LHDC_V3 ?= 0
ifeq ($(A2DP_LHDC_V3),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_V3
endif
core-y += thirdparty/audio_codec_lib/liblhdc-dec/
endif

export A2DP_LHDCV5_ON ?= 0
ifeq ($(A2DP_LHDCV5_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDCV5_ON
core-y += thirdparty/audio_codec_lib/liblhdcv5-dec/
endif

export A2DP_SCALABLE_ON ?= 0
ifeq ($(A2DP_SCALABLE_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SCALABLE_ON
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
#KBUILD_CPPFLAGS += -DA2DP_SCALABLE_UHQ_SUPPORT
core-y += thirdparty/audio_codec_lib/scalable/
endif

export A2DP_LDAC_ON ?= 0
ifeq ($(A2DP_LDAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LDAC_ON
core-y += thirdparty/audio_codec_lib/ldac/
endif

export A2DP_LC3_ON ?= 0
ifeq ($(A2DP_LC3_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LC3_ON
endif

ifneq ($(filter 1,$(A2DP_LC3_ON) $(BLE_AUDIO_ENABLED)),)
export LC3_CODEC_ON ?= 1
endif
ifeq ($(LC3_CODEC_ON),1)
KBUILD_CPPFLAGS += -DLC3_CODEC_ON
endif

export A2DP_LC3_HR ?= 0
ifeq ($(A2DP_LC3_HR),1)
KBUILD_CPPFLAGS += -DA2DP_LC3_HR
endif

export IS_USE_NEW_LC3_CODEC ?= 1
ifeq ($(IS_USE_NEW_LC3_CODEC),1)
KBUILD_CPPFLAGS += -DIS_USE_NEW_LC3_CODEC
endif

export FAST_RAM_RUN_LC3_CODEC ?= 1
ifeq ($(FAST_RAM_RUN_LC3_CODEC),1)
KBUILD_CPPFLAGS += -DFAST_RAM_RUN_LC3_CODEC
endif

export ADVANCE_FILL_ENABLED ?= 1
ifeq ($(ADVANCE_FILL_ENABLED),1)
KBUILD_CPPFLAGS += -DADVANCE_FILL_ENABLED
endif

export A2DP_SBC_PLC_ENABLED ?= 0

export A2DP_CP_ACCEL ?= 0
ifeq ($(A2DP_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DA2DP_CP_ACCEL
endif

export IS_DISABLE_A2DP_ALGORITHM_PLAYBACK_SPEED_TUNING ?= 0
ifeq ($(IS_DISABLE_A2DP_ALGORITHM_PLAYBACK_SPEED_TUNING),1)
KBUILD_CPPFLAGS += -DIS_DISABLE_A2DP_ALGORITHM_PLAYBACK_SPEED_TUNING
endif

export SCO_CP_ACCEL ?= 0
ifeq ($(SCO_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DSCO_CP_ACCEL
# spx fft will share buffer which is not fit for dual cores.
KBUILD_CPPFLAGS += -DUSE_CMSIS_F32_FFT
endif

export SMF_RPC_M55 ?= 0
ifeq (SMF_RPC_M55, 1)
KBUILD_CPPFLAGS += -DSMF_RPC_M55
KBUILD_CFLAGS  += -DSMF_RPC_M55
endif

export SCO_TRACE_CP_ACCEL ?= 0
ifeq ($(SCO_TRACE_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DSCO_TRACE_CP_ACCEL
endif

ifneq ($(AUDIO_DSP_ACCEL),)
export APP_MCPP_CLI := BTH
export APP_RPC_ENABLE := 1
ifeq ($(AUDIO_DSP_ACCEL),HIFI)
export APP_MCPP_BTH_HIFI := 1
else ifeq ($(AUDIO_DSP_ACCEL),M55)
export APP_MCPP_BTH_M55 := 1
else ifeq ($(AUDIO_DSP_ACCEL),SENS)
export APP_MCPP_BTH_SENS := 1
else ifeq ($(AUDIO_DSP_ACCEL),CP_SUBSYS)
export APP_MCPP_BTH_CP_SUBSYS := 1
else
$(error AUDIO_DSP_ACCEL is invalid, expected HIFI, SENS or CP_SUBSYS)
endif
KBUILD_CPPFLAGS += -DAUDIO_DSP_ACCEL
KBUILD_CPPFLAGS += -DAUDIO_DSP_ACCEL_$(AUDIO_DSP_ACCEL)
endif

ifeq ($(BTH_AS_MAIN_MCU),1)
ifneq ($(SPEECH_ALGO_DSP),)
export APP_MCPP_CLI := BTH
export APP_RPC_ENABLE := 1
ifeq ($(SPEECH_ALGO_DSP),BTH)
export APP_MCPP_SRV ?= thirdparty
else ifeq ($(SPEECH_ALGO_DSP),M55)
export APP_MCPP_BTH_M55 := 1
else ifeq ($(SPEECH_ALGO_DSP),HIFI)
export APP_MCPP_BTH_HIFI := 1
else ifeq ($(SPEECH_ALGO_DSP),SENS)
export APP_MCPP_BTH_SENS := 1
else
$(error SPEECH_ALGO_DSP is invalid, expected BTH, M55, HIFI or SENS)
endif
KBUILD_CPPFLAGS += -DSPEECH_ALGO_DSP
KBUILD_CPPFLAGS += -DSPEECH_ALGO_DSP_$(SPEECH_ALGO_DSP)
export SPEECH_TX_24BIT := 1
export SPEECH_RX_24BIT := 1
export SPEECH_TX_THIRDPARTY := 1
endif
else # ifeq ($(BTH_AS_MAIN_MCU),0)
ifneq ($(SPEECH_ALGO_DSP),)
ifeq ($(SENSOR_HUB),1)
export APP_MCPP_CLI := BTH
else
export APP_MCPP_CLI := M55
endif
export APP_RPC_ENABLE := 1
ifeq ($(SPEECH_ALGO_DSP),HIFI)
export APP_MCPP_M55_HIFI := 1
else ifeq ($(SPEECH_ALGO_DSP),SENS)
else
$(error SPEECH_ALGO_DSP is invalid, expected HIFI)
endif
KBUILD_CPPFLAGS += -DSPEECH_ALGO_DSP
KBUILD_CPPFLAGS += -DSPEECH_ALGO_DSP_$(SPEECH_ALGO_DSP)
export SPEECH_TX_24BIT := 1
export SPEECH_RX_24BIT := 1
export SPEECH_TX_THIRDPARTY := 1
endif
endif

ifeq ($(APP_MCPP_BTH_M55),1)
export APP_RPC_BTH_M55_EN := 1
endif

ifneq ($(APP_MCPP_BTH_HIFI_NO_RTOS),1)
ifeq ($(APP_MCPP_BTH_HIFI),1)
export APP_RPC_BTH_DSP_EN := 1
endif
endif

ifeq ($(APP_MCPP_BTH_SENS),1)
export APP_RPC_MCU_SENSOR_EN := 1
endif

ifeq ($(APP_MCPP_CLI),BTH)
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI_BTH
# export APP_MCPP_SRV ?= thirdparty
endif

ifeq ($(APP_MCPP_CLI),M55)
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI
KBUILD_CPPFLAGS += -DAPP_MCPP_CLI_M55
# export APP_MCPP_SRV ?= thirdparty
endif

ifneq ($(APP_MCPP_SRV),)
KBUILD_CPPFLAGS += -DAPP_MCPP_SRV
endif

ifeq ($(APP_RPC_BTH_DSP_EN), 1)
KBUILD_CPPFLAGS += -DAPP_RPC_BTH_DSP_EN
endif

ifeq ($(APP_RPC_M55_DSP_EN), 1)
KBUILD_CPPFLAGS += -DAPP_RPC_M55_DSP_EN
endif

ifeq ($(BT_XTAL_SYNC),1)
KBUILD_CPPFLAGS += -DBT_XTAL_SYNC_NEW_METHOD
KBUILD_CPPFLAGS += -DFIXED_BIT_OFFSET_TARGET
endif

ifeq ($(FPGA_A2DP_SINK),1)
KBUILD_CPPFLAGS += -DFPGA_A2DP_SINK
endif

ifeq ($(HSP_ENABLE),1)
KBUILD_CPPFLAGS += -D__HSP_ENABLE__
endif

export TX_RX_PCM_MASK ?= 0
ifeq ($(TX_RX_PCM_MASK),1)
KBUILD_CPPFLAGS += -DTX_RX_PCM_MASK
endif

export PCM_PRIVATE_DATA_FLAG ?= 0
ifeq ($(PCM_PRIVATE_DATA_FLAG),1)
KBUILD_CPPFLAGS += -DPCM_PRIVATE_DATA_FLAG
endif

export PCM_FAST_MODE ?= 0
ifeq ($(PCM_FAST_MODE),1)
KBUILD_CPPFLAGS += -DPCM_FAST_MODE
endif

export LOW_DELAY_SCO ?= 0
ifeq ($(LOW_DELAY_SCO),1)
KBUILD_CPPFLAGS += -DLOW_DELAY_SCO
endif

export CVSD_BYPASS ?= 0
ifeq ($(CVSD_BYPASS),1)
KBUILD_CPPFLAGS += -DCVSD_BYPASS
endif

export SCO_FORCE_CVSD ?= 0
ifeq ($(SCO_FORCE_CVSD),1)
KBUILD_CPPFLAGS += -DSCO_FORCE_CVSD
endif

export SCO_DMA_SNAPSHOT ?= 0
ifeq ($(CHIP_HAS_SCO_DMA_SNAPSHOT),1)
export SCO_DMA_SNAPSHOT := 1
KBUILD_CPPFLAGS += -DSCO_DMA_SNAPSHOT
endif

export BT_CLK_UNIT_IN_HSLOT ?= 0
ifeq ($(CHIP_BT_CLK_UNIT_IN_HSLOT),1)
export BT_CLK_UNIT_IN_HSLOT := 1
KBUILD_CPPFLAGS += -DBT_CLK_UNIT_IN_HSLOT
endif

export MULTI_DMA_TC ?= 0
ifeq ($(CHIP_HAS_MULTI_DMA_TC),1)
export MULTI_DMA_TC := 1
KBUILD_CPPFLAGS += -DMULTI_DMA_TC
endif

export SUPPORT_ME_MEDIATOR ?= 0
ifeq ($(CHIP_HAS_SUPPORT_ME_MEDIATOR),1)
export SUPPORT_ME_MEDIATOR := 1
KBUILD_CPPFLAGS += -DSUPPORT_ME_MEDIATOR
endif

ifeq ($(CHIP_ROM_UTILS_VER),)
$(warning "CHIP_ROM_UTILS_VER must be defined, CHIP after BEST1501")
$(warning "use \"export CHIP_ROM_UTILS_VER := 2\", other use \"export CHIP_ROM_UTILS_VER := 1\"")
export CHIP_ROM_UTILS_VER := 1
endif
KBUILD_CPPFLAGS += -DROM_UTILS_VER=$(CHIP_ROM_UTILS_VER)

export SCO_OPTIMIZE_FOR_RAM ?= 0
ifeq ($(SCO_OPTIMIZE_FOR_RAM),1)
KBUILD_CPPFLAGS += -DSCO_OPTIMIZE_FOR_RAM
endif

export SW_IIR_PROMPT_EQ_PROCESS ?= 0
ifeq ($(SW_IIR_PROMPT_EQ_PROCESS),1)
ifeq ($(SW_IIR_EQ_PROCESS),1)
$(error SW_IIR_PROMPT_EQ_PROCESS and SW_IIR_EQ_PROCESS cannot be enabled at the same time)
endif
endif

export AAC_TEXT_PARTIAL_IN_FLASH ?= 0
ifeq ($(AAC_TEXT_PARTIAL_IN_FLASH),1)
KBUILD_CPPFLAGS += -DAAC_TEXT_PARTIAL_IN_FLASH
endif

export IS_BES_BATTERY_MANAGER_ENABLED ?= 1
ifeq ($(IS_BES_BATTERY_MANAGER_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_BES_BATTERY_MANAGER_ENABLED
endif

ifeq ($(SUPPORT_BATTERY_REPORT),1)
KBUILD_CPPFLAGS += -DSUPPORT_BATTERY_REPORT
endif

ifeq ($(SUPPORT_HF_INDICATORS),1)
KBUILD_CPPFLAGS += -DSUPPORT_HF_INDICATORS
endif

ifeq ($(SUPPORT_SIRI),1)
KBUILD_CPPFLAGS += -DSUPPORT_SIRI
endif

export BQB_PROFILE_TEST ?= 0
ifeq ($(BQB_PROFILE_TEST),1)
KBUILD_CPPFLAGS += -DBQB_PROFILE_TEST
endif

export AUDIO_SPECTRUM ?= 0
ifeq ($(AUDIO_SPECTRUM),1)
KBUILD_CPPFLAGS += -D__AUDIO_SPECTRUM__
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
endif

ifeq ($(INTERCONNECTION),1)
export INTERCONNECTION
KBUILD_CPPFLAGS += -D__INTERCONNECTION__
endif

export INTERACTION ?= 0
ifeq ($(INTERACTION),1)
KBUILD_CPPFLAGS += -D__INTERACTION__
endif

export INTERACTION_FASTPAIR ?= 0
ifeq ($(INTERACTION_FASTPAIR),1)
KBUILD_CPPFLAGS += -D__INTERACTION_FASTPAIR__
KBUILD_CPPFLAGS += -D__INTERACTION_CUSTOMER_AT_COMMAND__
endif

export TWS_PROMPT_SYNC ?= 0
ifeq ($(TWS_PROMPT_SYNC), 1)
export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED ?= 1
KBUILD_CPPFLAGS += -DTWS_PROMPT_SYNC
endif

export IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED ?= 0
ifeq ($(IS_TWS_IBRT_DEBUG_SYSTEM_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_TWS_IBRT_DEBUG_SYSTEM_ENABLED
endif

export IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED ?= 0
ifeq ($(IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
endif

ifneq ($(filter 2 3,$(CHIP_HAS_DAC_FIFO)),)
export AUDIO_PROMPT_USE_DAC2_ENABLED ?= 1
else
export AUDIO_PROMPT_USE_DAC2_ENABLED ?= 0
endif

ifeq ($(ANC_ASSIST_PILOT_TONE_ALWAYS_ON),1)
ifneq ($(AUDIO_PROMPT_USE_DAC2_ENABLED),0)
$(error AUDIO_PROMPT_USE_DAC2_ENABLED should be 0 for ANC_ASSIST_PILOT_TONE_ALWAYS_ON !)
endif
endif

export PROMPT_SELF_MANAGEMENT ?= 0

ifeq ($(PROMPT_SELF_MANAGEMENT),1)
export HAS_BT_SYNC ?= 1
export AUDIO_PROMPT_USE_DAC2_ENABLED ?= 1
KBUILD_CPPFLAGS += -DPROMPT_SELF_MANAGEMENT
endif

ifeq ($(AUDIO_PROMPT_USE_DAC2_ENABLED),1)
export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED := 0
export AUDIO_OUTPUT_DAC2 := 1
ifneq ($(filter best2300a best1305,$(CHIP)),)
export APP_AUDIO_DELAY_AFTER_OPEN_FOR_TRIGGER := 1
endif
endif

ifeq ($(MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED),1)
KBUILD_CPPFLAGS += -DMIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
export RESAMPLE_ANY_SAMPLE_RATE ?= 1
export AUDIO_OUTPUT_SW_GAIN := 1
endif

ifeq ($(PROMPT_CTRL_SWITCH_ANC_MODE),1)
export PROMPT_CTRL_SWITCH_ANC_MODE
endif

export MEDIA_PLAY_24BIT ?= 1

ifeq ($(LBRT),1)
export LBRT
KBUILD_CPPFLAGS += -DLBRT
endif

ifeq ($(IBRT),1)
export IBRT
KBUILD_CPPFLAGS += -DIBRT
KBUILD_CPPFLAGS += -DIBRT_BLOCKED
KBUILD_CPPFLAGS += -DIBRT_NOT_USE
KBUILD_CPPFLAGS += -D__A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__

export TRACE_GLOBAL_TAG ?= 1
endif

export ENABLE_ROLE_SWITCH_MONITOR ?= 0
ifeq ($(ENABLE_ROLE_SWITCH_MONITOR),1)
KBUILD_CPPFLAGS += -DENABLE_ROLE_SWITCH_MONITOR
endif

export IBRT_TESTMODE ?= 0
ifeq ($(IBRT_TESTMODE),1)
KBUILD_CPPFLAGS += -D__IBRT_IBRT_TESTMODE__
endif

ifeq ($(IBRT),1)
TWS_SYSTEM_ENABLED := 1
endif
ifeq ($(TWS_SYSTEM_ENABLED),1)
export TWS_SYSTEM_ENABLED
KBUILD_CPPFLAGS += -DTWS_SYSTEM_ENABLED
endif

# IBRT_RIGHT_MASTER==1 means right bud is master, otherwise left bud is master
export IBRT_RIGHT_MASTER ?= 1
ifeq ($(IBRT_RIGHT_MASTER),1)
KBUILD_CPPFLAGS += -DIBRT_RIGHT_MASTER
endif

export BES_AUD ?= 0
ifeq ($(BES_AUD),1)
KBUILD_CPPFLAGS += -DBES_AUD
endif

export IBRT_SEARCH_UI ?= 0
ifeq ($(IBRT_SEARCH_UI),1)
KBUILD_CPPFLAGS += -DIBRT_SEARCH_UI
endif

export SEARCH_UI_COMPATIBLE_UI_V2 ?= 0
ifeq ($(SEARCH_UI_COMPATIBLE_UI_V2),1)
KBUILD_CPPFLAGS += -DIBRT_SEARCH_UI
KBUILD_CPPFLAGS += -DSEARCH_UI_COMPATIBLE_UI_V2
endif

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI),1)
export IBRT_V2_MULTIPOINT := 1
KBUILD_CPPFLAGS += -DIBRT_V2_MULTIPOINT
else
export IBRT_V2_MULTIPOINT := 0
endif
else
export IBRT_V2_MULTIPOINT := 0
endif

export IBRT_UI_MASTER_ON_TWS_DISCONNECTED ?= 0
ifeq ($(IBRT_UI_MASTER_ON_TWS_DISCONNECTED),1)
KBUILD_CPPFLAGS += -DIBRT_UI_MASTER_ON_TWS_DISCONNECTED
endif

export AUDIO_MANAGER_TEST_ENABLE ?= 0
ifeq ($(AUDIO_MANAGER_TEST_ENABLE),1)
KBUILD_CPPFLAGS += -DAUDIO_MANAGER_TEST_ENABLE
endif

export AF_STREAM_ID_0_PLAYBACK_FADEOUT ?= 1
ifneq ($(AF_STREAM_ID_0_PLAYBACK_FADEOUT),0)
KBUILD_CPPFLAGS += -DAF_STREAM_ID_0_PLAYBACK_FADEOUT
endif

export POWER_ON_ENTER_TWS_PAIRING_ENABLED ?= 0
ifeq ($(POWER_ON_ENTER_TWS_PAIRING_ENABLED),1)
IGNORE_POWER_ON_KEY_DURING_BOOT_UP ?= 1
KBUILD_CPPFLAGS += -DPOWER_ON_ENTER_TWS_PAIRING_ENABLED
endif

export POWER_ON_ENTER_FREEMAN_PAIRING_ENABLED ?= 0
ifeq ($(POWER_ON_ENTER_FREEMAN_PAIRING_ENABLED),1)
IGNORE_POWER_ON_KEY_DURING_BOOT_UP ?= 1
KBUILD_CPPFLAGS += -DPOWER_ON_ENTER_FREEMAN_PAIRING_ENABLED
endif

export POWER_ON_ENTER_BOTH_SCAN_MODE ?= 0
ifeq ($(POWER_ON_ENTER_BOTH_SCAN_MODE),1)
KBUILD_CPPFLAGS += -DPOWER_ON_ENTER_BOTH_SCAN_MODE
endif

# SEC CON based on Cod LE Audio bit
export BT_SEC_CON_BASED_ON_COD_LE_AUD ?= 1
ifeq ($(BT_SEC_CON_BASED_ON_COD_LE_AUD),1)
KBUILD_CPPFLAGS += -DBT_SEC_CON_BASED_ON_COD_LE_AUD
endif

# BLE_ADV_RPA_ENABLED shall always be 1. if not, RPA is disabled.
export BLE_ADV_RPA_ENABLED ?= 1
export BLE_ADV_REGENERATE_NEW_RPA_DURATION ?= 0
ifeq ($(BLE_ADV_RPA_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_ADV_RPA_ENABLED
KBUILD_CPPFLAGS += -DBLE_ADV_REGENERATE_NEW_RPA_DURATION=60*15
endif

export BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT ?= 0
ifneq ($(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT),0)
KBUILD_CPPFLAGS += -DBLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT=$(BLE_AUDIO_STEREO_CHAN_OVER_CIS_CNT)
endif

export AOB_GMAP_ENABLED ?= 0
ifneq ($(AOB_GMAP_ENABLED),0)
KBUILD_CPPFLAGS += -DAOB_GMAP_ENABLED=$(AOB_GMAP_ENABLED)
endif

export PRODUCTION_LINE_PROJECT_ENABLED ?= 0
ifeq ($(PRODUCTION_LINE_PROJECT_ENABLED),1)
POWER_ON_OPEN_BOX_ENABLED := 0
KBUILD_CPPFLAGS += -DPRODUCTION_LINE_PROJECT_ENABLED
endif

export POWER_ON_OPEN_BOX_ENABLED ?= 0
ifeq ($(POWER_ON_OPEN_BOX_ENABLED),1)
IGNORE_POWER_ON_KEY_DURING_BOOT_UP ?= 1
KBUILD_CPPFLAGS += -DPOWER_ON_OPEN_BOX_ENABLED
endif

export FREEMAN_OTA_ENABLE ?= 0
ifeq ($(FREEMAN_OTA_ENABLE),1)
KBUILD_CPPFLAGS += -DFREEMAN_OTA_ENABLED
endif

export IGNORE_POWER_ON_KEY_DURING_BOOT_UP ?= 0
ifeq ($(IGNORE_POWER_ON_KEY_DURING_BOOT_UP),1)
KBUILD_CPPFLAGS += -DIGNORE_POWER_ON_KEY_DURING_BOOT_UP
endif

ifeq ($(BT_ANC),1)
KBUILD_CPPFLAGS += -D__BT_ANC__
endif

ifeq ($(BTUSB_AUDIO_MODE),1)
export BTUSB_AUDIO_MODE
KBUILD_CPPFLAGS += -DBTUSB_AUDIO_MODE
endif

ifeq ($(BT_USB_AUDIO_DUAL_MODE),1)
export BT_USB_AUDIO_DUAL_MODE
KBUILD_CPPFLAGS += -DBT_USB_AUDIO_DUAL_MODE
endif

export UES_MIC_AS_THE_LE_AUD_INPUT ?= 0
ifeq ($(UES_MIC_AS_THE_LE_AUD_INPUT),1)
KBUILD_CPPFLAGS += -DUES_MIC_AS_THE_LE_AUD_INPUT
endif

export AOB_UC_TEST ?= 0
ifeq ($(AOB_UC_TEST),1)
KBUILD_CPPFLAGS += -DAOB_UC_TEST
endif

export USB_AUDIO_CUSTOM_USB_HID ?= 0
ifeq ($(USB_AUDIO_CUSTOM_USB_HID),1)
KBUILD_CPPFLAGS += -DUSB_AUDIO_CUSTOM_USB_HID
export USB_HID_COMMAND_ENABLE ?= 1
ifeq ($(USB_HID_COMMAND_ENABLE),1)
KBUILD_CPPFLAGS += -DUSB_HID_COMMAND_ENABLE
export USB_AUDIO_CUSTOM_USB_HID_KEY ?= 1
ifeq ($(USB_AUDIO_CUSTOM_USB_HID_KEY),1)
KBUILD_CPPFLAGS += -DUSB_AUDIO_CUSTOM_USB_HID_KEY
endif
export USB_AUDIO_CUSTOM_USB_HID_LED ?= 0
ifeq ($(USB_AUDIO_CUSTOM_USB_HID_LED),1)
KBUILD_CPPFLAGS += -DUSB_AUDIO_CUSTOM_USB_HID_LED
endif
export USB_AUDIO_CUSTOM_ASP ?= 1
ifeq ($(USB_AUDIO_CUSTOM_ASP),1)
KBUILD_CPPFLAGS += -DUSB_AUDIO_CUSTOM_ASP
endif
endif
endif

ifeq ($(HID_REPORT_DESCRIPTOR_CUSTOM),1)
KBUILD_CFLAGS += -D__HID_REPORT_DESCRIPTOR_CUSTOM__
endif
# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BT watch related features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

export BT_WATCH_MASTER ?= 0
export BT_WATCH_SLAVE ?= 0

export EPLAYER_ON ?= 0
export EPLAYER_TEST ?= 0
export ECOMM_ON ?= 0
export ESHELL_ON ?= 0
export EAUDIO_ON ?= 0
export EAUDIO_TEST ?= 0
export BT_SERVICE_ON ?= 0
export BT_SERVICE_NATIVE ?= 0
export BT_SERVICE_DISTRIBUTE ?= 0
export BT_SERVICE_DISTRIBUTE_CLIENT ?= 0
export BT_SERVICE_DISTRIBUTE_SERVER ?= 0
export BT_SERVICE_DISTRIBUTE_TRANSPORT_ECOMM ?= 0
export BT_SERVICE_DISTRIBUTE_TRANSPORT_INTERSYS ?= 0
export BT_SERVICE_TEST ?= 0

export EPLAYER_INCLUDES ?=
EPLAYER_INCLUDES += \
    -Iservices/eplayer/eplayer \
    -Iservices/eplayer/eplayer/source \
    -Iservices/eplayer/eplayer/sink \
    -Iservices/eplayer/eplayer/decoder \
    -Iservices/eplayer/eplayer/mediainfo \
    -Iservices/eplayer/eplayer/platform/besrtx \
    -Iutils/eindexfifo \
    -Iutils/evf/ \
    -Iutils/evf/impl

export ECOMM_INCLUDES ?=
ECOMM_INCLUDES += \
    -Iservices/ecomm/ecomm \
    -Iservices/ecomm/ecomm/transport \
    -Iservices/ecomm/ecomm/platform \
    -Iservices/ecomm/ecomm/platform/bes_cmsisos \
    -Iservices/ecomm/api \
    -Iservices/ecomm/component \
    -Iutils/eindexfifo \
    -Iutils/evf/ \
    -Iutils/evf/impl \
    -Iutils/crc32

export ESHELL_INCLUDES ?=
ESHELL_INCLUDES += \
    -Iutils/eshell

export EAUDIO_INCLUDES ?=
EAUDIO_INCLUDES += \
    -Iservices/eaudio/inc \
    -Iservices/eaudio/effect \
    -Iservices/eaudio/device \
    -Iservices/eaudio/policy \
    -Iservices/eaudio/heap \
    -Iservices/eaudio/resample \
    -Iservices/bt_app \
    -Iutils/cqueue \
    -Iutils/heap \
    -Imultimedia/inc/audio/process/resample/include \
    -Imultimedia/inc/audio/process/resample/coef/include

export BT_SERVICE_INCLUDES ?=
BT_SERVICE_INCLUDES += \
    -Iservices/bt_service \
    -Iservices/bt_service/inc \
    -Iservices/bt_service/distribute/inc \
    -Iservices/bt_service/distribute/client/inc \
    -Iservices/bt_service/distribute/server/inc \
    -Iservices/bt_service/distribute/transport/ecomm \
    -Iutils/transport_if \

EINDEXFIFO_ON ?= 0
EVF_ON ?= 0

ifeq ($(EPLAYER_ON),1)
EINDEXFIFO_ON := 1
EVF_ON := 1
KBUILD_CPPFLAGS += -D__BESRTX__
endif

ifeq ($(ECOMM_ON),1)
EINDEXFIFO_ON := 1
EVF_ON := 1
endif

ifeq ($(EINDEXFIFO_ON),1)
core-y += utils/eindexfifo/
endif

ifeq ($(EVF_ON),1)
core-y += utils/evf/
endif

ifeq ($(ESHELL_ON),1)
core-y += utils/eshell/
KBUILD_CPPFLAGS += -DESHELL_ON
endif

ifeq ($(EAUDIO_ON),1)
KBUILD_CPPFLAGS += -DEAUDIO_ON
endif

ifeq ($(BT_SERVICE_ON),1)
KBUILD_CPPFLAGS += -DBT_SERVICE_ON
endif

ifeq ($(BT_WATCH_MASTER),1)
KBUILD_CPPFLAGS += -DBT_WATCH_MASTER
endif

ifeq ($(BT_WATCH_SLAVE),1)
KBUILD_CPPFLAGS += -DBT_WATCH_SLAVE
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BT source features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

export BT_MULTI_SOURCE ?= 0
export APP_LINEIN_A2DP_SOURCE ?= 0
export A2DP_SOURCE_AAC_ON ?= 0
export A2DP_SOURCE_LHDC_ON ?= 0
export A2DP_SOURCE_LHDCV5_ON ?= 0
export APP_I2S_A2DP_SOURCE ?= 0
export APP_USB_A2DP_SOURCE ?= 0
export HFP_AG_SCO_AUTO_CONN ?= 0
export BT_HFP_AG_ROLE ?= 0
export BT_A2DP_SRC_ROLE ?= 0
export BT_AVRCP_TG_ROLE ?= 0
export SOURCE_TRACE_RX ?= 0
export A2DP_SOURCE_TEST ?= 0
export HFP_AG_TEST ?= 0
export BT_A2DP_SINK_SOURCE_BOTH_SUPPORT ?= 0

ifeq ($(BT_WATCH_APP),1)

export BT_WATCH_APP

export BT_SOURCE := 1

KBUILD_CPPFLAGS += -DBT_WATCH_APP

ifeq ($(BT_SOURCE),1)
KBUILD_CPPFLAGS += -DBT_SOURCE
KBUILD_CPPFLAGS += -DBT_A2DP_SRC_ROLE
KBUILD_CPPFLAGS += -DBT_AVRCP_TG_ROLE
endif

ifeq ($(BT_A2DP_SINK_SOURCE_BOTH_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_A2DP_SINK_SOURCE_BOTH_SUPPORT
endif

ifeq ($(SOURCE_TRACE_RX),1)
KBUILD_CPPFLAGS += -D__SOURCE_TRACE_RX__
endif

ifeq ($(BT_MULTI_SOURCE),1)
KBUILD_CPPFLAGS += -DBT_MULTI_SOURCE
endif

ifeq ($(BT_HFP_AG_ROLE),1)
KBUILD_CPPFLAGS += -DBT_HFP_AG_ROLE

ifeq ($(HFP_AG_SCO_AUTO_CONN),1)
KBUILD_CPPFLAGS += -DHFP_AG_SCO_AUTO_CONN
endif

ifeq ($(HFP_AG_TEST),1)
KBUILD_CPPFLAGS += -DHFP_AG_TEST
endif
endif # BT_HFP_AG_ROLE

ifeq ($(APP_LINEIN_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_LINEIN_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(A2DP_SOURCE_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_AAC_ON
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(A2DP_SOURCE_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_LHDC_ON
KBUILD_CPPFLAGS += -DFLASH_UNIQUE_ID
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
##KBUILD_CPPFLAGS += -DA2DP_TRACE_ENCODE_CP_ACCEL
core-y += thirdparty/audio_codec_lib/liblhdc-enc/
endif

ifeq ($(A2DP_SOURCE_LHDCV5_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_LHDCV5_ON
KBUILD_CPPFLAGS += -DFLASH_UNIQUE_ID
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
#KBUILD_CPPFLAGS += -DA2DP_TRACE_ENCODE_CP_ACCEL
core-y += thirdparty/audio_codec_lib/liblhdcv5-enc/
endif

ifeq ($(APP_I2S_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_I2S_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(APP_USB_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_USB_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
endif

else # !BT_WATCH_APP

ifeq ($(BT_SOURCE),1)

KBUILD_CPPFLAGS += -DBT_SOURCE
KBUILD_CPPFLAGS += -DBT_A2DP_SRC_ROLE
KBUILD_CPPFLAGS += -DBT_AVRCP_TG_ROLE

ifeq ($(BT_A2DP_SINK_SOURCE_BOTH_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_A2DP_SINK_SOURCE_BOTH_SUPPORT
endif

ifeq ($(BT_MULTI_SOURCE),1)
KBUILD_CPPFLAGS += -DBT_MULTI_SOURCE
endif

ifeq ($(SOURCE_TRACE_RX),1)
KBUILD_CPPFLAGS += -D__SOURCE_TRACE_RX__
endif

ifeq ($(BT_HFP_AG_ROLE),1)
KBUILD_CPPFLAGS += -DBT_HFP_AG_ROLE

ifeq ($(HFP_AG_SCO_AUTO_CONN),1)
KBUILD_CPPFLAGS += -DHFP_AG_SCO_AUTO_CONN
endif

ifeq ($(HFP_AG_TEST),1)
KBUILD_CPPFLAGS += -DHFP_AG_TEST
endif
endif # BT_HFP_AG_ROLE

ifeq ($(APP_LINEIN_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_LINEIN_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(A2DP_SOURCE_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_AAC_ON
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(A2DP_SOURCE_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_LHDC_ON
KBUILD_CPPFLAGS += -DFLASH_UNIQUE_ID
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
##KBUILD_CPPFLAGS += -DA2DP_TRACE_ENCODE_CP_ACCEL
core-y += thirdparty/audio_codec_lib/liblhdc-enc/
endif

ifeq ($(A2DP_SOURCE_LHDCV5_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_LHDCV5_ON
KBUILD_CPPFLAGS += -DFLASH_UNIQUE_ID
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
##KBUILD_CPPFLAGS += -DA2DP_TRACE_ENCODE_CP_ACCEL
core-y += thirdparty/audio_codec_lib/liblhdcv5-enc/
endif

ifeq ($(APP_I2S_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_I2S_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifeq ($(APP_USB_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_USB_A2DP_SOURCE
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
#KBUILD_CPPFLAGS += -DA2DP_ENCODE_CP_ACCEL
endif

ifeq ($(A2DP_SOURCE_TEST),1)
KBUILD_CPPFLAGS += -DA2DP_SOURCE_TEST
endif

endif # BT_SOURCE

endif # !BT_WATCH_APP

endif # BT_APP

# -------------------------------------------
# BLE features
# -------------------------------------------
export BLE_STACK_NEW_DESIGN ?= 1

ifeq ($(BLE_STACK_NEW_DESIGN),1)
KBUILD_CPPFLAGS += -DBLE_STACK_NEW_DESIGN
endif

export BLE_AUDIO_USB_DIR_PATH = apps/usbaudio/ble_usb_audio

BLE_AOB_APP_DIR_PATH := bthost/service/ble_audio/aob_app

ifeq ($(BLE_STACK_NEW_DESIGN),1)
BLE_APP_DIR_PATH := bthost/service/ble_app_new
BLE_GAF_CORE_DIR_PATH := bthost/service/ble_audio/blueelf_adapter
BLE_STACK_DIR_PATH := bthost/stack/ble_stack_new
else
BLE_APP_DIR_PATH := bthost/service/ble_app
BLE_GAF_CORE_DIR_PATH := bthost/service/ble_audio/gaf_core
BLE_STACK_DIR_PATH := bthost/stack/ble_stack
endif

export BLE_APP_INCLUDES := \
    -I$(BLUETOOTH_APP_DIR_COMMON) \
    -I$(BLE_APP_DIR_PATH)/inc \
    -I$(BLE_AOB_APP_DIR_PATH)/inc \
    -I$(BLE_GAF_CORE_DIR_PATH)/inc \

export BLE_STACK_INCLUDES := \
    -I$(BT_STACK_ROM_DIR_PATH) \
    -I$(BLE_STACK_DIR_PATH)/inc

ifeq ($(GOOGLE_SERVICE_ENABLE), 1)
export BISTO_ENABLE := 1
export GFPS_ENABLE := 1
export BISTO_IOS_DISABLE := 1
endif

# SASS feature
export SASS_ENABLE ?= 0
export SUPPORT_REMOTE_COD ?= 0

ifeq ($(BISTO_ENABLE),1)
export BISTO_ENABLE
export VOICE_DATAPATH_ENABLED := 1
export CRASH_REBOOT ?= 1
export BLE_SECURITY_ENABLED := 1
ifeq ($(CHIP),best1400)
export DUMP_CRASH_LOG ?= 0
else
export DUMP_CRASH_LOG ?= 0
endif
export VOICE_DATAPATH_TYPE ?= gsound
#export TRACE_DUMP2FLASH ?= 1
export FLASH_SUSPEND := 1
export AI_OTA := 1
KBUILD_CFLAGS += -DBISTO_IOS_DISABLED
endif # ifeq ($(BISTO_ENABLE),1)

# 1 to enable BLE, 0 to disable BLE
export BLE ?= 0

BLE_SWITCH := \
    $(BISTO_ENABLE) \
    $(GFPS_ENABLE) \
    $(AMA_VOICE) \
    $(DMA_VOICE) \
    $(CUSTOMIZE_VOICE) \
    $(GMA_VOICE) \
    $(SMART_VOICE) \
    $(TENCENT_VOICE) \
    $(TILE_DATAPATH_ENABLED) \
    $(BLE_ONLY_ENABLED)

ifneq ($(filter 1, $(BLE_SWITCH)),)
export BLE := 1
endif

ifeq ($(BLE),1)

export BLE_HOST_SUPPORT := 1
ifeq ($(BLE_HOST_SUPPORT),1)
KBUILD_CPPFLAGS += -DBLE_HOST_SUPPORT
endif

export APPEARANCE_VALUE ?= 0
KBUILD_CPPFLAGS += -DAPPEARANCE_VALUE=$(APPEARANCE_VALUE)

#export BLE_IP_VERSION ?= v11_0_7 \
ifeq ($(BLE_IP_VERSION),v11_0_7) \
KBUILD_CPPFLAGS += -DBLE_IP_VERSION=\"v11_0_7\"

#endif #BLE_IP_VERSION

# BLE custom if module
export IS_BLE_CUSTOM_IF_ENABLED ?= 1
ifeq ($(IS_BLE_CUSTOM_IF_ENABLED), 1)
KBUILD_CPPFLAGS += -DIS_BLE_CUSTOM_IF_ENABLED
endif

# BLE profiles enable flag
export IS_USE_CUSTOM_BLE_DATAPATH_PROFILE_UUID_ENABLED ?= 0
ifeq ($(IS_USE_CUSTOM_BLE_DATAPATH_PROFILE_UUID_ENABLED), 1)
KBUILD_CPPFLAGS += -DIS_USE_CUSTOM_BLE_DATAPATH_PROFILE_UUID_ENABLED
endif

## ANC service
export ANCS ?= 0
export ANCC ?= 0

# AMS service
export AMS ?= 0
export AMSC ?= 0

# BMS service
export BMS ?= 0

## HID service
export BLE_HID ?= 0

## BATT service
export BLE_BATT ?= 0

## DISS service
export BLE_DISS ?= 0

## Microsoft swift pair
export SWIFT_ENABLE ?= 0
ifeq ($(SWIFT_ENABLE),1)
KBUILD_CPPFLAGS += -DSWIFT_ENABLED
endif # SWIFT

# by default flags is configured by ble stack
# working when CUSTOMER_DEFINE_ADV_DATA is not defined as custom adv API can set-up whether a adv's flags data
# is configured by application layer or not via
# void app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_E actv_user,
#                    bool is_custom_adv_flags,
export IS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER ?= 0
ifeq ($(IS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER), 1)
KBUILD_CPPFLAGS += -DIS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER
endif

ifeq ($(NO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR),1)
KBUILD_CPPFLAGS += -DNO_DEFINE_SOURCE_PAC_NON_AUDIO_CHAR
endif

export CUSTOMER_DEFINE_ADV_DATA ?= 0
ifeq ($(CUSTOMER_DEFINE_ADV_DATA),1)
KBUILD_CPPFLAGS += -DCUSTOMER_DEFINE_ADV_DATA
endif

export RECORD_KE_MEM_USAGE_ENABLED ?= 0
ifeq ($(RECORD_KE_MEM_USAGE_ENABLED),1)
KBUILD_CPPFLAGS += -D__RECORD_KE_MEM_USAGE_ENABLED__
endif

export BLE_SEC_ACCEPT_BY_CUSTOMER ?= 0
ifeq ($(BLE_SEC_ACCEPT_BY_CUSTOMER),1)
KBUILD_CPPFLAGS += -DBLE_SEC_ACCEPT_BY_CUSTOMER
endif

export IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE ?= 0
ifeq ($(IS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE),1)
KBUILD_CPPFLAGS += -DIS_BLE_ACTIVITY_COUNT_MORE_THAN_THREE
endif

export BLE_WALKIE_TALKIE ?= 0
ifeq ($(BLE_WALKIE_TALKIE),1)
KBUILD_CPPFLAGS += -DBLE_WALKIE_TALKIE
export WT_AUTO_BUILD_NET ?= 0
ifeq ($(WT_AUTO_BUILD_NET),1)
KBUILD_CPPFLAGS += -DWT_AUTO_BUILD_NET
endif
export WT_ROUTE_ENABLE ?= 0
ifeq ($(WT_ROUTE_ENABLE),1)
KBUILD_CPPFLAGS += -DWT_ROUTE_ENABLE
endif
export WT_SUB_NETWORK_ENABLE ?= 0
ifeq ($(WT_SUB_NETWORK_ENABLE),1)
KBUILD_CPPFLAGS += -DWT_SUB_NETWORK_ENABLE
endif
export WT_ENABLE_ENC ?= 0
ifeq ($(WT_ENABLE_ENC),1)
KBUILD_CPPFLAGS += -DWT_ENABLE_ENC
endif
endif

# rwapp_config
export CFG_APP_DATAPATH_SERVER ?= 1
ifeq ($(CFG_APP_DATAPATH_SERVER),1)
KBUILD_CPPFLAGS += -DCFG_APP_DATAPATH_SERVER
endif # CFG_APP_DATAPATH_SERVER

export CFG_APP_DATAPATH_CLIENT ?= 1
ifeq ($(CFG_APP_DATAPATH_CLIENT),1)
KBUILD_CPPFLAGS += -DCFG_APP_DATAPATH_CLIENT
#datapath server must be enabled when datapath client is enabled
CFG_APP_DATAPATH_SERVER ?= 1
endif # CFG_APP_DATAPATH_CLIENT


ifeq ($(CFG_APP_MESH),1)
KBUILD_CPPFLAGS += -DCFG_APP_MESH
endif # CFG_APP_MESH

ifeq ($(CFG_APP_HT),1)
KBUILD_CPPFLAGS += -DCFG_APP_HT
endif # CFG_APP_HT

ifeq ($(CFG_APP_HR),1)
KBUILD_CPPFLAGS += -DCFG_APP_HR
endif # CFG_APP_HR

ifeq ($(GFPS_ENABLE),1)
KBUILD_CPPFLAGS += -DGFPS_ENABLED
ifeq ($(SASS_ENABLE),1)
export SUPPORT_REMOTE_COD := 1
AUTO_ACCEPT_SECOND_SCO := 0
BT_MUTE_A2DP := 1
BT_PAUSE_A2DP := 0
KBUILD_CPPFLAGS += -DSASS_ENABLED
KBUILD_CPPFLAGS += -DBT_MUTE_A2DP
endif
endif # GFPS_ENABLED

ifeq ($(CFG_APP_BAS),1)
KBUILD_CPPFLAGS += -DCFG_APP_BAS
endif # CFG_APP_BAS

export BES_AHP ?= 0
ifeq ($(BES_AHP),1)
KBUILD_CPPFLAGS += -DBES_AHP
KBUILD_CPPFLAGS += -DCFG_APP_AHP_SERVER
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# SAS feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export BES_SAS ?= 0
ifeq ($(BES_SAS),1)
KBUILD_CPPFLAGS += -DBES_SAS
KBUILD_CPPFLAGS += -DCFG_APP_SAS_SERVER
endif

export BES_MOBILE_SAS ?= 0
ifeq ($(BES_MOBILE_SAS),1)
KBUILD_CPPFLAGS += -DBES_MOBILE_SAS
endif

# rwip_config
KBUILD_CPPFLAGS += -D__IAG_BLE_INCLUDE__

ifeq ($(CFG_AHITL),1)
KBUILD_CPPFLAGS += -DCFG_AHITL
endif # Application Host Interface

ifeq ($(CFG_CON_CTE_REQ),1)
KBUILD_CPPFLAGS += -DCFG_CON_CTE_REQ
endif

ifeq ($(CFG_CON_CTE_RSP),1)
KBUILD_CPPFLAGS += -DCFG_CON_CTE_RSP
endif

ifeq ($(CFG_CONLESS_CTE_TX),1)
KBUILD_CPPFLAGS += -DCFG_CONLESS_CTE_TX
endif

ifeq ($(CFG_CONLESS_CTE_RX),1)
KBUILD_CPPFLAGS += -DCFG_CONLESS_CTE_RX
endif

ifeq ($(CFG_AOD),1)
KBUILD_CPPFLAGS += -DCFG_AOD
endif

export CFG_AOA ?= 0
ifeq ($(CFG_AOA),1)
KBUILD_CPPFLAGS += -DCFG_AOA
endif

export CFG_SUBRATE ?= 0
ifeq ($(CFG_SUBRATE),1)
KBUILD_CPPFLAGS += -DCFG_SUBRATE
endif
# rwip_config end

KBUILD_CPPFLAGS += -DBLE_STACK_PORTING_CHANGES
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH) \
    -I$(BLE_APP_DIR_PATH)/app_batt/ \
    -I$(BLE_APP_DIR_PATH)/app_ble_custom \
    -I$(BLE_APP_DIR_PATH)/app_datapath/ \
    -I$(BLE_APP_DIR_PATH)/app_gfps \
    -I$(BLE_APP_DIR_PATH)/app_hid/ \
    -I$(BLE_APP_DIR_PATH)/app_hrps/ \
    -I$(BLE_APP_DIR_PATH)/app_htp/ \
    -I$(BLE_APP_DIR_PATH)/app_main \
    -I$(BLE_APP_DIR_PATH)/app_ota \
    -I$(BLE_APP_DIR_PATH)/app_sec \
    -I$(BLE_APP_DIR_PATH)/app_tota \
    -I$(BLE_APP_DIR_PATH)/app_tws \
    -I$(BLE_APP_DIR_PATH)/app_vob \
    -Iutils/hsm
ifeq ($(AMSC),1)
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH)/app_amsc/
endif
ifeq ($(ANCS),1)
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH)/app_ancs/
endif
ifeq ($(ANCC),1)
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH)/app_ancc/
endif
ifeq ($(SWIFT_ENABLE),1)
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH)/app_swift/
endif
ifeq ($(BLE_WALKIE_TALKIE),1)
BLE_APP_INCLUDES += \
    -I$(BLE_APP_DIR_PATH)/app_walkie_talkie/
endif
ifeq ($(BLE_AUDIO_ENABLED),1)
BLE_APP_INCLUDES += \
    -I$(BLE_GAF_CORE_DIR_PATH) \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_bap \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_arc \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_acc \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_atc \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_cap \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_hap \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_tmap \
    -I$(BLE_GAF_CORE_DIR_PATH)/app_tm \
    -I$(BLE_AOB_APP_DIR_PATH)/inc/ \
    -I$(BLE_AOB_APP_DIR_PATH)/gaf_audio \
    -I$(BLE_AUDIO_CENTRAL_DIR_PATH) \
    -I$(BLE_AUDIO_USB_DIR_PATH)  \
    -I$(BLE_AUDIO_I2S_DIR_PATH)
endif

BLE_STACK_INCLUDES += \
    -Ibthost/stack/le_bridge \
    -I$(BLE_STACK_DIR_PATH) \
    -I$(BLE_STACK_DIR_PATH)/ip/ahi/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/inc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/gap/gapc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/gap/gapm/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/gap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/gatt/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/hl/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/inc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/hl/src/l2cap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/hci/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/hci/src/ \
    -I$(BLE_STACK_DIR_PATH)/modules/aes/api/ \
    -I$(BLE_STACK_DIR_PATH)/modules/common/api/ \
    -I$(BLE_STACK_DIR_PATH)/modules/ecc_p256/api/ \
    -I$(BLE_STACK_DIR_PATH)/modules/ke/api/ \
    -I$(BLE_STACK_DIR_PATH)/modules/rwip/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/iso/data_path/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/iso/data_path/isogen/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/iso/data_path/isoohci/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/iso/data_path/isogen/src/ \
    -I$(BLE_STACK_DIR_PATH)/ip/ble/iso/data_path/isoohci/src/

ifeq ($(BLE_AUDIO_ENABLED),1)
BLE_STACK_INCLUDES += \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/acc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/acc/mc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/acc/ot/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/acc/tb/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/arc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/arc/aic/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/arc/mic/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/arc/vc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/arc/voc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/atc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/atc/csi/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/bap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/bap/bc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/bap/capa/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/bap/uc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/cap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/hap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/iap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/tmap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/inc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/acc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/acc/mc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/acc/ot/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/acc/tb/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/al/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/arc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/arc/aic/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/arc/mic/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/arc/vc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/arc/voc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/atc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/atc/csi/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/bap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/bap/bc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/bap/capa/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/bap/codec/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/bap/uc/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/cap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/hap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/iap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/src/tmap/ \
    -I$(BLE_STACK_DIR_PATH)/ip/gaf/api/tb/
endif

export BLE_CONNECTION_MAX ?= 2
KBUILD_CPPFLAGS += -DBLE_CONNECTION_MAX=$(BLE_CONNECTION_MAX)

ifeq ($(IS_ENABLE_DEUGGING_MODE),1)
KBUILD_CPPFLAGS += -DIS_ENABLE_DEUGGING_MODE
endif

export BLE_AUDIO_FRAME_DUR_7_5MS ? = 0
ifeq ($(BLE_AUDIO_FRAME_DUR_7_5MS),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_FRAME_DUR_7_5MS
endif

export AOB_LOW_LATENCY_MODE ? = 0
ifeq ($(AOB_LOW_LATENCY_MODE),1)
KBUILD_CPPFLAGS += -DAOB_LOW_LATENCY_MODE
endif

export AOB_LATENCY_TEST_MODE ? = 0
ifeq ($(AOB_LATENCY_TEST_MODE),1)
KBUILD_CPPFLAGS += -DAOB_LATENCY_TEST_MODE
endif

export BLE_AUDIO_ENABLED ?= 0
ifeq ($(BLE_AUDIO_ENABLED),1)
BLE_ISO_ENABLED = 1
KBUILD_CPPFLAGS += -DINTERSYS_STACK_SIZE=2048
KBUILD_CPPFLAGS += -DBLE_AUDIO_ENABLED=1

export LC3PLUS_SUPPORT ?= 0
ifeq ($(LC3PLUS_SUPPORT),1)
KBUILD_CPPFLAGS += -DLC3PLUS_SUPPORT
endif

export HID_ULL_ENABLE ?= 0
ifeq ($(HID_ULL_ENABLE),1)
KBUILD_CPPFLAGS += -DHID_ULL_ENABLE
endif

export IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED ?= 0
ifeq ($(IS_OPEN_TPORT_AFTER_POWER_ON_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_OPEN_TPORT_AFTER_POWER_ON_ENABLED
endif

export mHDT_LE_SUPPORT ?= 0
ifeq ($(mHDT_LE_SUPPORT),1)
KBUILD_CPPFLAGS += -DmHDT_LE_SUPPORT=1
endif

export BLE_AUDIO_CONNECTION_CNT ?= 2
KBUILD_CPPFLAGS += -DBLE_AUDIO_CONNECTION_CNT=$(BLE_AUDIO_CONNECTION_CNT)

KBUILD_CPPFLAGS += -DBLE_AUDIO_CIS_CONN_CNT=4
KBUILD_CPPFLAGS += -DBLE_AUDIO_IS_ALWAYS_USE_TEST_MODE_CIG_BIG_CREATING

export BLE_AUDIO_IS_USE_24_BIT_DEPTH ?= 1
ifeq ($(BLE_AUDIO_IS_USE_24_BIT_DEPTH),1)
KBUILD_CPPFLAGS += -D__BLE_AUDIO_24BIT__
export SPEECH_TX_24BIT := 1
export SPEECH_RX_24BIT := 1
endif

export BLE_AUDIO_USE_TWO_MIC_SRC_FOR_DONGLE ?= 0
ifeq ($(BLE_AUDIO_USE_TWO_MIC_SRC_FOR_DONGLE),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_USE_TWO_MIC_SRC_FOR_DONGLE
endif
export BLE_AUDIO_USE_TWO_CHANNEL_SINK_FOR_DONGLE ?= 0
ifeq ($(BLE_AUDIO_USE_TWO_CHANNEL_SINK_FOR_DONGLE),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_USE_TWO_CHANNEL_SINK_FOR_DONGLE
endif

export BLE_USB_AUDIO_IS_DONGLE_ROLE ?= 0
ifeq ($(BLE_USB_AUDIO_IS_DONGLE_ROLE),1)
KBUILD_CPPFLAGS += -DBLE_USB_AUDIO_IS_DONGLE_ROLE
endif

# should use this common macro
export BLE_AUDIO_CENTRAL_SELF_PAIRING_FEATURE_ENABLED ?= 0
ifeq ($(BLE_AUDIO_CENTRAL_SELF_PAIRING_FEATURE_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_CENTRAL_SELF_PAIRING_FEATURE_ENABLED
endif

export BLE_USB_AUDIO_SELF_PAIRING_FEATURE_ENABLED ?= 0
ifeq ($(BLE_USB_AUDIO_SELF_PAIRING_FEATURE_ENABLED),1)
BLE_AUDIO_CENTRAL_SELF_PAIRING_FEATURE_ENABLED := 1
endif

ifeq ($(LEA_FOUR_CIS_ENABLED),1)
export LEA_FOUR_CIS_ENABLED
KBUILD_CPPFLAGS += -DLEA_FOUR_CIS_ENABLED
endif

export AOB_MOBILE_ENABLED ?= 0
ifeq ($(AOB_MOBILE_ENABLED),1)
KBUILD_CPPFLAGS += -DAOB_MOBILE_ENABLED
KBUILD_CPPFLAGS += -DBLE_AUDIO_IS_SUPPORT_CENTRAL_ROLE
endif

export AOB_SPLIT_PAC_RECORD_INTO_FOUR_RECORDS ?= 0
ifeq ($(AOB_SPLIT_PAC_RECORD_INTO_FOUR_RECORDS),1)
KBUILD_CPPFLAGS += -DAOB_SPLIT_PAC_RECORD_INTO_FOUR_RECORDS
endif

export IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE ?= 0
ifeq ($(IO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE),1)
KBUILD_CPPFLAGS += -DIO_CAPABILITY_NO_INPUT_NO_OUTPUT_MITM_FALSE
endif

ifeq ($(BINAURAL_RECORD_PROCESS),1)
export BINAURAL_RECORD_PROCESS
endif

ifneq ($(filter 192000,$(BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE)),)
KBUILD_CPPFLAGS += -DBINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE=$(BINAURAL_RECORD_ADC_HIGH_SAMPLE_RATE)
endif

else
KBUILD_CPPFLAGS += -DBLE_AUDIO_ENABLED=0
KBUILD_CPPFLAGS += -DTWS_SYNC_NVRECORD_BLE_DEVICE_INFO_ENABLED
KBUILD_CPPFLAGS += -DMASTER_SEND_ENCRYPT_REQUEST_ENABLED
endif

ifneq ($(filter 1,$(BLE_I2S_AUDIO_SUPPORT) $(BLE_USB_AUDIO_SUPPORT)),)
export BLE_AUDIO_CENTRAL_APP_ENABLED := 1
ifeq ($(BLE_AUDIO_CENTRAL_APP_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_CENTRAL_APP_ENABLED
endif
endif

ifeq ($(GFPS_ENABLE),1)
BLE_SECURITY_ENABLED := 1
endif
ifeq ($(BLE_SECURITY_ENABLED), 1)
export BLE_SECURITY_ENABLED
KBUILD_CPPFLAGS += -DCFG_APP_SEC
endif

ifeq ($(BISTO_ENABLE),1)
ifneq ($(IBRT),1)
#disbled before IBRT MAP role switch feature is ready
KBUILD_CPPFLAGS += -DBT_MAP_SUPPORT
KBUILD_CPPFLAGS += -DBT_OBEX_SUPPORT
endif
endif

KBUILD_CPPFLAGS += -D__BLE_TX_USE_BT_TX_QUEUE__
KBUILD_CPPFLAGS += -DBES_BLE_ACTIVITY_MAX=11
KBUILD_CPPFLAGS += -DBES_BLE_ADV_DATA_MAX=31

ifeq ($(ANCS),1)
KBUILD_CPPFLAGS += -DANCS_ENABLED
endif

ifeq ($(ANCC),1)
KBUILD_CPPFLAGS += -DANCC_ENABLED
endif

ifeq ($(AMS),1)
KBUILD_CPPFLAGS += -DAMS_ENABLED
endif

ifeq ($(AMSC),1)
KBUILD_CPPFLAGS += -DAMSC_ENABLED
endif

ifeq ($(BMS),1)
KBUILD_CPPFLAGS += -DBMS_ENABLED
endif

ifeq ($(BLE_HID),1)
KBUILD_CPPFLAGS += -DBLE_HID_ENABLE
endif

ifeq ($(BLE_BATT),1)
KBUILD_CPPFLAGS += -DBLE_BATT_ENABLE
endif

ifeq ($(BLE_DISS),1)
KBUILD_CPPFLAGS += -DBLE_DISS_ENABLE
endif

export BLE_HOST_PTS_TEST_ENABLED ?= 0
ifeq ($(BLE_HOST_PTS_TEST_ENABLED),1)
export BLE_ISO_ENABLED = 1
KBUILD_CPPFLAGS += -DBLE_HOST_PTS_TEST_ENABLED
endif

BLE_AUDIO_TEST_ENABLED ?= 1
ifeq ($(BLE_AUDIO_TEST_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_TEST_ENABLED
endif

ifeq ($(IS_BLE_AUDIO_PTS_TEST_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_BLE_AUDIO_PTS_TEST_ENABLED
endif

export USE_MS_AS_BLE_ADV_INTERVAL_UNIT ?= 1
ifeq ($(USE_MS_AS_BLE_ADV_INTERVAL_UNIT),1)
KBUILD_CPPFLAGS += -DUSE_MS_AS_BLE_ADV_INTERVAL_UNIT
endif

export IS_NEW_LE_AUDIO_SYNC_CAPTURE_ALG_ENABLED ?= 0
ifeq ($(IS_NEW_LE_AUDIO_SYNC_CAPTURE_ALG_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_NEW_LE_AUDIO_SYNC_CAPTURE_ALG_ENABLED
endif

export BLE_ISO_ENABLED ?= 0
ifeq ($(BLE_ISO_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_ISO_ENABLED
endif




export BLE_BIS_TRANSPORT ?= 0
ifeq ($(BLE_BIS_TRANSPORT),1)
KBUILD_CPPFLAGS += -DBLE_BIS_TRANSPORT
KBUILD_CPPFLAGS += -DBLE_BIS_TRANSPORT_BUF_SIZE=100*1024
export APP_BLE_BIS_SRC_ENABLE := 1
ifeq ($(APP_BLE_BIS_SRC_ENABLE), 1)
KBUILD_CPPFLAGS += -DAPP_BLE_BIS_SRC_ENABLE=1
endif
export BLE_BIS_TRAN_INPUT_BT ?= 1
export BLE_BIS_TRAN_INPUT_USB ?= 0
export BLE_BIS_TRAN_INPUT_LINEIN ?= 1
export BLE_BIS_TRAN_INPUT_WIFI ?= 1
export ISO_BUF_NB ?= 17
export ISO_BUF_SIZE ?= 450
ifeq ($(BLE_BIS_TRAN_INPUT_BT),1)
export AUDIO_OUTPUT_ROUTE_SELECT := 1
endif
endif

# GATT over BR/EDR features
export GATT_OVER_BR_EDR ?= 0
ifeq ($(GATT_OVER_BR_EDR),1)
KBUILD_CPPFLAGS += -D__GATT_OVER_BR_EDR__
endif

export BLE_API_UT_ENABLED ?= 0
ifeq ($(BLE_API_UT_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_API_UT_ENABLED
endif

endif # BLE

# -------------------------------------------
# Full application features (BT and/or BLE)
# -------------------------------------------

ifeq ($(FULL_APP_PROJECT),1)

export SPEECH_LIB ?= 1

# make sure the value of GFPS_ENABLE and GMA_VOICE is confirmed above here
ifneq ($(filter 1,$(GFPS_ENABLE) $(GMA_VOICE) $(TOTA) $(TOTA_v2) $(BLE) $(CTKD_ENABLE)),)
core-y += utils/encrypt/
endif

ifeq ($(CPPOBJ_ENABLE),1)
export CPPOBJ_ENABLE
core-y += utils/cppobject/
KBUILD_CPPFLAGS += -DCPPOBJ_ENABLE
endif

ifeq ($(CTKD_ENABLE),1)
export CTKD_ENABLE
core-y += utils/aes_cmac/
KBUILD_CPPFLAGS += -DCTKD_ENABLE
endif

export IS_CTKD_OVER_BR_EDR_ENABLED ?= 0
ifeq ($(IS_CTKD_OVER_BR_EDR_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_CTKD_OVER_BR_EDR_ENABLED
endif

ifeq ($(SIGNAL_GENERATOR),1)
core-y += utils/signal_generator/
endif

ifeq ($(LOCAL_PCM),1)
core-y += utils/local_pcm/
endif

export FLASH_PROTECTION ?= 1

export APP_TEST_AUDIO ?= 0

ifeq ($(APP_TEST_MODE),1)
export APP_TEST_MODE
KBUILD_CPPFLAGS += -DAPP_TEST_MODE
endif

export VOICE_PROMPT ?= 1

export AUDIO_QUEUE_SUPPORT ?= 1

export VOICE_RECOGNITION ?= 0

export FLASH_SUSPEND ?= 1

export ENGINEER_MODE ?= 1
ifeq ($(ENGINEER_MODE),1)
FACTORY_MODE := 1
endif
ifeq ($(FACTORY_MODE),1)
KBUILD_CPPFLAGS += -D__FACTORY_MODE_SUPPORT__
endif

export NEW_NV_RECORD_ENABLED ?= 1
ifeq ($(NEW_NV_RECORD_ENABLED),1)
KBUILD_CPPFLAGS += -DNEW_NV_RECORD_ENABLED
KBUILD_CPPFLAGS += -Iservices/nv_section/userdata_section
endif

ifeq ($(APP_USE_LED_INDICATE_IBRT_STATUS),1)
KBUILD_CPPFLAGS += -D__APP_USE_LED_INDICATE_IBRT_STATUS__
endif

ifeq ($(HEAR_THRU_PEAK_DET),1)
KBUILD_CPPFLAGS += -D__HEAR_THRU_PEAK_DET__
endif

ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -DBESBT_STACK_SIZE=1024*8+512
else
KBUILD_CPPFLAGS += -DBESBT_STACK_SIZE=1024*5+512
endif

ifeq ($(FREE_TWS_PAIRING_ENABLED),1)
KBUILD_CPPFLAGS += -DFREE_TWS_PAIRING_ENABLED
endif

ifeq ($(VOICE_TX_AEC),1)
export VOICE_TX_AEC
KBUILD_CPPFLAGS += -DVOICE_TX_AEC
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(APP_NOISE_ESTIMATION),1)
export APP_NOISE_ESTIMATION
KBUILD_CPPFLAGS += -DAPP_NOISE_ESTIMATION
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(VOICE_PROMPT),1)
KBUILD_CPPFLAGS += -DMEDIA_PLAYER_SUPPORT
endif

ifeq ($(TOTA),1)
KBUILD_CPPFLAGS += -DTOTA
export IS_BLE_TOTA_ENABLED ?= 1
ifeq ($(IS_BLE_TOTA_ENABLED),1)
ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -DBLE_TOTA_ENABLED
endif
endif
KBUILD_CPPFLAGS += -DSHOW_RSSI
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED
export TEST_OVER_THE_AIR ?= 1
endif

ifeq ($(TOTA_v2),1)
KBUILD_CPPFLAGS += -DTOTA_v2
export IS_BLE_TOTA_ENABLED ?= 1
ifeq ($(IS_BLE_TOTA_ENABLED),1)
ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -DBLE_TOTA_ENABLED
endif
endif
KBUILD_CPPFLAGS += -DSHOW_RSSI
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED
export TEST_OVER_THE_AIR_v2 ?= 1
endif

export RESUME_MUSIC_AFTER_CRASH_REBOOT ?= 0
ifeq ($(RESUME_MUSIC_AFTER_CRASH_REBOOT),1)
KBUILD_CPPFLAGS += -DRESUME_MUSIC_AFTER_CRASH_REBOOT
endif

ifeq ($(TEST_OVER_THE_AIR),1)
export TEST_OVER_THE_AIR
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED=1
endif

ifeq ($(TEST_OVER_THE_AIR_v2),1)
export TEST_OVER_THE_AIR_v2
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED=1
endif

ifeq ($(USE_THIRDPARTY),1)
KBUILD_CPPFLAGS += -D__THIRDPARTY
core-y += thirdparty/
endif

ifeq ($(SENSOR),1)
export SENSOR
KBUILD_CPPFLAGS += -D__SENSOR__
endif

export APP_KEY_ENABLE ?= 1
ifeq ($(APP_KEY_ENABLE),1)
KBUILD_CPPFLAGS += -DAPP_KEY_ENABLE
endif

ifeq ($(APP_ANC_TEST),1)
export APP_ANC_TEST
KBUILD_CPPFLAGS += -DAPP_ANC_TEST
endif

export SPEECH_TUNING_CALL_VIA_CMD ?= 0
ifeq ($(SPEECH_TUNING_CALL_VIA_CMD),1)
export AUDIO_DEBUG := 1
KBUILD_CPPFLAGS += -DSPEECH_TUNING_CALL_VIA_CMD
endif

ifeq ($(AUDIO_DEBUG_VIA_SPP),1)
export AUDIO_DEBUG_VIA_SPP
export AUDIO_DEBUG_VIA_TOTA ?= 1
KBUILD_CPPFLAGS += -DAUDIO_DEBUG_VIA_SPP
KBUILD_CPPFLAGS += -DSPP_DEBUG_TOOL
endif

ifeq ($(AUDIO_DEBUG_VIA_TOTA),1)
export AUDIO_DEBUG_VIA_TOTA
export AUDIO_DEBUG ?= 1
KBUILD_CPPFLAGS += -DAUDIO_DEBUG_VIA_TOTA
endif

# AUDIO_DEBUG
ifeq ($(AUDIO_DEBUG_CMD),1)
export AUDIO_DEBUG_CMD
endif

ifeq ($(AUDIO_DEBUG),1)
export AUDIO_DEBUG
export AUDIO_DEBUG_CMD ?= 1
KBUILD_CPPFLAGS += -DAUDIO_DEBUG
export TRACE_BAUD_RATE := 2000000
endif
# AUDIO_DEBUG END

ifneq ($(filter BT_HFP_AT BT_SPP,$(APP_DEBUG_TOOL)),)
export APP_DEBUG_TOOL
endif

export BT_WIFI_COEX_P12 ?=0
ifeq ($(DEBUG),1)
ifneq ($(filter 1, $(AUDIO_DEBUG_CMD) $(AUTO_TEST) $(BES_AUTOMATE_TEST)),)
export APP_TRACE_RX_ENABLE := 1
else
ifneq ($(BT_DEBUG_TPORTS),0)
ifeq ($(BT_WIFI_COEX_P12),0)
export APP_TRACE_RX_ENABLE := 1
endif
endif
endif
endif
ifeq ($(APP_TRACE_RX_ENABLE),1)
KBUILD_CPPFLAGS += -DAPP_TRACE_RX_ENABLE
endif

export TOTA_CROSS_CHIP_OTA ?= 0
ifeq ($(TOTA_CROSS_CHIP_OTA),1)
export TOTA_CROSS_CHIP_OTA
KBUILD_CFLAGS += -DTOTA_CROSS_CHIP_OTA

ifeq ($(TOTA_CROSS_CHIP_OTA_ROLE_IS_DONGLE),1)
KBUILD_CPPFLAGS += -DTOTA_CROSS_CHIP_OTA_ROLE_IS_DONGLE
else ifeq ($(TOTA_CROSS_CHIP_OTA_ROLE_IS_MASTER),1)
KBUILD_CPPFLAGS += -DTOTA_CROSS_CHIP_OTA_ROLE_IS_MASTER
else ifeq ($(TOTA_CROSS_CHIP_OTA_ROLE_IS_SLAVE),1)
KBUILD_CPPFLAGS += -DTOTA_CROSS_CHIP_OTA_ROLE_IS_SLAVE
else
$(error the ota role should be declared!)
endif
endif

ifeq ($(AUTO_TEST),1)
export AUTO_TEST
KBUILD_CFLAGS += -D_AUTO_TEST_
endif

ifeq ($(BES_AUTOMATE_TEST),1)
export BES_AUTOMATE_TEST
KBUILD_CFLAGS += -DBES_AUTOMATE_TEST
endif

ifeq ($(SPEECH_BONE_SENSOR),1)
export SPEECH_BONE_SENSOR

ifneq ($(VPU_NAME),)
export VPU_NAME
else
export VPU_NAME ?= LIS25BA
endif


# ADC and I2S use PLL
# export NO_SCO_RESAMPLE := 1
KBUILD_CPPFLAGS += -DSPEECH_BONE_SENSOR

# Configure IIC
KBUILD_CPPFLAGS += -DI2C_TASK_MODE
KBUILD_CPPFLAGS += -DVPU_I2C_ID=0

# Configure I2S/TDM
ifneq ($(CHIP_HAS_TDM),)
# 2300p/2300a...
export AF_DEVICE_TDM := 1
export SPEECH_BONE_SENSOR_TDM := 1
ifeq ($(VPU_NAME),LIS25BA)
KBUILD_CPPFLAGS += -DI2S_MCLK_FROM_SPDIF
KBUILD_CPPFLAGS += -DI2S_MCLK_PIN
KBUILD_CPPFLAGS += -DI2S_MCLK_IOMUX_INDEX=33
endif
# Configure use TDM0(=0)/TDM1(=1)
export VPU_STREAM_USE_TDM := 1
else
# 2300
export AF_DEVICE_I2S := 1
export SPEECH_BONE_SENSOR_I2S := 1
ifeq ($(VPU_NAME),LIS25BA)
KBUILD_CPPFLAGS += -DI2S_MCLK_FROM_SPDIF
KBUILD_CPPFLAGS += -DI2S_MCLK_PIN
KBUILD_CPPFLAGS += -DI2S_MCLK_IOMUX_INDEX=13
KBUILD_CPPFLAGS += -DCLKOUT_IOMUX_INDEX=13
endif
endif

# Sync ADC and I2S
ifeq ($(CHIP_HAS_I2S_TDM_TRIGGER),1)
# Hardware trigger method
KBUILD_CPPFLAGS += -DHW_I2S_TDM_TRIGGER
else
# Software method
export INT_LOCK_EXCEPTION ?= 1
KBUILD_CPPFLAGS += -DAF_ADC_I2S_SYNC
endif
ifeq ($(VPU_STREAM_SYNC_TUNING),1)
KBUILD_CPPFLAGS += -DVPU_STREAM_SYNC_TUNING
endif
export VPU_DEBUG_CODEC_I2S_SYNC ?= 0
endif # SPEECH_BONE_SENSOR

export IS_AUTOPOWEROFF_ENABLED ?= 1
ifeq ($(IS_AUTOPOWEROFF_ENABLED),1)
KBUILD_CFLAGS += -D__BTIF_AUTOPOWEROFF__
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BISTO feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BISTO_ENABLE),1)

KBUILD_CFLAGS += -DBISTO_ENABLED

KBUILD_CPPFLAGS += -DCFG_SW_KEY_LPRESS_THRESH_MS=1000

KBUILD_CPPFLAGS += -DDEBUG_BLE_DATAPATH=0

KBUILD_CFLAGS += -DCRC32_OF_IMAGE

ASSERT_SHOW_FILE_FUNC ?= 1

#KBUILD_CPPFLAGS += -DSAVING_AUDIO_DATA_TO_SD_ENABLED=1

KBUILD_CPPFLAGS += -DIS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED

ifeq ($(GSOUND_HOTWORD_ENABLE), 1)
export GSOUND_HOTWORD_ENABLE
KBUILD_CFLAGS += -DGSOUND_HOTWORD_ENABLED
ifeq ($(GSOUND_HOTWORD_EXTERNAL), 1)
export GSOUND_HOTWORD_EXTERNAL
KBUILD_CFLAGS += -DGSOUND_HOTWORD_EXTERNAL
endif
endif

REDUCED_GUESTURE_ENABLE ?= 0

ifeq ($(REDUCED_GUESTURE_ENABLE), 1)
KBUILD_CFLAGS += -DREDUCED_GUESTURE_ENABLED
endif
endif # BISTO_ENABLE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# GFPS feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(GFPS_ENABLE),1)
export BLE_SECURITY_ENABLED := 1
export GFPS_ENABLE

KBUILD_CPPFLAGS += -DGFPS_ENABLED

#spot feature
export SPOT_ENABLE := 1
ifeq ($(SPOT_ENABLE),1)
KBUILD_CPPFLAGS += -DSPOT_ENABLED
endif
endif # GFPS

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# AMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(AMA_VOICE),1)
export AMA_VOICE

KBUILD_CPPFLAGS += -D__AMA_VOICE__
ifeq ($(AMA_ENCODE_USE_SBC),1)
KBUILD_CPPFLAGS += -DAMA_ENCODE_USE_SBC
else
KBUILD_CPPFLAGS += -DAMA_ENCODE_USE_OPUS
endif
endif # AMA_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# DMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(DMA_VOICE),1)
export DMA_VOICE
KBUILD_CPPFLAGS += -D__DMA_VOICE__
KBUILD_CPPFLAGS += -D__BES__
KBUILD_CPPFLAGS += -DNVREC_BAIDU_DATA_SECTION
KBUILD_CPPFLAGS += -DBAIDU_DATA_RAND_LEN=8
KBUILD_CPPFLAGS += -DFM_MIN_FREQ=875
KBUILD_CPPFLAGS += -DFM_MAX_FREQ=1079
KBUILD_CPPFLAGS += -DBAIDU_DATA_DEF_FM_FREQ=893
KBUILD_CPPFLAGS += -DBAIDU_DATA_SN_LEN=16
export SHA256_ROM ?= 1
ifeq ($(LIBC_ROM),1)
$(error LIBC_ROM should be 0 when DMA_VOICE=1)
endif
endif # DMA_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# GMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(GMA_VOICE),1)
export GMA_VOICE
KBUILD_CPPFLAGS += -D__GMA_VOICE__

#KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
#KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=1
#KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0

KBUILD_CPPFLAGS += -D__GMA_OTA_TWS__
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# SMART_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(SMART_VOICE),1)
export SMART_VOICE
KBUILD_CPPFLAGS += -D__SMART_VOICE__
#SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 2
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
#KBUILD_CPPFLAGS += -DSPEECH_CAPTURE_TWO_CHANNEL
endif # SMART_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# TENCENT_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(TENCENT_VOICE),1)
export TENCENT_VOICE
KBUILD_CPPFLAGS += -D__TENCENT_VOICE__
endif # TENCENT_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# CUSTOMIZE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(CUSTOMIZE_VOICE),1)
export CUSTOMIZE_VOICE
KBUILD_CPPFLAGS += -D__CUSTOMIZE_VOICE__
#SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 2
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
#KBUILD_CPPFLAGS += -DSPEECH_CAPTURE_TWO_CHANNEL
endif # CUSTOMIZE_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Dual MIC recording feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(DUAL_MIC_RECORDING),1)
export DUAL_MIC_RECORDING
KBUILD_CPPFLAGS += -DDUAL_MIC_RECORDING
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT

ifeq ($(STEREO_RECORD_PROCESS),1)
export STEREO_RECORD_PROCESS
endif
endif # DUAL_MIC_RECORDING

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Bixby voice feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BIXBY_VOICE),1)
KBUILD_CPPFLAGS += -D__BIXBY_VOICE__
ifeq ($(SV_ENCODE_USE_SBC),1)
KBUILD_CPPFLAGS += -DSV_ENCODE_USE_SBC
endif
endif # BIXBY_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# AI_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifneq ($(filter 1,$(BISTO_ENABLE) $(AMA_VOICE) $(DMA_VOICE) $(SMART_VOICE) $(TENCENT_VOICE) $(GMA_VOICE) $(CUSTOMIZE_VOICE) $(DUAL_MIC_RECORDING) $(BIXBY_VOICE)),)
export AI_VOICE := 1
USE_THIRDPARTY := 1
endif

ifeq ($(AI_VOICE),1)
KBUILD_CPPFLAGS += -D__AI_VOICE__
BT_DIP_SUPPORT := 1

ifeq ($(GSOUND_HOTWORD_ENABLE)-$(GSOUND_HOTWORD_EXTERNAL)-$(ALEXA_WWE_LITE),1-1-1)
export AI_KWS_ENGINE_OVERLAY ?= 1
KBUILD_CPPFLAGS += -DAI_KWS_ENGINE_OVERLAY
endif

ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -D__AI_VOICE_BLE_ENABLE__
endif

ifeq ($(ALEXA_WWE),1)
KBUILD_CPPFLAGS += -DAPP_THREAD_STACK_SIZE=3072
endif
endif # AI_VOICE

ifeq ($(PSAP_SW_APP),1)
KBUILD_CPPFLAGS += -DPSAP_SW_APP
export USE_CMSIS_FFT_LEN_64 := 1
ifeq ($(AI_VOICE),1)
$(error PSAP_SW_APP is conflict with AI_VOICE now)
endif
ifeq ($(CP_AS_SUBSYS),1)
export AUDIO_PROMPT_USE_DAC2_ENABLED := 0
endif
endif

export AI_VOICE_MAX_TX_CREDIT ?= 3
export AI_VOICE_AI_CMD_TRANSPORT_BUFF_FIFO_SIZE ?= 1024

ifeq ($(THROUGH_PUT),1)
KBUILD_CPPFLAGS += -D__THROUGH_PUT__
AI_VOICE_MAX_TX_CREDIT := 6
AI_VOICE_AI_CMD_TRANSPORT_BUFF_FIFO_SIZE := 2048
endif # THROUGH_PUT

KBUILD_CPPFLAGS += -DMAX_TX_CREDIT=$(AI_VOICE_MAX_TX_CREDIT)
KBUILD_CPPFLAGS += -DAI_CMD_TRANSPORT_BUFF_FIFO_SIZE=$(AI_VOICE_AI_CMD_TRANSPORT_BUFF_FIFO_SIZE)

ifeq ($(USE_KNOWLES),1)
KBUILD_CPPFLAGS += -D__KNOWLES
KBUILD_CPPFLAGS += -DIDLE_ALEXA_KWD
export THIRDPARTY_LIB := knowles_uart
endif

AI_CAPTURE_CHANNEL_NUM ?= 0
ifneq ($(AI_CAPTURE_CHANNEL_NUM),0)
KBUILD_CPPFLAGS += -DAI_CAPTURE_CHANNEL_NUM=$(AI_CAPTURE_CHANNEL_NUM)
endif

AI_CAPTURE_DATA_AEC ?= 0
ifeq ($(AI_CAPTURE_DATA_AEC),1)
KBUILD_CPPFLAGS += -DAI_CAPTURE_DATA_AEC
KBUILD_CPPFLAGS += -DAI_ALGORITHM_ENABLE
KBUILD_CPPFLAGS += -DAEC_STERE_ON
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

export AI_AEC_CP_ACCEL ?= 0
ifeq ($(AI_AEC_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DAI_AEC_CP_ACCEL
endif

export BT_DIP_SUPPORT ?= 0
ifeq ($(BT_DIP_SUPPORT),1)
KBUILD_CPPFLAGS += -DBT_DIP_SUPPORT
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Print bt addr config
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export BT_ADDR_PRINT_CONFIG ?= 0
ifeq ($(BT_ADDR_PRINT_CONFIG),1)
KBUILD_CPPFLAGS += -DBT_ADDR_OUTPUT_PRINT_NUM=2
else
KBUILD_CPPFLAGS += -DBT_ADDR_OUTPUT_PRINT_NUM=6
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MULTI_AI feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
# NOTE: value of AI_VOICE and BISTO_ENABLE must already confirmed above here
ifeq ($(AI_VOICE)-$(BISTO_ENABLE),1-1)
IS_MULTI_AI_ENABLE := 1
endif
ifeq ($(IS_MULTI_AI_ENABLE),1)
export SHA256_ROM ?= 1
KBUILD_CPPFLAGS += -DIS_MULTI_AI_ENABLED
endif # MULTI_AI

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# VOICE COMPRESSION feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
DUAL_MIC_RECORDING ?= 0
RECORDING_USE_OPUS ?= 0
RECORDING_USE_OPUS_LOWER_BANDWIDTH ?= 0
RECORDING_USE_SCALABLE ?= 0
## determine the value of VOC_ENCODE_ENABLE
ifneq ($(filter 1, $(AMA_VOICE) $(DMA_VOICE) $(GMA_VOICE) $(SMART_VOICE) $(TENCENT_VOICE) $(CUSTOMIZE_VOICE) $(DUAL_MIC_RECORDING) $(BIXBY_VOICE)),)
export VOC_ENCODE_ENABLE := 1
KBUILD_CPPFLAGS += -DVOC_ENCODE_ENABLE
endif

## voice compression use ADPCM
# KBUILD_CPPFLAGS += -DVOC_ENCODE_ADPCM=1

## voice compression use OPUS
### record use OPUS
VOC_RECORD_ENCODE_USE_OPUS ?= 0
ifeq ($(filter 0, $(DUAL_MIC_RECORDING) $(RECORDING_USE_OPUS)),)
VOC_RECORD_ENCODE_USE_OPUS := 1
#### recording use opus config
KBUILD_CPPFLAGS += -DRECORDING_USE_OPUS
#### low bandwidth config
ifeq ($(RECORDING_USE_OPUS_LOWER_BANDWIDTH),1)
KBUILD_CPPFLAGS += -DRECORDING_USE_OPUS_LOW_BANDWIDTH
endif
endif
### AI use OPUS
VOC_AI_ENCODE_USE_OPUS ?= 0
ifneq ($(filter 1, $(AMA_VOICE) $(DMA_VOICE) $(GMA_VOICE) $(SMART_VOICE) $(TENCENT_VOICE) $(CUSTOMIZE_VOICE)),)
VOC_AI_ENCODE_USE_OPUS := 1
ifeq ($(AMA_ENCODE_USE_SBC),1)
VOC_AI_ENCODE_USE_OPUS := 0
endif
endif
### voice compression codec type determine
ifneq ($(filter 1, $(VOC_RECORD_ENCODE_USE_OPUS) $(VOC_AI_ENCODE_USE_OPUS)),)
export VOC_ENCODE_OPUS := 1
KBUILD_CPPFLAGS += -DVOC_ENCODE_OPUS=2
KBUILD_CPPFLAGS += -DOPUS_IN_OVERLAY
endif

## voice compression use SBC
ifneq ($(filter 1, $(AMA_ENCODE_USE_SBC) $(BISTO_ENABLE) $(SV_ENCODE_USE_SBC)),)
export VOC_ENCODE_SBC := 1
KBUILD_CPPFLAGS += -DVOC_ENCODE_SBC=3
endif

## voice compression use SCALABLE
ifeq ($(filter 0, $(DUAL_MIC_RECORDING) $(RECORDING_USE_SCALABLE)),)
export VOC_ENCODE_SCALABLE := 1
KBUILD_CPPFLAGS += -DVOC_ENCODE_SCALABLE=4
KBUILD_CPPFLAGS += -DRECORDING_USE_SCALABLE
endif


# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# VOICE_DATAPATH feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(VOICE_DATAPATH_ENABLED),1)
export VOICE_DATAPATH_ENABLED
KBUILD_CPPFLAGS += -DVOICE_DATAPATH
endif # VOICE_DATAPATH

export SLAVE_ADV_BLE_ENABLED ?= 0
ifeq ($(SLAVE_ADV_BLE_ENABLED),1)
KBUILD_CPPFLAGS += -DSLAVE_ADV_BLE
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# TILE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(TILE_DATAPATH_ENABLED),1)
export TILE_DATAPATH_ENABLED
KBUILD_CPPFLAGS += -DTILE_DATAPATH
endif

export CUSTOM_INFORMATION_TILE_ENABLE ?= 0
ifeq ($(CUSTOM_INFORMATION_TILE_ENABLE),1)
KBUILD_CPPFLAGS += -DCUSTOM_INFORMATION_TILE=1
endif # TILE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MFI feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(IOS_MFI),1)
KBUILD_CPPFLAGS += -DIOS_IAP2_PROTOCOL
endif # IOS_MFI

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# OTA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export VERSION_INFO

export OTA_ENABLE ?= 0
ifneq ($(filter 1,$(BES_OTA) $(AI_OTA) $(INTERCONNECTION) $(GMA_VOICE)),)
OTA_ENABLE := 1
endif


ifeq ($(OTA_ENABLE),1)
ifeq ($(FPGA),1)
export OTA_CODE_OFFSET ?= 0
else
ifneq ($(filter best1400 best2300 best2300p best1600 best1603,$(CHIP)),)
export OTA_CODE_OFFSET ?= 0x18000
export OTA_BOOT_INFO_OFFSET ?= 0x10000
else
export OTA_CODE_OFFSET ?= 0x20000
export OTA_BOOT_INFO_OFFSET ?= 0x18000
endif
KBUILD_CPPFLAGS += -DOTA_CODE_OFFSET=$(OTA_CODE_OFFSET)
endif
export OTA_OVER_TOTA_ENABLED ?= 0
ifeq ($(OTA_OVER_TOTA_ENABLED),1)
KBUILD_CPPFLAGS += -DOTA_OVER_TOTA_ENABLED
endif
KBUILD_CPPFLAGS += -DOTA_ENABLE
KBUILD_CPPFLAGS += -D__APP_IMAGE_FLASH_OFFSET__=$(OTA_CODE_OFFSET)
KBUILD_CPPFLAGS += -DFIRMWARE_REV

ifeq ($(OTA_BIN_COMPRESSED),1)
export FLASH_REMAP := 0
endif

ifeq ($(FLASH_REMAP),1)
# OTA_REMAP_OFFSET corresponds to the size of the code a or code b partition
# can change this value if it cannot fit the application image
export OTA_IMAGE_SIZE ?= $(FLASH_SIZE)/2-$(OTA_CODE_OFFSET)
export OTA_REMAP_OFFSET ?= $(OTA_IMAGE_SIZE)
KBUILD_CPPFLAGS += -DOTA_REMAP_OFFSET=$(OTA_REMAP_OFFSET)
#$(HALF_FLASH_SIZE)
KBUILD_CPPFLAGS += -DFLASH_REMAP
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=$(OTA_CODE_OFFSET)
else
ifeq ($(USE_MULTI_FLASH),1)
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=0
else
ifeq ($(FLASH_SIZE)-$(OTA_BIN_COMPRESSED), 0x400000-1)
export NEW_IMAGE_FLASH_OFFSET := 0x240000
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=$(NEW_IMAGE_FLASH_OFFSET)
else
ifeq ($(TOTA_CROSS_CHIP_OTA),1)
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=0x280000
else
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=$(FLASH_SIZE)/2
endif
endif
endif # USE_MULTI_FLASH
endif # REMAP
endif # OTA


ifeq ($(BES_OTA),1)
export BES_OTA
KBUILD_CPPFLAGS += -DBES_OTA

ifeq ($(IBRT),1)
export IBRT_OTA := 1
KBUILD_CPPFLAGS += -DIBRT_OTA
endif
endif

ifeq ($(AI_OTA),1)
export AI_OTA
KBUILD_CPPFLAGS += -DAI_OTA
endif

# shall not be updated by application project, as it influences the ibrt core library
ifneq ($(filter 1, $(GFPS_ENABLE) $(TILE_DATAPATH_ENABLED) $(AI_VOICE) $(BES_OTA)),)
export APP_TWS_MTU_SIZE := 672
else
export APP_TWS_MTU_SIZE := 348
endif
KBUILD_CPPFLAGS += -DAPP_TWS_CTRL_BUFFER_MAX_LEN=$(APP_TWS_MTU_SIZE)

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MIX_MIC_DURING_MUSIC feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(MIX_MIC_DURING_MUSIC_ENABLED),1)
KBUILD_CPPFLAGS += -DMIX_MIC_DURING_MUSIC
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# PROMPT_IN_FLASH feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(PROMPT_IN_FLASH),1)
export PROMPT_IN_FLASH
KBUILD_CPPFLAGS += -DPROMPT_IN_FLASH

ifeq ($(PROMPT_EMBEDED),1)
export PROMPT_EMBEDED
KBUILD_CPPFLAGS += -DPROMPT_EMBEDED
endif
endif

ifeq ($(COMBO_CUSBIN_IN_FLASH),1)
KBUILD_CPPFLAGS += -DCOMBO_CUSBIN_IN_FLASH
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Flash suspend features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(FLASH_SUSPEND), 1)
KBUILD_CPPFLAGS += -DFLASH_SUSPEND
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BLE features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BLE_ONLY_ENABLED),1)
export BLE_ONLY_ENABLED
KBUILD_CPPFLAGS += -DBLE_ONLY_ENABLED
KBUILD_CPPFLAGS += -DBLE_POWER_LEVEL_0
endif

ifeq ($(CFG_LE_PWR_CTRL),1)
KBUILD_CPPFLAGS += -DCFG_LE_PWR_CTRL
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Sensor Hub features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(SENSOR_HUB),1)
KBUILD_CPPFLAGS += -DSENSOR_HUB
endif

ifeq ($(DSP_M55),1)
KBUILD_CPPFLAGS += -DDSP_M55
endif

ifneq ($(HW_VERSION_STRING),)
export HW_VERSION_STRING
endif

KBUILD_CPPFLAGS += -DMULTIPOINT_DUAL_SLAVE

endif # FULL_APP_PROJECT

# -------------------------------------------
# Uart features
# -------------------------------------------

ifeq ($(APP_UART_MODULE),1)
export APP_UART_MODULE
KBUILD_CPPFLAGS += -DAPP_UART_MODULE
endif

ifeq ($(APP_CHIP_BRIDGE_MODULE),1)
export APP_CHIP_BRIDGE_MODULE
KBUILD_CPPFLAGS += -DAPP_CHIP_BRIDGE_MODULE
endif

# -------------------------------------------
# DSP features
# -------------------------------------------

export APP_RPC_ENABLE ?= 0
ifeq ($(APP_RPC_ENABLE),1)
KBUILD_CPPFLAGS += -DAPP_RPC_ENABLE
endif

# -------------------------------------------
# Speech features
# -------------------------------------------

ifeq ($(USB_AUDIO_SPEECH),1)
export USB_AUDIO_SPEECH
export USB_AUDIO_DYN_CFG := 0
export KEEP_SAME_LATENCY := 1
export SPEECH_LIB := 1
endif

ifeq ($(SPEECH_LIB),1)

export DSP_LIB ?= 1

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(info )
$(info CAUTION: Change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)) to SPEECH_CODEC_CAPTURE_CHANNEL_NUM($(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)))
$(info )
export USB_AUDIO_SEND_CHAN := $(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(error ERROR: Failed to change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)))
endif
endif
endif

#export HFP_DISABLE_NREC ?= 0

ifeq ($(SCO_ADD_CUSTOMER_CODEC),1)
export SCO_ADD_CUSTOMER_CODEC
KBUILD_CPPFLAGS += -DSCO_ADD_CUSTOMER_CODEC
endif

ifeq ($(SPEECH_ALGO_DSP_TEST),1)
export SPEECH_ALGO_DSP_TEST
export SPEECH_TX_24BIT := 1
endif

ifeq ($(SPEECH_TX_24BIT),1)
export SPEECH_TX_24BIT
KBUILD_CPPFLAGS += -DSPEECH_TX_24BIT
endif

ifeq ($(SPEECH_TX_DC_FILTER),1)
export SPEECH_TX_DC_FILTER
KBUILD_CPPFLAGS += -DSPEECH_TX_DC_FILTER
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_MIC_CALIBRATION),1)
export SPEECH_TX_MIC_CALIBRATION
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_CALIBRATION
endif

ifeq ($(SPEECH_TX_MIC_FIR_CALIBRATION),1)
export SPEECH_TX_MIC_FIR_CALIBRATION
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_FIR_CALIBRATION
endif

#export SPEECH_TX_AEC_CODEC_REF ?= 0

ifeq ($(SPEECH_TX_AEC),1)
export SPEECH_TX_AEC
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC
export HFP_DISABLE_NREC := 1
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SPEECH_TX_AEC2),1)
export SPEECH_TX_AEC2
$(error SPEECH_TX_AEC2 is not supported now, use SPEECH_TX_AEC2FLOAT instead)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2
export HFP_DISABLE_NREC := 1
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SPEECH_TX_AEC3),1)
export SPEECH_TX_AEC3
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC3
export HFP_DISABLE_NREC := 1
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SPEECH_TX_AEC2FLOAT),1)
export SPEECH_TX_AEC2FLOAT
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2FLOAT
export HFP_DISABLE_NREC := 1
#export DSP_LIB ?= 1
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SPEECH_TX_NS),1)
export SPEECH_TX_NS
KBUILD_CPPFLAGS += -DSPEECH_TX_NS
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_NN_NS),1)
export SPEECH_TX_NN_NS
KBUILD_CPPFLAGS += -DSPEECH_TX_NN_NS
export NN_LIB ?= 1
endif

ifeq ($(SPEECH_TX_NN_NS2),1)
export SPEECH_TX_NN_NS2
KBUILD_CPPFLAGS += -DSPEECH_TX_NN_NS2
export NN_LIB ?= 1
endif

ifeq ($(SPEECH_TX_NS2),1)
export SPEECH_TX_NS2
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2
export HFP_DISABLE_NREC := 1
KBUILD_CPPFLAGS += -DLC_MMSE_FRAME_LENGTH=$(LC_MMSE_FRAME_LENGTH)
endif

ifeq ($(SPEECH_TX_NS2FLOAT),1)
export SPEECH_TX_NS2FLOAT
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2FLOAT
export HFP_DISABLE_NREC := 1
#export DSP_LIB ?= 1
endif

ifeq ($(SPEECH_TX_NS3),1)
export SPEECH_TX_NS3
KBUILD_CPPFLAGS += -DSPEECH_TX_NS3
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_NS4),1)
export SPEECH_TX_NS4
KBUILD_CPPFLAGS += -DSPEECH_TX_NS4
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_NS5),1)
export SPEECH_TX_NS5
KBUILD_CPPFLAGS += -DSPEECH_TX_NS5
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 16000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
endif

ifeq ($(SPEECH_TX_NS7),1)
export SPEECH_TX_NS7
KBUILD_CPPFLAGS += -DSPEECH_TX_NS7
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 16000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
export NN_LIB ?= 1
endif

ifeq ($(SPEECH_TX_NS9),1)
export SPEECH_TX_NS9
KBUILD_CPPFLAGS += -DSPEECH_TX_NS9
export SPEECH_CODEC_FIXED_SAMPLE_RATE := 16000
export SPEECH_VQE_FIXED_SAMPLE_RATE := 16000
export NN_LIB ?= 1
export DSP_LIB ?= 1
export BECO ?= 1
endif


ifeq ($(SPEECH_TX_WNR),1)
export SPEECH_TX_WNR
KBUILD_CPPFLAGS += -DSPEECH_TX_WNR
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_CS_VAD),1)
export SPEECH_CS_VAD
KBUILD_CPPFLAGS += -DSPEECH_CS_VAD
export HFP_DISABLE_NREC := 1
endif

export SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 1

#export SPEECH_TX_2MIC_SWAP_CHANNELS ?= 0

ifeq ($(SPEECH_TX_2MIC_NS),1)
export SPEECH_TX_2MIC_NS
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_2MIC_NS2),1)
export SPEECH_TX_2MIC_NS2
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS2
export HFP_DISABLE_NREC := 1
KBUILD_CPPFLAGS += -DCOH_FRAME_LENGTH=$(COH_FRAME_LENGTH)
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_2MIC_NS3),1)
export SPEECH_TX_2MIC_NS3
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS3
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_2MIC_NS4),1)
export SPEECH_TX_2MIC_NS4
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS4
export HFP_DISABLE_NREC := 1
export NN_LIB ?= 1
ifeq ($(SPEECH_BONE_SENSOR),1)
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 1
else
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif
endif

ifeq ($(SPEECH_TX_2MIC_NS5),1)
export SPEECH_TX_2MIC_NS5
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS5
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_2MIC_NS6),1)
export SPEECH_TX_2MIC_NS6
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS6
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_2MIC_NS7),1)
export SPEECH_TX_2MIC_NS7
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS7
export HFP_DISABLE_NREC := 1
KBUILD_CPPFLAGS += -DCOH_FRAME_LENGTH=$(COH_FRAME_LENGTH)
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
export NN_LIB = 1
endif

ifeq ($(SPEECH_TX_2MIC_NS8),1)
export SPEECH_TX_2MIC_NS8
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS8
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

ifeq ($(SPEECH_TX_3MIC_AI_NS),1)
export SPEECH_TX_3MIC_AI_NS
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_AI_NS
export SPEECH_TX_3MIC_NS := 1
export NN_LIB ?= 1
endif

ifeq ($(SPEECH_TX_3MIC_NS),1)
export SPEECH_TX_3MIC_NS
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS
export HFP_DISABLE_NREC := 1
ifeq ($(SPEECH_BONE_SENSOR),1)
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
else
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
endif
endif

ifeq ($(SPEECH_TX_3MIC_NS2),1)
export SPEECH_TX_3MIC_NS2
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS2
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
endif

ifeq ($(SPEECH_TX_3MIC_NS3),1)
export SPEECH_TX_3MIC_NS3
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS3
export HFP_DISABLE_NREC := 1
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
endif

ifeq ($(SPEECH_TX_THIRDPARTY_ALANGO),1)
export SPEECH_TX_THIRDPARTY_ALANGO
export SPEECH_TX_THIRDPARTY := 1
core-y += thirdparty/alango_lib/
endif

ifeq ($(SPEECH_TX_THIRDPARTY),1)
export SPEECH_TX_THIRDPARTY
KBUILD_CPPFLAGS += -DSPEECH_TX_THIRDPARTY
export HFP_DISABLE_NREC := 1
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SPEECH_TX_NOISE_GATE),1)
export SPEECH_TX_NOISE_GATE
KBUILD_CPPFLAGS += -DSPEECH_TX_NOISE_GATE
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_COMPEXP),1)
export SPEECH_TX_COMPEXP
KBUILD_CPPFLAGS += -DSPEECH_TX_COMPEXP
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_AGC),1)
export SPEECH_TX_AGC
KBUILD_CPPFLAGS += -DSPEECH_TX_AGC
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_TX_EQ),1)
export SPEECH_TX_EQ
KBUILD_CPPFLAGS += -DSPEECH_TX_EQ
export HFP_DISABLE_NREC := 1
#export DSP_LIB ?= 1
endif

ifeq ($(SPEECH_TX_POST_GAIN),1)
export SPEECH_TX_POST_GAIN
KBUILD_CPPFLAGS += -DSPEECH_TX_POST_GAIN
endif

ifeq ($(BECO),1)
KBUILD_CPPFLAGS += -DBECO
endif

ifneq ($(SCO_DMA_SNAPSHOT),1)
export SPEECH_TX_AEC_CODEC_REF := 0
endif

# disable codec ref when 2300a enable anc
ifeq ($(CHIP),best2300a)
ifeq ($(ANC_APP),1)
export SPEECH_TX_AEC_CODEC_REF := 0
endif
endif

# disable codec ref when enable sidetone
ifeq ($(SPEECH_SIDETONE), 1)
export SPEECH_TX_AEC_CODEC_REF := 0
endif

ifeq ($(SPEECH_TX_AEC_CODEC_REF),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC_CODEC_REF
endif

ifeq ($(SPEECH_RX_NS),1)
export SPEECH_RX_NS
KBUILD_CPPFLAGS += -DSPEECH_RX_NS
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_NS2),1)
export SPEECH_RX_NS2
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_NS2FLOAT),1)
export SPEECH_RX_NS2FLOAT
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2FLOAT
export HFP_DISABLE_NREC := 1
#export DSP_LIB ?= 1
endif

ifeq ($(SPEECH_RX_NS3),1)
export SPEECH_RX_NS3
KBUILD_CPPFLAGS += -DSPEECH_RX_NS3
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_AGC),1)
export SPEECH_RX_AGC
KBUILD_CPPFLAGS += -DSPEECH_RX_AGC
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_COMPEXP),1)
export SPEECH_RX_COMPEXP
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_EQ),1)
export SPEECH_RX_EQ
KBUILD_CPPFLAGS += -DSPEECH_RX_EQ
export HFP_DISABLE_NREC := 1
#export DSP_LIB ?= 1
endif

ifeq ($(SPEECH_RX_HW_EQ),1)
export SPEECH_RX_HW_EQ
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_DAC_EQ),1)
export SPEECH_RX_DAC_EQ
export HFP_DISABLE_NREC := 1
endif

ifeq ($(SPEECH_RX_POST_GAIN),1)
export SPEECH_RX_POST_GAIN
KBUILD_CPPFLAGS += -DSPEECH_RX_POST_GAIN
endif

ifeq ($(SPEECH_RX_EQ),1)
# enable 24bit to fix low level signal distortion
export SPEECH_RX_24BIT := 1
endif

export SPEECH_PROCESS_FRAME_MS 	?= 16
ifeq ($(SPEECH_CODEC_CAPTURE_CHANNEL_NUM),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_NN_NS),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_NN_NS2),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_NS5),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_2MIC_NS2),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_2MIC_NS4),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_2MIC_NS5),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_2MIC_NS7),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_NS7),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_NS9),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_3MIC_NS),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
ifeq ($(SPEECH_TX_3MIC_AI_NS),1)
export SPEECH_PROCESS_FRAME_MS := 16
endif
ifeq ($(SPEECH_TX_THIRDPARTY),1)
export SPEECH_PROCESS_FRAME_MS := 15
endif
KBUILD_CPPFLAGS += -DSPEECH_PROCESS_FRAME_MS=$(SPEECH_PROCESS_FRAME_MS)

export SPEECH_SCO_FRAME_MS 		?= 15
KBUILD_CPPFLAGS += -DSPEECH_SCO_FRAME_MS=$(SPEECH_SCO_FRAME_MS)

ifeq ($(SPEECH_SIDETONE),1)
export SPEECH_SIDETONE
KBUILD_CPPFLAGS += -DSPEECH_SIDETONE
ifeq ($(HW_SIDETONE_IIR_PROCESS),1)
ifeq ($(ANC_APP),1)
$(error ANC_APP conflicts with HW_SIDETONE_IIR_PROCESS)
endif
export HW_SIDETONE_IIR_PROCESS
KBUILD_CPPFLAGS += -DHW_SIDETONE_IIR_PROCESS
endif
ifeq ($(CHIP),best2000)
# Disable SCO resample
export SW_SCO_RESAMPLE := 0
export NO_SCO_RESAMPLE := 1
endif
endif

ifeq ($(THIRDPARTY_LIB),aispeech)
#export DSP_LIB ?= 1
endif

ifeq ($(THIRDPARTY_LIB),kws/bes)
export AQE_KWS := 1
endif

ifeq ($(AQE_KWS),1)
export AQE_KWS
export NN_LIB := 1
KBUILD_CPPFLAGS += -DAQE_KWS
KBUILD_CPPFLAGS += -DAQE_KWS_$(AQE_KWS_NAME)
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(AUDIO_HEARING_COMPSATN),1)
export AUDIO_HEARING_COMPSATN
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
KBUILD_CPPFLAGS += -DAUDIO_HEARING_COMPSATN
KBUILD_CPPFLAGS += -DHEARING_MOD_VAL=$(HEARING_MOD_VAL)
endif

ifeq ($(HEARING_USE_STATIC_RAM),1)
KBUILD_CPPFLAGS += -DHEARING_USE_STATIC_RAM
endif

ifeq ($(AUDIO_CUSTOM_EQ),1)
export AUDIO_CUSTOM_EQ
export USE_CMSIS_FFT_LEN_1024 := 1
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(VOICE_ASSIST_ONESHOT_ADAPTIVE_ANC),1)
USE_CMSIS_FFT_LEN_1024 := 1
endif

ifeq ($(AUDIO_ANC_FIR_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_FIR_HW
VOICE_ASSIST_FF_FIR_LMS := 1
endif

ifeq ($(USE_CMSIS_FFT_LEN_64),1)
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_F32_32
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_RFFT_F32_64
KBUILD_CPPFLAGS += -DARM_TABLE_BITREVIDX_FLT_32
KBUILD_CPPFLAGS += -DARM_TABLE_BITREVIDX_FXT_32
endif

ifeq ($(USE_CMSIS_FFT_LEN_1024),1)
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_F32_512
KBUILD_CPPFLAGS += -DARM_TABLE_TWIDDLECOEF_RFFT_F32_1024
KBUILD_CPPFLAGS += -DARM_TABLE_BITREVIDX_FLT_512
endif

KBUILD_CPPFLAGS += -DSPEECH_CODEC_CAPTURE_CHANNEL_NUM=$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)

endif # SPEECH_LIB

# -------------------------------------------
# Common features
# -------------------------------------------

export DEBUG_PORT ?= 1

export BESLIB_INFO := $(GIT_REVISION)

ifneq ($(AUD_SECTION_STRUCT_VERSION),)
KBUILD_CPPFLAGS += -DAUD_SECTION_STRUCT_VERSION=$(AUD_SECTION_STRUCT_VERSION)
endif

ifeq ($(FLASH_CHIP),)
ifeq ($(PROGRAMMER),1)
FLASH_CHIP := ALL
else
FLASH_CHIP := GD25Q32C
endif
endif
VALID_FLASH_CHIP_LIST := ALL SIMU \
	GD25LE255E GD25LE128E GD25LF128E GD25LQ64C GD25LF64E GD25LQ32C GD25LQ16C GD25LF16E GD25LQ80C \
	GD25Q32C GD25Q80C GD25Q40C GD25D20C \
	GD25UF64E GD25UF128E \
	P25Q256L P25Q128L PY25Q128LAA P25Q64L P25Q64SLE P25Q64SLD P25Q64SN \
	P25Q32L P25Q32SL P25Q32SN P25Q16L P25Q16SL \
	P25Q80H P25Q40H P25Q21H \
	ZB25VQ128B ZB25LQ64A \
	XM25QU128C XM25QH16C XM25QH80B \
	XT25Q08B \
	EN25S80B \
	W25Q128JW W25Q32FW \
	SK25LE032 P25Q128H
export FLASH_CHIP_LIST := $(subst $(comma),$(space),$(FLASH_CHIP))
ifeq ($(SIMU),1)
ifeq ($(filter SIMU,$(FLASH_CHIP_LIST)),)
FLASH_CHIP_LIST += SIMU
endif
endif
BAD_FLASH_CHIP := $(filter-out $(VALID_FLASH_CHIP_LIST),$(FLASH_CHIP_LIST))
ifneq ($(BAD_FLASH_CHIP),)
$(error Invalid FLASH_CHIP: $(BAD_FLASH_CHIP))
endif

ifneq ($(NANDFLASH_CHIP),)
export NANDFLASH_CHIP_LIST := $(subst $(comma),$(space),$(NANDFLASH_CHIP))
endif

ifeq ($(WATCHER_DOG),1)
KBUILD_CPPFLAGS += -D__WATCHER_DOG_RESET__
endif

ifeq ($(OTA_BIN_COMPRESSED),1)
KBUILD_CPPFLAGS += -DOTA_BIN_COMPRESSED
endif

export FAULT_DUMP ?= 1

export CODEC_POWER_DOWN ?= 1

export AUDIO_OUTPUT_VOLUME_DEFAULT ?= 10
KBUILD_CPPFLAGS += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

NV_REC_DEV_VER ?= 2

ifeq ($(NO_PWRKEY),1)
export NO_PWRKEY
endif

ifeq ($(NO_GROUPKEY),1)
export NO_GROUPKEY
endif

ifeq ($(NO_SLEEP),1)
export NO_SLEEP
endif

ifeq ($(CRASH_BOOT),1)
export CRASH_BOOT
endif

ifeq ($(USB_HIGH_SPEED),1)
export USB_HIGH_SPEED
endif

ifeq ($(AUDIO_CODEC_ASYNC_CLOSE),1)
export AUDIO_CODEC_ASYNC_CLOSE
endif

ifeq ($(CODEC_PLAY_BEFORE_CAPTURE),1)
# Enable the workaround for BEST1000 version C & earlier chips
export CODEC_PLAY_BEFORE_CAPTURE
endif

ifeq ($(AUDIO_INPUT_CAPLESSMODE),1)
export AUDIO_INPUT_CAPLESSMODE
endif

ifeq ($(AUDIO_INPUT_LARGEGAIN),1)
export AUDIO_INPUT_LARGEGAIN
endif

ifeq ($(AUDIO_INPUT_MONO),1)
export AUDIO_INPUT_MONO
endif

ifeq ($(AUDIO_OUTPUT_MONO),1)
export AUDIO_OUTPUT_MONO
endif

ifeq ($(AUDIO_OUTPUT_INVERT_RIGHT_CHANNEL),1)
export AUDIO_OUTPUT_INVERT_RIGHT_CHANNEL
endif

ifeq ($(AUDIO_OUTPUT_CALIB_GAIN_MISSMATCH),1)
export AUDIO_OUTPUT_CALIB_GAIN_MISSMATCH
endif

ifeq ($(CODEC_DAC_MULTI_VOLUME_TABLE),1)
export CODEC_DAC_MULTI_VOLUME_TABLE
endif

ifeq ($(AUDIO_OUTPUT_DAC2_SW_GAIN),1)
export AUDIO_OUTPUT_DAC2_SW_GAIN
endif

ifeq ($(AUDIO_OUTPUT_DAC3_SW_GAIN),1)
export AUDIO_OUTPUT_DAC3_SW_GAIN
endif

ifeq ($(AUDIO_OUTPUT_DAC2),1)
export AUDIO_OUTPUT_DAC2
endif

ifeq ($(AUDIO_OUTPUT_DAC3),1)
export AUDIO_OUTPUT_DAC3
endif

ifeq ($(DAC_CLASSG_ENABLE),1)
export DAC_CLASSG_ENABLE
endif

ifeq ($(HW_FIR_DSD_PROCESS),1)
export HW_FIR_DSD_PROCESS
endif

ifeq ($(HW_FIR_EQ_PROCESS),1)
export HW_FIR_EQ_PROCESS
endif

ifeq ($(HW_IIR_EQ_PROCESS),1)
export HW_IIR_EQ_PROCESS
endif

ifeq ($(HW_DAC_IIR_EQ_PROCESS),1)
export HW_DAC_IIR_EQ_PROCESS
endif

ifeq ($(A2DP_VIRTUAL_SURROUND),1)
export BECO := 0
export A2DP_VIRTUAL_SURROUND
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
KBUILD_CPPFLAGS += -D__VIRTUAL_SURROUND_CP__
endif

ifeq ($(A2DP_VIRTUAL_SURROUND_HWFIR),1)
export BECO := 0
export A2DP_VIRTUAL_SURROUND_HWFIR
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
KBUILD_CPPFLAGS += -D__VIRTUAL_SURROUND_HWFIR__
endif

ifeq ($(A2DP_VIRTUAL_SURROUND_STEREO),1)
export BECO := 0
export A2DP_VIRTUAL_SURROUND_STEREO
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
KBUILD_CPPFLAGS += -D__VIRTUAL_SURROUND_STEREO__
endif

ifeq ($(AUDIO_DRC),1)
export AUDIO_DRC
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
endif

ifeq ($(AUDIO_LIMITER),1)
export AUDIO_LIMITER
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
endif

ifeq ($(AUDIO_DYNAMIC_BOOST),1)
export AUDIO_DYNAMIC_BOOST
export AUDIO_OUTPUT_SW_GAIN := 1
export AUDIO_OUTPUT_SW_GAIN_BEFORE_DRC := 1
KBUILD_CPPFLAGS += -DAUDIO_DYNAMIC_BOOST
endif

ifeq ($(AUDIO_REVERB),1)
export AUDIO_REVERB
endif

ifeq ($(AUDIO_BASS_ENHANCER),1)
export AUDIO_BASS_ENHANCER
KBUILD_CPPFLAGS += -DAUDIO_BASS_ENHANCER
endif

ifeq ($(AUDIO_HW_LIMITER),1)
export AUDIO_HW_LIMITER
endif

ifeq ($(HW_FIR_EQ_PROCESS_2CH),1)
export HW_FIR_EQ_PROCESS_2CH
KBUILD_CPPFLAGS += -D__HW_FIR_EQ_PROCESS_2CH__
endif

ifeq ($(NO_ISPI),1)
KBUILD_CPPFLAGS += -DNO_ISPI
endif

ifeq ($(USER_SECURE_BOOT),1)
core-y += utils/user_secure_boot/
KBUILD_CPPFLAGS += -DUSER_SECURE_BOOT
endif

ifeq ($(ASSERT_SHOW_FILE_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE_FUNC
else
ifeq ($(ASSERT_SHOW_FILE),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE
else
ifeq ($(ASSERT_SHOW_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FUNC
endif
endif
endif

ifeq ($(LARGE_SENS_RAM),1)
KBUILD_CPPFLAGS += -DLARGE_SENS_RAM
endif

ifeq ($(TWS),1)
LARGE_RAM ?= 1
endif
ifeq ($(LARGE_RAM),1)
ifeq ($(DSP_ENABLE), 1)
$(error LARGE_RAM conflicts with DSP_ENABLE)
endif
KBUILD_CPPFLAGS += -DLARGE_RAM
endif

ifeq ($(LARGE_DSP_RAM),1)
KBUILD_CPPFLAGS += -DLARGE_DSP_RAM
endif

ifeq ($(SMALL_RET_RAM),1)
KBUILD_CPPFLAGS += -DSMALL_RET_RAM
endif

ifeq ($(SIMPLE_TASK_SWITCH),1)
KBUILD_CPPFLAGS += -DSIMPLE_TASK_SWITCH
endif

ifeq ($(ALT_BOOT_FLASH),1)
KBUILD_CPPFLAGS += -DALT_BOOT_FLASH
endif

ifneq ($(FLASH_DUAL_CHIP),)
KBUILD_CPPFLAGS += -DFLASH_DUAL_CHIP=$(FLASH_DUAL_CHIP)
endif

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(BTUSB_AUDIO_MODE),1)
export NO_OVERLAY ?= 1
endif
endif

ifeq ($(NO_OVERLAY),1)
KBUILD_CPPFLAGS += -DNO_OVERLAY
endif

ifeq ($(NO_SPEECH_ALGO_OVERLAY),1)
KBUILD_CPPFLAGS += -DNO_SPEECH_ALGO_OVERLAY
endif

ifeq ($(CALIB_SLOW_TIMER),1)
KBUILD_CPPFLAGS += -DCALIB_SLOW_TIMER
endif

ifneq ($(CHIP_SUBSYS),dsp)
export INT_LOCK_EXCEPTION ?= 1
endif

ifeq ($(INT_LOCK_EXCEPTION),1)
KBUILD_CPPFLAGS += -DINT_LOCK_EXCEPTION
endif

ifeq ($(USE_TRACE_ID),1)
export USE_TRACE_ID
export TRACE_STR_SECTION ?= 1
KBUILD_CPPFLAGS += -DUSE_TRACE_ID
endif

ifeq ($(TRACE_STR_SECTION),1)
KBUILD_CPPFLAGS += -DTRACE_STR_SECTION
LDS_CPPFLAGS += -DTRACE_STR_SECTION
endif

export TRACE_MAXIMUM_LOG_LEN ?= 120
KBUILD_CPPFLAGS += -DTRACE_MAXIMUM_LOG_LEN=$(TRACE_MAXIMUM_LOG_LEN)

ifeq ($(ANC_APP),1)
export ANC_FF_ENABLED ?= 1
ifeq ($(ANC_FB_CHECK),1)
KBUILD_CPPFLAGS += -DANC_FB_CHECK
endif
ifeq ($(ANC_FF_CHECK),1)
KBUILD_CPPFLAGS += -DANC_FF_CHECK
endif
ifeq ($(ANC_TT_CHECK),1)
KBUILD_CPPFLAGS += -DANC_TT_CHECK
endif
endif

ifeq ($(PSAP_APP),1)
KBUILD_CPPFLAGS += -DPSAP_APP
# KBUILD_CPPFLAGS += -DPSAP_FORCE_STREAM_48K
endif

ifeq ($(AUDIO_PSAP_DEHOWLING_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_PSAP_DEHOWLING_HW
endif

ifeq ($(AUDIO_ANC_SPKCALIB_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_SPKCALIB_HW
endif

ifeq ($(AUDIO_PSAP_EQ_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_PSAP_EQ_HW
endif

ifeq ($(AUD_PLL_DOUBLE),1)
KBUILD_CPPFLAGS += -DAUD_PLL_DOUBLE
endif

ifeq ($(CHIP),best1000)
ifeq ($(DUAL_AUX_MIC),1)
KBUILD_CPPFLAGS += -D_DUAL_AUX_MIC_
ifeq ($(AUDIO_INPUT_MONO),1)
$(error Invalid talk mic configuration)
endif
endif
endif # best1000

ifeq ($(CAPTURE_ANC_DATA),1)
KBUILD_CPPFLAGS += -DCAPTURE_ANC_DATA
endif

ifeq ($(AUDIO_ANC_TT_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_TT_HW
endif

ifeq ($(AUDIO_ANC_FB_MC_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_MC_HW
endif

ifeq ($(AUDIO_ANC_FB_MC),1)
ifeq ($(AUDIO_RESAMPLE),1)
$(error AUDIO_ANC_FB_MC conflicts with AUDIO_RESAMPLE)
endif
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_MC
ifeq ($(ANC_FB_MC_96KHZ),1)
KBUILD_CPPFLAGS += -DANC_FB_MC_96KHZ
endif
endif

ifeq ($(ANC_NOISE_TRACKER),1)
export ANC_NOISE_TRACKER
ifeq ($(IBRT),1)
KBUILD_CPPFLAGS += -DANC_NOISE_TRACKER_CHANNEL_NUM=1
else
KBUILD_CPPFLAGS += -DANC_NOISE_TRACKER_CHANNEL_NUM=2
endif
endif

ifeq ($(PC_CMD_UART),1)
export PC_CMD_UART
KBUILD_CPPFLAGS += -D__PC_CMD_UART__
KBUILD_CPPFLAGS += -DAUDIO_EQ_TUNING
endif

ifeq ($(TOTA_EQ_TUNING),1)
export TOTA_EQ_TUNING
endif

ifeq ($(VOICE_DETECTOR_EN),1)
KBUILD_CPPFLAGS += -DVOICE_DETECTOR_EN
endif

ifeq ($(VOICE_DETECTOR_SENS_EN),1)
KBUILD_CPPFLAGS += -DVOICE_DETECTOR_SENS_EN
endif

ifneq ($(CODEC_VAD_CFG_BUF_SIZE),)
KBUILD_CPPFLAGS += -DCODEC_VAD_CFG_BUF_SIZE=$(CODEC_VAD_CFG_BUF_SIZE)
endif

ifeq ($(VAD_USE_SAR_ADC),1)
KBUILD_CPPFLAGS += -DVAD_USE_SAR_ADC
endif

ifeq ($(VAD_USE_8K_SAMPLE_RATE),1)
KBUILD_CPPFLAGS += -DVAD_USE_8K_SAMPLE_RATE
endif

ifeq ($(USB_ANC_MC_EQ_TUNING),1)
KBUILD_CPPFLAGS += -DUSB_ANC_MC_EQ_TUNING -DANC_PROD_TEST
endif

ifeq ($(MAX_DAC_OUTPUT),-60db)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_M60DB
else
ifeq ($(MAX_DAC_OUTPUT),3.75mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_3P75MW
else
ifeq ($(MAX_DAC_OUTPUT),5mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_5MW
else
ifeq ($(MAX_DAC_OUTPUT),10mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_10MW
else
ifneq ($(MAX_DAC_OUTPUT),30mw)
ifneq ($(MAX_DAC_OUTPUT),)
$(error Invalid MAX_DAC_OUTPUT value: $(MAX_DAC_OUTPUT) (MUST be one of: -60db 3.75mw 5mw 10mw 30mw))
endif
endif
endif
endif
endif
endif
export MAX_DAC_OUTPUT_FLAGS

export HAS_BT_SYNC ?= 0
ifeq ($(HAS_BT_SYNC),1)
KBUILD_CPPFLAGS += -D__BT_SYNC__
endif

ifeq ($(SINGLE_WIRE_UART_PMU),1)
KBUILD_CPPFLAGS += -DSINGLE_WIRE_UART_PMU
endif

ifeq ($(SINGLE_WIRE_UART_PMU_1802),1)
KBUILD_CPPFLAGS += -DSINGLE_WIRE_UART_PMU_1802
endif

ifeq ($(SINGLE_WIRE_UART_PMU_1803),1)
KBUILD_CPPFLAGS += -DSINGLE_WIRE_UART_PMU_1803
endif

ifeq ($(INTERSYS_RX_DMA_SPEEDUP),1)
LDS_CPPFLAGS += -DINTERSYS_RX_DMA_SPEEDUP
endif

ifeq ($(REPORT_EVENT_TO_CUSTOMIZED_UX),1)
KBUILD_CPPFLAGS += -D__REPORT_EVENT_TO_CUSTOMIZED_UX__
endif

export LIST_NUM ?= 6

ifeq ($(AOB_MOBILE_ENABLED),1)
LIST_NUM := $(LIST_NUM)+1
endif

ifeq ($(BLE_WALKIE_TALKIE),1)
LIST_NUM := $(LIST_NUM)+2
endif

KBUILD_CPPFLAGS += -D__LIST_NUM__=$(LIST_NUM)

#capsensor
ifeq ($(CAPSENSOR_ENABLE),1)
KBUILD_CPPFLAGS += -DCAPSENSOR_ENABLE
#CHIP_CAPSENSOR_VER:0 =>1501,1501p,1502x
#CHIP_CAPSENSOR_VER:1 =>1805
#CHIP_CAPSENSOR_VER:2 =>1809
#CHIP_CAPSENSOR_VER:3 =>1306
CHIP_CAPSENSOR_VER ?= 0
ifneq ($(CHIP_CAPSENSOR_VER),)
KBUILD_CPPFLAGS += -DCHIP_CAPSENSOR_VER=$(CHIP_CAPSENSOR_VER)
endif
ifeq ($(CAPSENSOR_SLIDE),1)
KBUILD_CFLAGS += -DCAPSENSOR_SLIDE
endif
ifeq ($(CAPSENSOR_SPP_SERVER), 1)
KBUILD_CPPFLAGS += -DCAPSENSOR_SPP_SERVER
endif
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Put customized features above
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Obsoleted features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OBSOLETED_FEATURE_LIST := EQ_PROCESS RB_CODEC AUDIO_EQ_PROCESS MEDIA_PLAYER_RBCODEC
USED_OBSOLETED_FEATURE := $(strip $(foreach f,$(OBSOLETED_FEATURE_LIST),$(if $(filter 1,$($f)),$f)))
ifneq ($(USED_OBSOLETED_FEATURE),)
$(error Obsoleted features: $(USED_OBSOLETED_FEATURE))
endif

# -------------------------------------------
# Size and LDS macros
# -------------------------------------------

export CUSTOMER_LOAD_RAM_DATA_SECTION_SIZE ?= 0
export CUSTOMER_LOAD_SRAM_TEXT_RAMX_SECTION_SIZE ?= 0

ifneq ($(CUSTOMER_LOAD_RAM_DATA_SECTION_SIZE),0)
KBUILD_CPPFLAGS += -DCUSTOMER_LOAD_RAM_DATA_SECTION_SIZE=$(CUSTOMER_LOAD_RAM_DATA_SECTION_SIZE)
endif
ifneq ($(CUSTOMER_LOAD_SRAM_TEXT_RAMX_SECTION_SIZE),0)
KBUILD_CPPFLAGS += -DCUSTOMER_LOAD_SRAM_TEXT_RAMX_SECTION_SIZE=$(CUSTOMER_LOAD_SRAM_TEXT_RAMX_SECTION_SIZE)
endif

ifneq (($(CUSTOMER_LOAD_SRAM_TEXT_RAMX_SECTION_SIZE)-$(CUSTOMER_LOAD_RAM_DATA_SECTION_SIZE)),0-0)
export CUSTOMER_LOAD_ENC_DEC_RECORD_SECTION_SIZE ?= 0x32000
endif

ifneq ($(ROM_SIZE),)
KBUILD_CPPFLAGS += -DROM_SIZE=$(ROM_SIZE)
endif

ifneq ($(RAM_SIZE),)
KBUILD_CPPFLAGS += -DRAM_SIZE=$(RAM_SIZE)
endif

ifeq ($(CHIP_HAS_CP),1)
ifneq ($(RAMCP_SIZE),)
KBUILD_CPPFLAGS += -DRAMCP_SIZE=$(RAMCP_SIZE)
endif
ifneq ($(RAMCPX_SIZE),)
KBUILD_CPPFLAGS += -DRAMCPX_SIZE=$(RAMCPX_SIZE)
endif

ifneq ($(SENS_RAMCP_TOP),)
KBUILD_CPPFLAGS += -DSENS_RAMCP_TOP=$(SENS_RAMCP_TOP)
endif
endif

export FLASH_SIZE ?= 0x100000
KBUILD_CPPFLAGS += -DFLASH_SIZE=$(FLASH_SIZE)

ifneq ($(FLASH_DUAL_CHIP),)
FLASH_SECTOR_SIZE ?= 0x2000
else
FLASH_SECTOR_SIZE ?= 0x1000
endif
ifeq ($(CHIP_HAS_PSRAM),1)
export PSRAM_ENABLE ?= 0
export PSRAM_SPEED ?= 200
#166:166M, 200:200M, 240:240M
export PSRAM_SIZE ?= 0x400000
ifneq ($(PSRAM_SIZE),)
KBUILD_CPPFLAGS += -DPSRAM_SIZE=$(PSRAM_SIZE)
endif
endif
ifeq ($(CHIP_HAS_PSRAMUHS),1)
export PSRAMUHS_ENABLE ?= 0
export PSRAMUHS_SPEED ?= 1000
#400:400M, 600:600M, 800:800M, 900:900M, 1000:1000M, 1066:1066M
ifeq ($(PSRAMUHS_DUAL_8BIT), 1)
export PSRAMUHS_SIZE ?= 0x1000000
ifeq ($(PSRAMUHS_DUAL_SWITCH), 1)
$(error PSRAMUHS_DUAL_8BIT conflicts with PSRAMUHS_DUAL_SWITCH)
endif
endif
export PSRAMUHS_SIZE ?= 0x800000
ifneq ($(PSRAMUHS_SIZE),)
KBUILD_CPPFLAGS += -DPSRAMUHS_SIZE=$(PSRAMUHS_SIZE)
endif
endif

# NOTE: This size cannot be changed so that audio section address is fixed.
#       This rule can be removed once audio tool can set audio section address dynamically.

FACTORY_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)

# NOTE: This size cannot be changed so that audio section address is fixed.
#       This rule can be removed once audio tool can set audio section address dynamically.
RESERVED_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
ifeq ($(CHIP), best1501simu)
ifeq ($(ASIC_SIMU),1)
RESERVED_SECTION_SIZE := 0x32000
endif
endif

ifeq ($(AUDIO_SECTION_ENABLE),1)
KBUILD_CPPFLAGS += -DAUDIO_SECTION_ENABLE
# depend on length of (ANC + AUDIO + SPEECH) in aud_section.c
AUD_SECTION_SIZE ?= 0x8000
ifeq ($(ANC_APP),1)
$(error Can not enable AUDIO_SECTION_ENABLE and ANC_APP together)
endif
endif

ifeq ($(ANC_APP),1)
ifeq ($(CHIP),best1000)
AUD_SECTION_SIZE ?= 0x8000
else
AUD_SECTION_SIZE ?= 0x10000
endif
else
AUD_SECTION_SIZE ?= 0
endif

USERDATA_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)

CUSTOM_PARAMETER_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)

ifeq ($(ENABLE_CALCU_CPU_FREQ_LOG),1)
export ENABLE_CALCU_CPU_FREQ_LOG
endif

ifeq ($(TOTA_CRASH_DUMP_TOOL_ENABLE),1)
export TOTA := 1
CRASH_DUMP_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
LOG_DUMP_SECTION_SIZE ?= 0x28000
#export DUMP_CRASH_LOG := 1
KBUILD_CPPFLAGS += -DTOTA_CRASH_DUMP_TOOL_ENABLE
endif

ifeq ($(DUMP_CRASH_LOG),1)
CRASH_DUMP_SECTION_SIZE ?= 0x4000
KBUILD_CPPFLAGS += -DDUMP_CRASH_LOG
else
CRASH_DUMP_SECTION_SIZE ?= 0
endif

ifeq ($(A2DP_LHDC_ON),1)
LHDC_LICENSE_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
else
LHDC_LICENSE_SECTION_SIZE ?= 0
endif

ifeq ($(A2DP_LHDCV5_ON),1)
LHDC_LICENSE_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
endif

export DUMP_LOG_ENABLE ?= 0
ifeq ($(DUMP_LOG_ENABLE),1)
ifeq ($(FLASH_SIZE),0x40000) # 2M bits
LOG_DUMP_SECTION_SIZE ?= 0x4000
endif
ifeq ($(FLASH_SIZE),0x80000) # 4M bits
LOG_DUMP_SECTION_SIZE ?= 0x8000
endif
ifeq ($(FLASH_SIZE),0x100000) # 8M bits
LOG_DUMP_SECTION_SIZE ?= 0x10000
endif
ifeq ($(FLASH_SIZE),0x200000) # 16M bits
LOG_DUMP_SECTION_SIZE ?= 0x80000
endif
ifeq ($(FLASH_SIZE),0x400000) # 32M bits
LOG_DUMP_SECTION_SIZE ?= 0x200000
endif
ifeq ($(FLASH_SIZE),0x800000) # 64M bits
LOG_DUMP_SECTION_SIZE ?= 0x200000
endif
ifeq ($(FLASH_SIZE),0x1000000) # 128M bits
LOG_DUMP_SECTION_SIZE ?= 0x200000
endif
ifeq ($(FLASH_SIZE),0x2000000) # 256M bits
LOG_DUMP_SECTION_SIZE ?= 0x800000
endif
KBUILD_CPPFLAGS += -DDUMP_LOG_ENABLE
endif

LOG_DUMP_SECTION_SIZE ?= 0

ifeq ($(OTA_ENABLE),1)
OTA_UPGRADE_LOG_SIZE ?= $(FLASH_SECTOR_SIZE)
else
OTA_UPGRADE_LOG_SIZE ?= 0
endif

export RAM_DUMP_TO_FLASH ?= 0

ifeq ($(RAM_DUMP_TO_FLASH),1)
RAM_DUMP_SECTION_SIZE ?= 0x100000
BT_CONTROLLER_RAM_DUMP_SECTION_SIZE ?= 0x18000
KBUILD_CPPFLAGS += -DRAM_DUMP_TO_FLASH
else
RAM_DUMP_SECTION_SIZE ?= 0
BT_CONTROLLER_RAM_DUMP_SECTION_SIZE ?= 0
endif

ifeq ($(CORE_DUMP_TO_FLASH),1)
CORE_DUMP_SECTION_SIZE ?= 0x100000
KBUILD_CPPFLAGS += -DCORE_DUMP_TO_FLASH
else
CORE_DUMP_SECTION_SIZE ?= 0
endif

ifeq ($(GSOUND_HOTWORD_ENABLE),1)
# used to store hotword model, currently 240KB
# this value is used in link file
HOTWORD_SECTION_SIZE ?= 0x3C000
else
HOTWORD_SECTION_SIZE ?= 0x0
endif

ifeq ($(PROMPT_IN_FLASH),1)
# 300K for prompt package
PROMPT_SECTION_SIZE ?= 0x4C000
else
PROMPT_SECTION_SIZE ?= 0x0
endif

ifeq ($(COMBO_CUSBIN_IN_FLASH),1)
export COMBO_CUSBIN_IN_FLASH
COMBO_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
else
COMBO_SECTION_SIZE ?= 0
endif

export LDS_SECTION_FLAGS := \
	-DCOMBO_SECTION_SIZE=$(COMBO_SECTION_SIZE) \
	-DPROMPT_SECTION_SIZE=$(PROMPT_SECTION_SIZE) \
	-DHOTWORD_SECTION_SIZE=$(HOTWORD_SECTION_SIZE) \
	-DCORE_DUMP_SECTION_SIZE=$(CORE_DUMP_SECTION_SIZE) \
	-DOTA_UPGRADE_LOG_SIZE=$(OTA_UPGRADE_LOG_SIZE) \
	-DLOG_DUMP_SECTION_SIZE=$(LOG_DUMP_SECTION_SIZE) \
	-DCRASH_DUMP_SECTION_SIZE=$(CRASH_DUMP_SECTION_SIZE) \
	-DCUSTOM_PARAMETER_SECTION_SIZE=$(CUSTOM_PARAMETER_SECTION_SIZE) \
	-DLHDC_LICENSE_SECTION_SIZE=$(LHDC_LICENSE_SECTION_SIZE) \
	-DUSERDATA_SECTION_SIZE=$(USERDATA_SECTION_SIZE) \
	-DAUD_SECTION_SIZE=$(AUD_SECTION_SIZE) \
	-DRESERVED_SECTION_SIZE=$(RESERVED_SECTION_SIZE) \
	-DFACTORY_SECTION_SIZE=$(FACTORY_SECTION_SIZE) \
	-DRAM_DUMP_SECTION_SIZE=$(RAM_DUMP_SECTION_SIZE) \
	-DBT_CONTROLLER_RAM_DUMP_SECTION_SIZE=$(BT_CONTROLLER_RAM_DUMP_SECTION_SIZE)

LDS_CPPFLAGS += \
	-DLINKER_SCRIPT \
	-Iplatform/hal

LDS_CPPFLAGS += $(LDS_SECTION_FLAGS)

ifneq ($(FAST_XRAM_SECTION_SIZE),)
LDS_CPPFLAGS +=-DFAST_XRAM_SECTION_SIZE=$(FAST_XRAM_SECTION_SIZE)


endif

ifneq ($(CUSTOMER_LOAD_ENC_DEC_RECORD_SECTION_SIZE),0)
LDS_SECTION_FLAGS += -DCUSTOMER_LOAD_ENC_DEC_RECORD_SECTION_SIZE=$(CUSTOMER_LOAD_ENC_DEC_RECORD_SECTION_SIZE)
endif


ifneq ($(OTA_BOOT_OFFSET),)
export OTA_BOOT_OFFSET
LDS_SECTION_FLAGS += -DOTA_BOOT_OFFSET=$(OTA_BOOT_OFFSET)
endif

ifneq ($(OTA_BOOT_SIZE),)
export OTA_BOOT_SIZE
LDS_SECTION_FLAGS += -DOTA_BOOT_SIZE=$(OTA_BOOT_SIZE)
endif

ifneq ($(OTA_BOOT_INFO_OFFSET),)
export OTA_BOOT_INFO_OFFSET
LDS_SECTION_FLAGS += -DOTA_BOOT_INFO_OFFSET=$(OTA_BOOT_INFO_OFFSET)
OTA_BOOT_INFO_SECTION_SIZE ?= $(FLASH_SECTOR_SIZE)
LDS_SECTION_FLAGS += -DOTA_BOOT_INFO_SECTION_SIZE=$(OTA_BOOT_INFO_SECTION_SIZE)
endif


ifneq ($(OTA_CODE_OFFSET),)
export OTA_CODE_OFFSET
LDS_CPPFLAGS += -DOTA_CODE_OFFSET=$(OTA_CODE_OFFSET)
endif

ifneq ($(OTA_REMAP_OFFSET),)
export OTA_REMAP_OFFSET
LDS_CPPFLAGS += -DOTA_REMAP_OFFSET=$(OTA_REMAP_OFFSET)
ifeq ($(OTA_CODE_OFFSET),)
$(error OTA_CODE_OFFSET should be set along with OTA_REMAP_OFFSET)
endif
endif

ifeq ($(FLASH_REMAP),1)
export FLASH_REMAP
ifneq ($(PROGRAMMER),1)
ifeq ($(OTA_REMAP_OFFSET),)
$(error OTA_REMAP_OFFSET should be set along with FLASH_REMAP)
endif
endif
endif

ifneq ($(FLASH_REGION_SIZE),)
LDS_CPPFLAGS += -DFLASH_REGION_SIZE=$(FLASH_REGION_SIZE)
endif

ifneq ($(FLASH1_REGION_SIZE),)
LDS_CPPFLAGS += -DFLASH1_REGION_SIZE=$(FLASH1_REGION_SIZE)
endif

ifneq ($(FLASH_IMAGE_SIZE),)
LDS_SECTION_FLAGS += -DFLASH_IMAGE_SIZE=$(FLASH_IMAGE_SIZE)
endif

LDS_CPPFLAGS += $(LDS_SECTION_FLAGS)

LDS_CPPFLAGS += \
	-DLINKER_SCRIPT \
	-Iplatform/hal

ifeq ($(PSRAM_ENABLE),1)
KBUILD_CPPFLAGS += -DPSRAM_ENABLE
BOOT_CODE_IN_RAM ?= 1
endif

ifeq ($(PSRAMUHS_ENABLE),1)
KBUILD_CPPFLAGS += -DPSRAMUHS_ENABLE
BOOT_CODE_IN_RAM ?= 1
endif

ifeq ($(BOOT_CODE_IN_RAM),1)
LDS_CPPFLAGS += -DBOOT_CODE_IN_RAM
endif
ifeq ($(TRACE_CODE_IN_RAM),1)
LDS_CPPFLAGS += -DTRACE_CODE_IN_RAM
endif

ifeq ($(GSOUND_HOTWORD_EXTERNAL),1)
LDS_CPPFLAGS += -DGSOUND_HOTWORD_EXTERNAL
endif

ifeq ($(MEM_POOL_IN_CP_RAM),1)
LDS_CPPFLAGS += -DMEM_POOL_IN_CP_RAM
endif

ifeq ($(PROGRAMMER_LOAD_SIMU),1)
LDS_CPPFLAGS += -DPROGRAMMER_LOAD_SIMU
endif

ifeq ($(RB_CODEC),1)
LDS_CPPFLAGS += -DRB_CODEC
endif

ifneq ($(SLAVE_BIN_FLASH_OFFSET),)
export SLAVE_BIN_FLASH_OFFSET
LDS_CPPFLAGS += -DSLAVE_BIN_FLASH_OFFSET=$(SLAVE_BIN_FLASH_OFFSET)
endif

ifneq ($(DATA_BUF_START),)
LDS_CPPFLAGS += -DDATA_BUF_START=$(DATA_BUF_START)
endif

ifeq ($(DEFAULT_CODE_IN_PSRAM),1)
LDS_CPPFLAGS += -DDEFAULT_CODE_IN_PSRAM
DEFAULT_DATA_IN_PSRAM ?= 1
export DEFAULT_BSS_IN_PSRAM ?= 1
endif

ifeq ($(DEFAULT_DATA_IN_PSRAM),1)
LDS_CPPFLAGS += -DDEFAULT_DATA_IN_PSRAM
endif

ifeq ($(DEFAULT_BSS_IN_PSRAM),1)
LDS_CPPFLAGS += -DDEFAULT_BSS_IN_PSRAM
endif

ifeq ($(PSRAM_HEAP_ONLY),1)
LDS_CPPFLAGS += -DPSRAM_HEAP_ONLY
endif

ifeq ($(OTA_BOOT_TO_DUAL_CHIP),1)
LDS_CPPFLAGS += -DOTA_BOOT_TO_DUAL_CHIP
endif

# -------------------------------------------
# General
# -------------------------------------------

ifneq ($(NO_CONFIG),1)
core-y += config/
endif

ifneq ($(NO_BOOT_STRUCT),1)
core-y += $(call add_if_exists,utils/boot_struct/)
endif

export DEFAULT_CFG_SRC ?= _default_cfg_src_

ifneq ($(wildcard $(srctree)/config/$(T)/tgt_hardware.h $(srctree)/config/$(T)/res/),)
KBUILD_CPPFLAGS += -Iconfig/$(T)
endif
KBUILD_CPPFLAGS += -Iconfig/$(DEFAULT_CFG_SRC)

KBUILD_CPPFLAGS += -Iinclude

CPU_EXT :=
ifeq ($(CPU_NO_DSP),1)
CPU_EXT := $(CPU_EXT)+nodsp
endif
ifneq ($(CHIP_HAS_FPU),1)
CPU_EXT := $(CPU_EXT)+nofp
endif

CPU_CFLAGS := -mthumb
ifeq ($(CPU),a7)
CPU_CFLAGS += -mcpu=cortex-a7
else ifeq ($(CPU),m55)
CPU_CFLAGS += -mcpu=cortex-m55
else ifeq ($(CPU),m33)
CPU_CFLAGS += -mcpu=cortex-m33$(CPU_EXT)
else
CPU_CFLAGS += -mcpu=cortex-m4$(CPU_EXT)
endif

ifneq ($(filter m33 m55,$(CPU)),)
ifeq ($(ARM_CMSE),1)
export ARM_CMSE
CPU_CFLAGS += -mcmse
KBUILD_CPPFLAGS += -DARM_CMSE
endif
ifeq ($(ARM_CMNS),1)
export ARM_CMNS
export ARM_CMSE_TARGET ?= arm_cmse
KBUILD_CPPFLAGS += -DARM_CMNS
ifeq ($(LIBC_ROM),1)
$(error LIBC_ROM should be 0 when ARM_CMNS=1)
endif
export BOOT_LOADER_ENTRY_HOOK := 0
endif
endif

ifeq ($(CHIP_HAS_FPU),1)
ifeq ($(CPU),a7)
CPU_CFLAGS += -mfpu=neon-vfpv4
else ifeq ($(CPU),m33)
CPU_CFLAGS += -mfpu=fpv5-sp-d16
else ifeq ($(CPU),m55)
# For GCC version >= 9
ifneq ($(TOOLCHAIN),armclang)
CPU_CFLAGS += -mfpu=auto
endif
else
CPU_CFLAGS += -mfpu=fpv4-sp-d16
endif
ifeq ($(SOFT_FLOAT_ABI),1)
CPU_CFLAGS += -mfloat-abi=softfp
else
CPU_CFLAGS += -mfloat-abi=hard
endif
else # !CHIP_HAS_FPU
CPU_CFLAGS += -mfloat-abi=soft
endif # !CHIP_HAS_FPU

ifeq ($(KERNEL),RTTHREAD)
CPU_CFLAGS += -Wa,-mimplicit-it=thumb
endif

export UNALIGNED_ACCESS ?= 1
ifeq ($(UNALIGNED_ACCESS),1)
KBUILD_CPPFLAGS += -DUNALIGNED_ACCESS
else
CPU_CFLAGS += -mno-unaligned-access
endif

ifneq ($(ALLOW_WARNING),1)
KBUILD_CPPFLAGS += -Werror
endif

ifeq ($(STACK_USAGE),1)
KBUILD_CPPFLAGS += -fstack-usage
endif

ifeq ($(PIE),1)
ifneq ($(TOOLCHAIN),armclang)
ifneq ($(NOSTD),1)
$(error PIE can only work when NOSTD=1)
endif
KBUILD_CPPFLAGS += -msingle-pic-base
endif
KBUILD_CPPFLAGS += -fPIE -fno-plt
# -pie option will generate .dynamic section
LDFLAGS += -pie
LDFLAGS += -z now
#LDFLAGS += -z relro
endif

KBUILD_CPPFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
LINK_CFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
CFLAGS_IMAGE += $(CPU_CFLAGS) $(SPECS_CFLAGS)

# Save 100+ bytes by filling less alignment holes
# TODO: Array alignment?
#LDFLAGS += --sort-common --sort-section=alignment

ifeq ($(CTYPE_PTR_DEF),1)
ifeq ($(TOOLCHAIN),armclang)
$(error CTYPE_PTR_DEF is not supported in $(TOOLCHAIN))
else
LDFLAGS_IMAGE += --defsym __ctype_ptr__=0
endif
endif

export USE_MS_AS_BLE_ADV_INTERVAL_UNIT ?= 1
ifeq ($(USE_MS_AS_BLE_ADV_INTERVAL_UNIT),1)
KBUILD_CPPFLAGS += -DUSE_MS_AS_BLE_ADV_INTERVAL_UNIT
endif

export QUICK_POWER_OFF_ENABLED ?= 0
ifeq ($(QUICK_POWER_OFF_ENABLED),1)
KBUILD_CPPFLAGS += -DQUICK_POWER_OFF_ENABLED
endif

export CODEC_ERROR_HANDLING ?= 0
ifeq ($(CODEC_ERROR_HANDLING),1)
KBUILD_CPPFLAGS += -DCODEC_ERROR_HANDLING
endif

export IS_CUSTOM_UART_APPLICATION_ENABLED ?= 0
ifeq ($(IS_CUSTOM_UART_APPLICATION_ENABLED),1)
KBUILD_CPPFLAGS += -DIS_CUSTOM_UART_APPLICATION_ENABLED
endif

export FACTORY_REVERT_INSTRUMENT_ADDR ?= 0
ifeq ($(FACTORY_REVERT_INSTRUMENT_ADDR),1)
KBUILD_CPPFLAGS += -DFACTORY_REVERT_INSTRUMENT_ADDR
endif

export TEST_UART_LOOP_BACK ?= 0
ifeq ($(TEST_UART_LOOP_BACK),1)
KBUILD_CPPFLAGS += -DTEST_UART_LOOP_BACK
endif

export DISABLE_AUTO_ACCEPT_TWS_CONN ?= 0
ifeq ($(DISABLE_AUTO_ACCEPT_TWS_CONN),1)
KBUILD_CPPFLAGS += -DDISABLE_AUTO_ACCEPT_TWS_CONN
endif

export BLE_AUDIO_TEST_ENABLED ?= 1
ifeq ($(BLE_AUDIO_TEST_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_AUDIO_TEST_ENABLED
endif

export TWS_RS_WITHOUT_MOBILE ?= 0
ifeq ($(TWS_RS_WITHOUT_MOBILE),1)
KBUILD_CPPFLAGS += -DTWS_RS_WITHOUT_MOBILE
endif

export TWS_RS_BY_BTC ?= 0
ifeq ($(TWS_RS_BY_BTC),1)
KBUILD_CPPFLAGS += -D__BES_FA_MODE__
KBUILD_CPPFLAGS += -DTWS_RS_BY_BTC
endif

export USE_SAFE_DISCONNECT ?= 0
ifeq ($(USE_SAFE_DISCONNECT),1)
KBUILD_CPPFLAGS += -DUSE_SAFE_DISCONNECT
endif

export BLE_AOB_VOLUME_SYNC_ENABLED ?= 1
ifeq ($(BLE_AOB_VOLUME_SYNC_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_AOB_VOLUME_SYNC_ENABLED
endif

export USB_BIS_AUDIO_STREAM ?= 0
ifeq ($(USB_BIS_AUDIO_STREAM),1)
KBUILD_CPPFLAGS += -DUSB_BIS_AUDIO_STREAM
endif

export USB_BLE_AUDIO_HW_TIMER_TRIGGER ?= 0
ifeq ($(USB_BLE_AUDIO_HW_TIMER_TRIGGER),1)
KBUILD_CPPFLAGS += -DUSB_BLE_AUDIO_HW_TIMER_TRIGGER
KBUILD_CPPFLAGS += -DAPP_BLE_USB_AUDIO_RESAMPLE_SYNC
KBUILD_CPPFLAGS += -DRESAMPLE_ANY_SAMPLE_RATE
endif

ifeq ($(SPA_AUDIO_SEC),1)
KBUILD_CPPFLAGS += -DSPA_AUDIO_SEC
endif

ifeq ($(SPA_AUDIO_SEC_TEST),1)
KBUILD_CPPFLAGS += -DSPA_AUDIO_SEC_TEST
endif

ifeq ($(TZ_ROM_UTILS_IF),1)
KBUILD_CPPFLAGS += -DTZ_ROM_UTILS_IF
endif

export USE_OVERLAY_TXT_GAP ?= 0
ifeq ($(USE_OVERLAY_TXT_GAP), 1)
KBUILD_CPPFLAGS += -DUSE_OVERLAY_TXT_GAP
endif

export UNIFY_HEAP_ENABLED ?= 0
ifeq ($(UNIFY_HEAP_ENABLED),1)
KBUILD_CPPFLAGS += -DUNIFY_HEAP_ENABLED
endif

ifeq ($(CUSTOM_BITRATE), 1)
KBUILD_CPPFLAGS += -DCUSTOM_BITRATE
endif

ifeq ($(PROMPT_USE_AAC),1)
KBUILD_CPPFLAGS += -DPROMPT_USE_AAC
endif

$(info ----------common.mk in flash---------------------)
$(info FLASH_REMAP: $(FLASH_REMAP))
$(info NEW_IMAGE_FLASH_OFFSET: $(NEW_IMAGE_FLASH_OFFSET))
$(info OTA_BOOT_SIZE: $(OTA_BOOT_SIZE))
$(info OTA_CODE_OFFSET: $(OTA_CODE_OFFSET))
$(info OTA_BOOT_INFO_OFFSET: $(OTA_BOOT_INFO_OFFSET))
$(info FLASH_SIZE: $(FLASH_SIZE))
$(info -------------------------------)
