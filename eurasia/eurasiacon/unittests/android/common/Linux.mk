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

modules := testwrap_jni common_jar

testwrap_jni_type := shared_library
testwrap_jni_target := libtestwrap.so
testwrap_jni_src := jni/testwrap.c
testwrap_jni_includes := \
 $(UNITTEST_INCLUDES) \
 ${ANDROID_ROOT}/dalvik/libnativehelper/include

common_jar_type := java_archive
common_jar_target := common.jar
common_jar_src := src/com/imgtec/powervr/ddk/TestProgram.java
common_jar_extcp := \
 $(TARGET_ROOT)/common/obj/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar
