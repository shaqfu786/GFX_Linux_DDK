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

modules := hal_blit_test

hal_blit_test_src := blit.c

hal_blit_test_type := executable
hal_blit_test_target := hal_blit_test
hal_blit_test_extlibs := EGL GLESv1_CM ui hardware

hal_blit_test_includes := \
 include4 \
 eurasiacon/android/graphicshal \
 $(UNITTEST_INCLUDES)

ifneq ($(SUPPORT_ANDROID_C110_NV12),)
hal_blit_test_cflags := -DSUPPORT_ANDROID_C110_NV12
else ifneq ($(SUPPORT_ANDROID_OMAP_NV12),)
hal_blit_test_cflags := -DSUPPORT_ANDROID_OMAP_NV12
endif
