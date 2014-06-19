/*!****************************************************************************
@File           buffer_omap.c

@Title          Graphics Allocator (gralloc) HAL component

@Author         Imagination Technologies

@Date           2011/07/05

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

#include "buffer_generic.h"
#include "gralloc.h"

#include <EGL/egl.h>

#include <errno.h>

#include "wsegl.h"
#include "pvr2d.h"

#if !defined(SUPPORT_NV12_FROM_2_HWADDRS)
#error SUPPORT_NV12_FROM_2_HWADDRS must be defined
#endif

#define HAL_PIXEL_FORMAT_TI_NV12		0x100

/***************************************************************
 * This enum and the struct come from include/linux/omap_ion.h */

enum {
    TILER_PIXEL_FMT_8BIT  = 0,
    TILER_PIXEL_FMT_16BIT = 1,
};

struct omap_ion_tiler_alloc_data
{
	size_t w;
   	size_t h;
	int fmt;
	unsigned int flags;
	/* Other fields are unused in UM */
};

/* Keep it in sync!                                            *
 ***************************************************************/

static int
TilerAllocCommon(PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hGeneralHeap,
				 int *piWidth, int *piHeight, int *piStride, int *piUsage,
				 unsigned int uiBpp, IMG_UINT32 ui32Flags,
				 PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				 int aiFd[MAX_SUB_ALLOCS],
				 IMG_buffer_format_compute_params_pfn pfnComputeParams)
{
	struct omap_ion_tiler_alloc_data sTilerPrivData;
	int iWidth, iHeight;
	int i, err;

	PVR_UNREFERENCED_PARAMETER(uiBpp);

	ENTERED();

	/* Block SW write usage. This usage should actually work fine, but we
	 * want to catch badly written HALs that depend on gralloc and use
	 * the wrong allocation flags.
	 */
	if(usage_sw_write(*piUsage))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: SW write usage bits are not supported "
								"for allocations going via TILER", __func__));
		err = -EINVAL;
		goto err_out;
	}

	/* Even if gralloc thought we were caching, we can't support this
	 * with TILER, so mask it out.
	 */
	ui32Flags &= ~PVRSRV_MEM_CACHED;

	/* Must be allocated through ION */
	ui32Flags |= PVRSRV_MEM_ION;

	/* The first plane (Y) fixes up the parameters Android sees */
	err = pfnComputeParams(0, piWidth, piHeight, piStride);
	PVR_ASSERT(err == 0);

	sTilerPrivData.w = *piWidth;
	sTilerPrivData.h = *piHeight;
	sTilerPrivData.fmt = TILER_PIXEL_FMT_8BIT;
	sTilerPrivData.flags = 0;

	err = GenericAlloc(psDevData, hGeneralHeap, *piStride * *piHeight,
					   ui32Flags, &sTilerPrivData,
					   sizeof(struct omap_ion_tiler_alloc_data),
					   &apsMemInfo[0], &aiFd[0]);
	if(err)
		goto err_out;

	/* Copy width/height for the second (UV) plane, before modifying,
	 * as we don't want Android to see the sub-sampled dimensions.
	 * The stride remains the same.
	 */
	iWidth = *piWidth;
	iHeight = *piHeight;
	err = pfnComputeParams(1, &iWidth, &iHeight, piStride);
	PVR_ASSERT(err == 0);

	sTilerPrivData.w = iWidth;
	sTilerPrivData.h = iHeight;
	sTilerPrivData.fmt = TILER_PIXEL_FMT_16BIT;
	sTilerPrivData.flags = 0;

	err = GenericAlloc(psDevData, hGeneralHeap, *piStride * iHeight,
					   ui32Flags, &sTilerPrivData,
					   sizeof(struct omap_ion_tiler_alloc_data),
					   &apsMemInfo[1], &aiFd[1]);
	if(err)
		goto err_out;

	/* Only two sub-allocations */
	for(i = 2; i < MAX_SUB_ALLOCS; i++)
	{
		apsMemInfo[i] = IMG_NULL;
		aiFd[i] = -1;
	}

err_out:
	EXITED();
	return err;
}

static int ComputeNV12Params(unsigned int uiSubAlloc,
							 int *piWidth, int *piHeight, int *piStride)
{
	const int iPageSize = getpagesize();

	PVR_UNREFERENCED_PARAMETER(piWidth);

	*piStride = ALIGN(*piStride, iPageSize);

	switch(uiSubAlloc)
	{
		case 1:
			*piWidth /= 2;
			*piHeight /= 2;
		case 0:
			return 0;
		default:
			return -EINVAL;
	}
}

static int
TilerAllocNV12(PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hGeneralHeap,
			   int *piWidth, int *piHeight, int *piStride, int *piUsage,
			   unsigned int uiBpp, IMG_UINT32 ui32Flags,
			   PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
			   int aiFd[MAX_SUB_ALLOCS])
{
	return TilerAllocCommon(psDevData, hGeneralHeap,
							piWidth, piHeight, piStride, piUsage,
							uiBpp, ui32Flags, apsMemInfo, aiFd,
							ComputeNV12Params);
}

/* NOTE: Must not be 'const' because psNext is updated */
static IMG_buffer_format_t gasBufferFormats[] =
{
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_TI_NV12,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_NV12,
			.szName				= "TI_NV12",
			.uiBpp				= 12,
			.bGPURenderable		= IMG_FALSE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_NV12,
		.bSkipMemset			= IMG_TRUE,
		.pfnAlloc				= TilerAllocNV12,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= IMG_NULL,
		.pfnComputeParams		= ComputeNV12Params,
	},
};

static void __attribute__((constructor(501))) buffer_omap_init(void)
{
	size_t i;
	for(i = 0; i < sizeof(gasBufferFormats) / sizeof(*gasBufferFormats); i++)
		RegisterBufferFormat(&gasBufferFormats[i]);
}
