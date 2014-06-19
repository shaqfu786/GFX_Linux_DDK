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
# $Revision: 1.2 $
# $Log: objects.mk $
#

# Rules for making intermediate object files, which can be linked into other
# modules with $(module)_obj

# Objects built from MODULE_SOURCES
MODULE_C_OBJECTS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(foreach _cobj,$(MODULE_SOURCES:.c=.o),$(notdir $(_cobj))))
MODULE_TARGETS := $(MODULE_C_OBJECTS)

#$(info [$(THIS_MODULE)] $(Host_or_target) objects: $(MODULE_TARGETS))

MODULE_GENERATED_DEPENDENCIES := $(MODULE_C_OBJECTS:.o=.d)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

-include $(MODULE_GENERATED_DEPENDENCIES)

define rule-for-objects-o-from-one-c
$(1): MODULE_HOST_CFLAGS := $$(MODULE_HOST_CFLAGS)
$(1): MODULE_CFLAGS := $$(MODULE_CFLAGS)
$(1): MODULE_INCLUDE_FLAGS := $$(MODULE_INCLUDE_FLAGS)
$(1): $$(MODULE_GENERATED_HEADERS) $$(THIS_MAKEFILE) | $$(MODULE_INTERMEDIATES_DIR)
$(1): $(2)
ifeq ($(MODULE_HOST_BUILD),true)
	$$(host-o-from-one-c)
else
	$$(target-o-from-one-c)
endif
endef

$(foreach _src_file,$(MODULE_SOURCES),$(eval $(call rule-for-objects-o-from-one-c,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.c=.o)),$(_src_file))))
