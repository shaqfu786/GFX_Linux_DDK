/******************************************************************************
 * Name         : android_ws.c
 *
 * Copyright    : Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any human or computer language in any form
 *              : by any means, electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Home Park
 *              : Estate, Kings Langley, Hertfordshire, WD4 8LZ, U.K.
 *
 * Platform     : ANSI
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <EGL/egl.h>
#include "wsegl.h"

#include "sgxapi_km.h"
#include "services.h"

#define I_KNOW_WHAT_I_AM_DOING
#include "framebuffer.h"
#include "gralloc.h"
#include "hal.h"

#if defined(PVR_ANDROID_HAS_ANDROID_NATIVE_BUFFER_H)
#include <ui/android_native_buffer.h>
#else
#include <hardware/gralloc.h>
#endif

#ifdef HAL_ENTRYPOINT_DEBUG
#define ENTERED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: entered function", __func__));
#define EXITED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: left function", __func__));
#else
#define ENTERED()
#define EXITED()
#endif

#if defined(PVR_ANDROID_NEEDS_ANATIVEWINDOW_TYPEDEF)
typedef struct android_native_window_t ANativeWindow;
#endif

#if defined(PVR_ANDROID_NEEDS_ANATIVEWINDOWBUFFER_TYPEDEF)
typedef struct android_native_buffer_t ANativeWindowBuffer;
#else
typedef struct ANativeWindowBuffer ANativeWindowBuffer;
#endif

static IMG_BOOL bAlreadyInitialised;
static IMG_gralloc_module_t *psHal;

/* This mutex is held inside the WSEGL entrypoints */
static PVRSRV_MUTEX_HANDLE hMutex;

static WSEGLConfig *gpsWSConfigs;

static WSEGLCaps asWSCaps[] =
{
	{ WSEGL_CAP_WINDOWS_USE_HW_SYNC, 1 },
	{ WSEGL_CAP_MIN_SWAP_INTERVAL, 0 },
	{ WSEGL_CAP_MAX_SWAP_INTERVAL, 5 },
	{ WSEGL_CAP_UNLOCKED, 1 },
	{ WSEGL_NO_CAPS, 0 }
};

#if defined(PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT) && \
    defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
#error PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT supersedes \
	   SUPPORT_ANDROID_COMPOSITION_BYPASS. Use it instead.
#endif

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) && \
    !defined(PVR_ANDROID_HAS_NATIVE_BUFFER_TRANSFORM)
#error SUPPORT_ANDROID_COMPOSITION_BYPASS is only supported on \
	   PVR_ANDROID_HAS_NATIVE_BUFFER_TRANSFORM platforms.
#endif

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)

/* For new and old bypass implementations, we can read the bypass usage
 * from the native handle, rather than the buffer handle.
 */

static inline int IsBypassBuffer(ANativeWindowBuffer *psNativeBuffer)
{
	IMG_native_handle_t *psNativeHandle =
		(IMG_native_handle_t *)psNativeBuffer->handle;
	return usage_bypass(psNativeHandle->usage);
}

#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

/*************************** WINDOW PRIVATE DATA *****************************/

typedef struct _ANDROID_WSEGL_window_private_data_
{
	/* Window associated with this private data */
	ANativeWindow *psNativeWindow;

	/* The buffer currently dequeued for this Window */
	ANativeWindowBuffer *psNativeBufferCurrent;

	/* Number of DC flips to wait for */
	IMG_UINT32 ui32SwapInterval;

	/* The rotation, dimensions and pixel format of the buffer 
	 * that was last dequeued
	 */
	WSEGLRotationAngle eLastDequeuedRotation;
	IMG_INT iLastDequeuedWidth;
	IMG_INT iLastDequeuedHeight;
	IMG_BOOL bResizing;

	/* Transform to be applied to this window */
	WSEGLRotationAngle ePendingRotation;

	/* How many times is this window current? */
	IMG_UINT32 ui32CurrentRefCount;

	/* Parameters for the "previous" buffer.
	 * FIXME: Probably not safe to cache these.
	 */
	struct
	{
		IMG_INT				iWidth;
		IMG_INT				iHeight;
		IMG_INT				iStride;
		IMG_INT				iFormat;
		WSEGLRotationAngle	eRotation;
		IMG_UINT64			ui64Stamp;
	}
	sPrevious;

	/* Next private data */
	struct _ANDROID_WSEGL_window_private_data_ *psNext;
}
ANDROID_WSEGL_window_private_data_t;

static ANDROID_WSEGL_window_private_data_t *psWindowPrivateDataList;

static ANDROID_WSEGL_window_private_data_t *
GetWindowPrivateData(ANativeWindow *psNativeWindow)
{
	ANDROID_WSEGL_window_private_data_t *psEntry;

	PVR_ASSERT(psNativeWindow != IMG_NULL);

	for(psEntry = psWindowPrivateDataList; psEntry; psEntry = psEntry->psNext)
		if(psEntry->psNativeWindow == psNativeWindow)
			break;

	/* May be IMG_NULL if window hasn't been seen */
	return psEntry;
}

static ANDROID_WSEGL_window_private_data_t *
AllocWindowPrivateData(ANativeWindow *psNativeWindow)
{
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;

	PVR_ASSERT(psNativeWindow != IMG_NULL);

	psWindowPrivateData =
		calloc(1, sizeof(ANDROID_WSEGL_window_private_data_t));

	psWindowPrivateData->ui32SwapInterval	= 1;
	psWindowPrivateData->psNativeWindow		= psNativeWindow;
	psWindowPrivateData->psNext				= psWindowPrivateDataList;

	psWindowPrivateDataList = psWindowPrivateData;
	return psWindowPrivateData;
}

static void
DeleteWindowPrivateData(ANativeWindow *psNativeWindow)
{
	ANDROID_WSEGL_window_private_data_t *psEntry, *psPrev = IMG_NULL;

	/* Impossible */
	if(!psNativeWindow)
		return;

	for(psEntry = psWindowPrivateDataList; psEntry; psEntry = psEntry->psNext)
	{
		if(psEntry->psNativeWindow == psNativeWindow)
			break;
		psPrev = psEntry;
	}

	/* Impossible */
	if(!psEntry)
		return;

	if(psPrev)
		psPrev->psNext = psEntry->psNext;
	else
		psWindowPrivateDataList = psEntry->psNext;

	free(psEntry);
}

/************************* END WINDOW PRIVATE DATA ***************************/

static WSEGLError ObtainWindowParams(ANativeWindow *psNativeWindow,
									 int *piFormat, int *piWidth,
									 int *piHeight)
{
	WSEGLError eError = WSEGL_BAD_DRAWABLE;

	ENTERED();

	if(psNativeWindow->query(psNativeWindow, NATIVE_WINDOW_FORMAT, piFormat))
		goto err_out;

	if(psNativeWindow->query(psNativeWindow, NATIVE_WINDOW_WIDTH, piWidth))
		goto err_out;

	if(psNativeWindow->query(psNativeWindow, NATIVE_WINDOW_HEIGHT, piHeight))
		goto err_out;

	eError = WSEGL_SUCCESS;

err_out:
	EXITED();
	return eError;
}

static WSEGLPixelFormat HAL2WSEGLPixelFormat(int iFormat)
{
#define	WSEGL_PIXELFORMAT_BAD 0xff
	const IMG_buffer_format_public_t *psBufferFormats =
		psHal->GetBufferFormats();

	while(psBufferFormats != IMG_NULL)
	{
		if(psBufferFormats->iHalPixelFormat == iFormat)
			return psBufferFormats->iWSEGLPixelFormat;
		psBufferFormats = psBufferFormats->psNext;
	}

	return WSEGL_PIXELFORMAT_BAD;
}

