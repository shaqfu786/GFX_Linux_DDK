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

modules := gles1test1 gles1test1_android

gles1test1_src := gles1test1.c

gles1test1_type := executable
gles1test1_target := gles1test1
gles1test1_includes := eurasiacon/unittests/include
gles1test1_libs = EGL $(call module-library,opengles1)

gles1test1_android_type := shared_library
gles1test1_android_target := libgles1test1.so
gles1test1_android_includes := $(UNITTEST_INCLUDES)
gles1test1_android_extlibs := EGL GLESv1_CM
gles1test1_android_src := $(gles1test1_src)
