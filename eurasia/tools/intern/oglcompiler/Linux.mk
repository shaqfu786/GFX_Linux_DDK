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

modules := glslcompiler glslcompiler_ogl

modules += glslparser

glslcompiler_type := shared_library
glslcompiler_target := libglslcompiler_SGX$(SGXCORE)_$(SGX_CORE_REV).so

glslcompiler_ogl_type := static_library
glslcompiler_ogl_target := libglslcompiler_ogl.a

glslcompiler_src := \
 glsl/astbuiltin.c \
 glsl/common.c \
 glsl/error.c \
 glsl/icbuiltin.c \
 glsl/icemul.c \
 glsl/icgen.c \
 glsl/icode.c \
 glsl/icunroll.c \
 glsl/glslfns.c \
 glsl/glsltabs.c \
 glsl/glsltree.c \
 glsl/glsl.c \
 glsl/prepro.c \
 glsl/semantic.c \
 parser/glsldebug.c \
 parser/lex.c \
 parser/memmgr.c \
 parser/metrics.c \
 parser/parser.c \
 parser/symtab.c \
 powervr/bindingsym.c \
 powervr/ic2uf.c \
 powervr/glsl2uf.c

ifeq ($(BUILD),debug)
glslcompiler_cflags += -DDUMP_LOGFILES=1
endif

ifeq ($(BUILD),timing)
glslcompiler_cflags += -DMETRICS=1
endif

glslcompiler_gensrc := glslparser/glsl_parser.tab.c
glslcompiler_genheaders := glslparser/glsl_parser.tab.h
glslcompiler_ogl_src := $(glslcompiler_src)

glslcompiler_ogl_cflags := $(glslcompiler_cflags)
glslcompiler_ogl_gensrc := $(glslcompiler_gensrc)
glslcompiler_ogl_genheaders := $(glslcompiler_genheaders)

# ES only
glslcompiler_cflags += \
 -DOUTPUT_USPBIN=1 -DGLSL_ES=1 -DGEN_HW_CODE=1 -DCOMPACT_MEMORY_MODEL=1
glslcompiler_src += binshader/esbinshader.c

glslcompiler_includes := \
 $(THIS_DIR) \
 include4 hwdefs \
 $(THIS_DIR)/binshader \
 $(THIS_DIR)/glsl \
 $(THIS_DIR)/parser \
 $(THIS_DIR)/powervr \
 tools/intern/usp \
 tools/intern/usc2 \
 tools/intern/useasm \
 $(TARGET_INTERMEDIATES)/glslparser

glslcompiler_ogl_includes = $(glslcompiler_includes)

glslcompiler_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV) usc_SGX$(SGXCORE)_$(SGX_CORE_REV)
glslcompiler_staticlibs := useasm
glslcompiler_extlibs := m

glslparser_type := bison_parser
glslparser_src := parser/glsl_parser.y
glslparser_bisonflags := --verbose -bglsl_parser
