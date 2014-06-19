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
# $Log: this_makefile.mk $
#

# Find out the path of the Linux.mk makefile currently being processed, and
# set paths used by the build rules

# This magic is used so we can use this_makefile.mk twice: first when reading
# in each Linux.mk, and then again when generating rules. There we set
# $(THIS_MAKEFILE), and $(REMAINING_MAKEFILES) should be empty
ifneq ($(strip $(REMAINING_MAKEFILES)),)

# Absolute path to the Linux.mk being processed
THIS_MAKEFILE := $(firstword $(REMAINING_MAKEFILES))

# The list of makefiles left to process
REMAINING_MAKEFILES := $(wordlist 2,$(words $(REMAINING_MAKEFILES)),$(REMAINING_MAKEFILES))

else

# When generating rules, we should have read in every Linux.mk
$(if $(INTERNAL_INCLUDED_ALL_MAKEFILES),,$(error No makefiles left in $$(REMAINING_MAKEFILES), but $$(INTERNAL_INCLUDED_ALL_MAKEFILES) is not set))

endif

# Path to the directory containing Linux.mk
THIS_DIR := $(patsubst %/,%,$(dir $(THIS_MAKEFILE)))
ifeq ($(strip $(THIS_DIR)),)
$(error Empty $$(THIS_DIR) for makefile "$(THIS_MAKEFILE)")
endif

modules :=
