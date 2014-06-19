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

modules := eglinfo_jar eglinfo_apk

eglinfo_jar_type := java_archive
eglinfo_jar_target := eglinfo.jar
eglinfo_jar_src := src/com/imgtec/powervr/ddk/eglinfo/EGLInfo.java
eglinfo_jar_cp := common
eglinfo_jar_extcp := \
 $(TARGET_ROOT)/common/obj/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar

eglinfo_apk_type := apk
eglinfo_apk_target := eglinfo.apk
eglinfo_apk_src := AndroidManifest.xml
eglinfo_apk_native := testwrap eglinfo
eglinfo_apk_classes := common eglinfo
