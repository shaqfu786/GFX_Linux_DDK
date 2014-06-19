/*!****************************************************************************
@File           hal.c

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

#include "hal_private.h"
#include "framebuffer.h"
#include "gralloc.h"
#include "mapper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "sgxapi_km.h"

static IMG_BOOL gbGraphicsHALInitialized;
static IMG_buffer_format_t *gpsBufferFormats;

#define CMDLINE_BUF_LEN 64

static IMG_BOOL bCompositorCheckInitialized;
static IMG_BOOL bIsSurfaceFlingerCompositor;

static IMG_BOOL IsCompositor(void)
{
	int cmdline;

	if(bCompositorCheckInitialized)
		return bIsSurfaceFlingerCompositor;

	cmdline = open("/proc/self/cmdline", O_RDONLY);
	if(cmdline >= 0)
	{
		char cmdline_buf[CMDLINE_BUF_LEN + 1];
		size_t i;
		int len;

		const char *compare[] = {
			"system_server",
			"surfaceflinger",
		};

		len = read(cmdline, cmdline_buf, CMDLINE_BUF_LEN);
		if(len > 0)
		{
			for(i = 0; i < sizeof(compare) / sizeof(*compare); i++)
			{
				const char *compare_item = strrchr(cmdline_buf, '/');

				if(compare_item)
					compare_item++;
				else
					compare_item = cmdline_buf;

				if(!strcmp(compare_item, compare[i]))
				{
					bIsSurfaceFlingerCompositor = IMG_TRUE;
					break;
				}
			}
		}
	}

	close(cmdline);

	bCompositorCheckInitialized = IMG_TRUE;
	return bIsSurfaceFlingerCompositor;
}

static int OpenPVRServices(IMG_private_data_t *psPrivateData)
{
	IMG_UINT32 ui32ClientHeapCount, ui32NumDevices, i, ui32Flags = 0;
	PVRSRV_DEVICE_IDENTIFIER asDevID[PVRSRV_MAX_DEVICES];
	PVRSRV_HEAP_INFO asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
	PVR2DDEVICEINFO *pDevInfo = IMG_NULL;
	PVRSRV_CONNECTION *psConnection;
	int nDevices, err = 0;
	IMG_UINT32 ui32DevId;

	if(IsCompositor())
		ui32Flags |= SRV_FLAGS_PERSIST;

	if(PVRSRVConnect(&psConnection, ui32Flags) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to open services connection",
				 __func__));
		err = -EFAULT;
		goto err_out;
	}

	if(PVRSRVEnumerateDevices(psConnection, &ui32NumDevices,
							  asDevID) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to enumerate devices",
				 __func__));
		err = -EFAULT;
		goto err_disconnect;
	}

	for(i = 0; i < ui32NumDevices; i++)
	{
		if(asDevID[i].eDeviceClass == PVRSRV_DEVICE_CLASS_3D)
		{
			if(PVRSRVAcquireDeviceData(psConnection,
									   asDevID[i].ui32DeviceIndex,
									   &psPrivateData->sDevData,
									   PVRSRV_DEVICE_TYPE_UNKNOWN) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire device data",
						 __func__));
				err = -EFAULT;
				goto err_disconnect;
			}
			break;
		}
	}

	if(PVRSRVCreateDeviceMemContext(&psPrivateData->sDevData,
									&psPrivateData->hDevMemContext,
									&ui32ClientHeapCount,
									asHeapInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create device memory context",
				 __func__));
		err = -EFAULT;
		goto err_disconnect;
	}

	for(i = 0; i < ui32ClientHeapCount; i++)
	{
		if(HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGX_GENERAL_HEAP_ID)
		{
			psPrivateData->hGeneralHeap = asHeapInfo[i].hDevMemHeap;
			break;
		}
	}

	if(SGXGetClientInfo(&psPrivateData->sDevData, &psPrivateData->sSGXInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get SGX client info",
				 __func__));
		err = -EFAULT;
		goto err_disconnect;
	}

	psPrivateData->sSGXInfo.sMiscInfo.ui32StateRequest =
		PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT;

	if(PVRSRVGetMiscInfo(psConnection,
						 &psPrivateData->sSGXInfo.sMiscInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get SGX client misc info",
				 __func__));
		err = -EFAULT;
		goto err_disconnect;
	}

	nDevices = PVR2DEnumerateDevices(0);
	if(nDevices <= 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: No devices (check permissions)",
				 __func__));
		err = -ENODEV;
		goto err_disconnect;
	}

	pDevInfo = calloc(nDevices, sizeof(PVR2DDEVICEINFO));
	if(!pDevInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate memory", __func__));
		err = -ENOMEM;
		goto err_disconnect;
	}

	if(PVR2DEnumerateDevices(pDevInfo) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to enumerate devices", __func__));
		goto err_disconnect;
	}

	/* Always use the first ID */
	ui32DevId = pDevInfo[0].ulDevID;

	if(PVR2DCreateDeviceContext(ui32DevId,
								&psPrivateData->hContext, 0) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create device context",
				 __func__));
		goto err_disconnect;
	}

