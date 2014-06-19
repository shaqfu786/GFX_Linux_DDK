ifneq ($(is_at_least_icecream_sandwich),1)
-include ../common/apis/opencl.mk
endif
include ../common/android/components.mk
COMPONENTS += inifiles_omap4
