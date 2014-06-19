/*!****************************************************************************
@File           buffer_generic.c

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

#include <errno.h>

#include <EGL/egl.h>
#include "wsegl.h"

#include "pvr2d.h"

/* IMG internal format for XRGB buffers */
#define HAL_PIXEL_FORMAT_BGRX_8888 0x1FF
#define HAL_PIXEL_FORMAT_NV12 0x105

static int GenericComputeParams(unsigned int uiSubAlloc,
								int *piWidth, int *piHeight, int *piStride)
{
	PVR_UNREFERENCED_PARAMETER(piWidth);
	PVR_UNREFERENCED_PARAMETER(piHeight);
	PVR_UNREFERENCED_PARAMETER(piStride);
	return (uiSubAlloc == 0) ? 0 : -EINVAL;
}

IMG_INTERNAL
int GenericAlloc(PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hGeneralHeap,
				 int iAllocBytes, IMG_UINT32 ui32Flags,
				 IMG_VOID *pvPrivData, IMG_UINT32 ui32PrivDataLength,
				 PVRSRV_CLIENT_MEM_INFO **ppsMemInfo, int *piFd)
{
	const int iPageSize = getpagesize();
	int err = 0;

	ENTERED();

	if(PVRSRVAllocDeviceMem2(psDevData, hGeneralHeap, ui32Flags,
							 ALIGN(iAllocBytes, iPageSize),
							 iPageSize, pvPrivData, ui32PrivDataLength,
							 ppsMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate device memory",
				 __func__));
		err = -ENOMEM;
		goto err_out;
	}

	if(PVRSRVExportDeviceMem2(psDevData, *ppsMemInfo, piFd) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to export device memory",
				 __func__));
		err = -EFAULT;
		goto err_out;
	}

err_out:
	EXITED();
	return err;
}

static int
GenericAlloc2D(PVRSRV_DEV_DATA *psDevData,
			   IMG_HANDLE hGeneralHeap,
			   int *piWidth, int *piHeight, int *piStride, int *piUsage,
			   unsigned int uiBpp, IMG_UINT32 ui32Flags,
			   PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
			   int aiFd[MAX_SUB_ALLOCS])
{
	int err, i;

	PVR_UNREFERENCED_PARAMETER(piWidth);

	ENTERED();

	/* This will cause non-nv12 buffers used in the HWC to be mapped
	 * into the GC MMU (2D Blitter) if HW is present
	 */
	if (*piUsage & GRALLOC_USAGE_HW_COMPOSER)
		ui32Flags |= PVRSRV_MAP_GC_MMU;

	err = GenericAlloc(psDevData, hGeneralHeap,
					   *piStride * *piHeight * uiBpp / 8, ui32Flags,
					   IMG_NULL, 0, &apsMemInfo[0], &aiFd[0]);
	if(err)
		goto err_out;

	/* Only one sub-allocation */
	for(i = 1; i < MAX_SUB_ALLOCS; i++)
	{
		apsMemInfo[i] = IMG_NULL;
		aiFd[i] = -1;
	}

err_out:
	EXITED();
	return err;
}

IMG_INTERNAL int
GenericFree(PVRSRV_DEV_DATA *psDevData,
			PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
			int aiFd[MAX_SUB_ALLOCS])
{
	int i;

	for (i = 0; i < MAX_SUB_ALLOCS; i++)
	{
		if(!apsMemInfo[i])
			break;

		if(PVRSRVFreeDeviceMem(psDevData, apsMemInfo[i]) != PVRSRV_OK)
			return -EFAULT;

		/* We're done with the file descriptor, close it to prevent more maps */
		PVR_ASSERT(aiFd[i] >= 0);
		close(aiFd[i]);
	}

	return 0;
}

#if defined(__arm__) && !defined(PVR_NO_FULL_CACHE_OPS)

/* The ARM cache flush instructions are privileged, meaning we can't do
 * CPU cache flushing from user-mode. As a result, we tell the kernel
 * that we want to clean (flush w/o invalidate) the whole CPU cache
 * just before SGX starts using the surfaces.
 *
 * TODO: Ideally we would pass through a minimal list of flush regions.
 */

