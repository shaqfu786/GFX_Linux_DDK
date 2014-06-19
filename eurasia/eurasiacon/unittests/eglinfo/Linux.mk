########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Strictly Confidential.
### ###########################################################################

modules := eglinfo eglinfo_android xeglinfo

eglinfo_src := \
 ex_linux.c \
 eglinfo.c \
 ex_gl.c \
 ex_gles.c \
 ex_gles2.c

eglinfo_type := executable
eglinfo_target := eglinfo
eglinfo_includes := eurasiacon/unittests/include
eglinfo_libs := EGL
eglinfo_extlibs := dl

eglinfo_android_type := shared_library
eglinfo_android_target := libeglinfo.so
eglinfo_android_includes := $(UNITTEST_INCLUDES)
eglinfo_android_extlibs := EGL dl
eglinfo_android_src := $(eglinfo_src)

xeglinfo_type := executable
xeglinfo_target := xeglinfo
xeglinfo_cflags := -DSUPPORT_XORG
xeglinfo_includes := \
 eurasiacon/unittests/include
xeglinfo_libs := EGL
xeglinfo_extlibs := dl
xeglinfo_src := $(eglinfo_src)
xeglinfo_packages := x11

$(addprefix $(TARGET_INTERMEDIATES)/xeglinfo/,\
 $(foreach _o,$(patsubst %.c,%.o,$(xeglinfo_src)),$(notdir $(_o)))): \
  $(TARGET_OUT)/.$(libX11)
