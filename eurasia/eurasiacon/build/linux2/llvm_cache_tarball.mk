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
# $Revision: 1.17 $
# $Log: llvm_cache_tarball.mk $
#

$(call target-build-only,llvm_cache_tarball)

ifeq ($(__LLVM_CACHE_CONFIGURED),1)
$(error There can be only one LLVM cache configuration)
endif
__LLVM_CACHE_CONFIGURED := 1

MODULE_LLVM_TARBALL := $($(THIS_MODULE)_llvm_tarball)
MODULE_LLVM_PATCHES := $($(THIS_MODULE)_llvm_patches)

MODULE_CLANG_TARBALL := $($(THIS_MODULE)_clang_tarball)
MODULE_CLANG_PATCHES := $($(THIS_MODULE)_clang_patches)

MODULE_CONFIGURE_FLAGS := $($(THIS_MODULE)_configure_flags)

MODULE_CROSS_TRIPLE := $(CROSS_TRIPLE)

# FIXME: If possible, move this out of here
ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
ifeq ($(MODULE_CROSS_TRIPLE),arm-eabi)
MODULE_CROSS_TRIPLE := arm-none-linux-gnueabi
MODULE_LLVM_CC := $(MODULE_CROSS_TRIPLE)-gcc -DANDROID $(SYS_CFLAGS)
MODULE_LLVM_CXX := $(MODULE_CROSS_TRIPLE)-g++ -DANDROID -fno-rtti -fno-exceptions $(SYS_CXXFLAGS)
endif
endif

MODULE_LLVM_CC ?= $(patsubst @%,%,$(CC))
MODULE_LLVM_CXX ?= $(patsubst @%,%,$(CXX))

ifneq ($(MODULE_CROSS_TRIPLE),)
MODULE_CONFIGURE_FLAGS += --host=$(MODULE_CROSS_TRIPLE) --target=$(MODULE_CROSS_TRIPLE)

# If the LLVM configure script isn't passed --build along with --host, it
# will compile a test program and try to run it. If the program runs, it
# disables cross-compilation. For some of our x86 builds, disabling cross
# compilation means the cross compiler will be used to build host tools
# (e.g. tblgen). The resulting tool will not run on the host.
#
# Force cross compilation by specifying --build.
#
MODULE_CONFIGURE_FLAGS += --build=$(shell $(patsubst @%,%,$(HOST_CC)) -dumpmachine)
endif

# Concatenate the following..
#
#  - List of USC source files symlinked into LLVM tree
#  - List of flags passed to `./configure' (includes triple for cross)
#  - Contents of LLVM and clang tarballs
#  - Contents of DDK patches to llvm/clang
#
# ..and pipe it through md5sum. The resulting hash is used to
# generate the filename for the cache. If any of the above things
# change, the cache will be regenerated, ensuring coherency.
#
LLVM_CACHE_FILE_NAME := $(LLVM_BUILD_CACHES)/llvm-$(shell \
	echo $(MODULE_CONFIGURE_FLAGS) | \
	cat - $(MODULE_SOURCES) \
		$(MODULE_LLVM_TARBALL) $(MODULE_LLVM_PATCHES) \
		$(MODULE_CLANG_TARBALL) $(MODULE_CLANG_PATCHES) | \
	md5sum -b - | \
	cut -d' ' -f1).tar.bz2

$(LLVM_OUT): $(LLVM_CACHE_FILE_NAME)
	$(MKDIR) -p $@
	$(TAR) -C $@ -jxf $<

MODULE_TARGETS := $(LLVM_CACHE_FILE_NAME)
#$(info [$(THIS_MODULE)] $(Host_or_target) llvm_cache_tarball: $(MODULE_TARGETS))

define llvm-extract-tarballs
$(RM) -r $(MODULE_INTERMEDIATES_DIR)/source
$(TAR) -C $(MODULE_INTERMEDIATES_DIR) -zxf $(MODULE_LLVM_TARBALL)
$(MV) $(MODULE_INTERMEDIATES_DIR)/$(basename $(notdir $(MODULE_LLVM_TARBALL))) $(MODULE_INTERMEDIATES_DIR)/source
$(TAR) -C $(MODULE_INTERMEDIATES_DIR)/source/tools -zxf $(MODULE_CLANG_TARBALL)
$(MV) $(MODULE_INTERMEDIATES_DIR)/source/tools/$(basename $(notdir $(MODULE_CLANG_TARBALL))) $(MODULE_INTERMEDIATES_DIR)/source/tools/clang
endef

define llvm-patch-source
$(if $(V),,@)for patch in $(addprefix $(TOP)/,$(MODULE_LLVM_PATCHES)); do \
	patch -d $(MODULE_INTERMEDIATES_DIR)/source -sNp0 -i $$patch; \
done
$(if $(V),,@)for patch in $(addprefix $(TOP)/,$(MODULE_CLANG_PATCHES)); do \
	patch -d $(MODULE_INTERMEDIATES_DIR)/source/tools/clang -sNp0 -i $$patch; \
done
endef

# FIXME: Remove the use of sed
define llvm-create-symlink-tree
$(if $(V),,@)for source in $(MODULE_SOURCES); do \
	source2=`echo $$source | sed -e 's,^tools/intern/oclcompiler/llvm/,,'` && \
	dir=$(MODULE_INTERMEDIATES_DIR)/source/`dirname $$source2` && \
	if [ ! -d $$dir ]; then mkdir -p $$dir; fi && \
	ln -sf $(TOP)/$$source $(MODULE_INTERMEDIATES_DIR)/source/$$source2; \
