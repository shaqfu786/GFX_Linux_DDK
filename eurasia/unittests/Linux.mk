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
# $Log: Linux.mk $
#

modules := \
 services_test \
 sgx_blit_test \
 sgx_clipblit_test \
 sgx_flip_test \
 sgx_init_test \
 sgx_render_flip_test \
 sgx_unittest_utils

ut := $(TOP)/unittests

services_test_src := $(ut)/services_test/services_test.c
services_test_type := executable
services_test_target := services_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
services_test_includes := include4 hwdefs
services_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

sgx_flip_test_src := $(ut)/sgx_flip_test/sgx_flip_test.c
sgx_flip_test_type := executable
sgx_flip_test_target := sgx_flip_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_flip_test_includes := include4 hwdefs
sgx_flip_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

sgx_init_test_src := $(ut)/sgx_init_test/sgx_init_test.c
sgx_init_test_type := executable
sgx_init_test_target := sgx_init_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_init_test_includes := include4 hwdefs
sgx_init_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

sgx_unittest_utils_type := objects
sgx_unittest_utils_src := common/sgx_unittest_utils.c
sgx_unittest_utils_includes := include4 hwdefs

sgx_blit_test_src := $(ut)/sgx_blit_test/sgx_blit_test.c
sgx_blit_test_obj := \
 $(TARGET_INTERMEDIATES)/sgx_unittest_utils/sgx_unittest_utils.o
sgx_blit_test_type := executable
sgx_blit_test_target := sgx_blit_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_blit_test_includes := include4 hwdefs services4/include unittests/common
sgx_blit_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

sgx_clipblit_test_src := $(ut)/sgx_clipblit_test/sgx_clipblit_test.c
sgx_clipblit_test_obj := \
 $(TARGET_INTERMEDIATES)/sgx_unittest_utils/sgx_unittest_utils.o
sgx_clipblit_test_type := executable
sgx_clipblit_test_target := sgx_clipblit_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_clipblit_test_includes := include4 hwdefs services4/include unittests/common
sgx_clipblit_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

sgx_render_flip_test_src := \
 $(ut)/sgx_render_flip_test/sgx_render_flip_test.c
sgx_render_flip_test_obj := \
 $(TARGET_INTERMEDIATES)/sgx_unittest_utils/sgx_unittest_utils.o \
 $(call target-intermediates-of,pixfmts,sgxpixfmts.o) \
 $(call target-intermediates-of,pixevent,pixevent.o pixeventpbesetup.o) \
 $(call target-intermediates-of,pdsogles,pds.o) \
 $(call target-intermediates-of,usegen,usegen.o)
sgx_render_flip_test_type := executable
sgx_render_flip_test_target := sgx_render_flip_test_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_render_flip_test_includes := \
 include4 \
 hwdefs \
 unittests/common \
 codegen/pds \
 codegen/pixevent \
 codegen/usegen
sgx_render_flip_test_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)
sgx_render_flip_test_extlibs := m pthread
sgx_render_flip_test_cflags := -DPDS_BUILD_OPENGLES -DSRFT_SUPPORT_PTHREADS
