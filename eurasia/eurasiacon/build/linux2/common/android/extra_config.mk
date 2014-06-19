# Copyright (c) Imagination Technologies Limited. All rights reserved.
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
# $Log: extra_config.mk $
#

$(eval $(call UserConfigMake,libpthread_ldflags,))
$(eval $(call UserConfigMake,librt_ldflags,))
$(eval $(call UserConfigMake,LINKER_RPATH,/system/lib))

$(eval $(call BothConfigC,ANDROID,))

$(eval $(call UserConfigC,OGLES1_BASEPATH,\"$(EGL_DESTDIR)/\"))
$(eval $(call UserConfigC,OGLES2_BASEPATH,\"$(EGL_DESTDIR)/\"))
$(eval $(call UserConfigC,OGLES1_BASENAME,\"GLESv1_CM_POWERVR_SGX$(SGXCORE)_$(SGX_CORE_REV)\"))
$(eval $(call UserConfigC,OGLES2_BASENAME,\"GLESv2_POWERVR_SGX$(SGXCORE)_$(SGX_CORE_REV)\"))
$(eval $(call UserConfigC,GLSL_COMPILER_BASENAME,\"glslcompiler_SGX$(SGXCORE)_$(SGX_CORE_REV)\"))
$(eval $(call UserConfigC,SUPPORT_GRAPHICS_HAL,))

$(eval $(call TunableUserConfigC,SUPPORT_ANDROID_PLATFORM,))
$(eval $(call TunableUserConfigC,SUPPORT_ANDROID_COMPOSER_HAL,))
$(eval $(call TunableUserConfigC,SUPPORT_ANDROID_COMPOSITION_BYPASS,))
$(eval $(call TunableUserConfigC,SUPPORT_ANDROID_REFERENCE_COMPOSER_HAL,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_CONNECT_DISCONNECT,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_CANCELBUFFER,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_USAGE_EXTERNAL_DISP,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_USAGE_PROTECTED,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_USAGE_PRIVATE,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_USAGE_HW_COMPOSER,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_USAGE_HW_VIDEO_ENCODER,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_GRALLOC_DUMP,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_NATIVE_BUFFER_TRANSFORM,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_ANDROID_NATIVE_BUFFER_H,))

$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_NEEDS_ANATIVEWINDOW_TYPEDEF,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_NEEDS_ANATIVEWINDOWBUFFER_TYPEDEF,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_SURFACE_FIELD_NAME,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_SOFTWARE_SYNC_OBJECTS,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND,))
$(eval $(call TunableUserConfigC,PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV,))

$(eval $(call TunableUserConfigC,PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_YV12,))
$(eval $(call TunableUserConfigC,SUPPORT_HAL_PIXEL_FORMAT_BGRX,))

$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_PLATFORM,))
$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_COMPOSITION_BYPASS,))
$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_REFERENCE_COMPOSER_HAL,))
$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_OMAP_NV12,))
$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_C110_NV12,))
$(eval $(call TunableUserConfigMake,TARGET_ROOT,))
$(eval $(call TunableUserConfigMake,HAL_VARIANT,))
$(eval $(call TunableUserConfigMake,UNITTEST_INCLUDES,))
$(eval $(call TunableUserConfigMake,HOST_OS,linux))
$(eval $(call TunableUserConfigMake,HOST_ARCH,x86))
$(eval $(call TunableUserConfigMake,JNI_CPU_ABI,))
$(eval $(call TunableUserConfigMake,API_LEVEL,))
