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
# $Log: pds_header.mk $
#

# Rules for making headers that contain assembled PDS code

# The file $(THIS_MODULE).h is made from the .asm file named in
# $(THIS_MODULE)_src. $(THIS_MODULE)_label gives the label prefix to pass to
# pdsasm.

$(call must-be-nonempty,$(THIS_MODULE)_src)
$(call one-word-only,$(THIS_MODULE)_src)
$(call must-be-nonempty,$(THIS_MODULE)_label)
$(call one-word-only,$(THIS_MODULE)_label)

MODULE_TARGETS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(THIS_MODULE).h)
MODULE_TARGETS += $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$(THIS_MODULE)_sizeof.h)

#$(info [$(THIS_MODULE)] PDS headers: $(MODULE_TARGETS))

# PDS headers are built like this: (* = final result)
# - Source: pixelevent.asm
# - Preprocessed source: pixelevent.asm --(cc -E)--> pixelevent.asm.preproc
# - Assembled header: pixelevent.asm.preproc --(pdsasm)--> pixelevent.h.pds, pixelevent.h_sizeof.pds
# - Correct filename: pixelevent.h_sizeof.pds --(cp)--> pixelevent_sizeof.h*
# - Postprocess header: pixelevent.h.pds --(sed)--> pixelevent.h*

# Each variable below represents the file produced by one of these steps,
# except for the last step above, which produces $(MODULE_TARGETS)

# There has to be just one source file - there's no pdslink, so it doesn't
# make sense to have several. Also, some places build the same source files
# several times with different options to produce different headers. Below,
# the names of the intermediates are constructed from the name of the header
# to be built, to avoid clashes
MODULE_ALL_PDSASM_SOURCES := $(MODULE_SOURCES)
$(call one-word-only,MODULE_ALL_PDSASM_SOURCES)

MODULE_SIZEOF_H := $(filter %_sizeof.h,$(MODULE_TARGETS))
MODULE_POSTPROCESSED_H := $(filter-out %_sizeof.h,$(MODULE_TARGETS))
MODULE_H_SIZEOF_PDS := $(MODULE_SIZEOF_H:_sizeof.h=.h_sizeof.pds)
MODULE_ASSEMBLED_H := $(MODULE_POSTPROCESSED_H:.h=.h.pds)
MODULE_PREPROCESSED_ASM := $(MODULE_ASSEMBLED_H:.h.pds=.asm.preproc)

MODULE_GENERATED_DEPENDENCIES := $(MODULE_PREPROCESSED_ASM:.preproc=.d)
-include $(MODULE_GENERATED_DEPENDENCIES)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_PREPROCESSED_ASM): MODULE_HOST_CFLAGS := $(MODULE_HOST_CFLAGS)
$(MODULE_PREPROCESSED_ASM): MODULE_INCLUDE_FLAGS := $(MODULE_INCLUDE_FLAGS)
$(MODULE_PREPROCESSED_ASM): $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_PREPROCESSED_ASM): $(MODULE_ALL_PDSASM_SOURCES)
	$(if $(V),,@echo "  HOST_CC " $(call relative-to-top,$<))
	$(HOST_CC) -MD -MT $@ -include $(OUT)/config.h $(MODULE_HOST_CFLAGS) \
		$(MODULE_INCLUDE_FLAGS) -x c -E $< -o $@
	$(SED) -i -e 's/# /#line /' $@

$(MODULE_ASSEMBLED_H): _PDSASM_LABEL := $($(THIS_MODULE)_label)
$(MODULE_ASSEMBLED_H): MODULE_PREPROCESSED_ASM := $(MODULE_PREPROCESSED_ASM)
$(MODULE_ASSEMBLED_H): $(HOST_OUT)/pdsasm
$(MODULE_ASSEMBLED_H): $(MODULE_PREPROCESSED_ASM)
	$(if $(V),,@echo "  PDSASM  " $(call relative-to-top,$<))
	$(PDSASM) -s $(MODULE_PREPROCESSED_ASM) $@ $(_PDSASM_LABEL)

$(MODULE_H_SIZEOF_PDS): $(MODULE_ASSEMBLED_H)
	$(TOUCH) -c $@

$(MODULE_SIZEOF_H): $(MODULE_H_SIZEOF_PDS)
	$(CP) $< $@

$(MODULE_POSTPROCESSED_H): $(MODULE_ASSEMBLED_H)
	$(SED) -e 's/FORCE_INLINE \(.*\) static/FORCE_INLINE \1/g' $< >$@
