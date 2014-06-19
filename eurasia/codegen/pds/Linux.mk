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
 pdsogles pixelevent pds_mte_state_copy pds_aux_vtx \
 pixelevent_tilexy \
 pixelevent_msnods_4x pixelevent_dsnoms_4x \
 pixelevent_msnods_2x_inX pixelevent_dsnoms_2x_inX \
 pixelevent_msnods_2x_inY pixelevent_dsnoms_2x_inY

pdsogles_type := objects
pdsogles_src := pds.c
pdsogles_genheaders := \
 pixelevent/pixelevent.h \
 pixelevent/pixelevent_sizeof.h \
 pixelevent_tilexy/pixelevent_tilexy.h \
 pixelevent_tilexy/pixelevent_tilexy_sizeof.h
pdsogles_includes := \
 include4 \
 hwdefs \
 $(TARGET_INTERMEDIATES)/pixelevent \
 $(TARGET_INTERMEDIATES)/pixelevent_tilexy
pdsogles_cflags := -fPIC -DPDS_BUILD_OPENGLES

pixelevent_type := pds_header
pixelevent_src := pixelevent.asm
pixelevent_label := PixelEvent
pixelevent_includes := include4 hwdefs
pixelevent_cflags := -DTILE_EXTRA_SHIFTX=0 -DTILE_EXTRA_SHIFTY=0

pixelevent_tilexy_type := pds_header
pixelevent_tilexy_src := pixelevent_tilexy.asm
pixelevent_tilexy_label := PixelEvent_TileXY
pixelevent_tilexy_includes := include4 hwdefs

pixelevent_msnods_4x_type := pds_header
pixelevent_msnods_4x_src := pixelevent.asm
pixelevent_msnods_4x_label := PixelEventMSAANoDownscale_4X
pixelevent_msnods_4x_includes := include4 hwdefs
pixelevent_msnods_4x_cflags := -DTILE_EXTRA_SHIFTX=1 -DTILE_EXTRA_SHIFTY=1

pixelevent_dsnoms_4x_type := pds_header
pixelevent_dsnoms_4x_src := pixelevent.asm
pixelevent_dsnoms_4x_label := PixelEventDownscaleNoMSAA_4X
pixelevent_dsnoms_4x_includes := include4 hwdefs
pixelevent_dsnoms_4x_cflags := -DTILE_EXTRA_SHIFTX=-1 -DTILE_EXTRA_SHIFTY=-1

pixelevent_msnods_2x_inX_type := pds_header
pixelevent_msnods_2x_inX_src := pixelevent.asm
pixelevent_msnods_2x_inX_label := PixelEventMSAANoDownscale_2X
pixelevent_msnods_2x_inX_includes := include4 hwdefs
pixelevent_msnods_2x_inX_cflags := -DTILE_EXTRA_SHIFTX=1 -DTILE_EXTRA_SHIFTY=0

pixelevent_dsnoms_2x_inX_type := pds_header
pixelevent_dsnoms_2x_inX_src := pixelevent.asm
pixelevent_dsnoms_2x_inX_label := PixelEventDownscaleNoMSAA_2X
pixelevent_dsnoms_2x_inX_includes := include4 hwdefs
pixelevent_dsnoms_2x_inX_cflags := -DTILE_EXTRA_SHIFTX=-1 -DTILE_EXTRA_SHIFTY=0

pixelevent_msnods_2x_inY_type := pds_header
pixelevent_msnods_2x_inY_src := pixelevent.asm
pixelevent_msnods_2x_inY_label := PixelEventMSAANoDownscale_2X
pixelevent_msnods_2x_inY_includes := include4 hwdefs
pixelevent_msnods_2x_inY_cflags := -DTILE_EXTRA_SHIFTX=0 -DTILE_EXTRA_SHIFTY=1

pixelevent_dsnoms_2x_inY_type := pds_header
pixelevent_dsnoms_2x_inY_src := pixelevent.asm
pixelevent_dsnoms_2x_inY_label := PixelEventDownscaleNoMSAA_2X
pixelevent_dsnoms_2x_inY_includes := include4 hwdefs
pixelevent_dsnoms_2x_inY_cflags := -DTILE_EXTRA_SHIFTX=0 -DTILE_EXTRA_SHIFTY=-1

pds_mte_state_copy_type := pds_header
pds_mte_state_copy_src := pds_mte_state_copy.asm
pds_mte_state_copy_label := MTEStateCopy
pds_mte_state_copy_includes := include4 hwdefs
pds_mte_state_copy_cflags := -DTILE_EXTRA_SHIFT=0

pds_aux_vtx_type := pds_header
pds_aux_vtx_src := pds_aux_vtx.asm
pds_aux_vtx_label := AuxiliaryVertex
pds_aux_vtx_includes := include4 hwdefs
pds_aux_vtx_cflags := -DTILE_EXTRA_SHIFT=0