static int
GenericFlushCache(IMG_native_handle_t *psNativeHandle,
				  const PVRSRV_CONNECTION *psConnection,
				  PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				  IMG_write_lock_rect_t *psWriteLockRect)
{
	PVRSRV_MISC_INFO sMiscInfo = {
		.ui32StateRequest	= PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT,
		.sCacheOpCtl = {
			.eCacheOpType	= PVRSRV_MISC_INFO_CPUCACHEOP_CLEAN,
			.bDeferOp		= IMG_TRUE,
		}
	};

	PVR_UNREFERENCED_PARAMETER(psWriteLockRect);
	PVR_UNREFERENCED_PARAMETER(psNativeHandle);
	PVR_UNREFERENCED_PARAMETER(apsMemInfo);

	if(PVRSRVGetMiscInfo(psConnection, &sMiscInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get client misc info",
								__func__));
		return -EFAULT;
	}

	return 0;
}

#else /* defined(__arm__) */

static int
GenericFlushCache(IMG_native_handle_t *psNativeHandle,
				  const PVRSRV_CONNECTION *psConnection,
				  PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				  IMG_write_lock_rect_t *psWriteLockRect)
{
	IMG_BYTE *pbBase = (IMG_BYTE *)apsMemInfo[0]->pvLinAddr;
	IMG_BYTE *pbRowStart, *pbRowEnd, *pbRowThresh;

	PVRSRV_MISC_INFO sMiscInfo = {
		.ui32StateRequest		= PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT,
		.sCacheOpCtl = {
			.eCacheOpType		= PVRSRV_MISC_INFO_CPUCACHEOP_CLEAN,
			.u.psClientMemInfo	= apsMemInfo[0]
		}
	};

	int iStride = ALIGN(psNativeHandle->iWidth, HW_ALIGN) *
				  psNativeHandle->uiBpp / 8;

	/* Makes code easier to read, compiler should optimize out */
	const int t = psWriteLockRect->t;
	const int l = psWriteLockRect->l;
	const int b = psWriteLockRect->t + psWriteLockRect->h;
	const int r = psWriteLockRect->l + psWriteLockRect->w;

	pbRowStart	= pbBase + t * iStride + l * psNativeHandle->uiBpp / 8;
	pbRowEnd	= pbBase + t * iStride + r * psNativeHandle->uiBpp / 8;
	pbRowThresh	= pbBase + b * iStride;

	if(iStride == pbRowEnd - pbRowStart)
	{
		/* If the row stride is equal to the surface stride, we can
		 * flush the surface in one go..
		 */
		sMiscInfo.sCacheOpCtl.pvBaseVAddr = (IMG_VOID *)pbRowStart;
		sMiscInfo.sCacheOpCtl.ui32Length = pbRowThresh - pbRowStart;
		sMiscInfo.sCacheOpCtl.bStridedCacheOp = IMG_FALSE;
	
		if(PVRSRVGetMiscInfo(psConnection, &sMiscInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get client misc info",
									__func__));
			return -EFAULT;
		}
	}
	else
	{
		/* Otherwise, we have to flush each row one at a time. */
		sMiscInfo.sCacheOpCtl.pbRowStart = (IMG_BYTE *)pbRowStart;
		sMiscInfo.sCacheOpCtl.pbRowEnd = (IMG_BYTE *)pbRowEnd;
		sMiscInfo.sCacheOpCtl.pbRowThresh = (IMG_BYTE *)pbRowThresh;
		sMiscInfo.sCacheOpCtl.bStridedCacheOp = IMG_TRUE;
		sMiscInfo.sCacheOpCtl.ui32Stride = (IMG_UINT32)iStride;

		if(PVRSRVGetMiscInfo(psConnection, &sMiscInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get client misc info",
						__func__));
			return -EFAULT;
		}
	}

	/* All flushed OK */
	return 0;
}

#endif /* defined(__arm__) */

#if defined(PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_YV12)
static int
YV12FlushCache(IMG_native_handle_t *psNativeHandle,
				  const PVRSRV_CONNECTION *psConnection,
				  PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				  IMG_write_lock_rect_t *psWriteLockRect)
{
	IMG_BYTE *pbBase = (IMG_BYTE *)apsMemInfo[0]->pvLinAddr;

	PVRSRV_MISC_INFO sMiscInfo = {
		.ui32StateRequest		= PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT,
		.sCacheOpCtl = {
			.eCacheOpType		= PVRSRV_MISC_INFO_CPUCACHEOP_CLEAN,
			.u.psClientMemInfo	= apsMemInfo[0]
		}
	};

	PVR_UNREFERENCED_PARAMETER( psNativeHandle );
	PVR_UNREFERENCED_PARAMETER( psWriteLockRect );
	
	sMiscInfo.sCacheOpCtl.pvBaseVAddr = (IMG_VOID *)pbBase;
	sMiscInfo.sCacheOpCtl.ui32Length = apsMemInfo[0]->uAllocSize;

	if(PVRSRVGetMiscInfo(psConnection, &sMiscInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get client misc info",
								__func__));
		return -EFAULT;
	}
	
	return 0;
}
#endif

