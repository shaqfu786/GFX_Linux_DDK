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
# $Revision: 1.16 $
# $Log: use_header.mk $
#

# Rules for making headers that contain assembled USE code

MODULE_H := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(THIS_MODULE).h)
MODULE_LABELS_H := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(THIS_MODULE)_labels.h)
MODULE_TARGETS := $(MODULE_H) $(MODULE_LABELS_H)

$(call must-be-defined,$(THIS_MODULE)_name)
$(call one-word-only,$(THIS_MODULE)_name)

# When modulename_assembleonly is set to 'true', useasm is run without -obj,
# producing a .bin file, and uselink isn't run
MODULE_ASSEMBLE_ONLY := $(if $(filter true,$($(THIS_MODULE)_assembleonly)),true,)

#$(info [$(THIS_MODULE)] USE headers: $(MODULE_TARGETS))

# USE headers are built like this: (* = final result)
# - Source: tasarestore.use.asm
# - Preprocessed source: tasarestore.use.asm --(cc -E)--> tasarestore.use.asm.preproc
# - Assembled object: tasarestore.use.asm.preproc --(useasm)--> tasarestore.use.obj
# - Linked binary: tasarestore.use.obj, ... --(uselink)--> ukernel.bin, ukernel_labels.h*
# - Header: ukernel.bin --(vhd2inc)--> ukernel.h*

# Each variable below represents the file produced by one of these steps

MODULE_PREPROCESSED_ASM := $(patsubst %,$(MODULE_INTERMEDIATES_DIR)/%.preproc,$(foreach _s,$(MODULE_SOURCES),$(notdir $(_s))))
MODULE_ASSEMBLED_OBJ := $(MODULE_PREPROCESSED_ASM:.preproc=.obj)
MODULE_LINKED_BIN := $(MODULE_H:.h=.bin)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

define use-object-rules
# These variables are overwritten by each eval
_PREPROC := $$(patsubst %,$$(MODULE_INTERMEDIATES_DIR)/%.preproc,$$(notdir $(1)))

# We add -DUSE_CODE to MODULE_HOST_CFLAGS, because we always want it when
# preprocessing useasm files
$$(_PREPROC): MODULE_HOST_CFLAGS := -DUSE_CODE $$(MODULE_HOST_CFLAGS)
$$(_PREPROC): MODULE_INCLUDE_FLAGS := $$(MODULE_INCLUDE_FLAGS)
$$(_PREPROC): $$(THIS_MAKEFILE) | $$(MODULE_INTERMEDIATES_DIR)
$$(_PREPROC): $(1)
	$(if $(V),,@echo "  HOST_CC " $$(call relative-to-top,$$<))
	$$(HOST_CC) -MD -MT $$@ -include $(CONFIG_H) $$(MODULE_HOST_CFLAGS) \
		$$(MODULE_INCLUDE_FLAGS) -x c -E $$< -o $$@
	$$(SED) -i -e 's/# /#line /' $$@

ifneq ($(MODULE_ASSEMBLE_ONLY),true)
# common case: we want to build .obj files, and later link them with uselink
# There is a temporary workaround for OpenVG: if $(module)_name starts with
# "OVG", don't use -cpp
_OBJ := $$(_PREPROC:.preproc=.obj)
$$(_OBJ): $(HOST_OUT)/useasm
$$(_OBJ): MODULE_USEASM_FLAGS := -quiet -fixpairs -obj $$(if $$(filter OVG%,$$($$(THIS_MODULE)_name)),,-cpp)
$$(_OBJ): $$(_PREPROC)
	$(if $(V),,@echo "  USEASM  " $$(call relative-to-top,$$<))
	$$(USEASM) $$(MODULE_USEASM_FLAGS) $$< $$@
else
# otherwise, we want to build a .bin file with useasm and not run uselink
$$(MODULE_LINKED_BIN): _USEASM_LABEL := $($(THIS_MODULE)_label)
$$(MODULE_LINKED_BIN): $(HOST_OUT)/useasm
$$(MODULE_LINKED_BIN): MODULE_USEASM_FLAGS := -quiet -fixpairs $$(if $$(filter OVG%,$$($$(THIS_MODULE)_name)),,-cpp)
$$(MODULE_LINKED_BIN): $$(_PREPROC)
	$(if $(V),,@echo "  USEASM  " $$(call relative-to-top,$$<))
	$$(USEASM) $$(MODULE_USEASM_FLAGS) -label_header=$$(basename $$@)_labels.h -label_prefix=$$(_USEASM_LABEL) $$< $$@
endif

endef

MODULE_ALL_SOURCES := $(MODULE_SOURCES)
MODULE_GENERATED_DEPENDENCIES := $(MODULE_PREPROCESSED_ASM:.preproc=.d)
-include $(MODULE_GENERATED_DEPENDENCIES)

# When MODULE_ASSEMBLE_ONLY is true, we should only be assembling a single
# source file
ifeq ($(MODULE_ASSEMBLE_ONLY),true)
$(call one-word-only,MODULE_ALL_SOURCES)
endif

$(foreach _src_file,$(MODULE_ALL_SOURCES),$(eval $(call use-object-rules,$(_src_file))))

ifneq ($(MODULE_ASSEMBLE_ONLY),true)
$(MODULE_LINKED_BIN): _USELINK_LABEL := $($(THIS_MODULE)_label)
$(MODULE_LINKED_BIN): MODULE_ASSEMBLED_OBJ := $(MODULE_ASSEMBLED_OBJ)
$(MODULE_LINKED_BIN): MODULE_LINKED_BIN := $(MODULE_LINKED_BIN)
$(MODULE_LINKED_BIN): $(HOST_OUT)/uselink
$(MODULE_LINKED_BIN): $(MODULE_ASSEMBLED_OBJ)
	$(if $(V),,@echo "  USELINK " $(call relative-to-top,$@))
	$(USELINK) $(if $(_USELINK_LABEL),-label_header=$(basename $(MODULE_LINKED_BIN))_labels.h -label_prefix=$(_USELINK_LABEL),) -out=$@ $(MODULE_ASSEMBLED_OBJ)
endif

$(MODULE_H): MODULE_PROGRAM_NAME := $($(THIS_MODULE)_name)
$(MODULE_H): $(MODULE_LINKED_BIN) $(HOST_OUT)/vhd2inc
	$(if $(V),,@echo "  VHD2INC " $(call relative-to-top,$<))
	$(VHD2INC) -b $< $@ $(MODULE_PROGRAM_NAME) "IMG_BYTE"
	$(SED) -i -e 's/IMG_BYTE /static const IMG_BYTE /' $@

$(MODULE_LABELS_H): $(MODULE_LINKED_BIN)
	$(TOUCH) -c $@
