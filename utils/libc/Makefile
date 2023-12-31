cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(filter a%,$(CPU)),)
OPT_LIBC ?= 1
endif

ifeq ($(NO_MPU_DEFAULT_MAP),1)
override UNALIGNED_ACCESS := 0
KBUILD_CPPFLAGS := $(filter-out -DUNALIGNED_ACCESS,$(KBUILD_CPPFLAGS))
KBUILD_CPPFLAGS += -mno-unaligned-access
ifeq ($(LIBC_ROM),1)
override LIBC_ROM := 0
override LIBC_OVERRIDE := 1
endif
endif

ARM_STRCPY_SRCS := machine/arm/strcpy.c machine/arm/memchr.c
ARM_MEMCPY_SRCS := machine/arm/memcpy-arm.S machine/arm/aeabi_memset.c machine/arm/aeabi_memset-soft.S

BES_MEMCPY_SRCS := machine/arm/memcpy_move-kp.S machine/arm/memset-kp.S
ifeq ($(BES_MEMCPY_SRC_VERSION),2)
BES_MEMCPY_SRCS :=
BES_MEMCPY_SRCS := machine/arm/memcpy_move-kpv2.S machine/arm/memset-kp.S
endif

ifeq ($(NOSTD)-$(NUTTX_BUILD),1-0)

# Use relative path to specify the include path (so that no change is needed after rename)
ccflags-y := -I$(obj)/inc

ifeq ($(LIBC_ROM),1)
	obj-y := libc_rom.c libc_rom_mem.c memcmp.c strlen.c

	CFLAGS_libc_rom.o += -Iutils/rom_utils
	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
else
	obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
	EXCLUDE_SRCS := libc_rom.c libc_rom_mem.c
	obj-y := $(filter-out $(EXCLUDE_SRCS),$(obj-y))

	ifeq ($(OPT_LIBC),1)
		EXCLUDE_SRCS := memchr.c memcpy.c memmove.c memset.c strcpy.c
		obj-y := $(filter-out $(EXCLUDE_SRCS),$(obj-y))
		obj-y += $(ARM_STRCPY_SRCS)
		ifeq ($(UNALIGNED_ACCESS),1)
			obj-y += $(BES_MEMCPY_SRCS)
		else
			obj-y += $(ARM_MEMCPY_SRCS)
		endif
	endif
endif

else ifeq ($(LIBC_ROM),1)

ifeq ($(NUTTX_BUILD),1)
	obj-y := libc_rom_mem.c

	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
else
	obj-y := libc_rom.c libc_rom_mem.c

	CFLAGS_libc_rom.o += -Iutils/rom_utils
	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
	# Avoid conflicting with libc in toolchain
	CFLAGS_libc_rom.o += -DNO_STRTOL
	CFLAGS_libc_rom.o += -DNO_STRTOUL
	CFLAGS_libc_rom.o += -DNO_VSSCANF
	CFLAGS_libc_rom.o += -DNO_ASSERT
endif

else ifeq ($(LIBC_OVERRIDE),1)

	# Add more LIBC functions to override
ifeq ($(OPT_LIBC),1)
	obj-y += $(ARM_STRCPY_SRCS)

	ifeq ($(UNALIGNED_ACCESS),1)
		obj-y += $(BES_MEMCPY_SRCS)
	else
		obj-y += $(ARM_MEMCPY_SRCS)
	endif
else
	obj-y := memcpy.c strcpy.c memchr.c memset.c
endif

else

$(error Missing LIBC options)

endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

# Avoid recursive calls when compiling C library codes
ifneq ($(TOOLCHAIN),armclang)
ccflags-y += -fno-tree-loop-distribute-patterns
endif

