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

OPTIM := -Os

SYS_CFLAGS := \
 -march=armv7-a -fno-short-enums -D__linux__ \
 -I$(ANDROID_ROOT)/bionic/libc/arch-arm/include \
 -I$(ANDROID_ROOT)/bionic/libc/include \
 -I$(ANDROID_ROOT)/bionic/libc/kernel/common \
 -I$(ANDROID_ROOT)/bionic/libc/kernel/arch-arm \
 -I$(ANDROID_ROOT)/bionic/libm/include \
 -I$(ANDROID_ROOT)/bionic/libm/include/arm \
 -I$(ANDROID_ROOT)/bionic/libthread_db/include \
 -I$(ANDROID_ROOT)/frameworks/base/include \
 -isystem $(ANDROID_ROOT)/system/core/include \
 -I$(ANDROID_ROOT)/hardware/libhardware/include \
 -I$(ANDROID_ROOT)/external/openssl/include

SYS_EXE_CRTBEGIN := $(TOOLCHAIN)/lib/crtbegin_dynamic.o
SYS_EXE_CRTEND := $(TOOLCHAIN)/lib/crtend_android.o
SYS_EXE_LDFLAGS := \
 -Bdynamic -Wl,-T$(ANDROID_ROOT)/build/core/armelf.x \
 -nostdlib -Wl,-dynamic-linker,/system/bin/linker \
 -lc -ldl -lcutils

SYS_LIB_LDFLAGS := \
 -Bdynamic -Wl,-T$(ANDROID_ROOT)/build/core/armelf.xsc \
 -nostdlib -Wl,-dynamic-linker,/system/bin/linker \
 -lc -ldl -lcutils

JNI_CPU_ABI := armeabi

# Android builds are usually GPL
#
LDM_PLATFORM ?= 1
