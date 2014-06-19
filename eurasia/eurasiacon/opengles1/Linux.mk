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

modules := opengles1

opengles1_type := shared_library

ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
opengles1_target := libGLESv1_CM_POWERVR_SGX$(SGXCORE)_$(SGX_CORE_REV).so
else
ifeq ($(SUPPORT_OPENGLES1_V1_ONLY),1)
opengles1_target := libGLESv1_CM.so
else
opengles1_target := libGLES_CM.so
opengles1_src := \
 $(TOP)/eurasiacon/egl/eglentrypoints.c
endif # SUPPORT_OPENGLES1_V1_ONLY
endif # SUPPORT_ANDROID_PLATFORM

opengles1_src += \
 accum.c \
 bufobj.c \
 clear.c \
 drawtex.c \
 drawvarray.c \
 eglglue.c \
 eglimage.c \
 fbo.c \
 ffgeo.c \
 fftex.c \
 fftexhw.c \
 fftnlgles.c \
 fog.c \
 get.c \
 gles1errata.c \
 makemips.c \
 matrix.c \
 metrics.c \
 misc.c \
 names.c \
 pdump.c \
 pixop.c \
 profile.c \
 shader.c \
 scissor.c \
 sgxif.c \
 spanpack.c \
 state.c \
 statehash.c \
 tex.c \
 texcombine.c \
 texdata.c \
 texformat.c \
 texmgmt.c \
 texrender.c \
 texstream.c \
 texyuv.c \
 tnlapi.c \
 usegles.c \
 useasmgles.c \
 validate.c \
 vertex.c \
 vertexarrobj.c \
 $(TOP)/common/tls/linux_tls.c \
 $(TOP)/common/tls/nothreads_tls.c

opengles1_obj := \
 $(call target-intermediates-of,dmscalc,dmscalc.o) \
 $(call target-intermediates-of,common,buffers.o codeheap.o kickresource.o twiddle.o) \
 $(call target-intermediates-of,pixfmts,sgxpixfmts.o) \
 $(call target-intermediates-of,pixevent,pixevent.o pixeventpbesetup.o) \
 $(call target-intermediates-of,pdsogles,pds.o) \
 $(call target-intermediates-of,usegen,usegen.o)

opengles1_cflags := \
 -DOGLES1_MODULE -DPDS_BUILD_OPENGLES \
 -DOPTIMISE_NON_NPTL_SINGLE_THREAD_TLS_LOOKUP \
 -DPROFILE_COMMON -DOUTPUT_USCHW

ifeq ($(BUILD),timing)
# FIXME: Support TIMING_LEVEL>1
opengles1_cflags += -DTIMING_LEVEL=1
else
opengles1_cflags += -DTIMING_LEVEL=0
endif

opengles1_includes := include4 hwdefs eurasiacon/include eurasiacon/common \
 common/tls common/dmscalc \
 codegen/pds codegen/pixevent codegen/pixfmts codegen/usegen codegen/ffgen \
 tools/intern/useasm \
 tools/intern/usc2 \
 $(TARGET_INTERMEDIATES)/pds_mte_state_copy \
 $(TARGET_INTERMEDIATES)/pds_aux_vtx

opengles1_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV) IMGegl_SGX$(SGXCORE)_$(SGX_CORE_REV) usc_SGX$(SGXCORE)_$(SGX_CORE_REV)
opengles1_staticlibs := ffgen useasm
opengles1_extlibs := m pthread

$(addprefix $(TARGET_INTERMEDIATES)/opengles1/,eglglue.o validate.o): \
 $(call target-intermediates-of,pds_mte_state_copy,pds_mte_state_copy.h pds_mte_state_copy_sizeof.h) \
 $(call target-intermediates-of,pds_aux_vtx,pds_aux_vtx.h pds_aux_vtx_sizeof.h)