#if defined(PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT) || \
	defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)

static WSEGLRotationAngle HAL2WSEGLRotationAngle(int iTransform)
{
	static const WSEGLRotationAngle aeHAL2WSEGLRotationAngle[] =
	{
#define WSEGL_ROTATE_BAD			0xff
#define NUM_KNOWN_HAL_ROTATIONS		8
		WSEGL_ROTATE_0,		/* No HAL enumerant                            */
		WSEGL_ROTATE_BAD,	/* HAL_TRANSFORM_FLIP_H                        */
		WSEGL_ROTATE_BAD,	/* HAL_TRANSFORM_FLIP_V                        */
		WSEGL_ROTATE_180,	/* HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_FLIP_V */
		WSEGL_ROTATE_270,	/* HAL_TRANSFORM_ROT_90                        */
		WSEGL_ROTATE_BAD,	/* HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90 */
		WSEGL_ROTATE_BAD,	/* HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90 */
		WSEGL_ROTATE_90,	/* HAL_TRANSFORM_ROT_270                       */
	};

	/* Ignore invalid or unsupported transforms */
	if(iTransform < 0 ||
	   iTransform >= NUM_KNOWN_HAL_ROTATIONS ||
	   aeHAL2WSEGLRotationAngle[iTransform] == WSEGL_ROTATE_BAD)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Unrecognized orientation 0x%x. ",
								  __func__, iTransform));
		return WSEGL_ROTATE_0;
	}

	return aeHAL2WSEGLRotationAngle[iTransform];
}

#endif /* defined(PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT) ||
		  defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

static WSEGLError DequeueLockStoreBuffer(ANativeWindow *psNativeWindow)
{
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLRotationAngle ePendingRotation = WSEGL_ROTATE_0;
	WSEGLError err = WSEGL_BAD_NATIVE_WINDOW;
	ANativeWindowBuffer *psNativeBuffer;

	ENTERED();

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	psNativeBuffer = psWindowPrivateData->psNativeBufferCurrent;

	PVRSRVUnlockMutex(hMutex);

#if defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
	/* If we support connect/disconnect, we will be cancel the buffer
	 * in make-non-current, so either we're swapping (we just queued
	 * a buffer) or this is make-current (and we would have no buffer
	 * dequeued).
	 *
	 * So if we see a buffer has already been dequeued, return an error.
	 */
	if(psNativeBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Can't dequeue a buffer when we "
								"already have another dequeued!", __func__));
		goto err_out;
	}
#else /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */
	/* If we don't support connect/disconnect, we're either a very
	 * old (pre-Froyo) or very new (post-HC) version of Android.
	 *
	 * In either case, if a buffer was made current, then non-current,
	 * then current again, it may already be dequeued and we shouldn't
	 * dequeue another.
	 */
	if(psNativeBuffer)
		goto exit_out;
#endif /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */

#if defined(PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT)
#if defined(PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT)
	/* We only kick-off the rotation logic if we have a HWC module
	 * loaded. Otherwise the compositor will handle rotation in GL
	 */
	{
		int iTransform;

		if(psNativeWindow->query(psNativeWindow,
								 NATIVE_WINDOW_TRANSFORM_HINT, &iTransform))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to query TRANSFORM_HINT",
									__func__));
			goto err_out;
		}

		ePendingRotation = HAL2WSEGLRotationAngle(iTransform);
	}
#endif /* defined(PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT) */
	{
		int iWidth, iHeight;

		if(psNativeWindow->query(psNativeWindow,
								 NATIVE_WINDOW_DEFAULT_WIDTH, &iWidth))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to query DEFAULT_WIDTH",
									__func__));
			goto err_out;
		}

		if(psNativeWindow->query(psNativeWindow,
								 NATIVE_WINDOW_DEFAULT_HEIGHT, &iHeight))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to query DEFAULT_HEIGHT",
									__func__));
			goto err_out;
		}

		/* If we have the transform hint, make sure we ask the compositor
		 * to swap the width/height of the next dequeued buffer so we can
		 * handle rotated rendering.
		 */
		if(ePendingRotation == WSEGL_ROTATE_90 ||
		   ePendingRotation == WSEGL_ROTATE_270)
		{
			int iTmp = iWidth;
			iWidth = iHeight;
			iHeight = iTmp;
		}

		if(native_window_set_buffers_dimensions(psNativeWindow,
												iWidth, iHeight))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set buffer dimensions",
									__func__));
			goto err_out;
		}
	}
#endif /* defined(PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT) */

	/* De-queue a buffer; the next render will use it */
	if(psNativeWindow->dequeueBuffer(psNativeWindow, &psNativeBuffer))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to de-queue buffer", __func__));
		goto err_out;
	}

	/* Bump refcount on buffer; we don't want it to be freed underneath us */
	psNativeBuffer->common.incRef(&psNativeBuffer->common);

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
	/* If we don't have the transform hint, we might support the old
	 * composition bypass scheme. In this case the transform is
	 * embedded on the ANativeWindowBuffer.
	 *
	 * We assume that the composition bypass platform patch has
	 * already re-allocated buffers with the required width/height.
	 */
	if(IsBypassBuffer(psNativeBuffer))
	{
		ePendingRotation = HAL2WSEGLRotationAngle(psNativeBuffer->transform);
	}
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

	/* Lock the buffer -- it will be used for render shortly */
	if(psNativeWindow->lockBuffer(psNativeWindow, psNativeBuffer))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lock buffer %p",
				 __func__, psNativeBuffer));
		goto err_out;
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	/* Store buffer on window private data */
	psWindowPrivateData->psNativeBufferCurrent = psNativeBuffer;

	/* Remember the rotation, so even if it changes under our feet,
	 * we'll call native_window_set_transform() appropriately before
	 * queueBuffer().
	 */
	psWindowPrivateData->ePendingRotation = ePendingRotation;

	PVRSRVUnlockMutex(hMutex);

	if(HAL2WSEGLPixelFormat(psNativeBuffer->format) == WSEGL_PIXELFORMAT_BAD)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Buffer pixel format was invalid",
				 __func__));
		goto err_out;
	}

#if defined(DEBUG)
	/* Pre-Froyo versions of Android may not have matching formats
	 * on the native window and buffers of that window. Warn if this
	 * is detected (it is a non-fatal condition). Don't bother checking
	 * full-screen composition bypass buffers -- they break the rules.
	 */
#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
	if(!IsBypassBuffer(psNativeBuffer))
