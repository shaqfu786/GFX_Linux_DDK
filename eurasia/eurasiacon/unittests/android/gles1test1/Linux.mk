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

modules := gles1test1_jar gles1test1_apk

gles1test1_jar_type := java_archive
gles1test1_jar_target := gles1test1.jar
gles1test1_jar_src := src/com/imgtec/powervr/ddk/gles1test1/GLES1Test1.java
gles1test1_jar_cp := common
gles1test1_jar_extcp := \
 $(TARGET_ROOT)/common/obj/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar

gles1test1_apk_type := apk
gles1test1_apk_target := gles1test1.apk
gles1test1_apk_src := AndroidManifest.xml
gles1test1_apk_native := testwrap gles1test1
gles1test1_apk_classes := common gles1test1
