cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

SBC_CLIENT ?= 0
SBC_SERVER ?= 0

ifeq ($(SBC_CLIENT),1)
obj-y += rpc_sbc_frontend.o
endif

ifeq ($(SBC_SERVER),1)
obj-y += rpc_sbc_backend.o
endif

subdir-ccflags-y += -Iplatform/hal \
					-Iutils/cqueue \
					-Imultimedia/inc/audio/codec/sbc/inc \
					-Iservices/bt_if_enhanced/inc \
					-Iservices/rpc/inc

