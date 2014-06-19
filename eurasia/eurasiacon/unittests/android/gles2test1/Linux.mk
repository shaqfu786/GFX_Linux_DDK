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

modules := gles2test1_jar gles2test1_apk

gles2test1_jar_type := java_archive
gles2test1_jar_target := gles2test1.jar
gles2test1_jar_src := src/com/imgtec/powervr/ddk/gles2test1/GLES2Test1.java
gles2test1_jar_cp := common
gles2test1_jar_extcp := \
 $(TARGET_ROOT)/common/obj/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar

gles2test1_apk_type := apk
gles2test1_apk_target := gles2test1.apk
gles2test1_apk_src := AndroidManifest.xml
gles2test1_apk_native := testwrap gles2test1
gles2test1_apk_classes := common gles2test1