done
endef

define llvm-configure
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR) && \
	CC="$(MODULE_LLVM_CC)" CXX="$(MODULE_LLVM_CXX)" \
		source/configure $(MODULE_CONFIGURE_FLAGS) && \
	cd $$pwd
endef

# We can't use $(wildcard ) here because it's evaluated too early. But if
# we want to rely on the shell to do the expansion, we need to be in the
# right working directory. So we can't use tar -C either.
define llvm-tar-binaries-and-headers
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR) && \
	tar -cf $(TOP)/$(MODULE_INTERMEDIATES_DIR)/binaries.tar \
		 $(liboclcompiler_llvmbuild)/lib/*.a include tools/clang/include && \
	cd source && \
	tar -rf $(TOP)/$(MODULE_INTERMEDIATES_DIR)/binaries.tar \
		include tools/clang/include && \
	cd $$pwd
$(BZIP2) <$(MODULE_INTERMEDIATES_DIR)/binaries.tar >$@
$(RM) $(MODULE_INTERMEDIATES_DIR)/binaries.tar
endef

define llvm-apply-android-workarounds
$(SED) -i -e 's,$(MODULE_CROSS_TRIPLE)-,$(CROSS_TRIPLE)-,' $(MODULE_INTERMEDIATES_DIR)/Makefile.config
$(ECHO) "NO_PEDANTIC := 1" >>$(MODULE_INTERMEDIATES_DIR)/Makefile.config
$(SED) -i \
	-e '/HAVE_MKDTEMP/d' \
	-e '/HAVE_POSIX_SPAWN/d' \
	-e '/HAVE_EXECINFO_H/d' \
	-e '/HAVE_DLFCN_H/d' \
	-e '/HAVE_BACKTRACE/d' \
	-e 's,LLVM_MULTITHREADED 1,LLVM_MULTITHREADED 0,' \
	$(MODULE_INTERMEDIATES_DIR)/include/llvm/Config/config.h
endef

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

.DELETE_ON_ERROR: $(MODULE_TARGETS)
$(MODULE_TARGETS): MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): | $(LLVM_BUILD_CACHES)
$(MODULE_TARGETS): $(MODULE_INTERMEDIATES_DIR)/.configure
	$(MKDIR) -p $(MODULE_INTERMEDIATES_DIR)/tools/clang/
	$(CP) $(MODULE_INTERMEDIATES_DIR)/source/tools/clang/Makefile $(MODULE_INTERMEDIATES_DIR)/tools/clang/Makefile
	TOP=$(TOP) $(MAKE) $(if $(V),VERBOSE=1,) SUPPORT_OPENCL_11=$(SUPPORT_OPENCL_11) -C $(MODULE_INTERMEDIATES_DIR) llvm-and-clang-libs
	$(llvm-tar-binaries-and-headers)

ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
$(MODULE_TARGETS): $(MODULE_INTERMEDIATES_DIR)/.android_wa

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.android_wa
$(MODULE_INTERMEDIATES_DIR)/.android_wa: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.android_wa: MODULE_CROSS_TRIPLE := $(MODULE_CROSS_TRIPLE)
$(MODULE_INTERMEDIATES_DIR)/.android_wa: $(MODULE_INTERMEDIATES_DIR)/.configure
	$(llvm-apply-android-workarounds)
	$(TOUCH) $@
endif

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.configure
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_CONFIGURE_FLAGS := $(MODULE_CONFIGURE_FLAGS)
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_LLVM_CC := $(MODULE_LLVM_CC)
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_LLVM_CXX := $(MODULE_LLVM_CXX)
$(MODULE_INTERMEDIATES_DIR)/.configure: | $(MODULE_INTERMEDIATES_DIR)/.symlink-tree
	$(llvm-configure)
	$(TOUCH) $@

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.symlink-tree
$(MODULE_INTERMEDIATES_DIR)/.symlink-tree: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.symlink-tree: MODULE_SOURCES := $(MODULE_SOURCES)
$(MODULE_INTERMEDIATES_DIR)/.symlink-tree: $(MODULE_SOURCES)
$(MODULE_INTERMEDIATES_DIR)/.symlink-tree: $(MODULE_INTERMEDIATES_DIR)/.extract
	$(llvm-create-symlink-tree)
	$(TOUCH) $@

# We don't separate extraction + patching because if either the tarballs
# or patches are touched, we need to freshly repeat both steps.
#
# If LLVM_SOURCE_DIR is non-empty, ignore the stock tarballs and just
# create a source symlink. (This method defeats caching and should only be
# used for development.)
#
.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.extract
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_LLVM_TARBALL := $(MODULE_LLVM_TARBALL)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_LLVM_PATCHES := $(MODULE_LLVM_PATCHES)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(MODULE_LLVM_TARBALL)  $(MODULE_LLVM_PATCHES)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_CLANG_TARBALL := $(MODULE_CLANG_TARBALL)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_CLANG_PATCHES := $(MODULE_CLANG_PATCHES)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(MODULE_CLANG_TARBALL) $(MODULE_CLANG_PATCHES)
ifeq ($(LLVM_SOURCE_DIR),)
	$(llvm-extract-tarballs)
	$(llvm-patch-source)
else
	$(if $(V),,@)ln -sf $(LLVM_SOURCE_DIR) $(MODULE_INTERMEDIATES_DIR)/source
endif
	$(TOUCH) $@
