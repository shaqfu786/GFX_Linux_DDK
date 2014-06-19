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

tq_pds := pds_tq_0src_primary pds_tq_1src_primary pds_tq_2src_primary \
          pds_tq_3src_primary \
          pds_iter_primary pds_secondary pds_dma_secondary \
          pds_1attr_secondary pds_3attr_secondary pds_4attr_secondary \
          pds_4attr_dma_secondary pds_5attr_dma_secondary pds_6attr_secondary \
          pds_7attr_secondary 
tq_use := transferqueue_use subtwiddled_eot

modules := libsrv_um $(tq_pds) $(tq_use)

all_tq_includes := include4 hwdefs

libsrv_um_type := shared_library
libsrv_um_src := \
 common/memchk.c \
 common/osfunc_um.c \
 common/osfunc_mutex.c \
 common/osfunc_mutexpg.c \
 common/pvr_apphint.c \
 common/pvr_bridge_u.c \
 common/pvr_debug.c \
 common/pvr_metrics.c \
 common/pvr_mmap.c \
 $(TOP)/services4/srvclient/bridged/bridged_pvr_glue.c \
 $(TOP)/services4/srvclient/bridged/bridged_pvr_bc_glue.c \
 $(TOP)/services4/srvclient/bridged/bridged_pvr_dc_glue.c \
 $(TOP)/services4/srvclient/bridged/bridged_pvr_pdump_glue.c \
 $(TOP)/services4/srvclient/common/resources.c \
 $(TOP)/services4/srvclient/common/graphics_buffer_interface_impl.c

libsrv_um_cflags := \
 -DPDS_BUILD_OPENGLES -DSRVUM_MODULE \
 $(if $(filter 1,$(W)),,-Werror)

libsrv_um_includes := \
 include4 hwdefs services4/include services4/include/env/linux \
 services4/system/$(PVR_SYSTEM) common/tls $(THIS_DIR) \
 codegen/pixevent codegen/pixfmts codegen/pds
libsrv_um_extlibs := dl pthread rt

ifeq ($(SUPPORT_SGX),1)
libsrv_um_src += \
 $(TOP)/services4/srvclient/bridged/sgx/bridged_sgx_glue.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxtransfer_context.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxtransfer_utils.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxtransfer_buffer.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxtransfer_queue.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxutils_client.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxkick_client.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxpb.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxrender_context.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxrender_targets.c \
 $(TOP)/services4/srvclient/devices/sgx/sgxtransfer_2d.c \

libsrv_um_includes += \
 $(TOP)/services4/srvclient/devices/sgx \
 $(TARGET_INTERMEDIATES)/libsrv_um \
 $(foreach _h,$(tq_pds) $(tq_use),$(TARGET_INTERMEDIATES)/$(_h))

libsrv_um_obj := $(call target-intermediates-of,pixfmts,sgxpixfmts.o) \
 $(call target-intermediates-of,pixevent,pixevent.o) \
 $(call target-intermediates-of,pdsogles,pds.o)

libsrv_um_src += \
 $(TOP)/services4/srvclient/common/blitlib_common.c \
 $(TOP)/services4/srvclient/common/blitlib_dst.c \
 $(TOP)/services4/srvclient/common/blitlib_op.c \
 $(TOP)/services4/srvclient/common/blitlib_pixel.c \
 $(TOP)/services4/srvclient/common/blitlib_src.c

libsrv_um_genheaders := \
 $(foreach _h,$(tq_pds) $(tq_use),$(_h)/$(_h).h) \
 $(foreach _h,$(tq_pds),$(_h)/$(_h)_sizeof.h) \
 $(foreach _h,$(tq_use),$(_h)/$(_h)_labels.h)

pds_tq_0src_primary_type := pds_header
pds_tq_0src_primary_src := $(TOP)/services4/srvclient/devices/sgx/pds_tq_primary.asm
pds_tq_0src_primary_cflags := -DTQ_PDS_PRIM_SRCS=0
pds_tq_0src_primary_label := TQ_KICKONLY
pds_tq_0src_primary_includes := $(all_tq_includes)

pds_tq_1src_primary_type := pds_header
pds_tq_1src_primary_src := $(TOP)/services4/srvclient/devices/sgx/pds_tq_primary.asm
pds_tq_1src_primary_cflags := -DTQ_PDS_PRIM_SRCS=1
pds_tq_1src_primary_label := TQ_1SRC
pds_tq_1src_primary_includes := $(all_tq_includes)

pds_tq_2src_primary_type := pds_header
pds_tq_2src_primary_src := $(TOP)/services4/srvclient/devices/sgx/pds_tq_primary.asm
pds_tq_2src_primary_cflags := -DTQ_PDS_PRIM_SRCS=2
pds_tq_2src_primary_label := TQ_2SRC
pds_tq_2src_primary_includes := $(all_tq_includes)

