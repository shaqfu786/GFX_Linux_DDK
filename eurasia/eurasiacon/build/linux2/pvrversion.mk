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
# $Log: pvrversion.mk $


# Version information
PVRVERSION_H	:= $(TOP)/include4/pvrversion.h
 
# scripts.mk uses these to set the install script's version suffix
PVRVERSION_MAJ        := $(shell perl -ne '/\sPVRVERSION_MAJ\s+(\w+)/          and print $$1' $(PVRVERSION_H))
PVRVERSION_MIN        := $(shell perl -ne '/\sPVRVERSION_MIN\s+(\w+)/          and print $$1' $(PVRVERSION_H))
PVRVERSION_FAMILY     := $(shell perl -ne '/\sPVRVERSION_FAMILY\s+"(\S+)"/     and print $$1' $(PVRVERSION_H))
PVRVERSION_BRANCHNAME := $(shell perl -ne '/\sPVRVERSION_BRANCHNAME\s+"(\S+)"/ and print $$1' $(PVRVERSION_H))
PVRVERSION_BUILD      := $(shell perl -ne '/\sPVRVERSION_BUILD\s+(\w+)/        and print $$1' $(PVRVERSION_H))

PVRVERSION := "${PVRVERSION_FAMILY}_${PVRVERSION_BRANCHNAME}\@${PVRVERSION_BUILD}"
