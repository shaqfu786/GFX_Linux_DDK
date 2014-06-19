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
# $Revision: 1.21 $
# $Log: shared_library.mk $
#

MODULE_TARGETS := $(addprefix $(MODULE_OUT)/,$(if $($(THIS_MODULE)_target),$($(THIS_MODULE)_target),$(THIS_MODULE)_SGX$(SGXCORE)_$(SGX_CORE_REV).so))
#$(info [$(THIS_MODULE)] $(Host_or_target) shared library: $(MODULE_TARGETS))

$(call target-build-only,shared library)

MODULE_C_SOURCES := $(filter %.c,$(MODULE_SOURCES))
MODULE_CXX_SOURCES := $(filter %.cpp,$(MODULE_SOURCES))

MODULE_UNRECOGNISED_SOURCES := $(filter-out %.c %.cpp,$(MODULE_SOURCES))

ifneq ($(strip $(MODULE_UNRECOGNISED_SOURCES)),)
$(error In makefile $(THIS_MAKEFILE): Module $(THIS_MODULE) specified source files with unrecognised suffixes: $(MODULE_UNRECOGNISED_SOURCES))
endif

# Objects built from MODULE_C_SOURCES and MODULE_CXX_SOURCES
MODULE_C_OBJECTS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(foreach _cobj,$(MODULE_C_SOURCES:.c=.o),$(notdir $(_cobj))))
MODULE_CXX_OBJECTS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(foreach _cxxobj,$(MODULE_CXX_SOURCES:.cpp=.o),$(notdir $(_cxxobj))))

# Objects built by other modules
MODULE_EXTERNAL_OBJECTS := $($(THIS_MODULE)_obj)

# Determine whether the C++ compiler is needed for linking.
MODULE_NEEDS_CXX_LINKER := $(if $($(THIS_MODULE)_force_cxx_linker),true,)
ifeq ($($(THIS_MODULE)_force_cxx_linker),)
# If this module contains any C++ source files, MODULE_NEEDS_CXX_LINKER is
# set, because we have to use the C++ compiler to link it
MODULE_NEEDS_CXX_LINKER := $(if $(strip $(MODULE_CXX_SOURCES)),true,)
endif
ifeq ($(MODULE_NEEDS_CXX_LINKER),true)
ALL_CXX_MODULES += $(THIS_MODULE)
endif

MODULE_ALL_OBJECTS := \
 $(MODULE_C_OBJECTS) $(MODULE_CXX_OBJECTS) \
 $(MODULE_EXTERNAL_OBJECTS)

MODULE_GENERATED_DEPENDENCIES := \
 $(MODULE_C_OBJECTS:.o=.d) $(MODULE_CXX_OBJECTS:.o=.d)

# Libraries that can be made, which this module links with
MODULE_BUILT_LIBRARIES := $(patsubst %,$(MODULE_OUT)/lib%.so,$($(THIS_MODULE)_libs))
MODULE_BUILT_STATIC_LIBRARIES := $(patsubst %,$(MODULE_OUT)/lib%.a,$($(THIS_MODULE)_staticlibs))

# Disallow undefined symbols, except for X.org video drivers
MODULE_LDFLAGS := \
 $(if $($(THIS_MODULE)_allow_undefined),,-Wl,--no-undefined) $(MODULE_LDFLAGS)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

# MODULE_GENERATED_DEPENDENCIES are generated as a side effect of running the
# rules below, but if we wanted to generate .d files for things that GCC
# couldn't handle, we could add a rule with $(MODULE_GENERATED_DEPENDENCIES)
# as a target
-include $(MODULE_GENERATED_DEPENDENCIES)

# This is the rule used to link the final shared library
.DELETE_ON_ERROR: $(MODULE_TARGETS)
$(MODULE_TARGETS): MODULE_LDFLAGS := $(MODULE_LDFLAGS)
$(MODULE_TARGETS): MODULE_CFLAGS := -fPIC $(MODULE_CFLAGS)
$(MODULE_TARGETS): CHECKEXPORTS_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): MODULE_LIBRARY_DIR_FLAGS := $(MODULE_LIBRARY_DIR_FLAGS)
$(MODULE_TARGETS): MODULE_LIBRARY_FLAGS := $(MODULE_LIBRARY_FLAGS)
$(MODULE_TARGETS): MODULE_ALL_OBJECTS := $(MODULE_ALL_OBJECTS)
$(MODULE_TARGETS): $(MODULE_BUILT_LIBRARIES) $(MODULE_BUILT_STATIC_LIBRARIES)
$(MODULE_TARGETS): $(MODULE_ALL_OBJECTS) $(THIS_MAKEFILE)
ifeq ($(MODULE_NEEDS_CXX_LINKER),true)
	$(target-shared-library-cxx-from-o)
else
	$(target-shared-library-from-o)
endif
ifeq ($(DEBUGLINK),1)
	$(target-copy-debug-information)
	$(target-strip-debug-information)
	$(target-add-debuglink)
endif
	$(check-exports)


# This rule is used to compile C source files
define rule-for-shared-library-o-from-one-c
$(1): MODULE_CFLAGS := -fPIC $$(MODULE_CFLAGS)
$(1): MODULE_INCLUDE_FLAGS := $$(MODULE_INCLUDE_FLAGS)
$(1): $$(MODULE_GENERATED_HEADERS) $$(THIS_MAKEFILE)
$(1): | $$(MODULE_INTERMEDIATES_DIR)
$(1): $(2)
	$$(target-o-from-one-c)
endef

$(foreach _src_file,$(MODULE_C_SOURCES),$(eval $(call rule-for-shared-library-o-from-one-c,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.c=.o)),$(_src_file))))

# This rule is used to compile C++ source files
define rule-for-shared-library-o-from-one-cxx
$(1): MODULE_CXXFLAGS := -fPIC $$(MODULE_CXXFLAGS)
$(1): MODULE_INCLUDE_FLAGS := $$(MODULE_INCLUDE_FLAGS)
$(1): $$(MODULE_GENERATED_HEADERS) $$(THIS_MAKEFILE)
$(1): | $$(MODULE_INTERMEDIATES_DIR)
$(1): $(2)
	$$(target-o-from-one-cxx)
endef

$(foreach _src_file,$(MODULE_CXX_SOURCES),$(eval $(call rule-for-shared-library-o-from-one-cxx,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.cpp=.o)),$(_src_file))))
