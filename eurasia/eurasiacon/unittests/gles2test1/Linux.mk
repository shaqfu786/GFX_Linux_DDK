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

modules := gles2test1 gles2test1_android gles2test1_shaders

gles2test1_src := gles2test1.c maths.c
gles2test1_extlibs := m

gles2test1_type := executable
gles2test1_target := gles2test1
gles2test1_includes := eurasiacon/unittests/include
gles2test1_libs = EGL $(call module-library,opengles2)

gles2test1_android_type := shared_library
gles2test1_android_target := libgles2test1.so
gles2test1_android_cflags := -DFILES_EMBEDDED
gles2test1_android_includes := $(UNITTEST_INCLUDES)
gles2test1_android_extlibs := $(gles2test1_extlibs) EGL GLESv2
gles2test1_android_src := $(gles2test1_src)

# FILES_EMBEDDED isn't on, so the shaders need to be copied
gles2test1: gles2test1_shaders

gles2test1_shaders_type := copy_files
gles2test1_shaders_src := \
 glsltest1_vertshader.txt \
 glsltest1_fragshaderA.txt \
 glsltest1_fragshaderB.txt