#endif
	{
		int iFormat, iWidth, iHeight;

		err = ObtainWindowParams(psNativeWindow, &iFormat, &iWidth, &iHeight);
		if(err)
			goto err_out;

		if(ePendingRotation == WSEGL_ROTATE_90 ||
		   ePendingRotation == WSEGL_ROTATE_270)
		{
			IMG_native_handle_t *psNativeHandle =
				(IMG_native_handle_t *)psNativeBuffer->handle;

			/* Make sure the buffer and handle dimensions match */
			if(psNativeBuffer->width != psNativeHandle->iWidth ||
			   psNativeBuffer->height != psNativeHandle->iHeight)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Transformed buffer dimensions "
										"{%d,%d} do not match handle "
										"dimensions {%d,%d}!", __func__,
										psNativeBuffer->width,
										psNativeBuffer->height,
										psNativeHandle->iWidth,
										psNativeHandle->iHeight));
			}

			/* And that they both match swapped window dimensions */
			if(psNativeBuffer->width != iHeight ||
			   psNativeBuffer->height != iWidth)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Transformed buffer dimensions "
										"{%d,%d} are not swapped window "
										"dimensions {%d,%d}!", __func__,
										psNativeBuffer->width,
										psNativeBuffer->height,
										iWidth, iHeight));
			}
		}
		else
		{
			if(psNativeBuffer->width != iWidth)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Buffer width %d did not match "
										"window width %d",
						 __func__, psNativeBuffer->width, iWidth));
			}

			if(psNativeBuffer->height != iHeight)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Buffer height %d did not match "
										"window height %d",
						 __func__, psNativeBuffer->height, iHeight));
			}
		}

		if(psNativeBuffer->format != iFormat)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Buffer format %d did not match "
									"window format %d",
					 __func__, psNativeBuffer->format, iFormat));
		}
	}
#endif /* defined(DEBUG) */

#if !defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
exit_out:
#endif
	err = WSEGL_SUCCESS;
err_out:
	EXITED();
	return err;
}

typedef int (*ANDROID_WSEGL_unlock_action_t)
	(ANativeWindow *window, ANativeWindowBuffer *buffer);

static WSEGLError UnlockPostBuffer(ANativeWindow *psNativeWindow,
								   ANDROID_WSEGL_unlock_action_t pfnUnlockAction)
{
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLRotationAngle eLastDequeuedRotation;
	ANativeWindowBuffer *psNativeBuffer;
	WSEGLError err = WSEGL_BAD_DRAWABLE;

	ENTERED();

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	psNativeBuffer = psWindowPrivateData->psNativeBufferCurrent;
	eLastDequeuedRotation = psWindowPrivateData->eLastDequeuedRotation;

	PVRSRVUnlockMutex(hMutex);

#if defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
	/* If we have connect/disconnect, the buffer should not have been
	 * queued or cancelled before this call. This is because this
	 * function is called only in DisconnectDrawable, which is
	 * called when a surface is made non-current. This can happen
	 * only if a surface was previously current, and therefore had
	 * a buffered dequeued.
	 *
	 * Hitting this case indicates a serious bug in the WSEGL module,
	 * or improper application behaviour.
	 */
	if(!psNativeBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Can't queue a buffer we "
								"never dequeued!", __func__));
		goto err_out;
	}
#else /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */
	/* If we don't support connect/disconnect, we're either a very
	 * old (pre-Froyo) or very new (post-HC) version of Android.
	 *
	 * In these versions, a surface might be created then destroyed
	 * without ever being made current. So there may be no dequeued
	 * buffer to be queued or cancelled.
	 */
	if(!psNativeBuffer)
		goto exit_out;
#endif /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */

#if defined(PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT)
	/* If we're about to cancel the buffer, we don't need to care
	 * about setting the buffer transform. In fact, doing so may
	 * fail and cause a message to be printed.
	 */
	if(pfnUnlockAction == psNativeWindow->queueBuffer)
	{
		/* WSEGL rotations are inverted wrt HAL rotations */
		static const int aiWSEGLRotationAngle2HALInverse[] =
		{
			0,						/* WSEGL_ROTATE_0   */
			HAL_TRANSFORM_ROT_90,	/* WSEGL_ROTATE_90  */
			HAL_TRANSFORM_ROT_180,	/* WSEGL_ROTATE_180 */
			HAL_TRANSFORM_ROT_270,	/* WSEGL_ROTATE_270 */
		};

		int iTransform =
			aiWSEGLRotationAngle2HALInverse[eLastDequeuedRotation];

		if(native_window_set_buffers_transform(psNativeWindow, iTransform))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set buffer transform",
									__func__));
			/* Fall-thru */
		}
	}
#endif /* defined(PVR_ANDROID_HAS_WINDOW_TRANSFORM_HINT) */

	/* The caller supplies a function to perform the desired unlock action.
	   This might queue and post the buffer, or cancel it. */
	if(pfnUnlockAction(psNativeWindow, psNativeBuffer))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to handle queue or cancel of "
				 "buffer %p", __func__, psNativeBuffer));
		goto err_out;
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	/* Strictly speaking, we should leave the last rendered buffer
	 * dequeued, and not drop its refcount, because the HW needs to
	 * read the surface to accumulate from it.
	 *
	 * In practice this not possible, especially on older versions of
	 * Android, because we will starve the compositor of buffers to
	 * composite from if we always have two dequeued in steady state.
	 *
	 * We only need to worry about cases where the buffer may be
	 * ripped out from underneath the driver. This is inside the call
	 * to dequeueBuffer(), where the framework may handle a window
	 * resize/destroy by calling unregisterBuffer(). Even if we defer
	 * the destruction of the underlying memory (with sync objects or
	 * SetAccumBuffer()), the platform may destroy the ANativeBuffer
	 * and native handle. So we can't use these structures directly
	 * to obtain information about the accumulation buffer.
	 *
	 * As a workaround, we copy the necessary psNativeBuffer metadata
	 * and buffer stamp, and whenever GL asks for the accumulation
	 * buffer parameters, we check the meminfo is still mapped in
	 * that process. If it isn't, we just won't accumulate.
	 */
	psWindowPrivateData->sPrevious.iWidth = psNativeBuffer->width;
	psWindowPrivateData->sPrevious.iHeight = psNativeBuffer->height;
	psWindowPrivateData->sPrevious.iFormat = psNativeBuffer->format;
	psWindowPrivateData->sPrevious.iStride = psNativeBuffer->stride;
	psWindowPrivateData->sPrevious.eRotation = eLastDequeuedRotation;
	psWindowPrivateData->sPrevious.ui64Stamp =
		((IMG_native_handle_t *)psNativeBuffer->handle)->ui64Stamp;

	psNativeBuffer->common.decRef(&psNativeBuffer->common);

	/* Explicitly clear psNativeBufferCurrent for sanity checking */
	psWindowPrivateData->psNativeBufferCurrent = IMG_NULL;

	PVRSRVUnlockMutex(hMutex);

#if !defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
exit_out:
#endif
	err = WSEGL_SUCCESS;
err_out:
	EXITED();
	return err;
}

static inline WSEGLError QueueUnlockPostBuffer(ANativeWindow *psNativeWindow)
{
	return UnlockPostBuffer(psNativeWindow, psNativeWindow->queueBuffer);
}

static inline WSEGLError CancelUnlockPostBuffer(ANativeWindow *psNativeWindow)
{
	return UnlockPostBuffer(psNativeWindow,
#if defined(PVR_ANDROID_HAS_CANCELBUFFER)
			/* We don't want to post the buffer - this might cause
			 * flickering, as the buffer might not have been rendered to. If
			 * a cancelBuffer function is available, use that in preference
			 * to queueBuffer.
			 */
			psNativeWindow->cancelBuffer ?
			psNativeWindow->cancelBuffer :
#endif /* defined(PVR_ANDROID_HAS_CANCELBUFFER) */
			psNativeWindow->queueBuffer);
}

