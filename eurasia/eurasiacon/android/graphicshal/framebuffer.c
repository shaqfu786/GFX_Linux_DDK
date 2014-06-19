/*!****************************************************************************
@File           framebuffer.c

@Title          Graphics Allocator (framebuffer) HAL component

@Author         Imagination Technologies

@Date           2009/03/26

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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "services.h"
#include "eglapi.h"
#include "pvr2d.h"

#include "hal_private.h"
#include "framebuffer.h"
#include "gralloc.h"
#include "modeset.h"
#include "mapper.h"

#define HAL_PIXEL_FORMAT_BAD			0xff
#define MAX_CREATE_FLIP_CHAIN_RETRIES	10

#if defined(PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV)
#include <linux/fb.h>
#endif

typedef struct
{
	IMG_framebuffer_device_t base;

	PVR2DFLIPCHAINHANDLE hFlipChain;

#if defined(SUPPORT_ANDROID_COMPOSER_HAL)
	PVRSRV_CLIENT_MEM_INFO *psLastPostedFrameBuffer;
#endif /* defined(SUPPORT_ANDROID_COMPOSER_HAL) */

	IMG_UINT32 ui32AllocMask;
	IMG_native_handle_t **ppsBufferHandles;
}
IMG_raw_framebuffer_device_t;

static const int aePVR2D2HAL[] =
{
	HAL_PIXEL_FORMAT_BAD,
	HAL_PIXEL_FORMAT_RGB_565,
	HAL_PIXEL_FORMAT_BAD,
	HAL_PIXEL_FORMAT_BAD,
	HAL_PIXEL_FORMAT_BGRA_8888,
	HAL_PIXEL_FORMAT_BAD,
};

static const long alPVR2D2HALBpp[] =
{
	0,
	2,
	0,
	0,
	4,
	0,
};

static inline IMG_private_data_t *
getDevicePrivateData(framebuffer_device_t *fb)
{
	IMG_private_data_t *psPrivateData =
		&((IMG_gralloc_module_t *)fb->common.module)->sPrivateData;
	PVRSRVLockMutex(psPrivateData->hMutex);
	return psPrivateData;
}

static inline void putDevicePrivateData(IMG_private_data_t *psPrivateData)
{
	PVRSRVUnlockMutex(psPrivateData->hMutex);
}

static inline void
setDeviceFrameBufferDevice(framebuffer_device_t *fb)
{
	((IMG_gralloc_module_public_t *)fb->common.module)->psFrameBufferDevice =
										(IMG_framebuffer_device_public_t *)fb;
}

static
IMG_BOOL validate_handle(IMG_raw_framebuffer_device_t *psRawFrameBufferDevice,
						 IMG_native_handle_t *psNativeHandle,
						 IMG_UINT32 *ui32FBID)
{
	IMG_UINT32 i;

	for(i = 0; i < psRawFrameBufferDevice->base.ui32NumBuffers; i++)
	{
		if((psRawFrameBufferDevice->ui32AllocMask & (1U << i)) &&
		   psRawFrameBufferDevice->ppsBufferHandles[i] == psNativeHandle)
			break;
	}

	if(ui32FBID)
		*ui32FBID = i;

	return i != psRawFrameBufferDevice->base.ui32NumBuffers;
}

#if defined(SUPPORT_ANDROID_COMPOSER_HAL)

/* FIXME: Try to get rid of file's dependency on pvr2d */
#include "devices/sgx/pvr2dint.h"

