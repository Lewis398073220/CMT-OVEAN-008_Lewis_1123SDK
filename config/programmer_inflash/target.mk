CHIP		?= best1600

DEBUG		?= 1

NOSTD		?= 1

LIBC_ROM	?= 1

PROGRAMMER	:= 1

FAULT_DUMP	?= 0

WATCHER_DOG ?= 1

export SINGLE_WIRE_UART_PMU_1803 ?= 1

init-y		:=
core-y		:= tests/programmer/ platform/cmsis/ platform/hal/
core-y      += tests/programmer_inflash/

ifeq ($(OTA_BIN_COMPRESSED),1)
core-y		+= utils/lzma/
core-y		+= utils/heap/

KBUILD_CPPFLAGS += -Iutils/lzma/
KBUILD_CPPFLAGS += -Iutils/heap/

export OTA_REBOOT_FLASH_REMAP := 0
endif
LDS_FILE	:= programmer_inflash.lds

export CRC32_ROM ?= 1

KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iplatform/hal

KBUILD_CPPFLAGS += -DPROGRAMMER_INFLASH

KBUILD_CFLAGS +=

LIB_LDFLAGS +=

CFLAGS_IMAGE +=

LDFLAGS_IMAGE +=

OTA_ENABLE ?= 1

ULTRA_LOW_POWER ?= 1

ifeq ($(OTA_BIN_COMPRESSED),1)
export OTA_REBOOT_FLASH_REMAP ?= 0
else
ifneq ($(filter best1501 best1502x best1306 best2001 best1600 best1603,$(CHIP)),)
export OTA_REBOOT_FLASH_REMAP ?= 1
else
export OTA_REBOOT_FLASH_REMAP ?= 0
endif
endif

export FLASH_SIZE ?= 0x400000

export USE_MULTI_FLASH ?= 0
ifeq ($(USE_MULTI_FLASH),1)
export BTH_USE_SYS_FLASH ?= 1
export BTH_USE_SYS_PERIPH ?= 1
export FLASH1_SIZE ?= 0x800000
KBUILD_CPPFLAGS += \
        -DUSE_MULTI_FLASH \
        -DFLASH1_SIZE=$(FLASH1_SIZE)
LDS_CPPFLAGS += \
        -DUSE_MULTI_FLASH \
        -DFLASH1_SIZE=$(FLASH1_SIZE)
export FLASH_SIZE ?= 0x800000
endif

ifneq ($(filter best1600 best1603,$(CHIP)),)
ifneq ($(SYS_AS_MAIN),1)
CHIP_SUBSYS ?= bth
export BTH_AS_MAIN_MCU ?= 1

ifeq ($(CHIP),best1600)
BTH_USE_SYS_FLASH ?= 1
else
BTH_USE_SYS_FLASH ?= 0
endif

else
KBUILD_CPPFLAGS += -D__SYS_AS_MAIN__
# export SYS_USE_BTH_FLASH ?= 1
# FLASH_CHIP	?= ALL
# NOAPP		?= 1
# export USE_MEM_CFG ?= 1
# ifeq ($(USE_MEM_CFG),1)
# -include $(srctree)/config/$(CHIP)/mem_cfg.mk
# endif
endif   #!SYS_AS_MAIN

# For ISPI iomux and DMA config
export PROGRAMMER_HAL_FULL_INIT ?= 1
ifeq ($(OTA_BIN_COMPRESSED),1)
KBUILD_CPPFLAGS += -DBTH_RAM_SIZE=0xE0000
endif   #OTA_BIN_COMPRESSED
endif

ifeq ($(OTA_BOOT_INFO_OFFSET),)
ifneq ($(OTA_BOOT_OFFSET),)
OTA_BOOT_INFO_OFFSET ?= $(OTA_BOOT_OFFSET)+$(OTA_BOOT_SIZE)
else
OTA_BOOT_INFO_OFFSET ?= $(OTA_BOOT_SIZE)
endif
ifeq ($(OTA_BOOT_TO_DUAL_CHIP),1)
# Avoid using (...)*2, for excape needed on Linux but not on Win
OTA_BOOT_INFO_OFFSET := $(OTA_BOOT_INFO_OFFSET)+$(OTA_BOOT_INFO_OFFSET)
endif
endif

ifneq ($(filter best1501 best1501p best1502x best1306 best2001 best1600 best1603,$(CHIP)),)
ifeq ($(CHIP),best1501)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x2c020000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else ifeq ($(CHIP),best1600)
ifeq ($(SYS_AS_MAIN),1)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x2c018000
else
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x34018000
endif

KBUILD_CPPFLAGS += -DDOWNLOAD_UART_BANDRATE=921600
else ifeq ($(CHIP),best1603)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x34018000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else ifeq ($(CHIP),best1501p)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x2c020000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else ifeq ($(CHIP),best1502x)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x2c020000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else ifeq ($(CHIP),best1306)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x34020000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x2c180000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
endif
else
ifneq ($(filter best1400 best2300 best2300p,$(CHIP)),)
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x3c018000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
else
KBUILD_CPPFLAGS += -DAPP_ENTRY_ADDRESS=0x3c020000 \
                   -DDOWNLOAD_UART_BANDRATE=921600
endif
endif

export SINGLE_WIRE_DOWNLOAD ?= 1
export UNCONDITIONAL_ENTER_SINGLE_WIRE_DOWNLOAD ?= 1

export PROGRAMMER_WATCHDOG ?= 1
export FLASH_UNIQUE_ID ?= 1
export TRACE_BAUD_RATE := 10*115200

ifeq ($(OTA_REBOOT_FLASH_REMAP),1)
KBUILD_CPPFLAGS += -DOTA_REBOOT_FLASH_REMAP
endif # REMAP

ifeq ($(USER_SECURE_BOOT),1)
export USER_SECURE_BOOT_JUMP_ENTRY_ADDR ?= $(OTA_CODE_OFFSET)
endif
