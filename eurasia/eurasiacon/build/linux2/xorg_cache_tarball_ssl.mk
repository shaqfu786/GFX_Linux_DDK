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
# $Revision: 1.5 $
# $Log: xorg_cache_tarball_ssl.mk $
#

$(call target-build-only,xorg_cache_tarball_ssl)

MODULE_TARBALL := $($(THIS_MODULE)_tarball)

MODULE_DEPENDS := $(foreach _d,$($(THIS_MODULE)_depends),$(TARGET_OUT)/.$(_d))

MODULE_WORKAROUND_OPENSSL := $(if $($(THIS_MODULE)_workaround_openssl),true,)

MODULE_BYPASS_CACHE := $(if $($(THIS_MODULE)_bypass_cache),true,)

# Concatenate the following..
#
#  - Value of ALL_XORG_CFLAGS (effectively debug vs release)
#  - CROSS_TRIPLE (because we don't pass a triple to configure)
#  - Contents of source tarball
#
# ..and pipe it through md5sum. The resulting hash is used to
# generate the filename for the cache. If any of the above things
# change, the cache will be regenerated, ensuring coherency.
#
XORG_CACHE_FILE_NAME := $(XORG_BUILD_CACHES)/$(THIS_MODULE)-$(shell \
	echo $(ALL_XORG_CFLAGS) $(CROSS_TRIPLE) | \
	cat - $(MODULE_TARBALL) | \
	md5sum -b - | \
	cut -d' ' -f1).tar.bz2

MODULE_TARGETS := $(TARGET_OUT)/.$(THIS_MODULE)

#$(info [$(THIS_MODULE)] $(Host_or_target) xorg_cache_tarball_ssl: $(MODULE_TARGETS))

define ssl-configure
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR) && \
		MAKEFLAGS="$(filter-out -Rr -j TOP=$(TOP),$(MAKEFLAGS))" \
			./Configure --prefix=$(XORG_PREFIX) linux-generic32 && \
		sed -i -e 's,-O3 -fomit-frame-pointer,$(ALL_XORG_CFLAGS) -fPIC,' Makefile && \
	cd $$pwd
endef

define ssl-install
$(ssl-make) INSTALL_PREFIX="$(MODULE_DESTDIR)" install_sw
$(MKDIR) -p $(MODULE_DESTDIR)/xorg_pkgconfig
$(if $(V),,@)for PC in $(MODULE_DESTDIR)$(XORG_PREFIX)/lib/pkgconfig/*.pc \
					   $(MODULE_DESTDIR)$(XORG_PREFIX)/share/pkgconfig/*.pc; do \
		[ ! -f $$PC ] && continue; \
		sed -e 's,$(XORG_PREFIX),$${pc_top_builddir}$(XORG_PREFIX),' \
			$$PC >$(MODULE_DESTDIR)/xorg_pkgconfig/`basename $$PC`; \
	done
endef

define ssl-make
$(if $(V),,@)+MAKEFLAGS="$(filter-out -Rr -j TOP=$(TOP),$(MAKEFLAGS))" \
	$(MAKE) -C $(MODULE_INTERMEDIATES_DIR) \
	CC="$(patsubst @%,%,$(CC))" \
	AR="$(patsubst @%,%,$(AR)) r" \
	RANLIB="$(patsubst @%,%,$(RANLIB))"
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
$(XORG_CACHE_FILE_NAME): | $(XORG_BUILD_CACHES)
$(XORG_CACHE_FILE_NAME): $(MODULE_INTERMEDIATES_DIR)/.configure
	$(ssl-make)
ifeq ($(MODULE_WORKAROUND_OPENSSL),true)
	$(TOUCH) $(MODULE_INTERMEDIATES_DIR)/fips/fipscanister.o
	$(TOUCH) $(MODULE_INTERMEDIATES_DIR)/fips/fipscanister.o.sha1
endif
	$(ssl-install)
ifeq ($(MODULE_WORKAROUND_OPENSSL),true)
	$(RM) $(MODULE_DESTDIR)$(XORG_PREFIX)/lib/fipscanister.o
	$(RM) $(MODULE_DESTDIR)$(XORG_PREFIX)/lib/fipscanister.o.sha1
endif
	$(TAR) -C $(MODULE_DESTDIR) -jcf $@ .

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.configure
$(MODULE_INTERMEDIATES_DIR)/.configure: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.configure: | $(MODULE_DEPENDS)
$(MODULE_INTERMEDIATES_DIR)/.configure: $(MODULE_INTERMEDIATES_DIR)/.extract
	$(ssl-configure)
	$(TOUCH) $@

# We don't separate extraction + patching because if either the tarballs
# or patches are touched, we need to freshly repeat both steps.
#
.SECONDARY: $(MODULE_INTERMEDIATES_DIR)/.extract
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: MODULE_TARBALL := $(MODULE_TARBALL)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.extract: $(MODULE_TARBALL)
	$(TAR) --strip-components 1 -C $(MODULE_INTERMEDIATES_DIR) -xf $(MODULE_TARBALL)
	$(TOUCH) $@