static int Post2(framebuffer_device_t *fb, buffer_handle_t *buffers,
				 int num_buffers, void *data, int data_length)
{
	IMG_UINT32 ui32TriesLeft = GRALLOC_DEFAULT_WAIT_RETRIES;
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)fb;
	IMG_UINT32 ui32MemInfo, ui32NumMemInfos = 0;
	IMG_BOOL bConsumedLastPosted = IMG_FALSE;
	IMG_mapper_meminfo_t **ppsMapperMemInfo;
	PVRSRV_CLIENT_MEM_INFO **ppsMemInfo;
	IMG_private_data_t *psPrivateData;
	PVR2D_FLIPCHAIN *psFlipChain;
	PVR2DCONTEXT *psContext;
	PVRSRV_ERROR eError;
	int i, j, err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psPrivateData = getDevicePrivateData(fb);

	if(psPrivateData->sAppHints.eHALPresentMode != HAL_PRESENT_MODE_FLIP)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Only flip is supported by Post2()",
								__func__));
		err = -EFAULT;
		goto err_put;
	}

	if(num_buffers < 1)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Must Post2() with at least one buffer",
								__func__));
		err = -EFAULT;
		goto err_put;
	}

	/* Allocate space for the mapper entries */
	ppsMapperMemInfo = (IMG_mapper_meminfo_t **)
		malloc(sizeof(IMG_mapper_meminfo_t *) * num_buffers);

	/* First, figure out how many meminfo slots we need */
	for(i = 0; i < num_buffers; i++)
	{
		IMG_native_handle_t *psNativeHandle =
			(IMG_native_handle_t *)buffers[i];

		/* Framebuffer slot? */
		if(!psNativeHandle)
		{
			if(psRawFrameBufferDevice->psLastPostedFrameBuffer)
			{
				bConsumedLastPosted = IMG_TRUE;
				ppsMapperMemInfo[i] = IMG_NULL;
				ui32NumMemInfos++;
				continue;
			}

			PVR_DPF((PVR_DBG_ERROR, "%s: Passed an empty slot for the "
									"composition buffer, but no composition "
									"was posted since last Post2()",
									__func__));
			err = -EFAULT;
			goto err_free;
		}

		if(usage_unsupported(psNativeHandle->usage))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unsupported usage bits "
									"(0xP...FHWR=0x%.8x) set on handle",
					 __func__, psNativeHandle->usage));
			err = -EINVAL;
			goto err_free;
		}

		ppsMapperMemInfo[i] =
			MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
		PVR_ASSERT(ppsMapperMemInfo[i] != IMG_NULL);

		/* Figure out how many valid meminfos this mapper entry has */
		for(j = 0; j < MAX_SUB_ALLOCS; j++)
		{
			if(!ppsMapperMemInfo[i]->apsMemInfo[j])
				break;
			ui32NumMemInfos++;
		}
	}

	/* This is probably a HWC bug. */
	if(psRawFrameBufferDevice->psLastPostedFrameBuffer && !bConsumedLastPosted)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Failed to consume post() composition "
								  "buffer in subsequent Post2()", __func__));
	}

	/* Allocate space for the meminfos.. */
	ppsMemInfo = (PVRSRV_CLIENT_MEM_INFO **)
		malloc(sizeof(PVRSRV_CLIENT_MEM_INFO *) * ui32NumMemInfos);

	/* Then pad the meminfo slots with the meminfos */
	for(i = 0, ui32MemInfo = 0; i < num_buffers; i++)
	{
		/* Pad a framebuffer slot */
		if(!ppsMapperMemInfo[i])
		{
			ppsMemInfo[ui32MemInfo] =
				psRawFrameBufferDevice->psLastPostedFrameBuffer;
			PVR_ASSERT(ui32MemInfo < ui32NumMemInfos);
			ui32MemInfo++;
			continue;
		}

		for(j = 0; j < MAX_SUB_ALLOCS; j++)
		{
			if(!ppsMapperMemInfo[i]->apsMemInfo[j])
				break;
			ppsMemInfo[ui32MemInfo] = ppsMapperMemInfo[i]->apsMemInfo[j];
			PVR_ASSERT(ui32MemInfo < ui32NumMemInfos);
			ui32MemInfo++;
		}
	}

	psRawFrameBufferDevice->psLastPostedFrameBuffer = IMG_NULL;
	PVR_ASSERT(ui32MemInfo == ui32NumMemInfos);

	psFlipChain = (PVR2D_FLIPCHAIN *)psRawFrameBufferDevice->hFlipChain;
	psContext = (PVR2DCONTEXT *)psPrivateData->hContext;

	/* Better not hold the private data lock if we plan to block */
	PVRSRVUnlockMutex(psPrivateData->hMutex);

#if defined(HAL_QUEUE_DEBUG)
	for(i = 0; i < (int)ui32MemInfo; i++)
	{
		IMG_gralloc_module_t *psGrallocModule =
			container(psPrivateData, IMG_gralloc_module_t, sPrivateData);
		psGrallocModule->DumpOps0(psGrallocModule, __func__,
								  ppsMemInfo[i]->ui64Stamp);
	}
