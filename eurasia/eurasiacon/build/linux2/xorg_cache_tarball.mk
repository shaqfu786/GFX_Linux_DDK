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
# $Revision: 1.11 $
# $Log: xorg_cache_tarball.mk $
#

$(call target-build-only,xorg_cache_tarball)

MODULE_TARBALL := $($(THIS_MODULE)_tarball)
MODULE_PATCHES := $($(THIS_MODULE)_patches)

MODULE_DEPENDS := $(foreach _d,$($(THIS_MODULE)_depends),$(TARGET_OUT)/.$(_d))

MODULE_CONFIGURE_FLAGS := $($(THIS_MODULE)_configure_flags)
MODULE_AUTORECONF := $(if $($(THIS_MODULE)_autoreconf),true,)
MODULE_INTLTOOLIZE := $(if $($(THIS_MODULE)_intltoolize),true,)

MODULE_WORKAROUND_FREETYPE := $(if $($(THIS_MODULE)_workaround_freetype),true,)

MODULE_BYPASS_CACHE := $(if $($(THIS_MODULE)_bypass_cache),true,)

# We don't want to pollute the X components with too many
# of our DDK LDFLAGS, so locally redefine it here.
#
MODULE_LDFLAGS := \
 -L$(abspath $(XORG_OUT))/lib \
 -Wl,-rpath-link,$(abspath $(XORG_OUT))/lib \
 -Wl,-rpath,$(XORG_PREFIX)/lib

MODULE_CROSS_TRIPLE := $(CROSS_TRIPLE)

ifneq ($(MODULE_CROSS_TRIPLE),)
MODULE_CONFIGURE_FLAGS += --host=$(MODULE_CROSS_TRIPLE) --target=$(MODULE_CROSS_TRIPLE)
MODULE_CONFIGURE_FLAGS += --build=$(shell $(patsubst @%,%,$(HOST_CC)) -dumpmachine)
endif

# Concatenate the following..
#
#  - List of flags passed to `./configure' (includes triple for cross)
#  - Value of ALL_XORG_CFLAGS (effectively debug vs release)
#  - Value of ALL_XORG_CXXFLAGS
#  - Contents of source tarball
#  - Contents of DDK patches to sources
#
# ..and pipe it through md5sum. The resulting hash is used to
# generate the filename for the cache. If any of the above things
# change, the cache will be regenerated, ensuring coherency.
#
# FIXME: Ideally we'd hash MODULE_LDFLAGS here too, but we can't because
#        it refer to paths (XORG_OUT) that depend on the location of OUT.
#
XORG_CACHE_FILE_NAME := $(XORG_BUILD_CACHES)/$(THIS_MODULE)-$(shell \
	echo $(MODULE_CONFIGURE_FLAGS) $(ALL_XORG_CFLAGS) $(ALL_XORG_CXXFLAGS) | \
	cat - $(MODULE_TARBALL) $(MODULE_PATCHES) | \
	md5sum -b - | \
	cut -d' ' -f1).tar.bz2

MODULE_TARGETS := $(TARGET_OUT)/.$(THIS_MODULE)

#$(info [$(THIS_MODULE)] $(Host_or_target) xorg_cache_tarball: $(MODULE_TARGETS))

define xorg-extract-tarball
$(RM) -r $(MODULE_INTERMEDIATES_DIR)/source
$(TAR) -C $(MODULE_INTERMEDIATES_DIR) -xf $(MODULE_TARBALL)
$(MV) $(MODULE_INTERMEDIATES_DIR)/$(patsubst %.tar.gz,%,$(patsubst %.tar.bz2,%,$(notdir $(MODULE_TARBALL)))) $(MODULE_INTERMEDIATES_DIR)/source
endef

define xorg-patch-source
$(if $(V),,@)for patch in $(addprefix $(TOP)/,$(MODULE_PATCHES)); do \
	patch -d $(MODULE_INTERMEDIATES_DIR)/source -sNp2 -i $$patch; \
done
endef

define xorg-autoreconf
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR)/source && \
		autoreconf --force --verbose --install && \
	cd $$pwd
endef

define xorg-intltoolize
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR)/source && \
		intltoolize && \
	cd $$pwd
endef

define xorg-configure
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR) && \
		CC="$(patsubst @%,%,$(CC)) $(ALL_XORG_CFLAGS)" \
		CXX="$(patsubst @%,%,$(CXX)) $(ALL_XORG_CXXFLAGS)" \
		LDFLAGS="$(MODULE_LDFLAGS)" \
		CFLAGS="" \
		CXXFLAGS="" \
		source/configure \
			--prefix=$(XORG_PREFIX) \
			$(MODULE_CONFIGURE_FLAGS) && \
	cd $$pwd
endef

