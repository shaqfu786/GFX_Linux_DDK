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
# $Revision: 1.12 $
# $Log: apk.mk $
#

MODULE_TARGETS := $(addprefix $(MODULE_OUT)/,$(if $($(THIS_MODULE)_target),$($(THIS_MODULE)_target),$(THIS_MODULE).apk))
#$(info [$(THIS_MODULE)] $(Host_or_target) APK: $(MODULE_TARGETS))

$(call target-build-only,APK)

MODULE_BUILT_CLASSES := $(patsubst %,$(MODULE_OUT)/%.jar,$($(THIS_MODULE)_classes))
MODULE_BUILT_NATIVE := $(patsubst %,$(MODULE_OUT)/lib%.so,$($(THIS_MODULE)_native))

define target-apk-dex-from-classes
$(if $(V),,@echo "  DX      " $(call relative-to-top,$(MODULE_INTERMEDIATES_DIR)/classes.dex))
$(DX) --dex --output=$(MODULE_INTERMEDIATES_DIR)/classes.dex $(MODULE_BUILT_CLASSES)
endef

# We use $(abspath $@) in the rule below because it's run while cded into
# another directory. Using abspath instead of adding $(TOP) is necessary to
# handle setting $(OUT) to somewhere outside $(TOP)
define target-apk-natives
@: $(if $(JNI_CPU_ABI),,$(error JNI_CPU_ABI must be set for APK targets))
$(MKDIR) -p $(MODULE_INTERMEDIATES_DIR)/lib/${JNI_CPU_ABI}/
$(CP) -f $(MODULE_BUILT_NATIVE) $(MODULE_INTERMEDIATES_DIR)/lib/${JNI_CPU_ABI}/
$(if $(V),,@)pwd=$$PWD && \
	cd $(MODULE_INTERMEDIATES_DIR) && \
	$(patsubst @%,%,$(ZIP)) -qr package.zip lib && \
	$(patsubst @%,%,$(MV)) -f package.zip $(abspath $@) && \
	cd $$pwd
endef

define target-apk-update-dex-sign-align
$(AAPT) add -k $@ $(MODULE_INTERMEDIATES_DIR)/classes.dex >/dev/null
$(if $(V),,@echo "  SIGN    " $(call relative-to-top,$@))
$(MV) -f $@ $@.unsigned
$(JAVA) -jar $(APK_SIGNAPK) $(APK_TESTKEYPEM) $(APK_TESTKEYPK8) $@.unsigned $@
$(RM) $@.unsigned
$(if $(V),,@echo "  ALIGN   " $(call relative-to-top,$@))
$(MV) -f $@ $@.unaligned
$(ZIPALIGN) -f 4 $@.unaligned $@
$(RM) $@.unaligned
endef

override APK_VERSION := \
 --min-sdk-version $(API_LEVEL) --target-sdk-version $(API_LEVEL) \
 --version-code $(API_LEVEL) --version-name 2.0
override APK_INCLUDE	:= $(TARGET_ROOT)/common/obj/APPS/framework-res_intermediates/package-export.apk
override APK_TESTKEYPEM	:= $(ANDROID_ROOT)/build/target/product/security/testkey.x509.pem
override APK_TESTKEYPK8	:= $(ANDROID_ROOT)/build/target/product/security/testkey.pk8
override APK_SIGNAPK	:= $(ANDROID_ROOT)/out/host/$(HOST_OS)-$(HOST_ARCH)/framework/signapk.jar
override APK_BINDIR		:= $(ANDROID_ROOT)/out/host/$(HOST_OS)-$(HOST_ARCH)/bin

override AAPT			:= $(if $(V),,@)$(APK_BINDIR)/aapt
override DX				:= $(if $(V),,@)$(APK_BINDIR)/dx
override ZIPALIGN		:= $(if $(V),,@)$(APK_BINDIR)/zipalign

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

$(MODULE_TARGETS): MODULE_INTERMEDIATES_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): MODULE_BUILT_CLASSES := $(MODULE_BUILT_CLASSES)
$(MODULE_TARGETS): MODULE_BUILT_NATIVE := $(MODULE_BUILT_NATIVE)
$(MODULE_TARGETS): $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): $(MODULE_SOURCES) $(MODULE_BUILT_CLASSES) $(MODULE_BUILT_NATIVE)
	$(target-apk-dex-from-classes)
ifneq ($(MODULE_BUILT_NATIVE),)
	$(target-apk-natives)
endif
	$(if $(V),,@echo "  AAPT    " $(call relative-to-top,$@))
	$(AAPT) package -f -u -z -c en_US,mdpi,nodpi -M $< -I $(APK_INCLUDE) $(APK_VERSION) -F $@
	$(target-apk-update-dex-sign-align)
