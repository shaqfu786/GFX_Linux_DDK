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

modules := pvr2d

pvr2d_type := shared_library
pvr2d_target := libpvr2d_SGX$(SGXCORE)_$(SGX_CORE_REV).so
pvr2d_src := \
 common/pvr2dmem.c \
 common/pvr2dmode.c \
 common/pvr2dflip.c
pvr2d_includes := include4 pvr2d hwdefs
pvr2d_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

ifeq ($(SUPPORT_SGX),1)

pvr2d_src += \
 devices/sgx/pvr2dinit.c \
 devices/sgx/pvr2dblt.c \
 devices/sgx/pvr2dblt3d.c

pvr2d_includes += pvr2d/devices/sgx

endif # SUPPORT_SGX == 1