/* GPU-renderable buffer formats will have corresponding EGL window
 * configs created for them by ANDROID_WSEGL.
 *
 * Non-GPU-renderables can be used as textures in GL ES, but buffers in
 * these formats cannot be rendered to. There will be no EGLConfig for them.
 *
 * All Android versions:
 *  - BGRA_8888 (common framebuffer format)
 *  - RGB_565
 *
 * Gingerbread and earlier:
 *  - RGBA_8888
 *  - RGBX_8888
 *
 * Honeycomb and later:
 *  - BGRX_8888
 */

#if defined(SUPPORT_HAL_PIXEL_FORMAT_BGRX)
#define LEGACY_RENDERABLE IMG_FALSE
#define MODERN_RENDERABLE IMG_TRUE
#else
#define LEGACY_RENDERABLE IMG_TRUE
#define MODERN_RENDERABLE IMG_FALSE
#endif

/* NOTE: Sort order matters here. For GPU renderables, this is the
 *       order the EGLConfig will be sorted. Don't change it!
 * NOTE: Must not be 'const' because psNext is updated
 */
static IMG_buffer_format_t gasBufferFormats[] =
{
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_BGRA_8888,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_ARGB8888,
			.szName				= "BGRA_8888",
			.uiBpp				= 32,
			.bGPURenderable		= IMG_TRUE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_ARGB8888,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_RGBA_8888,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_ABGR8888,
			.szName				= "RGBA_8888",
			.uiBpp				= 32,
			.bGPURenderable		= LEGACY_RENDERABLE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_ABGR8888,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_BGRX_8888,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_XRGB8888,
			.szName				= "BGRX_8888",
			.uiBpp				= 32,
			.bGPURenderable		= MODERN_RENDERABLE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_ARGB8888,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_RGBX_8888,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_XBGR8888,
			.szName				= "RGBX_8888",
			.uiBpp				= 32,
			.bGPURenderable		= LEGACY_RENDERABLE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_ABGR8888,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_RGB_565,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_RGB565,
			.szName				= "RGB_565",
			.uiBpp				= 16,
			.bGPURenderable		= IMG_TRUE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_RGB565,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_NV12,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_NV12,
			.szName				= "NV12",
			.uiBpp				= 12,
			.bGPURenderable		= IMG_FALSE,
		},
		.iPVR2DFormat			= PVR2D_FORMAT_PVRSRV |
								  PVRSRV_PIXEL_FORMAT_NV12,
		.bSkipMemset			= IMG_TRUE,
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= GenericFlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
#if defined(PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_YV12)
	{
		.base =
		{
			.iHalPixelFormat	= HAL_PIXEL_FORMAT_YV12,
			.iWSEGLPixelFormat	= WSEGL_PIXELFORMAT_YV12,
			.szName				= "YV12",
			.uiBpp				= 12,
			.bGPURenderable		= IMG_FALSE,
		},
		.pfnAlloc				= GenericAlloc2D,
		.pfnFree				= GenericFree,
		.pfnFlushCache			= YV12FlushCache,
		.pfnComputeParams		= GenericComputeParams,
	},
#endif /* defined(PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_YV12) */
};

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3)
#warning You are using an ancient compiler. Upgrade it.
#define constructor(x) constructor
#endif

static void __attribute__((constructor(500))) buffer_generic_init(void)
{
	IMG_BOOL bFirst;
	size_t i;

	bFirst = RegisterBufferFormat(&gasBufferFormats[0]);
	if(!bFirst)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Generic buffer formats must be "
								"registered first. Re-order your "
								"gralloc constructors.", __func__));
	}

	for(i = 1; i < sizeof(gasBufferFormats) / sizeof(*gasBufferFormats); i++)
		RegisterBufferFormat(&gasBufferFormats[i]);
}