static WSEGLError ValidateWindowConfigFormat(int iFormat, WSEGLConfig *psConfig)
{
	WSEGLPixelFormat eConfigFormat, eWindowFormat;

	eWindowFormat = HAL2WSEGLPixelFormat(iFormat);
	if(eWindowFormat == WSEGL_PIXELFORMAT_BAD)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unsupported framebuffer pixel "
								"format (%d)", __func__, iFormat));
		return WSEGL_BAD_NATIVE_WINDOW;
	}

	eConfigFormat = HAL2WSEGLPixelFormat(psConfig->ulNativeVisualID);
	if(eConfigFormat == WSEGL_PIXELFORMAT_BAD)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unsupported config pixel format (%lu)",
								__func__, psConfig->ulNativeVisualID));
		return WSEGL_BAD_MATCH;
	}

	/* Open-code extra-permissive compatibility check for BGRA/RGBA.
	 *
	 * EGL clients can't differentiate between these configs -- except
	 * with EGL_NATIVE_VISUAL_ID -- and most won't.
	 *
	 * On newer versions of Android we'll only have one or the other
	 * of these configs, not both. So we can be strict again.
	 */
#if defined(PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER) || \
	!defined(SUPPORT_HAL_PIXEL_FORMAT_BGRX)
	if(eConfigFormat == WSEGL_PIXELFORMAT_ARGB8888 ||
	   eConfigFormat == WSEGL_PIXELFORMAT_ABGR8888)
	{
		if(eWindowFormat != WSEGL_PIXELFORMAT_ARGB8888 &&
		   eWindowFormat != WSEGL_PIXELFORMAT_ABGR8888)
			goto err_match;
	}
	else
#endif /* !defined(SUPPORT_HAL_PIXEL_FORMAT_BGRX) */
	{
		if(eWindowFormat != eConfigFormat)
			goto err_match;
	}

	return WSEGL_SUCCESS;

err_match:
	PVR_DPF((PVR_DBG_ERROR, "%s: Window and Config pixel formats do "
							"not match", __func__));
	PVR_DPF((PVR_DBG_ERROR, "%s: Window format was %d, Config "
							"format was %lu",
			 __func__, iFormat, psConfig->ulNativeVisualID));
	return WSEGL_BAD_MATCH;
}

static WSEGLError WSEGL_IsDisplayValid(NativeDisplayType hNativeDisplay)
{
	WSEGL_UNREFERENCED_PARAMETER(hNativeDisplay);

	ENTERED();

	/* FIXME: Compositor running validation required? */

	EXITED();

	return WSEGL_SUCCESS;
}

static WSEGLError WSEGL_InitialiseDisplay(NativeDisplayType hNativeDisplay,
										  WSEGLDisplayHandle *phDisplay,
										  const WSEGLCaps **psCapabilities,
										  WSEGLConfig **psConfigs)
{
	const IMG_buffer_format_public_t *psBufferFormats, *psEntry;
	IMG_UINT32 ui32NumGPURenderables = 0;
	WSEGLError err = WSEGL_SUCCESS;
	int i, iError;

	WSEGL_UNREFERENCED_PARAMETER(hNativeDisplay);

	ENTERED();

	*phDisplay = IMG_NULL;

	if(bAlreadyInitialised)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Already initialized. Why are we being "
								  "called again?", __func__));
		/* Success */
		goto err_out;
	}

	iError = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
						   (const hw_module_t **)&psHal);
	if(iError)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to load HAL module", __func__));
		err = WSEGL_CANNOT_INITIALISE;
		goto err_out;
	}

	psBufferFormats = psHal->GetBufferFormats();

	/* Figure out how large the config list is */
	for(psEntry = psBufferFormats; psEntry; psEntry = psEntry->psNext)
		if(psEntry->bGPURenderable)
			ui32NumGPURenderables++;

	/* Allocate and propapate the config list */
	gpsWSConfigs = calloc(ui32NumGPURenderables + 1, sizeof(WSEGLConfig));
	for(psEntry = psBufferFormats, i = 0; psEntry; psEntry = psEntry->psNext)
	{
		if(!psEntry->bGPURenderable)
			continue;
		gpsWSConfigs[i].ui32DrawableType = WSEGL_DRAWABLE_WINDOW;
		gpsWSConfigs[i].ulNativeVisualID = psEntry->iHalPixelFormat;
		gpsWSConfigs[i].ePixelFormat = psEntry->iWSEGLPixelFormat;
		i++;
	}

	/* Terminate the config list */
	gpsWSConfigs[i].ui32DrawableType = WSEGL_NO_DRAWABLE;

	*psCapabilities = asWSCaps;
	*psConfigs = gpsWSConfigs;

	if (PVRSRVCreateMutex(&hMutex) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create mutex", __func__));
		err = WSEGL_CANNOT_INITIALISE;
		goto err_out;
	}

	bAlreadyInitialised = IMG_TRUE;

err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_CloseDisplay(WSEGLDisplayHandle hDisplay)
{
	WSEGL_UNREFERENCED_PARAMETER(hDisplay);

	ENTERED();

	if(!bAlreadyInitialised)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Already closed. Why are we being "
								  "called again?", __func__));
		goto err_out;
	}

	if (PVRSRVDestroyMutex(hMutex) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to destroy mutex", __func__));
	}

	free(gpsWSConfigs);

	bAlreadyInitialised = IMG_FALSE;

err_out:
	EXITED();
	return WSEGL_SUCCESS;
}

static WSEGLError WSEGL_CreateWindowDrawable(WSEGLDisplayHandle hDisplay,
											 WSEGLConfig *psConfig,
											 WSEGLDrawableHandle *phDrawable,
											 NativeWindowType hNativeWindow,
											 WSEGLRotationAngle *eRotationAngle)
{
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLError err = WSEGL_SUCCESS;
	ANativeWindow *psNativeWindow;
	int iFormat;

	WSEGL_UNREFERENCED_PARAMETER(hDisplay);

	ENTERED();

	/* Impossible; IMGEGL will never pass a NULL config */
	PVR_ASSERT(psConfig != IMG_NULL);

	psNativeWindow = (ANativeWindow *)hNativeWindow;
	if(!psNativeWindow)
	{
		err = WSEGL_BAD_NATIVE_WINDOW;
		goto err_out;
	}

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		err = WSEGL_BAD_NATIVE_WINDOW;
		goto err_out;
	}

	/* The philosophy for config validation is that we assume the WSConfigs
	 * table exports ALL formats with a valid mapping from Android to WSEGL.
	 *
	 * It is then enough to check that the window format matches one of the
	 * entries in this table, and additionally matches the format of the
	 * supplied EGLConfig.
	 */

	if(psNativeWindow->query(psNativeWindow, NATIVE_WINDOW_FORMAT, &iFormat))
	{
		err = WSEGL_BAD_NATIVE_WINDOW;
		goto err_out;
	}

	err = ValidateWindowConfigFormat(iFormat, psConfig);
	if(err)
		goto err_out;

	PVRSRVLockMutex(hMutex);

	/* We may be in the process of resizing; in this case, there's no real
	 * work to do, and we can just dummy process the rest for EGL's benefit.
	 */
	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	if(psWindowPrivateData)
	{
#if defined(PVRSRV_NEED_PVR_ASSERT)
		PVR_ASSERT(psWindowPrivateData->bResizing == IMG_TRUE);
#else
		if (!psWindowPrivateData->bResizing)
			err = WSEGL_BAD_NATIVE_WINDOW;
#endif
		goto err_unlock;
	}

	/* We know hNativeWindow is unique, because eglCreateWindowSurface()
	 * blocks calling it twice with the same NativeWindowType, before it
	 * gets into WSEGL.
	 */

	psNativeWindow->common.incRef(&psNativeWindow->common);

	/* Allocate private data for the window */
	psWindowPrivateData = AllocWindowPrivateData(psNativeWindow);

