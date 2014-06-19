# Copyright (C) Imagination Technologies Limited. All rights reserved.
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

modules := composerhal

composerhal_type := shared_library

ifeq ($(HAL_VARIANT),)
composerhal_target := hwcomposer.default.SGX$(SGXCORE)_$(SGX_CORE_REV).so
else
composerhal_target := hwcomposer.$(HAL_VARIANT).SGX$(SGXCORE)_$(SGX_CORE_REV).so
endif

composerhal_src := hwc.c hal.c

composerhal_allow_undefined := true

composerhal_includes := \
 include4 \
 hwdefs \
 eurasiacon/android/graphicshal \
 $(UNITTEST_INCLUDES)

composerhal_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)
composerhal_extlibs := EGL hardware
