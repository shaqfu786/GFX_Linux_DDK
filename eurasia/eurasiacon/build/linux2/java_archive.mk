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
# $Revision: 1.7 $
# $Log: java_archive.mk $
#

MODULE_TARGETS := $(addprefix $(MODULE_OUT)/,$(if $($(THIS_MODULE)_target),$($(THIS_MODULE)_target),$(THIS_MODULE).jar))
#$(info [$(THIS_MODULE)] $(Host_or_target) java archive: $(MODULE_TARGETS))

$(call target-build-only,java archive)

MODULE_BUILT_LIBRARIES := $(patsubst %,$(MODULE_OUT)/%.jar,$($(THIS_MODULE)_cp))
MODULE_ALL_JAVA_SOURCES := $(MODULE_SOURCES)
$(call must-be-nonempty,MODULE_ALL_JAVA_SOURCES)
$(call one-word-only,MODULE_ALL_JAVA_SOURCES)
MODULE_JAVAC_FLAGS := -target 1.5 -Xmaxerrs 9999999 -encoding ascii -extdirs ""

define target-java-archive-from-java
$(MKDIR) -p $(MODULE_INTERMEDIATES_DIR)/classes
$(if $(V),,@echo "  JAVAC   " $(call relative-to-top,$<))
$(JAVAC) $(MODULE_JAVAC_FLAGS) -classpath $(MODULE_JAVAC_CLASSPATH) \
 -d $(MODULE_INTERMEDIATES_DIR)/classes $<
$(JAR) cf $@ -C $(MODULE_INTERMEDIATES_DIR)/classes .
endef

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_TARGETS): MODULE_JAVAC_CLASSPATH := $($(THIS_MODULE)_extcp):$(MODULE_BUILT_LIBRARIES)
$(MODULE_TARGETS): MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): $(MODULE_BUILT_LIBRARIES) $(THIS_MAKEFILE)
$(MODULE_TARGETS): $(MODULE_ALL_JAVA_SOURCES)
	$(target-java-archive-from-java)
