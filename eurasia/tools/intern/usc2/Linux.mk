# Copyright	2010 Imagination Technologies Limited. All rights reserved.
#
# No part of this software, either material or conceptual may be
# copied or distributed, transmitted, transcribed, stored in a
# retrieval system or translated into any human or computer
# language in any form by any means, electronic, mechanical,
# manual or other-wise, or disclosed to third parties without
# the express written permission of: Imagination Technologies
# Limited, HomePark Industrial Estate, Kings Langley,
# Hertfordshire, WD4 8LZ, UK
#
# $Log: Linux.mk $
#

modules := libusc libusc_host

libusc_type := shared_library

libusc_src := \
 usc.c \
 data.c \
 dce.c \
 efo.c \
 intcvt.c \
 iregalloc.c \
 regalloc.c \
 ssa.c \
 groupinst.c \
 iselect.c \
 dgraph.c \
 reorder.c \
 regpack.c \
 icvt_c10.c \
 icvt_f32.c \
 icvt_core.c \
 icvt_f16.c \
 icvt_mem.c \
 precovr.c \
 pregalloc.c \
 finalise.c \
 asm.c \
 indexreg.c \
 f16opt.c \
 usc_utils.c \
 icvt_i32.c \
 domcalc.c \
 layout.c \
 inst.c \
 hw.c \
 dualissue.c \
 vec34.c \
 icvt_f32_vec.c \
 icvt_f16_vec.c \
 reggroup.c \
 usedef.c \
 pconvert.c \
 cfa.c \
 cdg.c \
 execpred.c

ifeq ($(BUILD),release)
ifeq ($(PDUMP),1)
libusc_src += debug.c
endif
else
libusc_src += debug.c
endif

libusc_includes := include4 hwdefs tools/intern/useasm tools/intern/usp

libusc_cflags := \
 $(call cc-option,-Wno-type-limits) \
 -DINCLUDE_SGX_FEATURE_TABLE -DINCLUDE_SGX_BUG_TABLE

ifeq ($(BUILD),timing)
libusc_cflags += -DMETRICS=1
endif

libusc_cflags += -DOUTPUT_USPBIN
libusc_src += uspbin.c

libusc_cflags += -DOUTPUT_USCHW
libusc_staticlibs := useasm

# libusc_host.a
libusc_host_type := host_static_library
libusc_host_src := $(libusc_src)
libusc_host_cflags := $(libusc_cflags)
libusc_host_includes := $(libusc_includes)
