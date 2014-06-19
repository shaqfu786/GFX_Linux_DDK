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

modules := libffgen libffgen_ogl

libffgen_type := static_library
libffgen_src := \
 ffgen.c \
 uscfns.c \
 reg.c \
 inst.c \
 source.c \
 codegen.c \
 lighting.c

libffgen_includes := include4 hwdefs tools/intern/useasm tools/intern/usc2
libffgen_cflags := -DOGLES1_MODULE

ifeq ($(GLES1_EXTENSION_CARNAV),1)
libffgen_src += geoshader.c
endif

# FIXME: We might be able to get rid of this & link both the
#        OGLES and OGL stuff into the same static library.
#        The linker should discard the OGL bits if not required.
libffgen_ogl_type := static_library
libffgen_ogl_src := $(libffgen_src) fftousc.c geoshader.c
libffgen_ogl_includes := $(libffgen_includes) tools/intern/usc2
libffgen_ogl_cflags := -DOGL_MODULE
ifeq ($(SGXCORE),535)
libffgen_ogl_cflags += -DOGL_LINESTIPPLE
endif