#endif /* defined(HAL_QUEUE_DEBUG) */

	do
	{
//SUPPORT_FORCE_FLIP_WORKAROUND
		if(ui32TriesLeft == (GRALLOC_DEFAULT_WAIT_RETRIES / 2))
		{
			PVRSRV_MISC_INFO sMiscInfo = {
				.ui32StateRequest =
					PVRSRV_MISC_INFO_FORCE_SWAP_TO_SYSTEM_PRESENT,
			};

			PVR_DPF((PVR_DBG_ERROR, "%s: Trying force-flip workaround",
									__func__));

			PVRSRVGetMiscInfo(psPrivateData->sDevData.psConnection,
							  &sMiscInfo);
		}
//SUPPORT_FORCE_FLIP_WORKAROUND
		if(!ui32TriesLeft)
		{
			IMG_gralloc_module_t *psGrallocModule =
				container(psPrivateData, IMG_gralloc_module_t, sPrivateData);
			PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for Post2() "
									"to complete", __func__));
			PVRSRVClientEvent(PVRSRV_CLIENT_EVENT_HWTIMEOUT,
							  &psPrivateData->sDevData, psGrallocModule);
			break;
		}

		eError = PVRSRVSwapToDCBuffer2(
			psContext->hDisplayClassDevice,
			psFlipChain->hDCSwapChain,
			psFlipChain->ui32SwapInterval,
			ppsMemInfo, ui32NumMemInfos,
			data, data_length);

		if(eError == PVRSRV_ERROR_RETRY)
		{
			PVRSRVEventObjectWait(psContext->psServices,
								  psContext->sMiscInfo.hOSGlobalEvent);
			ui32TriesLeft--;
		}
	}
	while (eError == PVRSRV_ERROR_RETRY);

	/* Re-take the private data lock */
	PVRSRVLockMutex(psPrivateData->hMutex);

	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to flip to buffer", __func__));
		err = -EFAULT;
	}

	free(ppsMemInfo);
err_free:
	free(ppsMapperMemInfo);
err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#endif /* defined(SUPPORT_ANDROID_COMPOSER_HAL) */

static int setSwapInterval(framebuffer_device_t *fb, int interval)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)fb;
	IMG_private_data_t *psPrivateData;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psPrivateData = getDevicePrivateData(fb);

	/* Front and blit don't do anything with setSwapInterval() */
	if(psPrivateData->sAppHints.eHALPresentMode != HAL_PRESENT_MODE_FLIP)
		goto err_put;

	if(PVR2DSetPresentFlipProperties(psPrivateData->hContext,
									 psRawFrameBufferDevice->hFlipChain,
									 PVR2D_PRESENT_PROPERTY_INTERVAL,
									 0, 0, 0, NULL, interval) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to alter flip properties",
				 __func__));
		err = -EFAULT;
	}

err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

static int post(framebuffer_device_t *fb, buffer_handle_t buffer)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)fb;
	IMG_native_handle_t *psNativeHandle =
		(IMG_native_handle_t *)buffer;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psPrivateData = getDevicePrivateData(fb);

	psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
	PVR_ASSERT(psMapperMemInfo != NULL);

	/* Can only post frame buffers we've leased */
	if(!validate_handle(psRawFrameBufferDevice, psNativeHandle, IMG_NULL))
	{
		err = -EINVAL;
		goto err_put;
	}

#if defined(SUPPORT_ANDROID_COMPOSER_HAL)
	if(psRawFrameBufferDevice->base.base.bBypassPost)
	{
		psRawFrameBufferDevice->psLastPostedFrameBuffer =
			psMapperMemInfo->apsMemInfo[0];
		goto err_put;
	}
#endif /* defined(SUPPORT_ANDROID_COMPOSER_HAL) */

	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
	{
		if(PVR2DPresentFlip(psPrivateData->hContext,
							psRawFrameBufferDevice->hFlipChain,
							psMapperMemInfo->hMemInfo, 0) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to flip to buffer", __func__));
			err = -EFAULT;
		}
	}
	else if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_BLIT)
	{
		if(PVR2DPresentBlt(psPrivateData->hContext,
						   psMapperMemInfo->hMemInfo, 0) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to blit buffer", __func__));
			err = -EFAULT;
		}
	}

err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#if !defined(OGLES1_BASEPATH) || \
	!defined(OGLES1_BASENAME) || \
	!defined(SUPPORT_OPENGLES1)
#error Must set OGLES1_BASEPATH, OGLES1_BASENAME & SUPPORT_OPENGLES1!
#endif

#define OGLES1LIBNAME OGLES1_BASEPATH "lib" OGLES1_BASENAME ".so"

