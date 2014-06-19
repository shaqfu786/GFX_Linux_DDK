ifeq ($(TARGET_BOARD_PLATFORM),omap4)
ifneq ($(BOARD_USES_PREBUILT_SGX),true)

LOCAL_PATH := $(call my-dir)
FROM_LOCAL_TO_TOP := ../../..
GFX_LOCAL_PATH := $(LOCAL_PATH)# This is necessary to be used in the target rule recipe.
GFX_INTERMEDIATES := $(PRODUCT_OUT)/target
GFX_DOS2UNIX := $(ANDROID_BUILD_TOP)/$(GFX_LOCAL_PATH)/dos2unix

GFX_STRIP_MODULE := false # or true
GFX_BUILD := release	# choose one of "debug, release and timing"
GFX_BUILD_COMMAND := \
	PATH=$(PATH):$(dir $(GFX_DOS2UNIX)) \
	ANDROID_ROOT=$(ANDROID_BUILD_TOP) \
	KERNELDIR=$(ANDROID_BUILD_TOP)/$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ \
	OUT=$(ANDROID_BUILD_TOP)/$(PRODUCT_OUT) \
	./build_DDK.sh --build $(GFX_BUILD)

GFX_PREREQUISITES := \
	crtend_android.o\
	crtbegin_dynamic.o\
	crtbegin_static.o\
	libc$(TARGET_SHLIB_SUFFIX) \
	libcutils$(TARGET_SHLIB_SUFFIX)\
	libhardware$(TARGET_SHLIB_SUFFIX)\
	libGLESv1_CM$(TARGET_SHLIB_SUFFIX)\
	libGLESv2$(TARGET_SHLIB_SUFFIX)

GFX_PREREQUISITES := $(addprefix $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/, $(GFX_PREREQUISITES)) \
	$(TARGET_OUT_COMMON_INTERMEDIATES)/JAVA_LIBRARIES/android_stubs_current_intermediates/classes.jar

.PHONY: gfx cgfx
gfx: $(GFX_PREREQUISITES) kernel
	cd $(GFX_LOCAL_PATH) && $(GFX_BUILD_COMMAND)
cgfx:
	rm -rf $(GFX_INTERMEDIATES)

GFX_MODULES_TO_INSTALL :=	\
	vendor/lib/libpvrANDROID_WSEGL_SGX540_120.so \
	vendor/lib/libIMGegl_SGX540_120.so \
	vendor/lib/hw/gralloc.omap4.so \
	vendor/lib/libPVRScopeServices_SGX540_120.so \
	vendor/lib/libglslcompiler_SGX540_120.so \
	vendor/lib/libsrv_init_SGX540_120.so \
	vendor/lib/libusc_SGX540_120.so \
	vendor/lib/libsrv_um_SGX540_120.so \
	vendor/lib/libpvr2d_SGX540_120.so \
	vendor/lib/egl/libGLESv1_CM_POWERVR_SGX540_120.so \
	vendor/lib/egl/libGLESv2_POWERVR_SGX540_120.so \
	vendor/lib/egl/libEGL_POWERVR_SGX540_120.so \
	vendor/bin/pvrsrvinit \
	vendor/bin/pvrsrvinit_SGX540_120 \
	modules/pvrsrvkm_sgx540_120.ko \
	modules/omaplfb_sgx540_120.ko

built_sgx_modules :=
$(foreach gfx_module,$(GFX_MODULES_TO_INSTALL), \
  $(eval include $(CLEAR_VARS)) \
  $(eval LOCAL_MODULE := $(basename $(notdir $(gfx_module)))) \
  $(eval LOCAL_MODULE_TAGS := optional) \
  $(eval LOCAL_MODULE_SUFFIX := $(suffix $(gfx_module))) \
  $(eval LOCAL_MODULE_PATH := $(TARGET_OUT)/$(dir $(gfx_module))) \
  $(eval LOCAL_SRC_FILES := $(FROM_LOCAL_TO_TOP)/$(GFX_INTERMEDIATES)/$(notdir $(gfx_module))) \
  $(if $(filter %.so,$(LOCAL_MODULE)), \
    $(eval LOCAL_MODULE_CLASS := SHARED_LIBRARIES), \
    $(eval LOCAL_MODULE_CLASS := EXECUTABLES)) \
  $(eval LOCAL_STRIP_MODULE := $(GFX_STRIP_MODULE)) \
  $(eval include $(BUILD_PREBUILT)) \
  $(eval $(LOCAL_PATH)/$(LOCAL_SRC_FILES): gfx) \
  $(eval built_sgx_modules += $(LOCAL_MODULE)) \
 )

include $(CLEAR_VARS)
LOCAL_MODULE := ti_omap4_sgx_libs
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(built_sgx_modules)
include $(BUILD_PHONY_PACKAGE)

endif
endif
