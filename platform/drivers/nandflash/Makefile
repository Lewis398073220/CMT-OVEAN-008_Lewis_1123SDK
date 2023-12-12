cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y := -I$(obj)/inc -Itests/programmer/inc -Iutils/boot_struct

CFLAGS_nandflash_drv.o += -DCHIP_FLASH_CTRL_VER=$(CHIP_FLASH_CTRL_VER)

ifeq ($(FLASH_DMA),1)
CFLAGS_nandflash_drv.o += -DFLASH_DMA
endif
ifeq ($(FLASH_CALIB_DEBUG),1)
CFLAGS_nandflash_drv.o += -DFLASH_CALIB_DEBUG
endif
ifeq ($(FLASH_SECURITY_REGISTER),1)
CFLAGS_nandflash_drv.o += -DFLASH_SECURITY_REGISTER
endif

ifneq ($(NANDFLASH_NC_BASE),)
CFLAGS_nandflash_drv.o +=-DNANDFLASH_NC_BASE=$(NANDFLASH_NC_BASE)
endif

ifeq ($(NANDFLASH_CHIP_LIST),)
NANDFLASH_CHIP_LIST := ALL
endif
ifeq ($(NANDFLASH_CHIP_LIST),)
$(error *** Error: NANDFLASH_CHIP_LIST is NOT defined!)
endif

CFLAGS_nandflash_drv.o += $(foreach flash,$(NANDFLASH_CHIP_LIST),-D__NANDFLASH_$(flash)__)
