ifeq ($(filter unittests,$(EXCLUDED_APIS)),)
 COMPONENTS += \
  services_test \
  sgx_flip_test sgx_init_test sgx_render_flip_test
 ifneq ($(filter opencl,$(COMPONENTS)),)
  COMPONENTS += ocl_unit_test
 endif
 ifeq ($(filter xorg,$(COMPONENTS)),)
  COMPONENTS += eglinfo$(apk)
  ifneq ($(filter opengles1,$(COMPONENTS)),)
   COMPONENTS += gles1test1$(apk)
   ifneq ($(GLES1_EXTENSION_EGL_IMAGE_EXTERNAL),1)
    COMPONENTS += gles1_texture_stream$(apk)
   endif
  endif
  ifneq ($(filter opengles2,$(COMPONENTS)),)
   COMPONENTS += gles2test1$(apk)
   ifneq ($(GLES2_EXTENSION_EGL_IMAGE_EXTERNAL),1)
    COMPONENTS += gles2_texture_stream$(apk)
   endif
  endif
  ifneq ($(filter openvg,$(COMPONENTS)),)
   COMPONENTS += ovg_unit_test
  endif
  ifneq ($(filter opengl,$(COMPONENTS)),)
   COMPONENTS += gltest2
  endif
  ifneq ($(filter ewslib,$(COMPONENTS)),)
   COMPONENTS += ews_test_swrender
   ifneq ($(filter opengles1,$(COMPONENTS)),)
    COMPONENTS += ews_test_gles1
   endif
   ifneq ($(filter opengles2,$(COMPONENTS)),)
    COMPONENTS += ews_test_gles2
   endif
  endif
 endif
endif
