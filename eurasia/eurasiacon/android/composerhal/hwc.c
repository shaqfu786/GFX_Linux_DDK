/*!****************************************************************************
@File           hwc.c

@Title          Hardware Composer (hwcomposer) HAL component

@Author         Imagination Technologies

@Date           2010/09/15

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

#include <hardware/hwcomposer.h>

#include "pvr_debug.h"
#include "services.h"

#define I_KNOW_WHAT_I_AM_DOING
#include "hwc_private.h"
#include "hwc.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <EGL/egl.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* Maximum number of layers this HWC can handle.
 * Right now this is one flipped layer and one blitted layer.
 */
#define MAX_HANDLED_LAYERS 2

static inline IMG_BOOL IsIntersecting(const hwc_rect_t *a, const hwc_rect_t *b)
{
	return ((a->bottom > b->top)    &&
			(a->top    < b->bottom) &&
			(a->right  > b->left)   &&
			(a->left   < b->right));
}

static int hwc_device_prepare(hwc_composer_device_t *dev,
							  hwc_layer_list_t *list)
{
	framebuffer_device_t *psFrameBufferDevice = (framebuffer_device_t *)
		((IMG_hwc_module_t *)dev->common.module)->psFrameBufferDevice;
	IMG_UINT32 ui32NumBypassLayers = 0;
	hwc_rect_t sAccumRect = {};
	size_t i;

	PVR_UNREFERENCED_PARAMETER(dev);

	ENTERED();

	PVR_ASSERT(psFrameBufferDevice != IMG_NULL);

	/* No geometry changed? We don't need to update anything */
	if(!list || list->flags == HWC_GEOMETRY_CHANGED)
		goto exit_out;

	/* Initially, clear bypass on all layers */
#if 0
	for(i = 0; i < list->numHwLayers; i++)
		list->hwLayers[i].extraUsage = 0;
#endif

	/* It's a non-starter if we have more layers than we can handle.
	 * FIXME: What about off-screen layers? Shouldn't we ignore these?
	 */
	if(list->numHwLayers > MAX_HANDLED_LAYERS)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Aborted HWC because we saw more "
								  "layers than we can handle", __func__));
		goto exit_out;
	}

	/* Invert the accumulation bounds to pass the intersection test */
	sAccumRect.left = psFrameBufferDevice->width;
	sAccumRect.top = psFrameBufferDevice->height;

	/* And there are other criteria for aborting */
	for(i = 0; i < list->numHwLayers; i++)
	{
		hwc_layer_t *psLayer = &list->hwLayers[i];
		IMG_native_handle_t *psNativeHandle =
			(IMG_native_handle_t *)psLayer->handle;
		const hwc_rect_t *psVisibleRect =
			&psLayer->visibleRegionScreen.rects[0];

		/* Shouldn't happen */
		if(!psNativeHandle)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because we saw a "
									"NULL native handle", __func__));
			goto exit_out;
		}

		/* This module is designed only to optimize out these GL compositions,
		 * so a SKIP_LAYER is an abort.
		 */
		if(psLayer->flags & HWC_SKIP_LAYER)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because we saw a "
									"SKIP_LAYER", __func__));
			goto exit_out;
		}

		/* If there's any blended layers, we can't bypass composition --
		 * PVR2D doesn't support any kind of blended blit.
		 */
		if(psLayer->blending != HWC_BLENDING_NONE)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because we saw a "
									"blended layer", __func__));
			goto exit_out;
		}

		/* If the visible layer is fragmented, or we overlap with another
		 * layer, there's some risk that composition bypass will involve
		 * overdraw blits.
		 *
		 * We won't attempt bypass if we risk wasting bandwidth.
		 */

		if(psLayer->visibleRegionScreen.numRects > 1)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because the layer had "
									"more than one visible screen rect",
									__func__));
			goto exit_out;
		}

		if(IsIntersecting(&sAccumRect, psVisibleRect))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because layer intersects "
									"with another", __func__));
			goto exit_out;
		}

		sAccumRect.left		= MIN(sAccumRect.left,   psVisibleRect->left);
		sAccumRect.top		= MIN(sAccumRect.top,    psVisibleRect->top);
		sAccumRect.right	= MAX(sAccumRect.right,  psVisibleRect->right);
		sAccumRect.bottom	= MAX(sAccumRect.bottom, psVisibleRect->bottom);

		/* We can support bypass (flipping) from HW-only surfaces, and
		 * blits from any kind of surface.
		 *
		 * FIXME: First-come first-served might not work very well.
		 * FIXME: Handle transformed layers.
		 */
#if 0
		if(usage_hw_render(psNativeHandle->usage) &&
		   !usage_sw(psNativeHandle->usage) &&
		   !ui32NumBypassLayers &&
		   !psLayer->transform)
		{
			psLayer->extraUsage = GRALLOC_USAGE_BYPASS;
			ui32NumBypassLayers++;
		}
