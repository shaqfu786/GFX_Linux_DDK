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

modules := common

# This module builds the common code with -DOGLES1_MODULE. This is because
# both GLES1 and GLES2 use this code, but everywhere the macros are tested,
# they're like
#    #if defined(OGLES1_MODULE) || defined(OGLES2_MODULE)
# and they're not tested at all except in buffers.c and kickresource.c. This
# means that we can build this code once and link it in to GLES1&2, and let
# OpenVG build its own version of buffers.o and kickresource.o.
common_type := objects
common_src := buffers.c codeheap.c kickresource.c twiddle.c
common_includes := \
 include4 \
 hwdefs \
 eurasiacon/include
common_cflags := -fPIC -DOGLES1_MODULE