IMG_INTERNAL IMG_BOOL gbForeignGLES;

static int compositionComplete(struct framebuffer_device_t *dev)
{
	static IMG_HANDLE hGLES1Library = IMG_NULL;
	static IMG_UINT8 *(IMG_CALLCONV *pfnglGetString)(int) = IMG_NULL;
	static IMG_EGLERROR (*pfnGLESFlushBuffersGCNoContext)(IMG_BOOL, IMG_BOOL);
	int err = 0;

	PVR_UNREFERENCED_PARAMETER(dev);

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto exit_out;
	}

	if(gbForeignGLES)
		goto exit_out;

	if(!hGLES1Library)
	{
		hGLES1Library = PVRSRVLoadLibrary(OGLES1LIBNAME);
		if(!hGLES1Library)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to load GLES1 library",
					 __func__));
			err = -EFAULT;
			goto err_out;
		}

		if(PVRSRVGetLibFuncAddr(hGLES1Library, "glGetString",
								(IMG_VOID **)&pfnglGetString) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Can't find glGetString in GLES1 "
									"library", __func__));
			err = -EFAULT;
			goto err_out;
		}
	}

	if(!pfnGLESFlushBuffersGCNoContext)
	{
		IMG_UINT8 *pszFunctionTable;
		IMG_OGLES1EGL_Interface sGLES1Interface;

		pszFunctionTable = pfnglGetString(IMG_OGLES1_FUNCTION_TABLE);
		if(!pszFunctionTable)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get GLES1 function "
									"table", __func__));
			err = -EFAULT;
			goto err_out;
		}

		PVRSRVMemCopy(&sGLES1Interface, pszFunctionTable,
					  sizeof(IMG_OGLES1EGL_Interface));

		if (sGLES1Interface.ui32APIVersion != IMG_OGLES1_FUNCTION_TABLE_VERSION)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Function table is the wrong version"
					 " (%d, wanted %d)", __func__,
					 sGLES1Interface.ui32APIVersion,
					 IMG_OGLES1_FUNCTION_TABLE_VERSION));
			err = -EFAULT;
			goto err_out;
		}

		pfnGLESFlushBuffersGCNoContext =
			sGLES1Interface.pfnGLESFlushBuffersGCNoContext;
		/* throw away the rest of the function table */
	}

	if(pfnGLESFlushBuffersGCNoContext(IMG_FALSE, IMG_FALSE))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: GLESFlushBuffersGCNoContext failed", __func__));
		err = -EAGAIN;
		goto err_out;
	}

exit_out:
	EXITED();
	return err;

err_out:
	PVR_DPF((PVR_DBG_ERROR, "%s: Foreign (probably software) GLES detected! "
							"Enabling workarounds.",  __func__));
	gbForeignGLES = IMG_TRUE;
	goto exit_out;
}

static int framebuffer_device_open(hw_device_t *device)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)device;
	IMG_framebuffer_device_t *psFrameBufferDevice =
		(IMG_framebuffer_device_t *)device;
	PVR2DMISCDISPLAYINFO sMiscDisplayInfo;
	IMG_private_data_t *psPrivateData;
	PVR2DDISPLAYINFO sDisplayInfo;
	const IMG_CHAR *pszRenderMode;
	unsigned long ulFlipCount;
	PVR2DMEMINFO **apsMemInfo;
#if defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
	PVR2DMEMINFO *psFBMemInfo;
#endif
	int err = -EFAULT;
	IMG_UINT32 i, j;
	long lStride;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psPrivateData = getDevicePrivateData((framebuffer_device_t *)device);

	if(!SetMode(psPrivateData->sDevData.psConnection))
		goto err_put;

	if(PVR2DGetDeviceInfo(psPrivateData->hContext, &sDisplayInfo) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get display info", __func__));
		goto err_put;
	}

	if(PVR2DGetMiscDisplayInfo(psPrivateData->hContext, &sMiscDisplayInfo) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get misc display info", __func__));
		goto err_put;
	}

	if(aePVR2D2HAL[sDisplayInfo.eFormat] == HAL_PIXEL_FORMAT_BAD)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Frame buffer uses non-Android "
								"compatible pixel format", __func__));
		goto err_put;
	}

	*((uint32_t *)&psFrameBufferDevice->base.base.width)  = sDisplayInfo.ulWidth;
	*((uint32_t *)&psFrameBufferDevice->base.base.height) = sDisplayInfo.ulHeight;

	/* Inherit the apphint value initially -- we might override it */
	*((float *)&psFrameBufferDevice->base.base.fps) =
		psPrivateData->sAppHints.ui32HALFrameBufferHz;

