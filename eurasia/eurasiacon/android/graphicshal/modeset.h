/*!****************************************************************************
@File           modeset.h

@Title          Graphics HAL framebuffer (modeset) component

@Author         Imagination Technologies

@Date           2010/05/07

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

#ifndef MODESET_H
#define MODESET_H

#include "services.h"

#if 1 || !defined(__i386__) || !defined(SUPPORT_DRI_DRM)

static inline IMG_BOOL SetMode(const PVRSRV_CONNECTION *psConnection)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	return IMG_TRUE;
}

#else /* !defined(__i386__) || !defined(SUPPORT_DRI_DRM) */

#include <inttypes.h>
#include <errno.h>

#if !defined(SUPPORT_DRI_DRM_NO_LIBDRM)
#include <xf86drm.h>
#else
#include "drmlite.h"
#endif

/* Uncomment one of the below, or generate a new mode with:
 *    gtf x y hz
 */

/* 640x480 */

#if 1
#define DEPTH	32
#define BPP		32
#define CLOCK	31500
#define RES_X	640
#define HSTART	664
#define HEND	704
#define HTOTAL	832
#define RES_Y	480
#define VSTART	489
#define VEND	491
#define VTOTAL	520
#endif

/* 1024x768 */

#if 0
#define DEPTH	32
#define BPP		32
#define CLOCK	64110
#define RES_X	1024
#define HSTART	1080
#define HEND	1184
#define HTOTAL	1344
#define RES_Y	768
#define VSTART	769
#define VEND	772
#define VTOTAL	795
#endif

/* 1368x768 */

#if 0
#define DEPTH	32
#define BPP		32
#define CLOCK	85860
#define RES_X	1368
#define HSTART	1440
#define HEND	1584
#define HTOTAL	1800
#define RES_Y	768
#define VSTART	769
#define VEND	772
#define VTOTAL	795
#endif

/* 1600x1200 */

#if 0
#define DEPTH	32
#define BPP		32
#define CLOCK	162000
#define RES_X	1600
#define HSTART	1664
#define HEND	1856
#define HTOTAL	2160
#define RES_Y	1200
#define VSTART	1201
#define VEND	1204
#define VTOTAL	1250
#endif

/* 800x480 */

#if 0
#define DEPTH	32
#define	BPP		32
#define CLOCK	29580
#define RES_X	800
#define HSTART	816
#define HEND	896
#define HTOTAL	992
#define RES_Y	480
#define VSTART	481
#define VEND	484
#define VTOTAL	497
#endif

#define STRIDE (ALIGN(RES_X, HW_ALIGN) * (BPP >> 3))

struct UserServicesHandle
{
	IMG_INT fd;
};

#if defined(DEBUG)
static inline const char *ConnectorTypeString(uint32_t connector_type)
{
	switch (connector_type)
	{
		case DRM_MODE_CONNECTOR_LVDS: return "LVDS";
		case DRM_MODE_CONNECTOR_DVID: return "DVID";
		case DRM_MODE_CONNECTOR_VGA: return "VGA";
		default: return "<unknown>";
	}
}
#endif

static inline IMG_BOOL SetMode(const PVRSRV_CONNECTION *psConnection)
{
	int err, i, iFd = ((struct UserServicesHandle *)psConnection->hServices)->fd;
	uint32_t ui32FbId, ui32OutputId = 0;
	drmModeResPtr psModeRes;

	struct drm_mode_modeinfo sMode = {
		.clock			= CLOCK,
		.hdisplay		= RES_X,
		.hsync_start	= HSTART,
		.hsync_end		= HEND,
		.htotal			= HTOTAL,
		.vdisplay		= RES_Y,
		.vsync_start	= VSTART,
		.vsync_end		= VEND,
		.vtotal			= VTOTAL,
		.flags			= 10,
	};

	psModeRes = drmModeGetResources(iFd);
	if (!psModeRes)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: couldn't get current mode resources",
				 __func__));
		return IMG_FALSE;
	}

	if (!psModeRes->count_crtcs)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: No CRTCs returned", __func__));
		return IMG_FALSE;
	}

	err = drmModeAddFB(iFd, RES_X, RES_Y, DEPTH, BPP, STRIDE, 0, &ui32FbId);
	if(err < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: couldn't add stolen memory buffer "
								"as FB (errno=%d)",
				 __func__, errno));
		return IMG_FALSE;
	}

	for (i = 0; i < psModeRes->count_connectors; i++)
	{
		drmModeConnectorPtr psModeConnector
			= drmModeGetConnector(iFd, psModeRes->connectors[i]);

		/* We don't care about these things */
		free(psModeConnector->props);
		free(psModeConnector->prop_values);
		free(psModeConnector->encoders);
		free(psModeConnector->modes);

		if (psModeConnector->connector_type == DRM_MODE_CONNECTOR_LVDS ||
			psModeConnector->connector_type == DRM_MODE_CONNECTOR_DVID ||
			psModeConnector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%s: Picked connector %u as output "
					 "(type %s)", __func__, psModeRes->connectors[i],
					 ConnectorTypeString(psModeConnector->connector_type)));
			ui32OutputId = psModeRes->connectors[i];
			goto found_output;
		}
	}

	/* No suitable connectors were found */
	PVR_DPF((PVR_DBG_ERROR, "%s: Unable to find an output connector "
			 "(checked %d)", __func__, i));
	return IMG_FALSE;

found_output:
	err = drmModeSetCrtc(iFd, psModeRes->crtcs[0], ui32FbId, 0, 0,
						 &ui32OutputId, 1, &sMode);
	if(err < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set mode (errno=%d)",
				 __func__, errno));
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

#undef DEPTH
#undef BPP
#undef CLOCK
#undef RES_X
#undef HSTART
#undef HEND
#undef HTOTAL
#undef RES_Y
#undef VSTART
#undef VEND
#undef VTOTAL
#undef STRIDE

#endif /* !defined(__i386__) || !defined(SUPPORT_DRI_DRM) */

#endif /* MODESET_H */