err_unlock:
	*phDrawable = (WSEGLDrawableHandle)psNativeWindow;
	*eRotationAngle = psWindowPrivateData->eLastDequeuedRotation;
	PVRSRVUnlockMutex(hMutex);
err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_CreatePixmapDrawable(WSEGLDisplayHandle hDisplay,
											 WSEGLConfig *psConfig,
											 WSEGLDrawableHandle *phDrawable,
											 NativePixmapType hNativePixmap,
											 WSEGLRotationAngle *eRotationAngle)
{
	ANativeWindowBuffer *psNativeBuffer;
	WSEGLError err = WSEGL_SUCCESS;

	WSEGL_UNREFERENCED_PARAMETER(hDisplay);

	ENTERED();

	/* Only eglCreateImageKHR should use this, and that passes no config */
	if(psConfig)
	{
		err = WSEGL_BAD_MATCH;
		goto err_out;
	}

	psNativeBuffer = (ANativeWindowBuffer *)hNativePixmap;
	if(!psNativeBuffer)
	{
		err = WSEGL_BAD_NATIVE_PIXMAP;
		goto err_out;
	}

	if(psNativeBuffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC)
	{
		err = WSEGL_BAD_NATIVE_PIXMAP;
		goto err_out;
	}

	/* We may need to use this for longer than SurfaceFlinger does */
	psNativeBuffer->common.incRef(&psNativeBuffer->common);

	*phDrawable = (WSEGLDrawableHandle)psNativeBuffer;
	*eRotationAngle = WSEGL_ROTATE_0;

err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_DeleteDrawable(WSEGLDrawableHandle hDrawable)
{
	android_native_base_t *psNativeBase = (android_native_base_t *)hDrawable;
	ANativeWindowBuffer *psNativeBuffer;
	WSEGLError err = WSEGL_SUCCESS;

	ENTERED();

	if(psNativeBase->magic == ANDROID_NATIVE_WINDOW_MAGIC)
	{
		ANativeWindow *psNativeWindow = (ANativeWindow *)psNativeBase;
		ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;

		PVRSRVLockMutex(hMutex);

		psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
		PVR_ASSERT(psWindowPrivateData != IMG_NULL);

		/* We may be in the process of resizing; in this case, there's no
		 * real work to do, and we can just dummy process the rest for EGL's
		 * benefit.
		 */
		if(psWindowPrivateData->bResizing)
		{
			psWindowPrivateData->bResizing = IMG_FALSE;
			goto exit_unlock;
		}

#if !defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
		/* Cancel/re-queue the buffer (and drop refcount on buffer) */
		PVRSRVUnlockMutex(hMutex);
		CancelUnlockPostBuffer(psNativeWindow);
		PVRSRVLockMutex(hMutex);
#endif /* !defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */

		/* Free private data */
		DeleteWindowPrivateData(psNativeWindow);

		/* Done with the window now */
		psNativeWindow->common.decRef(&psNativeWindow->common);

exit_unlock:
		PVRSRVUnlockMutex(hMutex);
	}
	else if(psNativeBase->magic == ANDROID_NATIVE_BUFFER_MAGIC)
	{
		/* Probably a buffer from EGLImage */
		psNativeBuffer = (ANativeWindowBuffer *)psNativeBase;

		/* We're done with the buffer now */
		psNativeBuffer->common.decRef(&psNativeBuffer->common);
	}
	else
	{
		/* No other magic is supported */
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic (drawable %p)",
				 __func__, psNativeBase));
		err = WSEGL_BAD_DRAWABLE;
	}

	EXITED();
	return err;
}

static WSEGLError WSEGL_SwapDrawable(WSEGLDrawableHandle hDrawable, unsigned long ui32Data)
{
	ANativeWindow *psNativeWindow = (ANativeWindow *)hDrawable;
	WSEGLError err = WSEGL_BAD_DRAWABLE;

	WSEGL_UNREFERENCED_PARAMETER(ui32Data);

	ENTERED();

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		goto err_out;
	}

	err = QueueUnlockPostBuffer(psNativeWindow);
	if(err)
		goto err_out;

	err = DequeueLockStoreBuffer(psNativeWindow);

err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_SwapControlInterval(WSEGLDrawableHandle hDrawable, unsigned long ulInterval)
{
	ANativeWindow *psNativeWindow = (ANativeWindow *)hDrawable;
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLError err = WSEGL_SUCCESS;

	ENTERED();

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		err = WSEGL_BAD_DRAWABLE;
		goto err_out;
	}

	/* FIXME: Currently, Android only implements setSwapInterval() on
	 *        framebuffer windows. As we want to be able to set client swap
	 *        intervals too, we'll call the hook but store the interval in
	 *        the WSEGL module. When the problem is fixed, this code should
	 *        be removed, and the implementation moved to fakehal.
	 */
	if(psNativeWindow->setSwapInterval(psNativeWindow, ulInterval))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set swap interval", __func__));
		err = WSEGL_BAD_MATCH;
		goto err_out;
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	psWindowPrivateData->ui32SwapInterval = (IMG_UINT32)ulInterval;

	PVRSRVUnlockMutex(hMutex);

err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_WaitNative(WSEGLDrawableHandle hDrawable, unsigned long ui32Engine)
{
	ANativeWindow *psNativeWindow = (ANativeWindow *)hDrawable;
	WSEGLError err = WSEGL_SUCCESS;

	ENTERED();

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		err = WSEGL_BAD_DRAWABLE;
		goto err_out;
	}

	/* Support only the default native engine */
	if(ui32Engine != WSEGL_DEFAULT_NATIVE_ENGINE)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized engine", __func__));
		err = WSEGL_BAD_NATIVE_ENGINE;
		goto err_out;
	}

err_out:
	EXITED();
	return err;
}

static WSEGLError WSEGL_CopyFromDrawable(WSEGLDrawableHandle hDrawable, NativePixmapType hNativePixmap)
{
	WSEGL_UNREFERENCED_PARAMETER(hDrawable);
	WSEGL_UNREFERENCED_PARAMETER(hNativePixmap);

	ENTERED();

	/* ANDROID does not support Pixmaps */
	PVR_DPF((PVR_DBG_ERROR, "%s: Unimplemented", __func__));

	EXITED();
	return WSEGL_BAD_NATIVE_PIXMAP;
}

static WSEGLError WSEGL_CopyFromPBuffer(void *pvAddress,
										unsigned long ui32Width,
										unsigned long ui32Height,
										unsigned long ui32Stride,
										WSEGLPixelFormat ePixelFormat,
										NativePixmapType hNativePixmap)
{
	WSEGL_UNREFERENCED_PARAMETER(pvAddress);
	WSEGL_UNREFERENCED_PARAMETER(ui32Width);
	WSEGL_UNREFERENCED_PARAMETER(ui32Height);
	WSEGL_UNREFERENCED_PARAMETER(ui32Stride);
	WSEGL_UNREFERENCED_PARAMETER(ePixelFormat);
	WSEGL_UNREFERENCED_PARAMETER(hNativePixmap);

	ENTERED();

	/* ANDROID does not support Pixmaps */
	PVR_DPF((PVR_DBG_ERROR, "%s: Unimplemented", __func__));

	EXITED();
	return WSEGL_BAD_NATIVE_PIXMAP;
}

