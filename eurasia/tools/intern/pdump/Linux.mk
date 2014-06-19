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

modules := pdump

pdump_type := executable

pdump_src := \
 pdump.c \
 common.c \
 $(TOP)/tools/intern/debug/client/umdbgdrvlnx.c

pdump_includes := \
 include4 tools/intern/debug/client services4/include/env/linux

ifeq ($(SUPPORT_DRI_DRM),1)
pdump_includes += services4/system/$(PVR_SYSTEM)
ifeq ($(SUPPORT_DRI_DRM_NO_LIBDRM),1)
pdump_includes += services4/srvclient/env/linux/common
pdump_libs := srv_um
else
pdump_packages := libdrm
$(addprefix $(TARGET_INTERMEDIATES)/pdump/,\
 $(foreach _o,$(patsubst %.c,%.o,$(pdump_src)),$(notdir $(_o)))): \
  $(TARGET_OUT)/.$(libdrm)
endif # SUPPORT_DRI_DRM_NO_LIBDRM
endif # SUPPORT_DRI_DRM
