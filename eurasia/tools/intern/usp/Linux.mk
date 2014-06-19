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

modules := libusp

libusp_type := static_library
libusp_src := \
 usp.c \
 uspshader.c \
 finalise.c \
 hwinst.c \
 usp_instblock.c \
 usp_inputdata.c \
 usp_resultref.c \
 usp_sample.c \
 usp_texwrite.c

libusp_cflags := \
 $(call cc-option,-Wno-type-limits) \
 -DOUTPUT_USPBIN -DINCLUDE_SGX_FEATURE_TABLE -DINCLUDE_SGX_BUG_TABLE
libusp_includes = \
 include4 hwdefs \
 tools/intern/useasm \
 tools/intern/usc2 \
 tools/intern/usp

ifeq ($(BUILD),debug)
libusp_cflags += -DUSP_DECODE_HWSHADER
endif
