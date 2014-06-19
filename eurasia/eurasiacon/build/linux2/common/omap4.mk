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
# $Revision: 1.2 $
# $Log: omap4.mk $
#

$(eval $(call TunableBothConfigC,PVR_NO_FULL_CACHE_OPS,))
$(eval $(call TunableKernelConfigC,PVR_NO_OMAP_TIMER,))
$(eval $(call TunableKernelConfigC,PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY,))
$(eval $(call TunableKernelConfigC,PVR_OMAPLFB_DRM_FB,))