#if defined(PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV)
	{
		struct fb_var_screeninfo sInfo;
		int iFbFd, iRefreshRate;

		iFbFd = open("/dev/graphics/fb0", O_RDWR, 0);
		if (!iFbFd)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to open /dev/graphics/fb0. "
									"Falling back to apphint.", __func__));
			goto skip_fbdev;
		}

		if(ioctl(iFbFd, FBIOGET_VSCREENINFO, &sInfo) < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: FBIOGET_VSCREENINFO failed. "
									"Falling back to apphint.", __func__));
			goto skip_fbdev;
		}

		close(iFbFd);

		/* If any of these are zero, we might generate a floating point
		 * exception due to div by zero, so we must catch it early.
		 */
		if(!sInfo.xres || !sInfo.yres || !sInfo.pixclock)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: FBDEV driver is buggy and returned "
									"xres=%d, yres=%d, pixclock=%d. Falling "
									"back to apphint.", __func__,
									sInfo.xres, sInfo.yres, sInfo.pixclock));
			goto skip_fbdev;
		}

		iRefreshRate = 1000000000000000LLU /
			((IMG_UINT64)(sInfo.upper_margin + sInfo.lower_margin +
						  sInfo.yres + sInfo.vsync_len) *
						 (sInfo.left_margin  + sInfo.right_margin +
						  sInfo.xres + sInfo.hsync_len) *
						 sInfo.pixclock);

		*((float *)&psFrameBufferDevice->base.base.fps) =
			iRefreshRate / 1000.0f;
	}
skip_fbdev:
#endif /* defined(PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV) */

#if defined(PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER)
	/* We may want to use the software GLES (libGLES_android.so) to render the
	 * composition. Unfortunately, this library lacks support for common pixel
	 * formats. Remap unsupported formats to supported ones.
	 *
	 * NOTE: This will cause discoloration of framebuffer clients.
	 */
	if(aePVR2D2HAL[sDisplayInfo.eFormat] == HAL_PIXEL_FORMAT_BGRA_8888)
		*((int *)&psFrameBufferDevice->base.base.format) = HAL_PIXEL_FORMAT_RGBA_8888;
	else
#endif /* defined(PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER) */
		*((int *)&psFrameBufferDevice->base.base.format) = aePVR2D2HAL[sDisplayInfo.eFormat];

	*((float *)&psFrameBufferDevice->base.base.xdpi) =
		(sMiscDisplayInfo.ulPhysicalWidthmm == 0)  ? 144.0f :
			(sDisplayInfo.ulWidth  * 25.4f / sMiscDisplayInfo.ulPhysicalWidthmm);

	*((float *)&psFrameBufferDevice->base.base.ydpi) =
		(sMiscDisplayInfo.ulPhysicalHeightmm == 0) ? 144.0f :
			(sDisplayInfo.ulHeight * 25.4f / sMiscDisplayInfo.ulPhysicalHeightmm);

	/* Android wants strides in pixels, not bytes */
	*((int *)&psFrameBufferDevice->base.base.stride) =
		sDisplayInfo.lStride / alPVR2D2HALBpp[sDisplayInfo.eFormat];

	psFrameBufferDevice->ui32NumBuffers =
		psPrivateData->sAppHints.ui32HALNumFrameBuffers;

	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
	{
		if(psPrivateData->sAppHints.ui32HALNumFrameBuffers >
		   sDisplayInfo.ulMaxBuffersInChain)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Cannot flip with requested "
									  "number of buffers.",
					 __func__));
			goto err_put;
		}
	}

	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
		pszRenderMode = "Flipping";
	else if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_BLIT)
		pszRenderMode = "Blitting";
	else
		pszRenderMode = "Front-buffer rendering";

	PVR_DPF((PVR_DBG_MESSAGE, "%s: %s with %d buffers",
			 __func__, pszRenderMode, psFrameBufferDevice->ui32NumBuffers));

	apsMemInfo = calloc(psFrameBufferDevice->ui32NumBuffers,
						sizeof(PVR2DMEMINFO *));

#if defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
	if (PVR2DGetFrameBuffer(psPrivateData->hContext,
							PVR2D_FB_PRIMARY_SURFACE,
							&psFBMemInfo) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get frame buffer",
				 __func__));
		goto err_free_apsMemInfo;
	}

	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
