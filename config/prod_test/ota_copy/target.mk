export SINGLE_WIRE_DOWNLOAD ?= 1
export UNCONDITIONAL_ENTER_SINGLE_WIRE_DOWNLOAD ?= 0

include config/programmer_inflash/target.mk

core-y += tests/programmer_ext/ota_copy/
