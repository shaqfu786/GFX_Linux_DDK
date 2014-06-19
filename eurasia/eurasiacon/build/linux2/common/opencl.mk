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
# $Revision: 1.4 $
# $Log: opencl.mk $

$(eval $(call TunableUserConfigMake,LLVM_BUILD,release))
$(eval $(call TunableUserConfigMake,LLVM_ASSERTS,))
$(eval $(call TunableUserConfigMake,LLVM_SOURCE_TARBALL,eurasiacon/external/llvm-2.8.tgz))
$(eval $(call TunableUserConfigMake,CLANG_SOURCE_TARBALL,eurasiacon/external/clang-2.8.tgz))
$(eval $(call TunableUserConfigMake,LLVM_SOURCE_DIR,))