pds_tq_3src_primary_type := pds_header
pds_tq_3src_primary_src := $(TOP)/services4/srvclient/devices/sgx/pds_tq_primary.asm
pds_tq_3src_primary_cflags := -DTQ_PDS_PRIM_SRCS=3
pds_tq_3src_primary_label := TQ_3SRC
pds_tq_3src_primary_includes := $(all_tq_includes)

pds_iter_primary_type := pds_header
pds_iter_primary_src := $(TOP)/services4/srvclient/devices/sgx/pds_iter_primary.asm
pds_iter_primary_label := TQ_ITER
pds_iter_primary_includes := $(all_tq_includes)

pds_secondary_type := pds_header
pds_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_secondary_cflags := -DTQ_PDS_SEC_ATTR=0
pds_secondary_label := TQ_BASIC
pds_secondary_includes := $(all_tq_includes)

pds_dma_secondary_type := pds_header
pds_dma_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_dma_secondary_cflags := -DTQ_PDS_SEC_ATTR=0 -DTQ_PDS_SEC_DMA
pds_dma_secondary_label := TQ_DMA_ONLY
pds_dma_secondary_includes := $(all_tq_includes)

pds_1attr_secondary_type := pds_header
pds_1attr_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_1attr_secondary_cflags := -DTQ_PDS_SEC_ATTR=1
pds_1attr_secondary_label := TQ_1ATTR
pds_1attr_secondary_includes := $(all_tq_includes)

pds_3attr_secondary_type := pds_header
pds_3attr_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_3attr_secondary_cflags := -DTQ_PDS_SEC_ATTR=3
pds_3attr_secondary_label := TQ_3ATTR
pds_3attr_secondary_includes := $(all_tq_includes)

pds_4attr_secondary_type := pds_header
pds_4attr_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_4attr_secondary_cflags := -DTQ_PDS_SEC_ATTR=4
pds_4attr_secondary_label := TQ_4ATTR
pds_4attr_secondary_includes := $(all_tq_includes)

pds_4attr_dma_secondary_type := pds_header
pds_4attr_dma_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_4attr_dma_secondary_cflags := -DTQ_PDS_SEC_ATTR=4 -DTQ_PDS_SEC_DMA
pds_4attr_dma_secondary_label := TQ_4ATTR_DMA
pds_4attr_dma_secondary_includes := $(all_tq_includes)

pds_5attr_dma_secondary_type := pds_header
pds_5attr_dma_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_5attr_dma_secondary_cflags := -DTQ_PDS_SEC_ATTR=5 -DTQ_PDS_SEC_DMA
pds_5attr_dma_secondary_label := TQ_5ATTR_DMA
pds_5attr_dma_secondary_includes := $(all_tq_includes)

pds_6attr_secondary_type := pds_header
pds_6attr_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_6attr_secondary_cflags := -DTQ_PDS_SEC_ATTR=6
pds_6attr_secondary_label := TQ_6ATTR
pds_6attr_secondary_includes := $(all_tq_includes)

pds_7attr_secondary_type := pds_header
pds_7attr_secondary_src := $(TOP)/services4/srvclient/devices/sgx/pds_secondary.asm
pds_7attr_secondary_cflags := -DTQ_PDS_SEC_ATTR=7
pds_7attr_secondary_label := TQ_7ATTR
pds_7attr_secondary_includes := $(all_tq_includes)

transferqueue_use_type := use_header
transferqueue_use_src := $(TOP)/services4/srvclient/devices/sgx/transferqueue_use.asm
transferqueue_use_name := transferqueue_use
transferqueue_use_label := TQUSE_
transferqueue_use_includes := $(all_tq_includes)

subtwiddled_eot_type := use_header
subtwiddled_eot_src := $(TOP)/services4/srvclient/devices/sgx/subtwiddled_eot.asm
subtwiddled_eot_name := subtwiddled_eot
subtwiddled_eot_label := TQUSE_
subtwiddled_eot_includes := $(all_tq_includes)

endif # SUPPORT_SGX == 1

ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
libsrv_um_includes += eurasiacon/android/graphicshal
libsrv_um_extlibs += hardware
endif

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(SUPPORT_DRI_DRM_NO_LIBDRM),1)
libsrv_um_src += common/drmlite.c
else
libsrv_um_packages := libdrm
$(addprefix $(TARGET_INTERMEDIATES)/libsrv_um/,\
 $(foreach _o,$(patsubst %.c,%.o,$(libsrv_um_src)),$(notdir $(_o)))): \
  $(TARGET_OUT)/.$(libdrm)
endif # SUPPORT_DRI_DRM_NO_LIBDRM
endif # SUPPORT_DRI_DRM