#endif /* defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER) */
	{
		/* If we're flipping, we must inherit the device's
		 * swap intervals.
		 */
		*((int *)&psFrameBufferDevice->base.base.minSwapInterval) =
			sDisplayInfo.ulMinFlipInterval;
		*((int *)&psFrameBufferDevice->base.base.maxSwapInterval) =
			sDisplayInfo.ulMaxFlipInterval;

		/* Try to create a flip chain a few times before aborting;
		 * there's a known race between SurfaceFlinger generations
		 * when using the "adb stop" "adb start" sequence on some
		 * devices. If we return an error here, SurfaceFlinger
		 * just crashes.
		 */
		for(i = 0; i < MAX_CREATE_FLIP_CHAIN_RETRIES; i++)
		{
			if(!PVR2DCreateFlipChain(psPrivateData->hContext,
									 0, /* No sharing, no query */
									 psFrameBufferDevice->ui32NumBuffers,
									 psFrameBufferDevice->base.base.width,
									 psFrameBufferDevice->base.base.height,
									 sDisplayInfo.eFormat, /* PVR2D (validated) */
									 &lStride,
									 NULL, /* No ID necessary if not sharing */
									 &psRawFrameBufferDevice->hFlipChain)
									 != PVR2D_OK)
			{
				break;
			}

			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create flip chain; retrying",
									__func__));
			sleep(1);
		}

		if(i == MAX_CREATE_FLIP_CHAIN_RETRIES)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create flip chain (after "
									"%d attempts)", __func__,
									MAX_CREATE_FLIP_CHAIN_RETRIES));
			goto err_free_apsMemInfo;
		}

		if(lStride != sDisplayInfo.lStride)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create valid flip chain",
					 __func__));
			goto err_destroy_chain;
		}

		if(PVR2DGetFlipChainBuffers(psPrivateData->hContext,
									psRawFrameBufferDevice->hFlipChain,
									&ulFlipCount, apsMemInfo) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to obtain flip chain buffers",
					 __func__));
			goto err_destroy_chain;
		}

		if(ulFlipCount != psFrameBufferDevice->ui32NumBuffers)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Mismatching flip chain buffer count "
									"(%d vs %lu)",
					 __func__, psFrameBufferDevice->ui32NumBuffers,
					 ulFlipCount));
			goto err_destroy_chain;
		}
	}
#if defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
	else if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_BLIT)
	{
		/* If we're blitting, the only valid swap interval
		 * is zero, and FPS has no meaning.
		 */

		for (i = 0; i < psFrameBufferDevice->ui32NumBuffers; i++)
		{
			int iFd;

			if(PVR2DMemAlloc(psPrivateData->hContext,
							 psFBMemInfo->ui32MemSize, 1, 0,
							 &apsMemInfo[i]) != PVR2D_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate back buffer",
						 __func__));
				goto err_destroy_chain;
			}

			/* Export this meminfo and immediately close the handle.
			 *
			 * We do this to get a valid ui64Stamp, without needing
			 * all memory allocations to be stamped.
			 */
			if(PVRSRVExportDeviceMem2(&psPrivateData->sDevData,
									  apsMemInfo[i]->hPrivateData,
									  &iFd) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Failed to export back buffer",
						 __func__));
				goto err_destroy_chain;
			}

			close(iFd);
		}

		if(PVR2DSetPresentBltProperties(psPrivateData->hContext,
										PVR2D_PRESENT_PROPERTY_SRCSTRIDE |
										PVR2D_PRESENT_PROPERTY_DSTSIZE,
										sDisplayInfo.lStride,
										sDisplayInfo.ulWidth,
										sDisplayInfo.ulHeight,
										0, 0, 0, IMG_NULL, 0) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set blit properties",
					 __func__));
			goto err_destroy_chain;
		}
	}
	else
	{
		/* If we're front buffer, the only valid swap interval
		 * is zero, and FPS has no meaning.
		 */

		apsMemInfo[0] = psFBMemInfo;
	}