static void CopyRenderToSource(WSEGLDrawableParams *psRenderParams, WSEGLDrawableParams *psSourceParams)
{
	psSourceParams->ui32Width		= psRenderParams->ui32Width;
	psSourceParams->ui32Height		= psRenderParams->ui32Height;
	psSourceParams->ui32Stride		= psRenderParams->ui32Stride;
	psSourceParams->ePixelFormat	= psRenderParams->ePixelFormat;
	psSourceParams->pvLinearAddress	= psRenderParams->pvLinearAddress;
	psSourceParams->ui32HWAddress	= psRenderParams->ui32HWAddress;
#if defined(SUPPORT_NV12_FROM_2_HWADDRS)
	psSourceParams->ui32HWAddress2	= psRenderParams->ui32HWAddress2;
#endif
}

static WSEGLError MapBufferObtainParams(IMG_UINT64 ui64BufferStamp,
										IMG_UINT64 ui64BufferStampPrevious,
										WSEGLDrawableParams *psSourceParams,
										WSEGLDrawableParams *psRenderParams,
										int usage)
{
	gralloc_module_t *psGralloc = (gralloc_module_t *)psHal;
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS];
	WSEGLError eError = WSEGL_BAD_DRAWABLE;

	ENTERED();

	/* Impossible; only call site dereferences a valid native_handle_t */
	PVR_ASSERT(ui64BufferStamp != 0);

	if(psHal->map(psGralloc, ui64BufferStamp, usage, apsMemInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Mapping buffer failed", __func__));
		goto err_out;
	}

	PVR_ASSERT(apsMemInfo[0] != IMG_NULL);
	psRenderParams->pvLinearAddress = apsMemInfo[0]->pvLinAddr;
	if( (psRenderParams->ePixelFormat == WSEGL_PIXELFORMAT_NV12) && (apsMemInfo[0]->ui32Flags & PVRSRV_MEM_ION))
	{
		psRenderParams->ui32HWAddress   = apsMemInfo[0]->sDevVAddr.uiAddr + apsMemInfo[0]->planeOffsets[0];
	}
	else
	{
		psRenderParams->ui32HWAddress   = apsMemInfo[0]->sDevVAddr.uiAddr;
	}
	psRenderParams->hPrivateData    = apsMemInfo[0];

#if defined(SUPPORT_NV12_FROM_2_HWADDRS)
	if(psRenderParams->ePixelFormat == WSEGL_PIXELFORMAT_NV12)
	{
		if( (apsMemInfo[0]->ui32Flags & PVRSRV_MEM_ION) && (apsMemInfo[0]->planeOffsets[1] != 0))
			psRenderParams->ui32HWAddress2 = apsMemInfo[0]->sDevVAddr.uiAddr + apsMemInfo[0]->planeOffsets[1];
		else
			psRenderParams->ui32HWAddress2 = 0;
	}
#endif

	/* If we're definitely the Android compositor, and this is a
	 * framebuffer window, we'll need to wait for renders to complete
	 * before posting the frame (for synchronization purposes).
	 *
	 * We'd better not do it for all framebuffer windows, otherwise it will
	 * negatively impact tests like GridMark3 when run to framebuffer.
	 */
	psRenderParams->bWaitForRender = usage_fb(usage) && psHal->IsCompositor();

	if(psHal->unmap(psGralloc, ui64BufferStamp))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unmapping buffer failed", __func__));
		goto err_out;
	}

	/* Default to no accum syncronization */
	psSourceParams->hPrivateData = IMG_NULL;

	if(ui64BufferStampPrevious)
	{
		/* The previous buffer might have been unmapped. If so, we
		 * can't accumulate on this frame and must abort cleanly.
	 	 */
		if(psHal->map(psGralloc, ui64BufferStampPrevious, usage, apsMemInfo))
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Mapping previous buffer failed",
					 __func__));
			goto skip_accum;
		}

		PVR_ASSERT(apsMemInfo[0] != IMG_NULL);
		psSourceParams->pvLinearAddress = apsMemInfo[0]->pvLinAddr;

		if( (psSourceParams->ePixelFormat == WSEGL_PIXELFORMAT_NV12) && (apsMemInfo[0]->ui32Flags & PVRSRV_MEM_ION))
		{
			psSourceParams->ui32HWAddress   = apsMemInfo[0]->sDevVAddr.uiAddr + apsMemInfo[0]->planeOffsets[0];
		}
		else
		{
			psSourceParams->ui32HWAddress   = apsMemInfo[0]->sDevVAddr.uiAddr;
		}

#if defined(SUPPORT_NV12_FROM_2_HWADDRS)
		if(psSourceParams->ePixelFormat == WSEGL_PIXELFORMAT_NV12)
		{
			if( (apsMemInfo[0]->ui32Flags & PVRSRV_MEM_ION) && (apsMemInfo[0]->planeOffsets[1] != 0))
				psSourceParams->ui32HWAddress2 = apsMemInfo[0]->sDevVAddr.uiAddr + apsMemInfo[0]->planeOffsets[1];
			else
				psSourceParams->ui32HWAddress2 = 0;
		}
#endif

		if(psHal->unmap(psGralloc, ui64BufferStampPrevious))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unmapping previous buffer failed",
					 __func__));
			goto err_out;
		}

#if !defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
		/* If these are frame buffers, we don't need to pass the accumulation
		 * meminfo through. NOTE: This is NOT equivalent to checking for whether
		 * we are the compositor; this also applies to framebuffer tests.
		 *
		 * The meminfo is only used for accumulation source synchronization,
		 * which causes unresolved performance problems in combination with
		 * the display controller. Additionally, the extra buffer destruction
		 * safety concerns don't apply to frame buffers.
		 */
		if(!usage_bypass_fb(usage))
			psSourceParams->hPrivateData = (IMG_HANDLE)apsMemInfo[0];
#endif
	}
	else
	{
skip_accum:
		/* If there's no previous buffer handle, we can't accumulate, so we
		 * need to overwrite any parameters we copied from the window private
		 * data with the render parameters.
		 */
		CopyRenderToSource(psRenderParams, psSourceParams);
	}

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
	/* Using sync objects for accumulation doesn't work if the display
	 * controller pins buffers indefinitely. Implement an alternative
	 * scheme for ensuring accumulation buffers are not prematurely
	 * destroyed.
	 *
	 * "Pair" the render params buffer with the accumulation buffer
	 * so gralloc knows to wait on the render buffer, not the
	 * accumulation buffer.
	 */
	{
		int err;
		err = psHal->SetAccumBuffer(psGralloc, ui64BufferStamp,
									ui64BufferStampPrevious);
		if(err)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: SetAccumBuffer failed", __func__));
		}
	}
#endif

	eError = WSEGL_SUCCESS;
err_out:
	EXITED();
	return eError;
}

