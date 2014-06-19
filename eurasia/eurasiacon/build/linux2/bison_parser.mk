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
# $Revision: 1.9 $
# $Log: bison_parser.mk $
#

# There should only be one source file
$(call one-word-only,MODULE_SOURCES)

MODULE_TARGETS := $(MODULE_INTERMEDIATES_DIR)/$(notdir $(MODULE_SOURCES:.y=.tab.c))

#$(info [$(THIS_MODULE)] Bison parser: $(MODULE_TARGETS))

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_TARGETS): MODULE_BISON_FLAGS := $(MODULE_BISON_FLAGS)
$(MODULE_TARGETS): | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): $(MODULE_SOURCES) $(THIS_MAKEFILE)
	$(tab-c-from-y)

# The tab-c-from-y recipe passes -d to bison, which causes it to create
# headers
$(MODULE_TARGETS:.c=.h): $(MODULE_TARGETS)
	$(TOUCH) -c $@
