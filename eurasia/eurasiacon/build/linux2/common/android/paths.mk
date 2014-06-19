# Copyright	(c) Imagination Technologies Limited. All rights reserved.
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
# $Log: $
#

TARGET_BUILD_TYPE ?= release

ifeq ($(TARGET_BUILD_TYPE),debug)
TARGET_ROOT := $(ANDROID_ROOT)/out/debug/target
else
TARGET_ROOT := $(ANDROID_ROOT)/out/target
endif

TOOLCHAIN ?= $(TARGET_ROOT)/product/$(TARGET_PRODUCT)/obj

LIBGCC := $(shell $(CROSS_COMPILE)gcc -print-libgcc-file-name)