static WSEGLError
UpdateWindowPrivateData(ANDROID_WSEGL_window_private_data_t *psWindowPrivateData,
						int iWidth, int iHeight)
{
	WSEGLRotationAngle eLast = psWindowPrivateData->eLastDequeuedRotation;
	WSEGLRotationAngle eNext = psWindowPrivateData->ePendingRotation;
	WSEGLError eError = WSEGL_SUCCESS;

	/* This code deals with two kinds of "invalidation".
	 *
	 * Drawable invalidation is most common. We always invalidate the
	 * drawable if the dimensions or rotation change, as the client
	 * drivers need to know what the new surface size is, the PBE
	 * rotation, or both.
	 *
	 * Accumulation buffer invalidation is less common. We'll only
	 * invalidate this if we have to:
	 *
	 *  - The window dimensions changed but the rotation did not;
	 *  - The (rotation adjusted) window dimensions changed.
	 *
	 * Accumulation invalidation is a grey area in the EGL spec -- the
	 * spec says that if pixels are "added or removed" from a window
	 * that the app must handle this itself. We interpret this as
	 * "EGL_BUFFER_PRESERVED can be ignored during transition frames".
	 */
	if((psWindowPrivateData->iLastDequeuedWidth > 0 &&
	    iWidth != psWindowPrivateData->iLastDequeuedWidth) ||
	   (psWindowPrivateData->iLastDequeuedHeight > 0 &&
	    iHeight != psWindowPrivateData->iLastDequeuedHeight) ||
	   (eLast != eNext))
	{
		IMG_BOOL bInvalidateAccum = IMG_TRUE;

		/* 0, 90, 180, 270 vs 0, 90, 180, 270 */
		static const IMG_BOOL bNeedMatchingWidthHeight[4][4] =
		{
			{ IMG_TRUE,  IMG_FALSE, IMG_TRUE,  IMG_FALSE, },
			{ IMG_FALSE, IMG_TRUE,  IMG_FALSE, IMG_TRUE,  },
			{ IMG_TRUE,  IMG_FALSE, IMG_TRUE,  IMG_FALSE, },
			{ IMG_FALSE, IMG_TRUE,  IMG_FALSE, IMG_TRUE,  },
		};

		/* We can handle accumulation of all rotation changes. But we
		 * need to ensure only the rotation changed, and the difference
		 * in the two rotations determines the sanity check to make.
		 */
		if(bNeedMatchingWidthHeight[eLast][eNext])
		{
			/* These rotation pairs need matching width/height */
			if(psWindowPrivateData->iLastDequeuedWidth  == iWidth &&
			   psWindowPrivateData->iLastDequeuedHeight == iHeight)
				bInvalidateAccum = IMG_FALSE;
		}
		else
		{
			/* These rotation pairs need swapped width/height */
			if(psWindowPrivateData->iLastDequeuedWidth  == iHeight &&
			   psWindowPrivateData->iLastDequeuedHeight == iWidth)
				bInvalidateAccum = IMG_FALSE;
		}

		if(bInvalidateAccum)
			psWindowPrivateData->sPrevious.ui64Stamp = 0;

		/* The client APIs may still have references to our (now invalid)
		 * resized render targets. Return WSEGL_RETRY to start the ball
		 * rolling with invalidating these pointers and re-creating the
		 * window.
		 */
		psWindowPrivateData->bResizing = IMG_TRUE;
		eError = WSEGL_RETRY;
	}

	/* Unconditionally update LastDequeued, which will only matter if
	 * it's the first time GetDrawableParameters has been called on
	 * this window, or a resize was detected. Updating these fields
	 * prevents the resize being incorrectly detected subsequently.
	 */
	psWindowPrivateData->iLastDequeuedWidth = iWidth;
	psWindowPrivateData->iLastDequeuedHeight = iHeight;
	psWindowPrivateData->eLastDequeuedRotation =
		psWindowPrivateData->ePendingRotation;
	return eError;
}

static WSEGLError
ObtainMinimalParams(ANDROID_WSEGL_window_private_data_t *psWindowPrivateData,
					WSEGLDrawableParams *psSourceParams,
					WSEGLDrawableParams *psRenderParams)
{
	int iFormat, iWidth, iHeight;
	WSEGLError eError;

	ENTERED();

	eError = ObtainWindowParams(psWindowPrivateData->psNativeWindow,
								&iFormat, &iWidth, &iHeight);
	if(eError)
		goto err_out;

	/* This works around an issue where an app has made this window
	 * surface current in the past, but makes it non-current and then
	 * starts querying surface attributes. Normally we'd fall back on
	 * the framework for reliable width/height, but if the surface was
	 * pre-rotated, the framework will give us the "fake" width/height
	 * of the buffers, causing us to incorrectly decide to invalidate
	 * and resize the drawable. Use the last dequeued parameters if
	 * they exist to override the framework's values.
	 *
	 * This fixes poor performance in applications that create two EGL
	 * window surfaces, one current, one not, switching each frame.
	 */
	if((psWindowPrivateData->iLastDequeuedWidth > 0 &&
		psWindowPrivateData->iLastDequeuedWidth != iWidth) ||
	   (psWindowPrivateData->iLastDequeuedHeight > 0 &&
		psWindowPrivateData->iLastDequeuedHeight != iHeight))
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Platform window dimension "
								  "discrepancy (was %dx%d, expected "
								  "%dx%d); correcting",
								  __func__,
								  iWidth, iHeight,
								  psWindowPrivateData->iLastDequeuedWidth,
								  psWindowPrivateData->iLastDequeuedHeight));
		iWidth = psWindowPrivateData->iLastDequeuedWidth;
		iHeight = psWindowPrivateData->iLastDequeuedHeight;
	}

	eError = UpdateWindowPrivateData(psWindowPrivateData, iWidth, iHeight);
	if(eError)
		goto err_out;

	/* The framework has given us these, and it's enough for things
	 * like eglQuerySurface().
	 */
	psRenderParams->ui32Width		= (IMG_UINT32)iWidth;
	psRenderParams->ui32Height		= (IMG_UINT32)iHeight;
	psRenderParams->ePixelFormat	= HAL2WSEGLPixelFormat(iFormat);

	CopyRenderToSource(psRenderParams, psSourceParams);

err_out:
	EXITED();
	return eError;
}

static WSEGLError WSEGL_GetDrawableParameters(WSEGLDrawableHandle hDrawable,
											  WSEGLDrawableParams *psSourceParams,
											  WSEGLDrawableParams *psRenderParams)
{
	android_native_base_t *psNativeBase = (android_native_base_t *)hDrawable;
	ANativeWindowBuffer *psNativeBuffer = IMG_NULL;
	IMG_UINT64 ui64Stamp, ui64AccumStamp = 0;
	IMG_native_handle_t *psNativeHandle;
	WSEGLError eError = WSEGL_SUCCESS;

	ENTERED();

	memset(psSourceParams, 0, sizeof(WSEGLDrawableParams));
	memset(psRenderParams, 0, sizeof(WSEGLDrawableParams));

	if(psNativeBase->magic == ANDROID_NATIVE_WINDOW_MAGIC)
	{
		ANativeWindow *psNativeWindow = (ANativeWindow *)psNativeBase;
		ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;

		PVRSRVLockMutex(hMutex);

		psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
		PVR_ASSERT(psWindowPrivateData != IMG_NULL);

		/* If this window has not been made current, we'd better not start
		 * dequeuing frames from the compositor.
		 *
		 * EGL might call GetDrawableParameters, but we can get away with
		 * returning a subset of the usual information.
		 */
		if(psWindowPrivateData->ui32CurrentRefCount == 0)
		{
			eError = ObtainMinimalParams(psWindowPrivateData,
										 psSourceParams, psRenderParams);
			switch(eError)
			{
				case WSEGL_SUCCESS:
					/* NOTE: Extra unlock */
					PVRSRVUnlockMutex(hMutex);
					goto err_out;
				default:
					PVR_DPF((PVR_DBG_ERROR, "%s: Failed to obtain minimal "
											"parameters", __func__));
					/* Fall-thru */
				case WSEGL_RETRY:
				case WSEGL_BAD_DRAWABLE:
					/* This might be a resize, or it might really be bad. But
					 * EGL responds the same in both cases -- it tries to
					 * recreate the drawable. If this ultimately fails, EGL
					 * will print a message, so we don't have to here.
					 */
					goto err_unlock;
			}
		}

		psNativeBuffer = psWindowPrivateData->psNativeBufferCurrent;
		if(!psNativeBuffer)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Native buffer became NULL",
									__func__));
			eError = WSEGL_BAD_NATIVE_WINDOW;
			goto err_unlock;
		}

		/* Update the window private data. If we get WSEGL_RETRY
		 * from this call, it means we're going into the resize state.
		 */
		psNativeHandle = (IMG_native_handle_t *)psNativeBuffer->handle;
		eError = UpdateWindowPrivateData(psWindowPrivateData,
										 psNativeHandle->iWidth,
										 psNativeHandle->iHeight);
		if(eError)
			goto err_unlock;

		/* If we're a framebuffer or a client we'll have a notional "previous"
		 * buffer. This buffer might be the same as the one we're rendering
		 * to (if the other buffer was locked), but normally it will be
		 * another buffer. Could also be NULL on the first pass.
		 */
		ui64AccumStamp = psWindowPrivateData->sPrevious.ui64Stamp;

		/* Copy over the other "previous" buffer parameters */
		psSourceParams->ui32Width  = psWindowPrivateData->sPrevious.iWidth;
		psSourceParams->ui32Height = psWindowPrivateData->sPrevious.iHeight;
		psSourceParams->ui32Stride = psWindowPrivateData->sPrevious.iStride;
		psSourceParams->eRotationAngle =
			psWindowPrivateData->sPrevious.eRotation;
		psSourceParams->ePixelFormat =
			HAL2WSEGLPixelFormat(psWindowPrivateData->sPrevious.iFormat);

