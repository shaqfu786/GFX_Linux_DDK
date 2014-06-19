/*!****************************************************************************
@File           hal_private.h

@Title          Graphics HAL core

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

#ifndef HAL_PRIVATE_H
#define HAL_PRIVATE_H

#define I_KNOW_WHAT_I_AM_DOING

#include <hardware/gralloc.h>

#include "hal_public.h"

#include "services.h"
#include "sgxapi.h"
#include "pvr2d.h"

#ifndef HWC_PRIVATE_H
#ifdef HAL_ENTRYPOINT_DEBUG
#define ENTERED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: entered function", __func__));
#define EXITED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: left function", __func__));
#else
#define ENTERED()
#define EXITED()
#endif /* HAL_ENTRYPOINT_DEBUG */
#endif /* HWC_PRIVATE_H */

typedef struct _IMG_mapper_meminfo_ IMG_mapper_meminfo_t;

typedef enum
{
	HAL_PRESENT_MODE_FLIP,
	HAL_PRESENT_MODE_BLIT,
	HAL_PRESENT_MODE_FRONT,
}
IMG_HAL_PRESENT_MODE_T;

typedef struct
{
	/* It's safer if we serialize access to the HAL.
	 * This also serializes accesses to the map list.
	 */
	PVRSRV_MUTEX_HANDLE hMutex;

	/* Services private data */
	PVRSRV_DEV_DATA sDevData;
	IMG_HANDLE hDevMemContext;
	IMG_HANDLE hGeneralHeap;
	PVRSRV_SGX_CLIENT_INFO sSGXInfo;

	/* PVR2D private data (for blitops, framebuffer HAL) */
	PVR2DCONTEXTHANDLE hContext;

	struct
	{
		IMG_HAL_PRESENT_MODE_T eHALPresentMode;
#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
		IMG_UINT32 ui32HALCompositionBypass;
#endif
		IMG_UINT32 ui32HALNumFrameBuffers;
		IMG_UINT32 ui32HALFrameBufferHz;
	}
	sAppHints;

	/* List of memory mappings */
	IMG_mapper_meminfo_t *psMapList;
}
IMG_private_data_t;

typedef int (*IMG_buffer_format_alloc_pfn)(
	PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hGeneralHeap,
	int *piWidth, int *piHeight, int *piStride, int *piUsage,
	unsigned int uiBpp,	IMG_UINT32 ui32Flags,
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
	int aiFd[MAX_SUB_ALLOCS]);

typedef int (*IMG_buffer_format_free_pfn)(
	PVRSRV_DEV_DATA *psDevData,
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
	int aiFd[MAX_SUB_ALLOCS]);

typedef int (*IMG_buffer_format_flush_cache_pfn)(
	IMG_native_handle_t *psNativeHandle,
	const PVRSRV_CONNECTION *psConnection,
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
	IMG_write_lock_rect_t *psWriteLockRect);

typedef int (*IMG_buffer_format_compute_params_pfn)(
	unsigned int iSubAlloc, int *piWidth, int *piHeight, int *piStride);

#if defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)
typedef IMG_BOOL (*IMG_buffer_format_getphyaddrs_pfn)(
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
	IMG_SYS_PHYADDR asPhyAddr[MAX_SUB_ALLOCS]);
#endif /* defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */

typedef struct
{
	IMG_buffer_format_public_t base;

	unsigned long iPVR2DFormat;
	IMG_BOOL bSkipMemset;

	IMG_buffer_format_alloc_pfn				pfnAlloc;
	IMG_buffer_format_free_pfn				pfnFree;
	IMG_buffer_format_flush_cache_pfn		pfnFlushCache;
	IMG_buffer_format_compute_params_pfn	pfnComputeParams;

#if defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)
	IMG_buffer_format_getphyaddrs_pfn	pfnGetPhyAddrs;
#endif /* defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */
}
IMG_buffer_format_t;

const IMG_buffer_format_t *GetBufferFormat(int iFormat);
IMG_BOOL RegisterBufferFormat(IMG_buffer_format_t *psBufferFormat);

#define IsInitialized() IsInitializedFunc(__func__)
IMG_BOOL IsInitializedFunc(const char *func);

#endif /* HAL_PRIVATE_H */
