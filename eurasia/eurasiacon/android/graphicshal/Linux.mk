# Copyright	(c) Imagination Technologies Limited. All rights reserved.
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

modules := graphicshal graphicshal_use

graphicshal_use_type := use_header
graphicshal_use_src := blitops.asm
graphicshal_use_name := BlitopsUSECode
graphicshal_use_label := BLITOPS_
graphicshal_use_includes := include4 hwdefs

graphicshal_type := shared_library

ifeq ($(HAL_VARIANT),)
 $(error Need to specify a name for this HAL)
else
 graphicshal_target := gralloc.$(HAL_VARIANT).so
endif

graphicshal_src := \
 framebuffer.c \
 gralloc.c \
 hal.c \
 mapper.c \
 buffer_generic.c \
 gralloc_defer.c

graphicshal_cflags := -DOGLES1_MODULE -DPDS_BUILD_OPENGLES

graphicshal_includes := \
 include4 \
 hwdefs \
 pvr2d \
 eurasiacon/include \
 eurasiacon/common \
 codegen/pds \
 codegen/pixevent \
 common/tls \
 $(UNITTEST_INCLUDES)

graphicshal_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV) pvr2d_SGX$(SGXCORE)_$(SGX_CORE_REV)

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(SUPPORT_DRI_DRM_NO_LIBDRM),1)
graphicshal_includes += services4/srvclient/env/linux/common
else
graphicshal_extlibs += drm
endif
endif

ifeq ($(SUPPORT_ANDROID_REFERENCE_COMPOSER_HAL),1)
graphicshal_genheaders := \
	graphicshal_use/graphicshal_use.h \
	graphicshal_use/graphicshal_use_labels.h
graphicshal_includes += \
	$(TARGET_INTERMEDIATES)/graphicshal_use
endif

ifeq ($(SUPPORT_ANDROID_OMAP_NV12),1)
graphicshal_src += buffer_omap.c
endif

ifeq ($(SUPPORT_ANDROID_C110_NV12),1)
graphicshal_src += buffer_c110.c
endif
