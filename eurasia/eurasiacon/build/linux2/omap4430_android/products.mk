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
# $Log: $
#

ifeq ($(TARGET_PRODUCT),crespo)
$(warning *** Building crespo from omap4430_android is deprecated.)
$(error Use s5pc110_android instead)
endif

ifneq ($(filter blaze blaze_tablet tuna maguro toro mysid yakju p2 u2,$(TARGET_PRODUCT)),)
ifeq ($(SUPPORT_ANDROID_COMPOSITION_BYPASS),1)
$(error SUPPORT_ANDROID_COMPOSITION_BYPASS=1 is obsolete for this product)
endif
#ifeq ($(SUPPORT_ANDROID_OMAP_NV12),1)
#SUPPORT_NV12_FROM_2_HWADDRS := 1
#endif
# These default on in tuna_defconfig
PVRSRV_USSE_EDM_STATUS_DEBUG := 1
PVRSRV_DUMP_MK_TRACE := 1
SUPPORT_SGX_HWPERF := 1
$(info "USSE_EDM_STATUS")
# Go back to the old compiler for tuna kernel modules
KERNEL_CROSS_COMPILE := arm-eabi-
endif
