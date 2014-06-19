# Copyright	(C) Imagination Technologies Limited. All rights reserved.
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

# We don't support building scripts from this build directory.
#
ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS := components kbuild install_script
endif
ifneq ($(filter scripts,$(MAKECMDGOALS)),)
$(error Building $(MAKECMDGOALS) is not supported for this target)
endif

TARGET_PRODUCT ?= blaze

ifeq ($(TARGET_SGX),544sc)
 SGXCORE := 544
 SGX_CORE_REV := 112
 HAL_VARIANT := omap4470
else ifeq ($(TARGET_SGX),540)
 SGXCORE := 540
 SGX_CORE_REV := 120
 HAL_VARIANT := omap4
else
 #default config
 SGXCORE := 540
 SGX_CORE_REV := 120
 HAL_VARIANT := omap4
endif
# We need to export these to generate the install script
export SGXCORE
export SGX_CORE_REV


SGX_DYNAMIC_TIMING_INFO := 1
SYS_SGX_ACTIVE_POWER_LATENCY_MS := 100

SUPPORT_LINUX_USING_WORKQUEUES := 1

DISPLAY_CONTROLLER := omaplfb

PVR_SYSTEM := omap4

KERNEL_CROSS_COMPILE ?= arm-none-linux-gnueabi-

include ../common/android/paths.mk
include ../common/android/armv7-a.mk
include ../common/android/features.mk

#
# The expectation here is that customers will have a very good idea of
# the kernel configuration that they expect to use.
#
# From TI perspective it is recommended that rather than using KERNELDIR
# which can be pretty error prone (kernel version is only occasionally a
# good guess) that customers specify the kernel revision here in order
# to pickup the right PVR configuration options
#
# If you want to override the warning you can set EXPECTED_KERNEL_ID but
# suggest you change the Makefile and test carefully when changing kernel
# revisions
ifeq ($(strip $(KERNELDIR)),)
override KERNEL_VERSION := 3
override KERNEL_PATCHLEVEL := 0
override KERNEL_SUBLEVEL := 19
override KERNEL_EXTRAVERSION :=
endif

ifeq ($(strip $(EXPECTED_KERNEL_ID)),)
EXPECTED_KERNEL_ID := 3.0.19
endif

include ../kernel_version.mk

$(warning KERNEL_ID $(KERNEL_ID))

ifneq ($(KERNEL_ID),$(EXPECTED_KERNEL_ID))
$(warning WARNING!!! expected $(EXPECTED_KERNEL_ID) but using $(KERNEL_ID))
endif

ifneq ($(strip $(KERNEL_ID)),)
 ifeq ($(call kernel-version-at-least,2,6,35),true)
  PVR_NO_FULL_CACHE_OPS := 0
  PVR_NO_OMAP_TIMER := 1
 endif
 ifeq ($(call kernel-version-at-least,2,6,39),true)
  ifeq ($(LDM_PLATFORM),1)
   PVR_LDM_PLATFORM_PRE_REGISTERED := 1
   PVR_LDM_PLATFORM_PRE_REGISTERED_DEV := "\"pvrsrvkm\""
  endif
 endif
 ifeq ($(is_at_least_icecream_sandwich),1)
  ifeq ($(call kernel-version-at-least,3,0),true)
   SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 1
   PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND := 1
   SYS_OMAP4_HAS_DVFS_FRAMEWORK := 1
  endif
 endif
else
 $(error Did not expect to not have KERNEL_ID set)
 ifeq ($(is_at_least_icecream_sandwich),1)
  $(warning "KERNELDIR is not set, so can't feature check DVFS or dsscomp.")
  $(warning "Assuming we want DVFS and dsscomp support.")
  SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 1
  PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND := 1
  SYS_OMAP4_HAS_DVFS_FRAMEWORK := 1
 endif
endif

ifneq ($(LDM_PLATFORM),1)
SUPPORT_LINUX_USING_WORKQUEUES := 0
SUPPORT_LINUX_USING_SHARED_WORKQUEUES := 1
SUPPORT_ACTIVE_POWER_MANAGEMENT := 0
DISPLAY_CONTROLLER := pvrlfb
DISPLAY_CONTROLLER_COMPONENT := linux_framebuffer
OMAP_NON_FLIP_DISPLAY := 1
else
DISPLAY_CONTROLLER_COMPONENT := dc_omapfb3_linux
endif

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(PVR_LDM_PLATFORM_PRE_REGISTERED),1)
PVR_DRI_DRM_PLATFORM_DEV := 1
PVR_DRI_DRM_STATIC_BUS_ID := 1
PVR_DRI_DRM_DEV_BUS_ID := "\"platform:pvrsrvkm"\"
else
PVR_DRI_DRM_NOT_PCI := 1
KERNEL_COMPONENTS += linux_drm
endif
EXTRA_PVRSRVKM_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
EXTRA_KBUILD_SOURCE := $(KERNELDIR)
# FIXME: Only required for comparison with X's KM
PVR_SECURE_DRM_AUTH_EXPORT := 1
ifneq ($(OMAP_NON_FLIP_DISPLAY),1)
PVR_DISPLAY_CONTROLLER_DRM_IOCTL := 1
endif
else
KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
endif

SUPPORT_ANDROID_OMAP_NV12 := 1

ifeq ($(is_at_least_icecream_sandwich),1)
ifeq ($(SUPPORT_ANDROID_COMPOSER_HAL),1)
PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT := 1
endif
endif

PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV := 1

ifeq ($(is_at_least_icecream_sandwich),1)
-include products.mk
endif

ifeq ($(NO_HARDWARE),1)
ifeq ($(SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED),1)
$(info WARNING: SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=1 is incompatible with NO_HARDWARE=1)
$(info WARNING: Setting SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=0 and switching to dc_nohw)
override SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 0
override DISPLAY_CONTROLLER := dcnohw
KERNEL_COMPONENTS += dc_nohw
endif
endif

include ../config/core.mk
include ../common/android/extra_config.mk
include ../common/dridrm.mk
include ../common/opencl.mk
include ../common/omap4.mk

# Not all OMAP4 kernels have a compatible DVFS framework
#
$(eval $(call TunableKernelConfigC,SYS_OMAP4_HAS_DVFS_FRAMEWORK,))

# If set, services allows two flips to enter the processing queue,
# and does not add read dependencies to the set of buffers last
# flipped to. This is necessary for DSS composition on OMAP4.
# 
$(eval $(call TunableKernelConfigC,SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED,))
