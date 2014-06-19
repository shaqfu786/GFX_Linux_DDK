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

modules := launcher_jar launcher_apk

launcher_jar_type := java_archive
launcher_jar_target := launcher.jar
launcher_jar_src := src/com/imgtec/powervr/ddk/launcher/Launcher.java
launcher_jar_extcp := \
 $(TARGET_ROOT)/common/obj/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar

launcher_apk_type := apk
launcher_apk_target := launcher.apk
launcher_apk_src := AndroidManifest.xml
launcher_apk_classes := launcher
