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

static int ComputeNV12Params(unsigned int uiSubAlloc,
							 int *piWidth, int *piHeight, int *piStride)
{
	const int iPageSize = getpagesize();

	PVR_UNREFERENCED_PARAMETER(piWidth);
	PVR_UNREFERENCED_PARAMETER(piHeight);

	*piStride = ALIGN(*piStride, iPageSize);

	switch(uiSubAlloc)
	{
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
	struct omap_ion_tiler_alloc_data asTilerPrivData[2];
	int i, err;

	PVR_UNREFERENCED_PARAMETER(uiBpp);

	ENTERED();

	/* Allocating Tiler 2D buffer does not allow for
	 * cached access. If the client needs WRITE access
	 * all we can do is WRITECOMBINE.
	*/
	if(usage_sw_write(*piUsage))
	{
		ui32Flags |= PVRSRV_HAP_WRITECOMBINE;
	}

	/* Even if gralloc thought we were caching, we can't support this
	 * with TILER, so mask it out.
	 */
	ui32Flags &= ~PVRSRV_MEM_CACHED;

	/* Must be allocated through ION */
	ui32Flags |= PVRSRV_MEM_ION;

	/* Fixes up the parameters Android sees */
	err = ComputeNV12Params(0, piWidth, piHeight, piStride);
	PVR_ASSERT(err == 0);

	asTilerPrivData[0].w = *piWidth;
	asTilerPrivData[0].h = *piHeight;
	asTilerPrivData[0].fmt = TILER_PIXEL_FMT_8BIT;
	asTilerPrivData[0].flags = 0;

	asTilerPrivData[1].w = *piWidth / 2;
	asTilerPrivData[1].h = *piHeight / 2;
	asTilerPrivData[1].fmt = TILER_PIXEL_FMT_16BIT;
	asTilerPrivData[1].flags = 0;

	err = GenericAlloc(psDevData, hGeneralHeap,
					   *piStride * (*piHeight + (*piHeight / 2)),
					   ui32Flags, asTilerPrivData,
					   sizeof(struct omap_ion_tiler_alloc_data) * 2,
					   &apsMemInfo[0], &aiFd[0]);

	/* Only one sub-allocation */
	for(i = 1; i < MAX_SUB_ALLOCS; i++)
	{
		apsMemInfo[i] = IMG_NULL;
		aiFd[i] = -1;
	}

	EXITED();
	return err;
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
