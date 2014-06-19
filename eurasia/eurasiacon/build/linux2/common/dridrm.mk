# Copyright 2010 Imagination Technologies Limited. All rights reserved.
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
# $Revision: 1.15 $
# $Log: dridrm.mk $
#

$(eval $(call TunableBothConfigC,SUPPORT_DRI_DRM,))
$(eval $(call TunableBothConfigC,SUPPORT_DRI_DRM_EXT,))
$(eval $(call TunableKernelConfigC,SUPPORT_DRI_DRM_PLUGIN,))

$(eval $(call TunableUserConfigC,SUPPORT_DRI_DRM_NO_DROPMASTER,))

$(eval $(call TunableBothConfigMake,SUPPORT_DRI_DRM,))
$(eval $(call TunableUserConfigMake,DISPLAY_CONTROLLER_DIR,))

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(SUPPORT_DRI_DRM_NO_LIBDRM),1)
$(eval $(call UserConfigC,SUPPORT_DRI_DRM_NO_LIBDRM,1))
$(eval $(call UserConfigMake,SUPPORT_DRI_DRM_NO_LIBDRM,1))
endif
$(eval $(call TunableKernelConfigC,PVR_SECURE_DRM_AUTH_EXPORT,))
endif

$(eval $(call TunableKernelConfigC,PVR_DISPLAY_CONTROLLER_DRM_IOCTL,))
$(eval $(call TunableUserConfigC,PVR_USE_DISPLAY_CONTROLLER_DRM_IOCTL,$(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)))

$(eval $(call TunableBothConfigC,PVR_DRI_DRM_NOT_PCI))
$(eval $(call TunableBothConfigMake,PVR_DRI_DRM_NOT_PCI))

$(eval $(call TunableKernelConfigC,PVR_DRI_DRM_PLATFORM_DEV,))
$(eval $(call TunableUserConfigC,PVR_DRI_DRM_STATIC_BUS_ID,))

$(eval $(call TunableUserConfigC,PVR_DRI_DRM_DEV_BUS_ID,))

export EXTERNAL_3PDD_TARBALL