err_unlock:
		PVRSRVUnlockMutex(hMutex);
		if(eError)
			goto err_out;
	}
	else if(psNativeBase->magic == ANDROID_NATIVE_BUFFER_MAGIC)
	{
		psNativeBuffer = (ANativeWindowBuffer *)psNativeBase;
		if(!psNativeBuffer)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Native buffer became NULL",
									__func__));
			goto err_bad_drawable;
		}

		/* Use implicit internal synchronisation for composition */
		if(psHal->IsCompositor())
		{
			psRenderParams->ulFlags = WSEGL_FLAGS_EGLIMAGE_COMPOSITION_SYNC;
		}

	}
	else
	{
		/* We don't support other magic types.. */
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		goto err_bad_drawable;
	}

	psRenderParams->ui32Width = psNativeBuffer->width;
	psRenderParams->ui32Height = psNativeBuffer->height;
	psRenderParams->ui32Stride = psNativeBuffer->stride;

	/* No need to validate this format because it was
	 * already validated in Dequeue().
	 */
	psNativeHandle = (IMG_native_handle_t *)psNativeBuffer->handle;
	ui64Stamp = ((IMG_native_handle_t *)psNativeHandle)->ui64Stamp;
	psRenderParams->ePixelFormat = HAL2WSEGLPixelFormat(psNativeHandle->iFormat);

	eError = MapBufferObtainParams(ui64Stamp,
								   ui64AccumStamp,
								   psSourceParams, psRenderParams,
								   usage_hw(psNativeBuffer->usage));
	if(eError != WSEGL_SUCCESS)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to obtain buffer parameters",
				 __func__));
		goto err_out;
	}

err_out:
	EXITED();
	return eError;

err_bad_drawable:
	eError = WSEGL_BAD_DRAWABLE;
	goto err_out;
}

static WSEGLError WSEGL_ConnectDrawable(WSEGLDrawableHandle hDrawable)
{
	ANativeWindow *psNativeWindow = (ANativeWindow *)hDrawable;
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLError eError = WSEGL_SUCCESS;
	IMG_UINT32 ui32CurrentRefCount;

	ENTERED();

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		eError = WSEGL_BAD_DRAWABLE;
		goto err_out;
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	ui32CurrentRefCount = psWindowPrivateData->ui32CurrentRefCount;

	PVRSRVUnlockMutex(hMutex);

	if(ui32CurrentRefCount == 0)
	{
		/* Make sure we've got hardware render buffers */
		if(native_window_set_usage(psNativeWindow, GRALLOC_USAGE_HW_RENDER))
		{
			eError = WSEGL_BAD_DRAWABLE;
			goto err_out;
		}

		/* Dequeue a buffer if we don't already have one dequeued.
		 * If we do, this call does nothing.
		 */
		eError = DequeueLockStoreBuffer(psNativeWindow);
		if(eError != WSEGL_SUCCESS)
			goto err_out;

#if defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
		/* Tell the platform we're connecting to the surface */
		if(native_window_connect(psNativeWindow, NATIVE_WINDOW_API_EGL))
		{
			eError = WSEGL_BAD_DRAWABLE;
			goto err_out;
		}
#endif /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	psWindowPrivateData->ui32CurrentRefCount++;

	PVRSRVUnlockMutex(hMutex);

err_out:
	EXITED();
	return eError;
}

static WSEGLError WSEGL_DisconnectDrawable(WSEGLDrawableHandle hDrawable)
{
	ANativeWindow *psNativeWindow = (ANativeWindow *)hDrawable;
	ANDROID_WSEGL_window_private_data_t *psWindowPrivateData;
	WSEGLError eError = WSEGL_SUCCESS;
	IMG_UINT32 ui32CurrentRefCount;

	ENTERED();

	if(psNativeWindow->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unrecognized magic", __func__));
		eError = WSEGL_BAD_DRAWABLE;
		goto err_out;
	}

	PVRSRVLockMutex(hMutex);

	psWindowPrivateData = GetWindowPrivateData(psNativeWindow);
	PVR_ASSERT(psWindowPrivateData != IMG_NULL);

	PVR_ASSERT(psWindowPrivateData->ui32CurrentRefCount > 0);
	psWindowPrivateData->ui32CurrentRefCount--;

	ui32CurrentRefCount = psWindowPrivateData->ui32CurrentRefCount;

	PVRSRVUnlockMutex(hMutex);

#if defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT)
	if(ui32CurrentRefCount == 0)
	{
		/* Cancel/re-queue the buffer (and drop refcount on buffer) */
		CancelUnlockPostBuffer(psNativeWindow);

		/* Tell the platform we're disconnecting from the surface */
		native_window_disconnect(psNativeWindow, NATIVE_WINDOW_API_EGL);
	}
#endif /* defined(PVR_ANDROID_HAS_CONNECT_DISCONNECT) */

err_out:
	EXITED();
	return eError;
}

static const WSEGL_FunctionTable sFunctionTable =
{
	WSEGL_VERSION,
	WSEGL_IsDisplayValid,
	WSEGL_InitialiseDisplay,
	WSEGL_CloseDisplay,
	WSEGL_CreateWindowDrawable,
	WSEGL_CreatePixmapDrawable,
	WSEGL_DeleteDrawable,
	WSEGL_SwapDrawable,
	WSEGL_SwapControlInterval,
	WSEGL_WaitNative,
	WSEGL_CopyFromDrawable,
	WSEGL_CopyFromPBuffer,
	WSEGL_GetDrawableParameters,
	WSEGL_ConnectDrawable,
	WSEGL_DisconnectDrawable,
};

WSEGL_EXPORT const WSEGL_FunctionTable *WSEGL_GetFunctionTablePointer(void)
{
	return &sFunctionTable;
}
