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
# $Revision: 1.6 $
# $Log: flex_lexer.mk $
#

MODULE_TARGETS := $(addprefix $(MODULE_INTERMEDIATES_DIR)/,$($(THIS_MODULE)_src:.l=.l.c))

#$(info [$(THIS_MODULE)] $(Host_or_target) flex lexer: $(MODULE_TARGETS))

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_TARGETS): MODULE_FLEX_FLAGS := $(MODULE_FLEX_FLAGS)
$(MODULE_TARGETS): | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): $(MODULE_SOURCES) $(THIS_MAKEFILE)
	$(l-c-from-l)