err_out:
	free(pDevInfo);
	return err;

err_disconnect:
	if(PVRSRVDisconnect(psConnection) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to disconnect services",
				 __func__));
		err = -EFAULT;
		/* Fall-through */
	}
	goto err_out;
}

static int ClosePVRServices(IMG_private_data_t *psPrivateData)
{
	int err = -EFAULT;

	if(PVR2DDestroyDeviceContext(psPrivateData->hContext) != PVR2D_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to destroy PVR2D context",
				 __func__));
		goto err_out;
	}

	if(PVRSRVReleaseMiscInfo(psPrivateData->sDevData.psConnection,
							 &psPrivateData->sSGXInfo.sMiscInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to release SGX client misc info",
				 __func__));
		goto err_out;
	}

	if(PVRSRVDestroyDeviceMemContext(&psPrivateData->sDevData,
									 psPrivateData->hDevMemContext) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to destroy device memory context",
				 __func__));
		goto err_out;
	}

	if(PVRSRVDisconnect(psPrivateData->sDevData.psConnection) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to disconnect services",
				 __func__));
		goto err_out;
	}

	err = 0;
err_out:
	return err;
}

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)

static void LoadGenericAppHints(IMG_private_data_t *psPrivateData,
								IMG_VOID *pvHintState)
{
	IMG_UINT32 ui32Default;

	ui32Default = 1;
	PVRSRVGetAppHint(pvHintState, "HALCompositionBypass",
					 IMG_UINT_TYPE, &ui32Default,
					 &psPrivateData->sAppHints.ui32HALCompositionBypass);
}

#else /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

static inline void LoadGenericAppHints(IMG_private_data_t *psPrivateData,
									   IMG_VOID *pvHintState)
{
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
	PVR_UNREFERENCED_PARAMETER(pvHintState);
}

#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

static void LoadFrameBufferAppHints(IMG_private_data_t *psPrivateData,
									IMG_VOID *pvHintState)
{
	IMG_UINT32 ui32Default, ui32PresentMode;

	ui32Default = 0;
	PVRSRVGetAppHint(pvHintState, "HALPresentMode",
					 IMG_UINT_TYPE, &ui32Default, &ui32PresentMode);

	if(ui32PresentMode == 1)
		psPrivateData->sAppHints.eHALPresentMode = HAL_PRESENT_MODE_BLIT;
	else if (ui32PresentMode == 2)
		psPrivateData->sAppHints.eHALPresentMode = HAL_PRESENT_MODE_FRONT;
	else
		psPrivateData->sAppHints.eHALPresentMode = HAL_PRESENT_MODE_FLIP;

	ui32Default = 2;
	PVRSRVGetAppHint(pvHintState, "HALNumFrameBuffers",
					 IMG_UINT_TYPE, &ui32Default,
					 &psPrivateData->sAppHints.ui32HALNumFrameBuffers);

	ui32Default = 68;
	PVRSRVGetAppHint(pvHintState, "HALFrameBufferHz",
					 IMG_UINT_TYPE, &ui32Default,
					 &psPrivateData->sAppHints.ui32HALFrameBufferHz);

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
	if(psPrivateData->sAppHints.ui32HALCompositionBypass > 0)
	{
		if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FRONT)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Front-buffer rendering is "
									"unsupported in combination with "
									"composition bypass.",
					 __func__));
			PVR_DPF((PVR_DBG_ERROR, "%s: Switching to flip mode.", __func__));
			psPrivateData->sAppHints.eHALPresentMode = HAL_PRESENT_MODE_FLIP;
		}

		if(psPrivateData->sAppHints.ui32HALNumFrameBuffers < 4)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Composition bypass requires at "
									"least 4 frame buffers. Requesting 4 "
									"frame buffers.",
					 __func__));
			psPrivateData->sAppHints.ui32HALNumFrameBuffers = 4;
		}
	}
	else
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */
	{
		/* Compatibility hack; previously HALNumFrameBuffers=1 was front buffer */
		if((psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FLIP) &&
		   (psPrivateData->sAppHints.ui32HALNumFrameBuffers < 2))
			psPrivateData->sAppHints.eHALPresentMode = HAL_PRESENT_MODE_FRONT;

		/* If we're front buffer, we only have one frame buffer */
		if(psPrivateData->sAppHints.eHALPresentMode == HAL_PRESENT_MODE_FRONT)
			psPrivateData->sAppHints.ui32HALNumFrameBuffers = 1;
	}
}

