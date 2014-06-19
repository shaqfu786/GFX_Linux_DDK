# Copyright (C) Imagination Technologies Limited. All rights reserved.
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
# $Log: llvm.mk $

# Location of output cache tarballs. Ideally not nested under eurasia.
LLVM_BUILD_CACHES ?= eurasiacon/caches

ifneq ($(LLVM_BUILD_CACHES),$(XORG_BUILD_CACHES))
.SECONDARY: $(LLVM_BUILD_CACHES)
$(LLVM_BUILD_CACHES):
	$(MKDIR) -p $@
endif

# Location the cache package is extracted to
LLVM_OUT := $(TARGET_OUT)/llvm

# GCC may have already generated depends which point to files in
# LLVM_OUT. If this directory goes away, we need to make sure
# to re-extract it.
$(TARGET_OUT)/llvm/%: | $(LLVM_OUT) ;

# NOTE: The rule to build LLVM_OUT is in llvm_cache_tarball
