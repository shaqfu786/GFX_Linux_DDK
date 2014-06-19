/*!****************************************************************************
@File           gralloc.h

@Title          Graphics HAL gralloc component

@Author         Imagination Technologies

@Date           2009/03/19

@Copyright      Copyright (C) Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

******************************************************************************/

#ifndef GRALLOC_H
#define GRALLOC_H

#include "hal.h"

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
# if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PRIVATE)
#  define GRALLOC_USAGE_BYPASS GRALLOC_USAGE_PRIVATE_0
# else
#  define GRALLOC_USAGE_BYPASS (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER)
# endif
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PRIVATE)
#define GRALLOC_USAGE_PHYS_CONTIG GRALLOC_USAGE_PRIVATE_1
#endif

#if defined(CLIENT_DRIVER_DEFAULT_WAIT_RETRIES)
#define GRALLOC_DEFAULT_WAIT_RETRIES CLIENT_DRIVER_DEFAULT_WAIT_RETRIES
#else
#define GRALLOC_DEFAULT_WAIT_RETRIES 50
#endif

#define container(p, t, m)							\
	({												\
		IMG_CONST typeof(((t *)0)->m) *__p = (p);	\
		(t *)((IMG_CHAR *)__p - offsetof(t, m));	\
	 })

typedef struct
{
	/* "Live" complete value */
	volatile IMG_UINT32 *pui32OpsComplete;

	/* Snapshot of pending value */
	IMG_UINT32 ui32OpsPending;
}
IMG_flush_ops_pair_t;

static inline int usage_sw_cached(int usage)
{
	return (usage & GRALLOC_USAGE_SW_READ_OFTEN);
}

static inline int usage_sw(int usage)
{
	return usage & (GRALLOC_USAGE_SW_READ_OFTEN |
					GRALLOC_USAGE_SW_READ_RARELY |
					GRALLOC_USAGE_SW_WRITE_OFTEN |
					GRALLOC_USAGE_SW_WRITE_RARELY);
}

static inline int usage_sw_write(int usage)
{
	return usage & (GRALLOC_USAGE_SW_WRITE_OFTEN |
			GRALLOC_USAGE_SW_WRITE_RARELY);
}

static inline int usage_sw_read(int usage)
{
	return usage & (GRALLOC_USAGE_SW_READ_OFTEN |
			GRALLOC_USAGE_SW_READ_RARELY);
}

static inline int usage_hw(int usage)
{
	return usage & (GRALLOC_USAGE_HW_RENDER  |
					GRALLOC_USAGE_HW_TEXTURE |
					GRALLOC_USAGE_HW_FB);
}

static inline int usage_unsupported(int usage)
{
	return usage & ~(GRALLOC_USAGE_SW_READ_OFTEN   |
					GRALLOC_USAGE_SW_READ_RARELY   |
					GRALLOC_USAGE_SW_WRITE_OFTEN   |
					GRALLOC_USAGE_SW_WRITE_RARELY  |
					GRALLOC_USAGE_HW_RENDER        |
					GRALLOC_USAGE_HW_TEXTURE       |
					GRALLOC_USAGE_HW_FB            |
#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_HW_COMPOSER)
					 GRALLOC_USAGE_HW_COMPOSER      |
#endif /* defined(PVR_ANDROID_HAS_GRALLOC_USAGE_HW_COMPOSER) */
#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_HW_VIDEO_ENCODER)
					 GRALLOC_USAGE_HW_VIDEO_ENCODER |
#endif /* defined(PVR_ANDROID_HAS_GRALLOC_USAGE_HW_VIDEO_ENCODER) */
#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_EXTERNAL_DISP)
					 GRALLOC_USAGE_EXTERNAL_DISP    |
#endif /* defined(PVR_ANDROID_HAS_GRALLOC_USAGE_EXTERNAL_DISP) */
#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PROTECTED)
					 GRALLOC_USAGE_PROTECTED        |
#endif /* defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PROTECTED) */
#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
					 GRALLOC_USAGE_BYPASS           |
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */
#if defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PRIVATE)
					 GRALLOC_USAGE_PHYS_CONTIG      |
#endif /* defined(PVR_ANDROID_HAS_GRALLOC_USAGE_PRIVATE) */
					 0);
}

static inline int usage_fb(int usage)
{
	return usage & GRALLOC_USAGE_HW_FB;
}

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)

static inline int usage_bypass(int usage)
{
	if(!usage_sw(usage) &&
	   ((usage & GRALLOC_USAGE_BYPASS) == GRALLOC_USAGE_BYPASS))
		return GRALLOC_USAGE_BYPASS;
	return 0;
}

#else /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

/* Compiler should optimize away */
static inline int usage_bypass(int usage)
{
	PVR_UNREFERENCED_PARAMETER(usage);
	return 0;
}

#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

static inline int usage_bypass_fb(int usage)
{
	return usage_bypass(usage) | usage_fb(usage);
}

static inline int usage_persistent(int usage)
{
	return usage & (GRALLOC_USAGE_HW_RENDER |
					GRALLOC_USAGE_HW_TEXTURE |
					GRALLOC_USAGE_HW_FB |
					GRALLOC_USAGE_SW_READ_OFTEN |
					GRALLOC_USAGE_SW_READ_RARELY |
					GRALLOC_USAGE_SW_WRITE_OFTEN |
					GRALLOC_USAGE_SW_WRITE_RARELY);
}

static inline int usage_read(int usage)
{
	return usage & (GRALLOC_USAGE_HW_TEXTURE |
					GRALLOC_USAGE_SW_READ_OFTEN |
					GRALLOC_USAGE_SW_READ_RARELY);
}

static inline int usage_write(int usage)
{
	return usage & (GRALLOC_USAGE_HW_RENDER |
					GRALLOC_USAGE_SW_WRITE_OFTEN |
					GRALLOC_USAGE_SW_WRITE_RARELY);
}

IMG_BOOL OpsFlushed(PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS]);

int gralloc_module_map(gralloc_module_t const* module, IMG_UINT64 ui64Stamp,
					   int usage, PVRSRV_CLIENT_MEM_INFO **ppsMemInfo);

int gralloc_module_unmap(gralloc_module_t const* module,
						 IMG_UINT64 ui64Stamp);

int gralloc_module_lock(gralloc_module_t const* module, buffer_handle_t handle,
						int usage, int l, int t, int w, int h, void **vaddr);

int gralloc_module_unlock(gralloc_module_t const* module,
						  buffer_handle_t handle);

int gralloc_register_buffer(gralloc_module_t const* module,
							buffer_handle_t handle);

int gralloc_unregister_buffer(gralloc_module_t const* module,
							  buffer_handle_t handle);

#if defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)
int gralloc_module_getphyaddrs(IMG_gralloc_module_public_t const* module,
							   buffer_handle_t handle,
							   unsigned int auiPhyAddr[MAX_SUB_ALLOCS]);
#endif /* defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
int gralloc_module_setaccumbuffer(gralloc_module_t const* module,
								  IMG_UINT64 ui64Stamp,
								  IMG_UINT64 ui64AccumStamp);
#endif /* defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND) */

int gralloc_module_blit_to_vaddr(const IMG_gralloc_module_public_t *module,
								 buffer_handle_t src,
								 void *dest[MAX_SUB_ALLOCS], int format);

int gralloc_module_blit_to_handle(IMG_gralloc_module_public_t const *module,
								  buffer_handle_t src, buffer_handle_t dest,
								  int w, int h, int x, int y);

int gralloc_setup(const hw_module_t *module, hw_device_t **device);

#endif /* GRALLOC_H */
