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
# $Log: prepare_tree.mk $
#

-include $(OUT)/config.mk.new

.PHONY: prepare_tree

-include eurasiacon/build/linux2/kbuild/external_tarball.mk

# If there's no external tarball, there's nothing to do
#
prepare_tree:

INTERNAL_INCLUDED_PREPARE_HEADERS :=
-include eurasiacon/build/linux2/prepare_headers.mk
ifneq ($(INTERNAL_INCLUDED_PREPARE_HEADERS),true)
missing_headers := $(strip $(shell test ! -e include4/pvrversion.h && echo true))
ifdef missing_headers
$(info )
$(info ** include4/pvrversion.h is missing, and cannot be rebuilt.)
$(info ** Cannot continue.)
$(info )
$(error Missing headers)
endif
endif