#endif /* defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER) */

	psRawFrameBufferDevice->ppsBufferHandles =
		malloc(sizeof(IMG_native_handle_t *) * psFrameBufferDevice->ui32NumBuffers);
	if(!psRawFrameBufferDevice->ppsBufferHandles)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate memory", __func__));
		err = -ENOMEM;
		goto err_destroy_chain;
	}

	for(i = 0; i < psFrameBufferDevice->ui32NumBuffers; i++)
	{
		PVRSRV_CLIENT_MEM_INFO *psMemInfo =
			(PVRSRV_CLIENT_MEM_INFO *)apsMemInfo[i]->hPrivateData;
		IMG_native_handle_t *psNativeHandle;

		/* If this handle is sent cross-process, there will only
		 * ever be one file descriptor. Patch the rest to ints.
		 */
		psNativeHandle = (IMG_native_handle_t *)
			native_handle_create(1, IMG_NATIVE_HANDLE_NUMFDS - 1 +
									IMG_NATIVE_HANDLE_NUMINTS);

		psNativeHandle->usage		= GRALLOC_USAGE_HW_FB;
		psNativeHandle->ui64Stamp	= psMemInfo->ui64Stamp;

		for(j = 0; j < MAX_SUB_ALLOCS; j++)
			psNativeHandle->fd[j] = IMG_FRAMEBUFFER_FD;

		if(!MapperAddMappedFB(psPrivateData, psNativeHandle,
							  psMemInfo, apsMemInfo[i]))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to add framebuffer handle "
									"to mapper", __func__));
			native_handle_delete((native_handle_t *)psNativeHandle);
			err = -EFAULT;
			goto err_destroy_chain;
		}

		if(!MapperSanityCheck(psPrivateData))
		{
			err = -EFAULT;
			goto err_destroy_chain;
		}

		psRawFrameBufferDevice->ppsBufferHandles[i] = psNativeHandle;
	}

	/* Allocator needs a hook to the framebuffer device */
	setDeviceFrameBufferDevice((framebuffer_device_t *)psFrameBufferDevice);

	psFrameBufferDevice->base.base.setSwapInterval		= setSwapInterval;
	psFrameBufferDevice->base.base.post					= post;
	psFrameBufferDevice->base.base.compositionComplete	= compositionComplete;

#if defined(SUPPORT_ANDROID_COMPOSER_HAL)
	psFrameBufferDevice->base.Post2 = Post2;
#endif

	free(apsMemInfo);
	err = 0;
err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;

err_destroy_chain:
	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
	{
		PVR2DDestroyFlipChain(psPrivateData->hContext,
							  psRawFrameBufferDevice->hFlipChain);
	}
	else if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_BLIT)
	{
		for(i = 0; i < psFrameBufferDevice->ui32NumBuffers; i++)
		{
			if(!apsMemInfo[i])
				continue;
			PVR2DMemFree(psPrivateData->hContext, apsMemInfo[i]);
		}
	}
err_free_apsMemInfo:
	free(apsMemInfo);
	goto err_out;
}

static int framebuffer_device_close(hw_device_t *device)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)device;
	IMG_framebuffer_device_t *psFrameBufferDevice =
		(IMG_framebuffer_device_t *)device;
	IMG_private_data_t *psPrivateData;
	IMG_UINT32 i;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psPrivateData = getDevicePrivateData((framebuffer_device_t *)device);

	if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP)
	{
		PVR2DDestroyFlipChain(psPrivateData->hContext,
							  psRawFrameBufferDevice->hFlipChain);
	}

	for(i = 0; i < psFrameBufferDevice->ui32NumBuffers; i++)
	{
		IMG_native_handle_t *psNativeHandle =
			psRawFrameBufferDevice->ppsBufferHandles[i];
		IMG_mapper_meminfo_t *psMapperMemInfo;

		psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
		PVR_ASSERT(psMapperMemInfo != NULL);

		if(psMapperMemInfo->ui32LockCount)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to free locked frame buffer",
					 __func__));
			err = -EFAULT;
			break;
		}

		if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_BLIT)
		{
			PVR2DMemFree(psPrivateData->hContext,
						 psMapperMemInfo->hMemInfo);
		}

		MapperRemove(psPrivateData, psMapperMemInfo);

		/* Just need to do this; no fd to close */
		native_handle_delete((native_handle_t *)psNativeHandle);
	}

	PVR2DDestroyDeviceContext(psPrivateData->hContext);
	putDevicePrivateData(psPrivateData);

err_out:
	EXITED();
	return err;
}

/* NOTE: This should only be called after taking the private-data lock */

