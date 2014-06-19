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

modules := imgegl

imgegl_type := shared_library
imgegl_target := libIMGegl_SGX$(SGXCORE)_$(SGX_CORE_REV).so
imgegl_src := cfg.c \
 cfg_core.c \
 egl_eglimage.c \
 egl_linux.c \
 egl_sync.c \
 function_table.c \
 generic_ws.c \
 khronos_egl.c \
 metrics.c \
 qsort_s.c \
 srv.c \
 tls.c \
 $(TOP)/common/tls/linux_tls.c \
 $(TOP)/common/tls/nothreads_tls.c
ifeq ($(SUPPORT_SGX),1)
imgegl_src += srv_sgx.c
endif

ifeq ($(EGL_EXTENSION_ANDROID_BLOB_CACHE),1)
imgegl_src += blobcache.c
endif

imgegl_cflags := -DIMGEGL_MODULE -DPDS_BUILD_OPENGLES -DPROFILE_COMMON
imgegl_includes := \
 include4 hwdefs \
 codegen/pds \
 codegen/pixevent \
 codegen/usegen \
 common/dmscalc \
 common/tls \
 eurasiacon/common \
 eurasiacon/include

imgegl_obj := $(call target-intermediates-of,usegen,usegen.o)
imgegl_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)
imgegl_extlibs := pthread
