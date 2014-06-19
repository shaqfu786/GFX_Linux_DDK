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

modules := egl

ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
egl_target := libEGL_POWERVR_SGX$(SGXCORE)_$(SGX_CORE_REV).so
else
egl_target := libEGL.so
endif

egl_type := shared_library
egl_src := eglentrypoints.c
egl_cflags := -DEGL_MODULE
egl_includes = include4 eurasiacon/include
egl_libs := IMGegl_SGX$(SGXCORE)_$(SGX_CORE_REV)