IMG_INTERNAL
int framebuffer_device_alloc(framebuffer_device_t *fb, int w, int h,
							 int format, int usage,
							 IMG_native_handle_t **ppsNativeHandle,
							 int *stride)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)fb;
	IMG_framebuffer_device_t *psFrameBufferDevice =
		(IMG_framebuffer_device_t *)fb;
	IMG_UINT32 ui32NextFreeBuffer;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	/* If we're running front-buffer, we'll only have one buffer. However,
	 * SurfaceFlinger requires at least two buffers (it expects to be able
	 * to flip) so we simulate double buffering by always returning the
	 * front buffer.
	 *
	 * This workaround isn't compatible with full-screen compositor bypass,
	 * but we don't need to check for that here because we've already blocked
	 * setting NumBuffers too low when parsing the apphint.
	 */
	if(psFrameBufferDevice->ui32NumBuffers == 1)
		ui32NextFreeBuffer = 0;
	else
	{
		ui32NextFreeBuffer = ffs(~psRawFrameBufferDevice->ui32AllocMask) - 1;
		if(ui32NextFreeBuffer >= psFrameBufferDevice->ui32NumBuffers)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate frame-buffer. "
									"Pool is exhausted.", __func__));
			err = -ENOMEM;
			goto err_out;
		}
	}

	/* Assuming these have been passed by Android, they should always be
	 * valid. We won't bail gracefully; we'd rather fail hard.
	 */
#if 0 //defined(DEBUG)
	PVR_ASSERT((uint32_t)w == fb->width);
	PVR_ASSERT((uint32_t)h == fb->height);
	PVR_ASSERT(format == fb->format);
	PVR_ASSERT(usage_bypass_fb(usage));
#else
	PVR_UNREFERENCED_PARAMETER(w);
	PVR_UNREFERENCED_PARAMETER(h);
	PVR_UNREFERENCED_PARAMETER(format);
	PVR_UNREFERENCED_PARAMETER(usage);
#endif

	/* Pass back stride for comparison */
	PVR_ASSERT(stride != NULL);
	*stride = fb->stride;

	psRawFrameBufferDevice->ui32AllocMask |= (1U << ui32NextFreeBuffer);
	*ppsNativeHandle =
		psRawFrameBufferDevice->ppsBufferHandles[ui32NextFreeBuffer];

	PVR_DPF((PVR_DBG_WARNING, "%s: Leased frame buffer %d (of %d) "
							  "for usage 0xP...FHWR=0x%.8x",
			 __func__, ui32NextFreeBuffer,
			 psFrameBufferDevice->ui32NumBuffers, usage));

err_out:
	EXITED();
	return err;
}

/* NOTE: This should only be called after taking the private-data lock */

IMG_INTERNAL
int framebuffer_device_free(framebuffer_device_t *fb,
							IMG_native_handle_t *psNativeHandle)
{
	IMG_raw_framebuffer_device_t *psRawFrameBufferDevice =
		(IMG_raw_framebuffer_device_t *)fb;
	int err = -EINVAL;
	IMG_UINT32 i;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!validate_handle(psRawFrameBufferDevice, psNativeHandle, &i))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Can only free handles allocated by "
								"this module", __func__));
		goto err_out;
	}

	psRawFrameBufferDevice->ui32AllocMask &= ~(1U << i);

	PVR_DPF((PVR_DBG_WARNING, "%s: Released frame buffer %d (of %d)",
			 __func__, i, psRawFrameBufferDevice->base.ui32NumBuffers));
	err = 0;
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int framebuffer_setup(const hw_module_t *module, hw_device_t **device)
{
	IMG_raw_framebuffer_device_t *psFrameBufferDevice;
	hw_device_t *psHwDevice;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psFrameBufferDevice = malloc(sizeof(IMG_raw_framebuffer_device_t));
	if(!psFrameBufferDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate memory", __func__));
		err = -ENOMEM;
		goto err_out;
	}

	memset(psFrameBufferDevice, 0, sizeof(IMG_raw_framebuffer_device_t));
	psHwDevice = (hw_device_t *)psFrameBufferDevice;

	psFrameBufferDevice->base.base.base.common.tag		= HARDWARE_DEVICE_TAG;
	psFrameBufferDevice->base.base.base.common.version	= 0;
	psFrameBufferDevice->base.base.base.common.module	= (hw_module_t *)module;
	psFrameBufferDevice->base.base.base.common.close	= framebuffer_device_close;

	err = framebuffer_device_open(psHwDevice);
	if(err)
		goto err_out;

	*device = psHwDevice;
err_out:
	EXITED();
	return err;
}
