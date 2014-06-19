# Copyright	2011 Imagination Technologies Limited. All rights reserved.
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
# $Log: user-defs.mk $
#

# The purpose of this file is to keep User-only macros isolated
# from Both or Kernel macros. The prune.sh script, invoked by
# a build spreadsheet, will remove this file and all affected
# macro invocations.
#
# The purpose of this is to remove User-only options which may
# contain sensitive information, and are not required for the
# split GPL-KM/IMG-UM packages.
#

# Write out a user GNU make option.
#
define UserConfigMake
$$(shell echo "override $(1) := $(2)" >>$(CONFIG_MK).new)
endef

# Conditionally write out a user GNU make option
#
define TunableUserConfigMake
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
$$(eval $$(call UserConfigMake,$(1),$$($(1))))
endif
else
ifneq ($(2),)
$$(eval $$(call UserConfigMake,$(1),$(2)))
endif
endif
endef

# Write out a user-only config option
#
define UserConfigC
$$(shell echo "#define $(1) $(2)" >>$(CONFIG_H).new)
endef

# Conditionally write out a user-only config option
#
define TunableUserConfigC
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
ifeq ($$($(1)),1)
$$(eval $$(call UserConfigC,$(1),))
else
$$(eval $$(call UserConfigC,$(1),$$($(1))))
endif
endif
else
ifneq ($(2),)
ifeq ($(2),1)
$$(eval $$(call UserConfigC,$(1),))
else
$$(eval $$(call UserConfigC,$(1),$(2)))
endif
endif
endif
endef

# We don't currently support compiling either the host or target
# tools or libraries in 64-bit mode on any processor architecture.
# Detect if the user is trying to do this and warn about it. We'll
# try to set -m32 if this is detected, but it doesn't always work.
# In such a case, the user must set CC, CXX or HOST_CC explicitly.

CC_CHECK	:= $(TOP)/eurasiacon/build/linux2/tools/cc-check.sh
CHMOD		:= chmod

CC			:= $(CROSS_COMPILE)gcc
CXX			:= $(CROSS_COMPILE)g++
HOST_CC		?= gcc

ifeq ($(call cc-is-64bit,$(CC)),true)
$(warning $$(CC) generates 64-bit code, which is not supported. Trying to work around.)
override CC := $(CC) $(call cc-option,-m32)
$(eval $(call UserConfigMake,CC,$(CC)))
endif
ifeq ($(call cc-is-64bit,$(CXX)),true)
$(warning $$(CXX) generates 64-bit code, which is not supported. Trying to work around.)
override CXX := $(CXX) $(call cxx-option,-m32)
$(eval $(call UserConfigMake,CXX,$(CXX)))
endif
ifeq ($(call cc-is-64bit,$(HOST_CC)),true)
$(warning $$(HOST_CC) generates 64-bit code, which is not supported. Trying to work around.)
override HOST_CC := $(HOST_CC) $(call host-cc-option,-m32)
$(eval $(call UserConfigMake,HOST_CC,$(HOST_CC)))
endif

# Workaround our lack of support for non-Linux HOST_CCs

HOST_CC_IS_LINUX := $(shell \
 $(HOST_CC) -dM -E - </dev/null | grep -q __linux__ || echo false)

ifeq ($(HOST_CC_IS_LINUX),false)
$(warning $$(HOST_CC) is non-Linux. Trying to work around.)
override HOST_CC := $(HOST_CC) -D__linux__
$(eval $(call UserConfigMake,HOST_CC,$(HOST_CC)))
endif
