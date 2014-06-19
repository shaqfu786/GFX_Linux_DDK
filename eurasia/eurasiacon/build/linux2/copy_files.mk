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
# $Revision: 1.5 $
# $Log: copy_files.mk $
#

# Rules for copying files to $(MODULE_OUT). This is meant to be used for
# things like shader source files and headers that should go in the binary
# package, but don't need any kind of processing.

MODULE_COPIED_SOURCES := $(addprefix $(MODULE_OUT)/,$(notdir $(MODULE_SOURCES)))
MODULE_TARGETS := $(MODULE_COPIED_SOURCES)
$(call must-be-nonempty,MODULE_TARGETS)

MODULE_DOS2UNIX := $(if $($(THIS_MODULE)_dos2unix),true,)
MODULE_EXECUTABLE := $(if $($(THIS_MODULE)_executable),true,)

#$(info [$(THIS_MODULE)] Copied files: $(MODULE_TARGETS))

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

define rule-for-copying-file
$(1): $(2) | $$(MODULE_OUT)
	$(if $(V),,@echo "  CP      " $$(call relative-to-top,$$@))
	$$(CP) -f $$< $$@
ifeq ($$(MODULE_DOS2UNIX),true)
	$$(DOS2UNIX) $$@
endif
ifeq ($$(MODULE_EXECUTABLE),true)
	$$(CHMOD) +x $$@
endif
endef

$(foreach _file,$(MODULE_SOURCES),$(eval $(call rule-for-copying-file,$(MODULE_OUT)/$(notdir $(_file)),$(_file))))