define xorg-install
$(xorg-make) DESTDIR="$(MODULE_DESTDIR)" install
$(if $(V),,@)( find $(MODULE_DESTDIR) -name '*.la' -exec rm -f '{}' ';' || /bin/true )
$(MKDIR) -p $(MODULE_DESTDIR)/xorg_pkgconfig
$(if $(V),,@)for PC in $(MODULE_DESTDIR)$(XORG_PREFIX)/lib/pkgconfig/*.pc \
					   $(MODULE_DESTDIR)$(XORG_PREFIX)/share/pkgconfig/*.pc; do \
		[ ! -f $$PC ] && continue; \
		sed -e '/^appdefaultdir=.*/ !s,$(XORG_PREFIX),$${pc_top_builddir}$(XORG_PREFIX),' \
			$$PC >$(MODULE_DESTDIR)/xorg_pkgconfig/`basename $$PC`; \
	done
endef

define xorg-make
$(if $(V),,@)+MAKEFLAGS="$(filter-out -Rr TOP=$(TOP),$(MAKEFLAGS))" \
	$(MAKE) -C $(MODULE_INTERMEDIATES_DIR) LDFLAGS="$(MODULE_LDFLAGS)"
endef

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_TARGETS): XORG_CACHE_FILE_NAME := $(XORG_CACHE_FILE_NAME)
$(MODULE_TARGETS): $(XORG_CACHE_FILE_NAME) | $(MODULE_DEPENDS) $(TARGET_OUT)
	@: $(if $(filter j3.81,$(findstring j,$(MAKEFLAGS))$(MAKE_VERSION)),\
		$(error This makefile triggers bug #15919 in GNU Make 3.81 \
				when parallel make (-j) is used. Please upgrade \
				to GNU Make >= 3.82, or switch off parallel make))
	$(TAR) -C $(TARGET_OUT) -jxf $(XORG_CACHE_FILE_NAME)
	$(TOUCH) $@

ifeq ($(MODULE_BYPASS_CACHE),true)
# Marking XORG_CACHE_FILE_NAME as phony will cause it to always
# be rebuilt. This acts as an xorg cache bypass and is useful
# for development.
#
.PHONY: $(XORG_CACHE_FILE_NAME)
endif

.DELETE_ON_ERROR: $(XORG_CACHE_FILE_NAME)
$(XORG_CACHE_FILE_NAME): MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(XORG_CACHE_FILE_NAME): MODULE_DESTDIR := $(abspath $(MODULE_INTERMEDIATES_DIR))/.build
$(XORG_CACHE_FILE_NAME): MODULE_LDFLAGS := $(MODULE_LDFLAGS)
$(XORG_CACHE_FILE_NAME): | $(XORG_BUILD_CACHES)
$(XORG_CACHE_FILE_NAME): $(MODULE_INTERMEDIATES_DIR)/.configure
	$(xorg-make)
ifeq ($(MODULE_WORKAROUND_FREETYPE),true)
	$(MKDIR) -p $(MODULE_DESTDIR)$(XORG_PREFIX)/include/freetype2/freetype/internal
endif
	$(xorg-install)
	$(TAR) -C $(MODULE_DESTDIR) -jcf $@ .

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.configure
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_CONFIGURE_FLAGS := $(MODULE_CONFIGURE_FLAGS)
$(MODULE_INTERMEDIATES_DIR)/.configure: | $(MODULE_DEPENDS)
ifeq ($(MODULE_AUTORECONF),true)
$(MODULE_INTERMEDIATES_DIR)/.configure: $(MODULE_INTERMEDIATES_DIR)/.autoreconf
else
$(MODULE_INTERMEDIATES_DIR)/.configure: $(MODULE_INTERMEDIATES_DIR)/.extract
endif
	$(xorg-configure)
	$(TOUCH) $@

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.autoreconf
$(MODULE_INTERMEDIATES_DIR)/.autoreconf: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.autoreconf: | $(MODULE_DEPENDS) $(XORG_ACLOCAL_OUT)
ifeq ($(MODULE_INTLTOOLIZE),true)
$(MODULE_INTERMEDIATES_DIR)/.autoreconf: $(MODULE_INTERMEDIATES_DIR)/.intltoolize
else
$(MODULE_INTERMEDIATES_DIR)/.autoreconf: $(MODULE_INTERMEDIATES_DIR)/.extract
endif
	$(xorg-autoreconf)
	$(TOUCH) $@

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.intltoolize
$(MODULE_INTERMEDIATES_DIR)/.intltoolize: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.intltoolize: | $(MODULE_DEPENDS)
$(MODULE_INTERMEDIATES_DIR)/.intltoolize: $(MODULE_INTERMEDIATES_DIR)/.extract
	$(xorg-intltoolize)
	$(TOUCH) $@

# We don't separate extraction + patching because if either the tarball
# or patches are touched, we need to freshly repeat both steps.
#
.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.extract
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_TARBALL := $(MODULE_TARBALL)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_PATCHES := $(MODULE_PATCHES)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(MODULE_TARBALL) $(MODULE_PATCHES)
	$(xorg-extract-tarball)
	$(xorg-patch-source)
	$(TOUCH) $@