static int hal_open(const hw_module_t *module, const char *id,
					hw_device_t **device)
{
	int (*pfnSetupFn)(const hw_module_t *module,
					  hw_device_t **device) = IMG_NULL;
	IMG_private_data_t *psPrivateData =
		&((IMG_gralloc_module_t *)module)->sPrivateData;
	IMG_VOID *pvHintState;

	if(!IsInitialized())
		return -ENOTTY;

	PVRSRVLockMutex(psPrivateData->hMutex);

	PVRSRVCreateAppHintState(IMG_ANDROID_HAL, 0, &pvHintState);
	LoadGenericAppHints(psPrivateData, pvHintState);

	if(!strcmp(id, GRALLOC_HARDWARE_GPU0))
		pfnSetupFn = gralloc_setup;

	else if(!strcmp(id, GRALLOC_HARDWARE_FB0))
	{
		LoadFrameBufferAppHints(psPrivateData, pvHintState);
		pfnSetupFn = framebuffer_setup;
	}

	PVRSRVFreeAppHintState(IMG_ANDROID_HAL, pvHintState);
	PVRSRVUnlockMutex(psPrivateData->hMutex);

	if(pfnSetupFn)
		return pfnSetupFn(module, device);

	PVR_DPF((PVR_DBG_ERROR, "%s: Invalid open ID '%s'", __func__, id));
	return -EINVAL;
}

static hw_module_methods_t hal_module_methods =
{
	.open = hal_open,
};

static void LogSyncs(IMG_gralloc_module_t *psGrallocModule)
{
	IMG_private_data_t *psPrivateData = &psGrallocModule->sPrivateData;

	if(!gbGraphicsHALInitialized)
		return;

	PVRSRVLockMutex(psPrivateData->hMutex);
	MapperLogSyncObjects(psPrivateData);
	PVRSRVUnlockMutex(psPrivateData->hMutex);
}

static const IMG_buffer_format_public_t *GetBufferFormats(void)
{
	return (const IMG_buffer_format_public_t *)gpsBufferFormats;
}

#if !defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)

static int GetPhyAddrs(IMG_gralloc_module_public_t const* module,
					   buffer_handle_t handle,
					   unsigned int auiPhyAddr[MAX_SUB_ALLOCS])
{
	PVR_UNREFERENCED_PARAMETER(module);
	PVR_UNREFERENCED_PARAMETER(handle);
	PVR_UNREFERENCED_PARAMETER(auiPhyAddr);
	PVR_DPF((PVR_DBG_ERROR, "%s: unimplemented", __func__));
	return 0;
}

#endif /* !defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */

IMG_EXPORT IMG_gralloc_module_t HAL_MODULE_INFO_SYM =
{
	.base =
	{
		/* Must be first (Android casts to gralloc_module_t) */
		.base =
		{
			/* Must be first (Android casts to hw_module_t) */
			.common	=
			{
				.tag			= HARDWARE_MODULE_TAG,
				.version_major	= 1,
				.version_minor	= 0,
				.id				= GRALLOC_HARDWARE_MODULE_ID,
				.name			= "IMG SGX Graphics HAL",
				.author			= "Imagination Technologies",
				.methods		= &hal_module_methods,
			},

			.lock				= gralloc_module_lock,
			.unlock				= gralloc_module_unlock,

			.registerBuffer		= gralloc_register_buffer,
			.unregisterBuffer	= gralloc_unregister_buffer,
		},
#if defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)
		.GetPhyAddrs			= gralloc_module_getphyaddrs,
#else
		.GetPhyAddrs			= GetPhyAddrs,
#endif /* defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */

		.Blit					= gralloc_module_blit_to_vaddr,
		.Blit2					= gralloc_module_blit_to_handle
	},

	/* Used only by IMG driver components */
	.map				= gralloc_module_map,
	.unmap				= gralloc_module_unmap,
	.IsCompositor		= IsCompositor,
	.LogSyncs			= LogSyncs,
	.GetBufferFormats	= GetBufferFormats,
#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
	.SetAccumBuffer		= gralloc_module_setaccumbuffer,
#endif

	/* Private data implicitly initialised to NULL */
};

IMG_INTERNAL IMG_BOOL IsInitializedFunc(const char *func __unused)
{
	if(!gbGraphicsHALInitialized)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Graphics HAL not initialized", func));
	}
	return gbGraphicsHALInitialized;
}

IMG_INTERNAL const IMG_buffer_format_t *GetBufferFormat(int iFormat)
{
	IMG_buffer_format_public_t *psEntry =
		(IMG_buffer_format_public_t *)gpsBufferFormats;

	while(psEntry)
	{
		if(psEntry->iHalPixelFormat == iFormat)
			return (const IMG_buffer_format_t *)psEntry;
		psEntry = psEntry->psNext;
	}

	return IMG_NULL;
}

IMG_INTERNAL
IMG_BOOL RegisterBufferFormat(IMG_buffer_format_t *psBufferFormat)
{
	IMG_buffer_format_public_t *psEntry =
		(IMG_buffer_format_public_t *)gpsBufferFormats;

	/* Make sure the format passed in has no next entry */
	PVR_ASSERT(psBufferFormat->base.psNext == IMG_NULL);

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Registered buffer format %s",
							  __func__, psBufferFormat->base.szName));

	/* Normally we'd link the list backwards, but as sort order matters
	 * for EGLConfig enumeration, so we'll do it the hard way.
	 */
	if(psEntry)
	{
		while(psEntry->psNext)
			psEntry = psEntry->psNext;
		psEntry->psNext = (IMG_buffer_format_public_t *)psBufferFormat;
		return IMG_FALSE;
	}

	gpsBufferFormats = psBufferFormat;
	return IMG_TRUE;
}

/* Called on first dlopen() */
static void __attribute__((constructor)) hal_init(void)
{
	IMG_private_data_t *psPrivateData = &HAL_MODULE_INFO_SYM.sPrivateData;
	int err;

	/* Make sure this constructor has only been run once.
	 * Buggy toolchains can cause this to be run multiple times.
	 */
	PVR_ASSERT(!gbGraphicsHALInitialized && psPrivateData->hMutex == 0);

	err = OpenPVRServices(psPrivateData);
	if(err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to open services (err=%d)",
								__func__, err));
		return;
	}

	PVRSRVCreateMutex(&psPrivateData->hMutex);

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Graphics HAL loaded (err=%d)",
							  __func__, err));
	gbGraphicsHALInitialized = IMG_TRUE;
}

/* Called on dlclose() */
static void __attribute__((destructor)) hal_exit(void)
{
	IMG_private_data_t *psPrivateData = &HAL_MODULE_INFO_SYM.sPrivateData;
	int err;

	PVR_ASSERT(gbGraphicsHALInitialized && psPrivateData->hMutex != 0);

	PVRSRVDestroyMutex(psPrivateData->hMutex);
	psPrivateData->hMutex = NULL;

	if(psPrivateData->psMapList)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Stale maps left in cache; probably "
								"an application bug", __func__));
		PVR_DPF((PVR_DBG_ERROR, "%s: You may see failing kernel assertions "
								"after this message", __func__));
	}

	err = ClosePVRServices(psPrivateData);

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Graphics HAL unloaded (err=%d)",
							  __func__, err));
	gbGraphicsHALInitialized = IMG_FALSE;
}
