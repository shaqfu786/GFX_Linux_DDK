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
# $Log: $
#

apk := _apk
include ../common/apis/pvrsrvctl.mk
include ../common/apis/services.mk
include ../common/apis/egl.mk
include ../common/apis/pvr2d.mk
include ../common/apis/opengles1.mk
include ../common/apis/opengles2.mk
COMPONENTS += graphicshal android_ws PVRScopeServices

ifeq ($(is_at_least_honeycomb),1)
ifeq ($(SUPPORT_ANDROID_REFERENCE_COMPOSER_HAL),1)
COMPONENTS += composerhal
endif
endif

-include ../common/apis/unittests.mk
ifneq ($(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER),0)
-include ../common/apis/unittests_system_buffer.mk
endif
ifeq ($(filter unittests,$(EXCLUDED_APIS)),)
COMPONENTS += \
 launcher_apk testwrap texture_benchmark framebuffer_test \
 hal_client_test hal_server_test
ifeq ($(is_at_least_honeycomb),1)
COMPONENTS += hal_blit_test
endif
endif
