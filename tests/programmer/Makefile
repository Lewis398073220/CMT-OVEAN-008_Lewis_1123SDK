cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.cpp))

ifeq ($(OTA_BIN_COMPRESSED),1)
PROGRAMMER_LIB_NAME := libprogrammer_compressed
else
PROGRAMMER_LIB_NAME := libprogrammer
endif

$(PROGRAMMER_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y += $(PROGRAMMER_LIB_NAME).a

obj-y += ../../utils/hexdump/ ../../utils/hwtimer_list/ ../../utils/crc32/

obj-y += ../../platform/drivers/norflash/ ../../platform/drivers/ana/

ifeq ($(CHIP_HAS_USB),1)
obj-y += ../../platform/drivers/usb/usb_dev/
CFLAGS_sys_api_programmer.o += -Iplatform/drivers/usb/usb_dev/inc
endif

#ccflags-y += -I$(obj)/../../utils/hexdump -I$(obj)/../../utils/boot_struct -I$(obj)/../../utils/rom_utils
ccflags-y += -Itests/programmer/inc -Iutils/hexdump -Iutils/hwtimer_list -Iutils/boot_struct -Iutils/rom_utils -Iutils/crc32

CFLAGS_sys_api_programmer.o += -Iplatform/drivers/ana

ifneq ($(DEBUG_PORT),)
CFLAGS_sys_api_programmer.o += -DDEBUG_PORT=$(DEBUG_PORT)
endif
ifneq ($(DOWNLOAD_UART),)
CFLAGS_sys_api_programmer.o += -DDOWNLOAD_UART=$(DOWNLOAD_UART)
endif

ifeq ($(PROGRAMMER_WATCHDOG),1)
CFLAGS_sys_api_programmer.o += -DPROGRAMMER_WATCHDOG
endif

ifeq ($(FLASH_ULTRA_LOW_SPEED),1)
CFLAGS_sys_api_programmer.o += -DFLASH_ULTRA_LOW_SPEED
endif
ifeq ($(FLASH_LOW_SPEED),1)
CFLAGS_sys_api_programmer.o += -DFLASH_LOW_SPEED
endif
ifeq ($(OSC_26M_X4_AUD2BB),1)
CFLAGS_sys_api_programmer.o += -DOSC_26M_X4_AUD2BB
endif
ifeq ($(FLASH_DTR),1)
CFLAGS_sys_api_programmer.o += -DFLASH_DTR
endif

ifeq ($(FLASH_UNIQUE_ID),1)
CFLAGS_flash_programmer.o += -DFLASH_UNIQUE_ID
CFLAGS_sys_api_programmer.o += -DFLASH_UNIQUE_ID
endif

ifeq ($(FLASH_SECURITY_REGISTER),1)
CFLAGS_flash_programmer.o += -DFLASH_SECURITY_REGISTER
CFLAGS_sys_api_programmer.o += -DFLASH_SECURITY_REGISTER
endif

ifeq ($(FLASH_REMAP),1)
CFLAGS_flash_programmer.o += -DFLASH_REMAP
CFLAGS_sys_api_programmer.o += -DFLASH_REMAP
endif

ifeq ($(PROGRAMMER_LOAD_SIMU),1)
CFLAGS_sys_api_programmer.o += -DPROGRAMMER_LOAD_SIMU
endif

ifeq ($(PROGRAMMER_ERROR_ASSERT),1)
CFLAGS_sys_api_programmer.o += -DPROGRAMMER_ERROR_ASSERT
endif

ifeq ($(JTAG_ENABLE),1)
CFLAGS_sys_api_programmer.o += -DJTAG_ENABLE
endif

ifeq ($(NO_UART_IOMUX),1)
CFLAGS_sys_api_programmer.o += -DNO_UART_IOMUX
endif

ifeq ($(FORCE_ANALOG_I2C),1)
CFLAGS_sys_api_programmer.o += -DFORCE_ANALOG_I2C
endif

ifeq ($(SINGLE_WIRE_DOWNLOAD),1)
CFLAGS_flash_programmer.o += -DSINGLE_WIRE_DOWNLOAD
CFLAGS_sys_api_programmer.o += -DSINGLE_WIRE_DOWNLOAD
endif

ifeq ($(CODE_IN_EXTRA_RAM),1)
CFLAGS_sys_api_programmer.o += -DCODE_IN_EXTRA_RAM
endif
