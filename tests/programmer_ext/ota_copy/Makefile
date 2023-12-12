cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifeq ($(OTA_BIN_COMPRESSED),1)
PROGRAMMER_EXT_LIB_NAME := libprogrammer_ext_compressed
else
PROGRAMMER_EXT_LIB_NAME := libprogrammer_ext
endif

$(PROGRAMMER_EXT_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)
obj-y += $(PROGRAMMER_EXT_LIB_NAME).a

ccflags-y += \
    -Iplatform/drivers/ana \
    -Itests/programmer/inc \
    -Iutils/boot_struct \
	-Iutils/crc32 \
	-Iutils/crc16 \
    -Iutils/rom_utils

ifeq ($(SINGLE_WIRE_DOWNLOAD),1)
ccflags-y += -DSINGLE_WIRE_DOWNLOAD
endif

ifeq ($(PROGRAMMER_WATCHDOG),1)
ccflags-y += -DPROGRAMMER_WATCHDOG
endif

ifeq ($(FORCE_COPY_NEW_IMAGE),1)
ccflags-y += -DFORCE_COPY_NEW_IMAGE -DFLASH_SIZE=$(FLASH_SIZE)
endif

ifneq ($(filter best1400 best2300 best2300p,$(CHIP)),)
export OTA_CODE_OFFSET ?= 0x18000
else
export OTA_CODE_OFFSET ?= 0x20000
endif
KBUILD_CPPFLAGS += -D__APP_IMAGE_FLASH_OFFSET__=$(OTA_CODE_OFFSET)

export FLASH_REMAP ?= 1
ifeq ($(OTA_BIN_COMPRESSED),1)
export FLASH_REMAP := 0
endif

ifeq ($(OTA_BOOT_DEBUG),1)
ccflags-y += -DOTA_BOOT_DEBUG
endif

$(info ----------APP in flash---------------------)
$(info FLASH_REMAP: $(FLASH_REMAP))
$(info NEW_IMAGE_FLASH_OFFSET: $(NEW_IMAGE_FLASH_OFFSET))
$(info OTA_CODE_OFFSET: $(OTA_CODE_OFFSET))
$(info FLASH_SIZE: $(FLASH_SIZE))
$(info -------------------------------)