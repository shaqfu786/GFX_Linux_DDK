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
# $Revision: 1.10 $
# $Log: static_library.mk $
#

MODULE_TARGETS := $(addprefix $(MODULE_OUT)/,$(if $($(THIS_MODULE)_target),$($(THIS_MODULE)_target),$(THIS_MODULE).a))
#$(info [$(THIS_MODULE)] $(Host_or_target) static library: $(MODULE_TARGETS))

# Objects built from MODULE_SOURCES
MODULE_C_OBJECTS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(foreach _cobj,$(MODULE_SOURCES:.c=.o),$(notdir $(_cobj))))

# Objects built by other modules
MODULE_EXTERNAL_C_OBJECTS := $($(THIS_MODULE)_obj)

MODULE_ALL_OBJECTS := \
 $(MODULE_C_OBJECTS) \
 $(MODULE_EXTERNAL_C_OBJECTS)

MODULE_GENERATED_DEPENDENCIES := $(MODULE_C_OBJECTS:.o=.d)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

# MODULE_GENERATED_DEPENDENCIES are generated as a side effect of running the
# rules below, but if we wanted to generate .d files for things that GCC
# couldn't handle, we could add a rule with $(MODULE_GENERATED_DEPENDENCIES)
# as a target

-include $(MODULE_GENERATED_DEPENDENCIES)
# TODO: MODULE_ARFLAGS?
$(MODULE_TARGETS): MODULE_ALL_OBJECTS := $(MODULE_ALL_OBJECTS)
$(MODULE_TARGETS): $(MODULE_ALL_OBJECTS) $(THIS_MAKEFILE)
ifeq ($(MODULE_HOST_BUILD),true)
	$(host-static-library-from-o)
else
	$(target-static-library-from-o)
endif

define rule-for-static-library-o-from-one-c
$(1): MODULE_CFLAGS := -fPIC $$(MODULE_CFLAGS)
$(1): MODULE_HOST_CFLAGS := $$(MODULE_HOST_CFLAGS)
$(1): MODULE_INCLUDE_FLAGS := $$(MODULE_INCLUDE_FLAGS)
$(1): $$(MODULE_GENERATED_HEADERS) $$(THIS_MAKEFILE)
$(1): | $$(MODULE_INTERMEDIATES_DIR)
$(1): $(2)
ifeq ($(MODULE_HOST_BUILD),true)
	$$(host-o-from-one-c)
else
	$$(target-o-from-one-c)
endif
endef

$(foreach _src_file,$(MODULE_SOURCES),$(eval $(call rule-for-static-library-o-from-one-c,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.c=.o)),$(_src_file))))