#endif
	}

	if((sAccumRect.left   != 0) &&
	   (sAccumRect.top    != 0) &&
	   (sAccumRect.right  != (int)psFrameBufferDevice->width) &&
	   (sAccumRect.bottom != (int)psFrameBufferDevice->height))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Aborted HWC because combined layer "
								"coverage is partial", __func__));
		goto exit_out;
	}

	if(!ui32NumBypassLayers)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Aborted HWC because no layers were "
								  "suitable for bypass", __func__));
		goto exit_out;
	}

	/* We can probably do bypass with this layer setup, but we need to
	 * wait until the bypass layer is re-allocated.
	 */
	for(i = 0; i < list->numHwLayers; i++)
	{
		hwc_layer_t *psLayer = &list->hwLayers[i];
		IMG_native_handle_t *psNativeHandle =
			(IMG_native_handle_t *)psLayer->handle;

#if 0
		if(psLayer->extraUsage && !usage_bypass(psNativeHandle->usage))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Waiting for re-allocation of "
									"bypass layer..", __func__));
			goto exit_out;
		}
#endif
	}

	/* Everything's in place -- we can flip the bypass layer and blit
	 * the other layers. Mark all the layers HWC_OVERLAY to bypass
	 * composition for this frame.
	 */
	for(i = 0; i < list->numHwLayers; i++)
		list->hwLayers[i].compositionType = HWC_OVERLAY;

exit_out:
	EXITED();
	return 0;
}

static int hwc_device_set(hwc_composer_device_t *dev, hwc_display_t dpy,
						  hwc_surface_t sur, hwc_layer_list_t *list)
{
	IMG_framebuffer_device_public_t *psFrameBufferDevice =
		((IMG_hwc_module_t *)dev->common.module)->psFrameBufferDevice;
	IMG_gralloc_module_public_t *psGrallocModule =
		((IMG_hwc_module_t *)dev->common.module)->psGrallocModule;
	buffer_handle_t psExtraHandles[1] = {};
	hwc_layer_t *psBypassLayer = IMG_NULL;
	IMG_CHAR *pcPrivData = "Testing 123";
	IMG_UINT32 ui32NumExtraHandles = 0;
	int err = 0;
	size_t i;

	ENTERED();

	PVR_ASSERT(psFrameBufferDevice != IMG_NULL);

	/* No list is probably SF releasing the framebuffer. Ignore it. */
	if(!list)
		goto exit_out;

	/* If we see any layers that aren't HWC_OVERLAY, we have to composite */

	for(i = 0; i < list->numHwLayers; i++)
	{
		hwc_layer_t *psLayer = &list->hwLayers[i];

		if(psLayer->compositionType != HWC_OVERLAY)
			goto exit_swap_buffers;

#if 0
		if(psLayer->extraUsage)
			psBypassLayer = psLayer;
#endif
	}

	PVR_ASSERT(psBypassLayer != IMG_NULL);

	/* Pipeline blits for the non-bypass layers */

	for(i = 0; i < list->numHwLayers; i++)
	{
		hwc_layer_t *psLayer = &list->hwLayers[i];
		const hwc_rect_t *psVisibleRect =
			&psLayer->visibleRegionScreen.rects[0];

		/* Skip the bypass layer -- we've handled it already */
		if(psLayer == psBypassLayer)
			continue;

		/* Blit this layer (relative to dest) */
		err = psGrallocModule->Blit2(
			psGrallocModule,
			psLayer->handle,							/* Source	*/
			psBypassLayer->handle,						/* Dest		*/
			psVisibleRect->right - psVisibleRect->left, /* Width	*/
			psVisibleRect->bottom - psVisibleRect->top, /* Height	*/
			psVisibleRect->left,						/* X		*/
			psVisibleRect->top);						/* Y		*/
		if(err)
			goto exit_swap_buffers;
	}

	/* Then pipeline the presentation flip */
	psExtraHandles[0] = psBypassLayer->handle;
	ui32NumExtraHandles++;

exit_post:
	err = psFrameBufferDevice->Post2(
		(framebuffer_device_t *)psFrameBufferDevice,
		psExtraHandles, ui32NumExtraHandles,
		pcPrivData, strlen(pcPrivData));
	if(err)
		goto exit_swap_buffers;

exit_out:
	EXITED();
	return err;

exit_swap_buffers:
	if(!eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur))
	{
		err = HWC_EGL_ERROR;
		goto exit_out;
	}
	goto exit_post;
}

static int hwc_device_close(hw_device_t *device)
{
	free(device);
	return 0;
}

IMG_INTERNAL
int hwc_setup(const hw_module_t *module, hw_device_t **device)
{
	hwc_composer_device_t *psHwcDevice;
	hw_device_t *psHwDevice;
	int err = 0;

	ENTERED();

	psHwcDevice = malloc(sizeof(hwc_composer_device_t));
	if(!psHwcDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate memory", __func__));
		err = -ENOMEM;
		goto err_out;
	}

	memset(psHwcDevice, 0, sizeof(hwc_composer_device_t));
	psHwDevice = (hw_device_t *)psHwcDevice;

	psHwcDevice->common.tag		= HARDWARE_DEVICE_TAG;
	psHwcDevice->common.version	= 0;
	psHwcDevice->common.module	= (hw_module_t *)module;
	psHwcDevice->common.close	= hwc_device_close;

	psHwcDevice->prepare		= hwc_device_prepare;
	psHwcDevice->set			= hwc_device_set;

	*device = psHwDevice;
err_out:
	EXITED();
	return err;
}
