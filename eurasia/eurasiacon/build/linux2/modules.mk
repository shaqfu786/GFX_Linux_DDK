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
# $Revision: 1.4 $
# $Log: modules.mk $
#

# Bits for processing $(modules) after reading in each Linux.mk

#$(info ---- $(modules) ----)
$(call must-be-nonempty,modules)

$(foreach _m,$(modules),$(if $(filter $(_m),$(ALL_MODULES)),$(error In makefile $(THIS_MAKEFILE): Duplicate module $(_m) (first seen in $(INTERNAL_MAKEFILE_FOR_MODULE_$(_m))) listed in $$(modules)),$(eval $(call register-module,$(_m)))))

ALL_MODULES += $(modules)
