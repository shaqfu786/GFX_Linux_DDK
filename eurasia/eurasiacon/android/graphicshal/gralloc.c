/*!****************************************************************************
@File           gralloc.c

@Title          Graphics Allocator (gralloc) HAL component

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "hal_private.h"
#include "gralloc.h"
#include "gralloc_defer.h"
#include "mapper.h"
#include "buffer_generic.h"
#include "framebuffer.h"

#include "sgxapi.h"

static IMG_BOOL validate_handle(IMG_native_handle_t *psNativeHandle)
{
	if(!psNativeHandle)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid handle", __func__));
		return IMG_FALSE;
	}

	if(usage_unsupported(psNativeHandle->usage))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unsupported usage bits (0xP...FHWR=0x%.8x)"
								" set on handle",
				 __func__, psNativeHandle->usage));
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

static IMG_BOOL validate_lock_rectangle(IMG_native_handle_t *psNativeHandle,
										int l, int t, int w, int h)
{
	if(l < 0 || t < 0 || w < 0 || h < 0 ||
	   l     > psNativeHandle->iWidth   ||
	   t     > psNativeHandle->iHeight  ||
	   l + w > psNativeHandle->iWidth   ||
	   t + h > psNativeHandle->iHeight)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid lock rectangle (%d,%d->%d,%d) "
								"for buffer dimensions (%dx%d)",
				 __func__, l, t, l + w, t + h,
				 psNativeHandle->iWidth, psNativeHandle->iHeight));
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

static PVR2DFORMAT HAL2PVR2DPixelFormat(int format)
{
	const IMG_buffer_format_t *psBufferFormat = GetBufferFormat(format);
	if(psBufferFormat && psBufferFormat->iPVR2DFormat)
		return psBufferFormat->iPVR2DFormat;
#define PVR2D_BAD 0xff
	return PVR2D_BAD;
}

static inline IMG_private_data_t *getDevicePrivateData(alloc_device_t *psAllocDevice)
{
	IMG_private_data_t *psPrivateData =
		&((IMG_gralloc_module_t *)psAllocDevice->common.module)->sPrivateData;
	PVRSRVLockMutex(psPrivateData->hMutex);
	return psPrivateData;
}

static inline void putDevicePrivateData(IMG_private_data_t *psPrivateData)
{
	PVRSRVUnlockMutex(psPrivateData->hMutex);
}

static inline IMG_private_data_t *getModulePrivateData(gralloc_module_t const* psGrallocModule)
{
	IMG_private_data_t *psPrivateData =
		&((IMG_gralloc_module_t *)psGrallocModule)->sPrivateData;
	PVRSRVLockMutex(psPrivateData->hMutex);
	return psPrivateData;
}

static inline void putModulePrivateData(IMG_private_data_t *psPrivateData)
{
	PVRSRVUnlockMutex(psPrivateData->hMutex);
}

static inline framebuffer_device_t *
getDeviceFrameBufferDevice(alloc_device_t *psAllocDevice)
{
	return (framebuffer_device_t *)
		((IMG_gralloc_module_public_t *)psAllocDevice->common.module)->
													psFrameBufferDevice;
}

static inline framebuffer_device_t *
getModuleFrameBufferDevice(gralloc_module_t const* psGrallocModule)
{
	return (framebuffer_device_t *)
		((IMG_gralloc_module_public_t *)psGrallocModule)->psFrameBufferDevice;
}

IMG_INTERNAL
IMG_BOOL OpsFlushed(PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	IMG_flush_ops_pair_t asOps[MAX_SUB_ALLOCS * 3];
	PVRSRV_SYNC_DATA *psSyncData;
	int i, j;

	for(i = 0; (i < MAX_SUB_ALLOCS) && apsMemInfo[i]; i++)
	{
		psSyncData = apsMemInfo[i]->psClientSyncInfo->psSyncData;

		/* SGX read operations */
		asOps[(i*3)+0].pui32OpsComplete = &psSyncData->ui32ReadOpsComplete;
		asOps[(i*3)+0].ui32OpsPending   = psSyncData->ui32ReadOpsPending;

		/* Display read operations */
		asOps[(i*3)+1].pui32OpsComplete = &psSyncData->ui32ReadOps2Complete;
		asOps[(i*3)+1].ui32OpsPending   = psSyncData->ui32ReadOps2Pending;

		/* SGX write operations */
		asOps[(i*3)+2].pui32OpsComplete = &psSyncData->ui32WriteOpsComplete;
		asOps[(i*3)+2].ui32OpsPending   = psSyncData->ui32WriteOpsPending;
	}

	for(j = 0; j < i * 3; j++)
		if(*asOps[j].pui32OpsComplete < asOps[j].ui32OpsPending)
			break;

	/* If all sOps were processed, all were flushed */
	return (j == i * 3) ? IMG_TRUE : IMG_FALSE;
}

static void
FlushAllKindOps(IMG_private_data_t *psPrivateData,
				IMG_flush_ops_pair_t *psOps,
				IMG_UINT32 ui32NumOpPairs,
				const char *const szKind __unused)
{
	IMG_UINT32 i, ui32TriesLeft = GRALLOC_DEFAULT_WAIT_RETRIES;

	while(1)
	{
		/* If any operation is incomplete, consider all to be incomplete */
		for (i = 0; i < ui32NumOpPairs; i++)
			if(*psOps[i].pui32OpsComplete < psOps[i].ui32OpsPending)
				break;

		/* All operations were completed */
		if(i == ui32NumOpPairs)
			break;
	
//SUPPORT_FORCE_FLIP_WORKAROUND
		if(ui32TriesLeft == (GRALLOC_DEFAULT_WAIT_RETRIES -10))
//		if((ui32TriesLeft%10)== 0)
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
			PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for %s ops "
									"to complete", __func__, szKind));
			PVRSRVClientEvent(PVRSRV_CLIENT_EVENT_HWTIMEOUT,
							  &psPrivateData->sDevData, psGrallocModule);
			break;
		}

		if(PVRSRVEventObjectWait(psPrivateData->sDevData.psConnection,
				psPrivateData->sSGXInfo.sMiscInfo.hOSGlobalEvent) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%s: Timeout waiting for %s ops "
									  "to complete", szKind, __func__));
			ui32TriesLeft--;
		}
	}
}

static void
FlushAllReadOps(IMG_private_data_t *psPrivateData,
				PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	IMG_flush_ops_pair_t asOps[2 * MAX_SUB_ALLOCS];
	PVRSRV_SYNC_DATA *psSyncData;
	int i;

	/* Better not hold the private data lock if we plan to block */
	PVRSRVUnlockMutex(psPrivateData->hMutex);

	for (i = 0; (i < MAX_SUB_ALLOCS) && apsMemInfo[i]; i++)
	{
		psSyncData = apsMemInfo[i]->psClientSyncInfo->psSyncData;

		/* SGX read operations (if any) */
		asOps[(i*2)+0].pui32OpsComplete = &psSyncData->ui32ReadOpsComplete;
		asOps[(i*2)+0].ui32OpsPending   = psSyncData->ui32ReadOpsPending;

		/* Display read operations (if any) */
		asOps[(i*2)+1].pui32OpsComplete = &psSyncData->ui32ReadOps2Complete;
		asOps[(i*2)+1].ui32OpsPending   = psSyncData->ui32ReadOps2Pending;
	}

	FlushAllKindOps(psPrivateData, asOps, 2 * i, "read");

	/* Re-take the private data lock */
	PVRSRVLockMutex(psPrivateData->hMutex);
}

static void
FlushAllWriteOps(IMG_private_data_t *psPrivateData,
				 PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	IMG_flush_ops_pair_t asOps[MAX_SUB_ALLOCS];
	PVRSRV_SYNC_DATA *psSyncData;
	int i;

	/* Better not hold the private data lock if we plan to block */
	PVRSRVUnlockMutex(psPrivateData->hMutex);

	for (i = 0; (i < MAX_SUB_ALLOCS) && apsMemInfo[i]; i++)
	{
		psSyncData = apsMemInfo[i]->psClientSyncInfo->psSyncData;

		/* SGX write operations (if any) */
		asOps[i].pui32OpsComplete = &psSyncData->ui32WriteOpsComplete;
		asOps[i].ui32OpsPending   = psSyncData->ui32WriteOpsPending;
	}

	FlushAllKindOps(psPrivateData, asOps, i, "write");

	/* Re-take the private data lock */
	PVRSRVLockMutex(psPrivateData->hMutex);
}

#if defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS)

static int __lock_buffer(IMG_private_data_t *psPrivateData,
						 IMG_mapper_meminfo_t *psMapperMemInfo,
						 IMG_BOOL bIsFrameBuffer, int usage)
{
	IMG_UINT32 ui32TriesLeft = GRALLOC_DEFAULT_WAIT_RETRIES;
	int err = -EFAULT, flags = 0;
	PVRSRV_ERROR eError;

	/* Framebuffer ops don't do anything clever; they probably should,
	 * but it interacts badly with services at the moment, and the
	 * feature is only required for software composition (or our tests).
	 */

	if(usage_write(usage))
	{
		if(bIsFrameBuffer)
		{
			FlushAllReadOps(psPrivateData, psMapperMemInfo->apsMemInfo);
			goto exit_out;
		}

		flags |= PVRSRV_MODIFYSYNCOPS_FLAGS_WO_INC;
	}

	if(usage_read(usage))
	{
		if(bIsFrameBuffer)
		{
			FlushAllWriteOps(psPrivateData, psMapperMemInfo->apsMemInfo);
			goto exit_out;
		}

		flags |= PVRSRV_MODIFYSYNCOPS_FLAGS_RO_INC;
	}

	/* Didn't lock for read or write -- no-op */
	if(!flags)
		goto exit_out;

	/* FIXME: Support sub-allocs in SW buffers */
	do
	{
		eError = PVRSRVModifyPendingSyncOps(
			psPrivateData->sDevData.psConnection,
			psMapperMemInfo->hKernelSyncInfoModObj,
			psMapperMemInfo->apsMemInfo[0]->psClientSyncInfo,
			flags, IMG_NULL, IMG_NULL);
	}
	while(eError == PVRSRV_ERROR_RETRY);

	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVModifyPendingSyncOps failed",
				 __func__));
		goto err_out;
	}

	/* Better not hold the private data lock if we plan to block */
	PVRSRVUnlockMutex(psPrivateData->hMutex);

	do
	{
		eError = PVRSRVSyncOpsFlushToModObj(
			psPrivateData->sDevData.psConnection,
			psMapperMemInfo->hKernelSyncInfoModObj,
			IMG_FALSE);

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
			PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for read ops "
									"to complete", __func__));
			PVRSRVClientEvent(PVRSRV_CLIENT_EVENT_HWTIMEOUT,
							  &psPrivateData->sDevData, psGrallocModule);
			eError = PVRSRV_ERROR_TIMEOUT;
			break;
		}

		if(PVRSRVEventObjectWait(psPrivateData->sDevData.psConnection,
				psPrivateData->sSGXInfo.sMiscInfo.hOSGlobalEvent) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%s: Timeout waiting for "
									  "PVRSRVSyncOpsFlushToModObj "
									  "to complete", __func__));
			ui32TriesLeft--;
		}
	}
	while(eError == PVRSRV_ERROR_RETRY);

	/* Re-take the private data lock */
	PVRSRVLockMutex(psPrivateData->hMutex);

	if(eError != PVRSRV_OK)
		goto err_out;

exit_out:
	err = 0;
err_out:
	return err;
}

static int __unlock_buffer(IMG_private_data_t *psPrivateData,
						   IMG_mapper_meminfo_t *psMapperMemInfo,
						   IMG_BOOL bIsFrameBuffer)
{
	/* Framebuffer ops don't do anything clever; they probably should,
	 * but it interacts badly with services at the moment, and the
	 * feature is only required for software composition (or our tests).
	 */

	if(bIsFrameBuffer)
		return 0;

	if(PVRSRVModifyCompleteSyncOps(psPrivateData->sDevData.psConnection,
								   psMapperMemInfo->hKernelSyncInfoModObj)
									!= PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVModifyCompleteSyncOps failed",
				 __func__));
		return -EFAULT;
	}

	return 0;
}

#else /* defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS) */

static int __lock_buffer(IMG_private_data_t *psPrivateData,
						 IMG_mapper_meminfo_t *psMapperMemInfo,
						 IMG_BOOL bIsFrameBuffer, int usage)
{
	PVR_UNREFERENCED_PARAMETER(bIsFrameBuffer);

	if(usage_write(usage))
		FlushAllReadOps(psPrivateData, psMapperMemInfo->apsMemInfo);

	if(usage_read(usage))
		FlushAllWriteOps(psPrivateData, psMapperMemInfo->apsMemInfo);

	return 0;
}

/* Optimized out */

static inline int __unlock_buffer(IMG_private_data_t *psPrivateData,
								  IMG_mapper_meminfo_t *psMapperMemInfo,
								  IMG_BOOL bIsFrameBuffer)
{
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
	PVR_UNREFERENCED_PARAMETER(psMapperMemInfo);
	PVR_UNREFERENCED_PARAMETER(bIsFrameBuffer);

	return 0;
}

#endif /* defined(ANDROID_SOFTWARE_SYNC_OBJECTS) */

static int gralloc_device_close(hw_device_t *device)
{
	free(device);
	return 0;
}

static void validate_map_unmap_params(IMG_private_data_t *psPrivateData,
									  IMG_native_handle_t *psNativeHandle,
									  IMG_mapper_meminfo_t *psMapperMemInfo)
{
#if defined(DEBUG)
	PVR_ASSERT(psPrivateData != NULL);
	PVR_ASSERT(psNativeHandle != NULL);
	PVR_ASSERT(psMapperMemInfo != NULL);

	PVR_ASSERT(psMapperMemInfo->ui32LockCount == 0);
	PVR_ASSERT(psMapperMemInfo->bRegistered == IMG_TRUE);

	PVR_ASSERT(psNativeHandle->fd[0] >= 0);
#else
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
	PVR_UNREFERENCED_PARAMETER(psNativeHandle);
	PVR_UNREFERENCED_PARAMETER(psMapperMemInfo);
#endif
}

#if (defined(PVRSRV_NEED_PVR_DPF) && !defined(PVRSRV_NEW_PVR_DPF)) || \
	(defined(PVRSRV_NEED_PVR_DPF) && defined(PVRSRV_NEW_PVR_DPF) && \
	 defined(DEBUG))

/* UINT_MAX = 4294967295 = 10 chars
 * Additional space for commas and \0 = 1 chars
 */
#define DEBUG_FD_LEN		(10 + 1)

/* ULLONG_MAX = 18446744073709551615 = 20 chars
 * Additional space for commas and \0 = 1 chars
 */
#define DEBUG_STAMPS_LEN	(20 + 1)

/* 0xDEADBEEF = 10 chars
 * Additional space for commas and \0 = 1 chars
 */
#define DEBUG_DEVVADDRS_LEN	(10 + 1)

static void
StringifyMetaData(IMG_native_handle_t *psNativeHandle,
				  PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				  char szDevVAddrs[DEBUG_DEVVADDRS_LEN * MAX_SUB_ALLOCS + 1],
				  char szStamps[DEBUG_STAMPS_LEN * MAX_SUB_ALLOCS + 1],
				  char szFds[DEBUG_FD_LEN * MAX_SUB_ALLOCS + 1])
{
	char *pszDevVAddrs = szDevVAddrs;
	char *pszStamps = szStamps;
	char *pszFds = szFds;
	int i;

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
	{
		int iWritten;

		if(!apsMemInfo[i])
			break;

		iWritten = snprintf(pszDevVAddrs, 10 + 1 + 1, "0x%.8x,",
							apsMemInfo[i]->sDevVAddr.uiAddr);
		PVR_ASSERT(iWritten >= 0);
		pszDevVAddrs += iWritten;

		iWritten = snprintf(pszStamps, 20 + 1 + 1, "%llu,",
							apsMemInfo[i]->ui64Stamp);
		PVR_ASSERT(iWritten >= 0);
		pszStamps += iWritten;

		iWritten = snprintf(pszFds, 10 + 1 + 1, "%u,",
							psNativeHandle->fd[i]);
		PVR_ASSERT(iWritten >= 0);
		pszFds += iWritten;
	}

	/* Replace the final commas with nuls */
	*(pszDevVAddrs - 1) = 0;
	*(pszStamps - 1) = 0;
	*(pszFds - 1) = 0;
}

static void DumpAllocInfo(IMG_native_handle_t *psNativeHandle,
						  PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
						  const IMG_buffer_format_t *psInputBufferFormat,
						  const IMG_buffer_format_t *psRealBufferFormat,
						  int iReqWidth, int iReqHeight, int iReqFormat,
						  int iStride)
{
	char szDevVAddrs[DEBUG_DEVVADDRS_LEN * MAX_SUB_ALLOCS + 1];
	char szStamps[DEBUG_STAMPS_LEN * MAX_SUB_ALLOCS + 1];
	char szFds[DEBUG_FD_LEN * MAX_SUB_ALLOCS + 1];

	StringifyMetaData(psNativeHandle, apsMemInfo,
					  szDevVAddrs, szStamps, szFds);

	PVR_DPF((PVR_DBG_WARNING,
		"%s: Allocated a new surface (requested / allocated):\n"
		"      width -> %d / %d\n"
		"     height -> %d / %d\n"
		"     format -> %d (HAL_PIXEL_FORMAT_%s) / %d (HAL_PIXEL_FORMAT_%s)\n"
		" 0xP...FHWR -> 0x%.8x\n"
		"     stride -> %d (pixels)\n"
		"        fds -> %s\n"
		" ui64Stamps -> %s\n"
		"  DevVAddrs -> %s",
		__func__,
		iReqWidth, psNativeHandle->iWidth,
		iReqHeight, psNativeHandle->iHeight,
		iReqFormat, psInputBufferFormat->base.szName,
		psNativeHandle->iFormat, psRealBufferFormat->base.szName,
		psNativeHandle->usage, iStride,
		szFds, szStamps, szDevVAddrs));
}

static void DumpMapInfo(IMG_native_handle_t *psNativeHandle,
						PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	char szDevVAddrs[DEBUG_DEVVADDRS_LEN * MAX_SUB_ALLOCS + 1];
	char szStamps[DEBUG_STAMPS_LEN * MAX_SUB_ALLOCS + 1];
	char szFds[DEBUG_FD_LEN * MAX_SUB_ALLOCS + 1];
	const IMG_buffer_format_t *psRealBufferFormat;

	StringifyMetaData(psNativeHandle, apsMemInfo,
					  szDevVAddrs, szStamps, szFds);

	psRealBufferFormat = GetBufferFormat(psNativeHandle->iFormat);
	PVR_ASSERT(psRealBufferFormat != IMG_NULL);

	PVR_DPF((PVR_DBG_WARNING,
		"%s: Mapped a new surface:\n"
		"      width -> %d\n"
		"     height -> %d\n"
		"     format -> %d (HAL_PIXEL_FORMAT_%s)\n"
		" 0xP...FHWR -> 0x%.8x\n"
		"        fds -> %s\n"
		" ui64Stamps -> %s\n"
		"  DevVAddrs -> %s",
		__func__,
		psNativeHandle->iWidth,
		psNativeHandle->iHeight,
		psNativeHandle->iFormat, psRealBufferFormat->base.szName,
		psNativeHandle->usage, szFds, szStamps, szDevVAddrs));
}

#endif /* (defined(PVRSRV_NEED_PVR_DPF) && !defined(PVRSRV_NEW_PVR_DPF)) ||
		  (defined(PVRSRV_NEED_PVR_DPF) && defined(PVRSRV_NEW_PVR_DPF) &&
		   defined(DEBUG)) */

static int __map(IMG_private_data_t *psPrivateData,
				 IMG_native_handle_t *psNativeHandle,
				 IMG_mapper_meminfo_t *psMapperMemInfo)
{
	int i, err = 0;

	ENTERED();

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
	{
		if(psNativeHandle->fd[i] < 0)
		{
			psMapperMemInfo->apsMemInfo[i] = IMG_NULL;
			break;
		}

		if(PVRSRVMapDeviceMemory2(&psPrivateData->sDevData,
								  psNativeHandle->fd[i],
								  psPrivateData->hGeneralHeap,
								  &psMapperMemInfo->apsMemInfo[i])
														!= PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Map device memory failed",
									__func__));
			err = -EFAULT;
			goto err_out;
		}
	}

#if (defined(PVRSRV_NEED_PVR_DPF) && !defined(PVRSRV_NEW_PVR_DPF)) || \
	(defined(PVRSRV_NEED_PVR_DPF) && defined(PVRSRV_NEW_PVR_DPF) && \
	 defined(DEBUG))
	DumpMapInfo(psNativeHandle, psMapperMemInfo->apsMemInfo);
#endif

err_out:
	EXITED();
	return err;
}

static int __unmap(IMG_private_data_t *psPrivateData,
				   IMG_mapper_meminfo_t *psMapperMemInfo,
				   int aiFd[MAX_SUB_ALLOCS])
{
	int i, err = 0;

	PVR_UNREFERENCED_PARAMETER(aiFd);

	ENTERED();

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
	{
		if(!psMapperMemInfo->apsMemInfo[i])
			break;

		if(PVRSRVUnmapDeviceMemory(&psPrivateData->sDevData,
			   psMapperMemInfo->apsMemInfo[i]) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unmap device memory failed",
									__func__));
			err = -EFAULT;
			goto err_out;
		}
	}

err_out:
	EXITED();
	return err;
}

static int __unmap_remove(IMG_private_data_t *psPrivateData,
						  IMG_mapper_meminfo_t *psMapperMemInfo,
						  int aiFd[MAX_SUB_ALLOCS])
{
	int err;

	ENTERED();

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
	{
		IMG_mapper_meminfo_t *psAccumPairMapperMemInfo =
			MapperPeekAccumPair(psPrivateData, psMapperMemInfo);
		if(psAccumPairMapperMemInfo)
			if(!OpsFlushed(psAccumPairMapperMemInfo->apsMemInfo))
				return -EAGAIN;
	}
#endif /* defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND) */

	err = __unmap(psPrivateData, psMapperMemInfo, aiFd);
	if(err)
		goto exit_out;

	MapperRemove(psPrivateData, psMapperMemInfo);
exit_out:
	EXITED();
	return err;
}

static int __free(IMG_private_data_t *psPrivateData,
				  IMG_mapper_meminfo_t *psMapperMemInfo,
				  int aiFd[MAX_SUB_ALLOCS])
{
	const IMG_buffer_format_t *psBufferFormat;
	int err;

	ENTERED();

	psBufferFormat = GetBufferFormat(psMapperMemInfo->iFormat);
	PVR_ASSERT(psBufferFormat != IMG_NULL);

	err = psBufferFormat->pfnFree(&psPrivateData->sDevData,
								  psMapperMemInfo->apsMemInfo,
								  aiFd);
	if(err)
		goto exit_out;

	MapperRemove(psPrivateData, psMapperMemInfo);
exit_out:
	EXITED();
	return err;
}

static int gralloc_device_alloc(alloc_device_t *psAllocDevice, int w, int h,
	int format, int usage, buffer_handle_t *handle, int *stride)
{
	const IMG_buffer_format_t *psInputBufferFormat, *psRealBufferFormat;
	IMG_UINT32 ui32Flags = PVRSRV_MEM_XPROC | PVRSRV_MEM_READ;
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS];
	int i, err = 0, iStride, iWidth, iHeight, iFormat;
	IMG_native_handle_t *psNativeHandle;
	IMG_private_data_t *psPrivateData;
	IMG_UINT32 ui32NumValidFds = 0;
	int aiFd[MAX_SUB_ALLOCS];

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!psAllocDevice || !handle || !stride)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p, %p, %p)",
				 __func__, psAllocDevice, handle, stride));
		err = -EINVAL;
		goto err_out;
	}

	if(usage_unsupported(usage))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unsupported usage bits (0xP...FHWR=0x%.8x)"
								" for allocation", __func__, usage));
		err = -EINVAL;
		goto err_out;
	}

	if(usage_write(usage))
		ui32Flags |= PVRSRV_MEM_WRITE;

	if(usage_sw_cached(usage))
	{
		ui32Flags |= PVRSRV_MEM_CACHED;
	}
	else if(usage_sw_write(usage))
	{
		ui32Flags |= PVRSRV_MEM_WRITECOMBINE;
	}

	psInputBufferFormat = GetBufferFormat(format);
	if(!psInputBufferFormat)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid color format (%d)",
								__func__, format));
		err = -EINVAL;
		goto err_out;
	}

#if !defined(PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER)
	if(usage_bypass_fb(usage) && !psInputBufferFormat->base.bGPURenderable)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Framebuffer/bypass usage bits are "
								"incompatible with non-GPU-renderable "
								"pixel format (%d)", __func__, format));
		err = -EINVAL;
		goto err_out;
	}
#endif

	psPrivateData = getDevicePrivateData(psAllocDevice);

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
	if(usage_bypass(usage))
	{
		framebuffer_device_t *psFrameBufferDevice =
			getDeviceFrameBufferDevice(psAllocDevice);

		if(!psFrameBufferDevice)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate frame buffer "
									"(must open first).", __func__));
			err = -EFAULT;
			goto err_put;
		}

		if(psPrivateData->sAppHints.ui32HALCompositionBypass == 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Cannot allocate bypass buffers "
									"when HALCompositionBypass=0", __func__));
			err = -EINVAL;
			goto err_put;
		}

		/* The "real" parameters of this allocation match the framebuffer */
		iWidth	= psFrameBufferDevice->width;
		iHeight	= psFrameBufferDevice->height;
		iFormat	= psFrameBufferDevice->format;
		iStride	= psFrameBufferDevice->stride;

		psRealBufferFormat = GetBufferFormat(iFormat);
		PVR_ASSERT(psRealBufferFormat != IMG_NULL);
	}
	else
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */
	{
		iWidth	= w;
		iHeight	= h;
		iFormat	= format;
		iStride	= ALIGN(w, HW_ALIGN);
		psRealBufferFormat = psInputBufferFormat;
	}

	/* We're allocating the framebuffer; we must defer */
	if(usage_bypass_fb(usage))
	{
		framebuffer_device_t *psFrameBufferDevice =
			getDeviceFrameBufferDevice(psAllocDevice);
		IMG_mapper_meminfo_t *psMapperMemInfo;

		if(!psFrameBufferDevice)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate frame buffer "
									"(must open first).", __func__));
			err = -EFAULT;
			goto err_put;
		}

		/* Most of these flags are used only for validation */
		err = framebuffer_device_alloc(psFrameBufferDevice,
									   iWidth, iHeight, iFormat, usage,
									   &psNativeHandle, stride);
		if(err)
			goto err_put;

		/* FIXME: Temporarily disabled for NP */
		/* Strides must match */
		/*PVR_ASSERT(*stride == iStride);*/

		/* Look up the buffer we just allocated so we can clear it */
		psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
		PVR_ASSERT(psMapperMemInfo != NULL);

		for(i = 0; i < MAX_SUB_ALLOCS; i++)
			apsMemInfo[i] = psMapperMemInfo->apsMemInfo[i];

#if defined(SUPPORT_ANDROID_COMPOSITION_BYPASS)
		if(usage_bypass(usage))
		{
			/* If this buffer is being used as a HW texture, it's implicitly
			 * for the composition bypass workaround, and we need to export it.
			 */

			/* We might have already done a bypass with this buffer, in which
			 * case it'll already be exported. Only need to export if we see
			 * the fake file descriptor.
			 */
			if(!usage_bypass(psMapperMemInfo->usage))
			{
				if(PVRSRVExportDeviceMem2(&psPrivateData->sDevData,
										  apsMemInfo[0], &aiFd[0]) != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Failed to export device memory",
											__func__));
					err = -EFAULT;
					goto err_put;
				}

				/* Update the "fake" file descriptor with a real one */
				psNativeHandle->fd[0] = aiFd[0];

				/* Patch in the updated stamp */
				psMapperMemInfo->ui64Stamp = apsMemInfo[0]->ui64Stamp;
				psNativeHandle->ui64Stamp = apsMemInfo[0]->ui64Stamp;

				/* Patch the buffer usage */
				psMapperMemInfo->usage = usage;
				psNativeHandle->usage = usage;
			}
		}
#endif /* defined(SUPPORT_ANDROID_COMPOSITION_BYPASS) */

		goto exit_assign_misc;
	}

	err = psRealBufferFormat->pfnAlloc(&psPrivateData->sDevData,
									   psPrivateData->hGeneralHeap,
									   &iWidth, &iHeight, &iStride, &usage,
									   psRealBufferFormat->base.uiBpp,
									   ui32Flags, apsMemInfo, aiFd);
	if(err)
		goto err_put;

	/* These must match, or we'll get weird memory corruption */
	PVR_ASSERT(IMG_NATIVE_HANDLE_NUMFDS == MAX_SUB_ALLOCS);
	PVR_ASSERT(sizeof(IMG_native_handle_t) ==
			   sizeof(native_handle_t) + sizeof(int) *
				(IMG_NATIVE_HANDLE_NUMFDS + IMG_NATIVE_HANDLE_NUMINTS));

	/* Figure out how many fds this handle has */
	for (i = 0; i < MAX_SUB_ALLOCS; i++)
		if(aiFd[i] >= 0)
			ui32NumValidFds++;

	/* Any invalid fds get converted to ints so the dup(2) won't
	 * fail when copying the native handle.
	 */
	psNativeHandle = (IMG_native_handle_t *)
		native_handle_create(ui32NumValidFds,
							 IMG_NATIVE_HANDLE_NUMFDS - ui32NumValidFds +
							 IMG_NATIVE_HANDLE_NUMINTS);
	if(!psNativeHandle)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate buffer handle", __func__));
		err = -ENOMEM;
		goto err_put;
	}

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
		psNativeHandle->fd[i] = aiFd[i];

	psNativeHandle->usage	  = usage;
	psNativeHandle->ui64Stamp = apsMemInfo[0]->ui64Stamp;

	// FIXME: Don't duplicate this
	psNativeHandle->iFormat	= iFormat;

	if(!MapperAddMapped(psPrivateData, psNativeHandle, apsMemInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to add buffer handle to mapper",
				 __func__));
		native_handle_delete((native_handle_t *)psNativeHandle);
		err = -EFAULT;
		goto err_put;
	}

	if(!MapperSanityCheck(psPrivateData))
	{
		err = -EFAULT;
		goto err_put;
	}

	*stride = iStride;

exit_assign_misc:
	psNativeHandle->iWidth	= iWidth;
	psNativeHandle->iHeight	= iHeight;
	psNativeHandle->iFormat	= iFormat;
	psNativeHandle->uiBpp	= psRealBufferFormat->base.uiBpp;
	*handle					= (buffer_handle_t)psNativeHandle;

	if(!psRealBufferFormat->bSkipMemset)
	{
		/* If the buffer format didn't opt out of memsetting, wipe all
		 * of the buffer planes. Some allocators don't like us touching
		 * pixels between width and stride.
		 *
		 * Android implicitly requires buffers are cleared to black
		 * if used for software rendering. But it is not really known
		 * if this is a problem for other buffers. In theory, GPU-only
		 * buffers should not need to be cleared.
		 */
		for(i = 0; i < MAX_SUB_ALLOCS; i++)
		{
			if(!apsMemInfo[i])
				break;
			memset(apsMemInfo[i]->pvLinAddr, 0, apsMemInfo[i]->uAllocSize);
		}
	}

#if (defined(PVRSRV_NEED_PVR_DPF) && !defined(PVRSRV_NEW_PVR_DPF)) || \
	(defined(PVRSRV_NEED_PVR_DPF) && defined(PVRSRV_NEW_PVR_DPF) && \
	 defined(DEBUG))
	DumpAllocInfo(psNativeHandle, apsMemInfo, psInputBufferFormat,
				  psRealBufferFormat, w, h, format, *stride);
#endif

err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

static int gralloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	alloc_device_t *psAllocDevice = (alloc_device_t *)dev;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!psAllocDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter", __func__));
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
		goto err_out;

	psPrivateData = getDevicePrivateData(psAllocDevice);

	if(usage_bypass_fb(psNativeHandle->usage))
	{
		framebuffer_device_t *psFrameBufferDevice =
			getDeviceFrameBufferDevice(psAllocDevice);

		if(psFrameBufferDevice)
		{
			err = framebuffer_device_free(psFrameBufferDevice,
										  psNativeHandle);
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate frame buffer "
									"(must open first).", __func__));
			err = -EFAULT;
		}

		goto err_put;
	}

	psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);

	if(!psMapperMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot find buffer to free", __func__));
		goto err_put;
	}

	if(!psMapperMemInfo->bAllocatedByThisProcess)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot free a buffer not allocated "
								"by this process", __func__));
		goto err_put;
	}

	if(psMapperMemInfo->ui32LockCount)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to free locked buffer %p "
								"(ID=%llu)", __func__,
				 psNativeHandle,
				 (unsigned long long)psNativeHandle->ui64Stamp));
		err = -EFAULT;
		goto err_put;
	}

	err = CheckDeferFlushOp(psPrivateData, __free,
							psMapperMemInfo, psNativeHandle->fd);
	if(err)
		goto err_put;

	native_handle_delete((native_handle_t *)psNativeHandle);
err_put:
	putDevicePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#if defined(PVR_ANDROID_HAS_GRALLOC_DUMP)

static void gralloc_device_dump(alloc_device_t *dev, char *buff, int buff_len)
{
	IMG_private_data_t *psPrivateData;

	ENTERED();

	if(!IsInitialized())
		goto exit_out;

	psPrivateData = getDevicePrivateData(dev);
	MapperDumpSyncObjects(psPrivateData, buff, (size_t)buff_len);
	putDevicePrivateData(psPrivateData);

exit_out:
	EXITED();
	return;
}

#endif /* defined(PVR_ANDROID_HAS_GRALLOC_DUMP) */

IMG_INTERNAL
int gralloc_register_buffer(gralloc_module_t const* module,
							buffer_handle_t handle)
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
		goto err_out;

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);

	if(psMapperMemInfo)
  	{
		/* The compositing process may call registerBuffer() redundantly
		 * (redundant because the spec says alloc() implies registerBuffer()).
		 * This is bizarre, but not exactly forbidden by the specification,
		 * so we'll just skip the sanity check.
		 */
		if(psMapperMemInfo->bAllocatedByThisProcess)
			goto exit_out;

		if(psMapperMemInfo->bRegistered)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Cannot register a buffer twice "
									"(ID=%llu)",
					 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
			goto err_put;
		}

		/* Flag this buffer as registered */
		psMapperMemInfo->bRegistered = IMG_TRUE;
	}
	else
	{
		if(!MapperAddUnmapped(psPrivateData, psNativeHandle))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to add unmapped buffer to "
									"mapper (ID=%llu)",
					 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
			err = -EFAULT;
			goto err_put;
		}

		/* Map in "persistent" buffers now */
		if(usage_persistent(psNativeHandle->usage))
		{
			/* We just added this, so it can't fail */
			psMapperMemInfo =
				MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
			PVR_ASSERT(psMapperMemInfo != NULL);

			/* We'll map in the buffer now, since HW rendering always needs
			 * it, and it isn't unmapped until unregister time.
			 */
			validate_map_unmap_params(psPrivateData, psNativeHandle,
									  psMapperMemInfo);

			err = __map(psPrivateData, psNativeHandle, psMapperMemInfo);
			if(err)
				goto err_put;
		}

		if(!MapperSanityCheck(psPrivateData))
		{
			CheckDeferFlushOp(psPrivateData, __unmap,
							  psMapperMemInfo, psNativeHandle->fd);
			err = -EFAULT;
			goto err_put;
		}
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Registered buffer (ID=%llu)",
			 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
exit_out:
	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_unregister_buffer(gralloc_module_t const* module,
							  buffer_handle_t handle)
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
		goto err_out;

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);

	if(!psMapperMemInfo || !psMapperMemInfo->bRegistered)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot unregister unregistered "
								"buffer (ID=%llu)",
				 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
		goto err_put;
	}

	/* The compositing process may call unregisterBuffer() redundantly
	 * (redundant because the spec says free() implies unregisterBuffer()).
	 */
	if(psMapperMemInfo->bAllocatedByThisProcess)
		goto exit_out;

	if(psMapperMemInfo->ui32LockCount)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot unregister a locked buffer "
								"(ID=%llu)",
				 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
		goto err_put;
	}

	if(!psMapperMemInfo->bAllocatedByThisProcess)
	{
		/* We only need to do the "real" unmap if we're a buffer for
		 * hardware rendering. Software should already have been
		 * unmapped in gralloc_module_unlock().
		 */
		if(usage_persistent(psMapperMemInfo->usage))
		{
			validate_map_unmap_params(psPrivateData, psNativeHandle,
									  psMapperMemInfo);

			/* Really unmap the buffer from this process */
			err = CheckDeferFlushOp(psPrivateData, __unmap_remove,
									psMapperMemInfo, psNativeHandle->fd);
			if(err)
				goto err_put;
		}
		else
		{
			MapperRemove(psPrivateData, psMapperMemInfo);
		}
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Unregistered buffer (ID=%llu)",
			 __func__, (unsigned long long)psNativeHandle->ui64Stamp));
exit_out:
	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_module_map(gralloc_module_t const* module,
					   IMG_UINT64 ui64Stamp, int usage,
					   PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int i, err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	PVR_ASSERT(module != NULL);
	PVR_ASSERT(apsMemInfo != NULL);

	if(usage_sw(usage))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot map buffer with SW usage bits",
				 __func__));
		goto err_out;
	}

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo =
		MapperPeek(psPrivateData, ui64Stamp);

	if(!psMapperMemInfo || (!psMapperMemInfo->bAllocatedByThisProcess &&
							!psMapperMemInfo->bRegistered))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot map buffer ID=%llu before "
								"register (%p)",
				 __func__, (unsigned long long)ui64Stamp,
				 psMapperMemInfo));
		goto err_put;
	}

	if(psMapperMemInfo->ui32LockCount > 0)
	{
		/* Can't lock for write if it's locked for anything else */
		if(usage_write(usage))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Buffer cannot be locked for any "
									"write operation when it is already "
									"locked", __func__));
			goto err_put;
		}

		/* Can't lock for read if we've already locked for writing */
		if(usage_write(psMapperMemInfo->lockUsage) && usage_read(usage))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Mismatching lock usage bits "
									"0xP...FHWR=0x%.8x vs requested usage bits "
									"0xP...FHWR=0x%.8x",
					 __func__, psMapperMemInfo->lockUsage, usage));
			goto err_put;
		}
	}
	else
	{
		PVR_ASSERT(psMapperMemInfo->usage != 0);

		if((psMapperMemInfo->usage & usage) != usage)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Mismatching buffer usage bits "
									  "0xP...FHWR=0x%.8x vs requested usage bits "
									  "0xP...FHWR=0x%.8x (ID=%llu)",
					 __func__, psMapperMemInfo->usage, usage,
					 (unsigned long long)ui64Stamp));
		}

		psMapperMemInfo->lockUsage = usage;
	}

	psMapperMemInfo->ui32LockCount++;

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
		apsMemInfo[i] = psMapperMemInfo->apsMemInfo[i];

	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_module_unmap(gralloc_module_t const* module, IMG_UINT64 ui64Stamp)
{
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	PVR_ASSERT(module != NULL);

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo =
		MapperPeek(psPrivateData, ui64Stamp);

	if(!psMapperMemInfo || (!psMapperMemInfo->bAllocatedByThisProcess &&
							!psMapperMemInfo->bRegistered))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot unmap unregistered buffer",
				 __func__));
		goto err_put;
	}

	PVR_ASSERT(psMapperMemInfo->ui32LockCount != 0);
	psMapperMemInfo->ui32LockCount--;

	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_module_lock(gralloc_module_t const* module, buffer_handle_t handle,
					    int usage, int l, int t, int w, int h, void **vaddr)
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_BOOL bErrNeedsUnmap = IMG_FALSE;
	IMG_private_data_t *psPrivateData;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		err = -EINVAL;
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
	{
		err = -EINVAL;
		goto err_out;
	}

	if(!validate_lock_rectangle(psNativeHandle, l, t, w, h))
	{
		err = -EINVAL;
		goto err_out;
	}

	if(!vaddr && usage_sw(usage))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (for software map)",
				 __func__));
		err = -EINVAL;
		goto err_out;
	}

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo =
		MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);

	if(!psMapperMemInfo || !psMapperMemInfo->bRegistered)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot lock buffer ID=%llu before "
								"register (%p)",
				 __func__, (unsigned long long)psNativeHandle->ui64Stamp,
				 psMapperMemInfo));
		err = -EINVAL;
		goto err_put;
	}

	if(psMapperMemInfo->ui32LockCount > 0)
	{
		if(usage_write(usage))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Buffer cannot be locked for any "
									"write operation when it is already "
									"locked", __func__));
			err = -EINVAL;
			goto err_put;
		}

		if(psMapperMemInfo->lockUsage != usage)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Mismatching lock usage bits "
									"0xP...FHWR=0x%.8x vs requested usage "
									"bits 0xP...FHWR=0x%.8x",
					 __func__, psMapperMemInfo->lockUsage, usage));
			err = -EINVAL;
			goto err_put;
		}
	}
	else
	{
		PVR_ASSERT(psMapperMemInfo->usage != 0);

		/* Warn if the usage bits don't match. If we know we're
		 * using a foreign GL (software implementation) we can
		 * expect some bogus usage bits, and don't need to warn.
		 */
		if(!gbForeignGLES && ((psMapperMemInfo->usage & usage) != usage))
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Mismatching buffer usage bits "
									  "0xP...FHWR=0x%.8x vs requested usage bits "
									  "0xP...FHWR=0x%.8x",
					 __func__, psMapperMemInfo->usage, usage));
			PVR_DPF((PVR_DBG_WARNING, "%s: This may be fine, but should be "
									  "checked", __func__));
		}

		/* Shouldn't map if we own the buffer or it's persistently mapped */
		if(!psMapperMemInfo->bAllocatedByThisProcess &&
		   !usage_persistent(psMapperMemInfo->usage))
		{
			validate_map_unmap_params(psPrivateData, psNativeHandle,
									  psMapperMemInfo);

			err = __map(psPrivateData, psNativeHandle, psMapperMemInfo);
			if(err)
				goto err_put;

			bErrNeedsUnmap = IMG_TRUE;

			if(!MapperSanityCheck(psPrivateData))
			{
				err = -EFAULT;
				goto err_put;
			}
		}

		/* Only need to do HW synchronization for this buffer if there are
		 * HW usage bits set on it.
		 */
		if(usage_hw(psMapperMemInfo->usage))
		{
			err = __lock_buffer(psPrivateData, psMapperMemInfo,
								usage_bypass_fb(psNativeHandle->usage), usage);
			if(err)
				goto err_put;

			/* If we're locking the buffer for a write operation, we need to
			 * record the lock rectangle so we can compute the virtual address
			 * region(s) for flushing the CPU cache.
			 */
			if(usage_write(usage))
			{
				psMapperMemInfo->sWriteLockRect.l = l;
				psMapperMemInfo->sWriteLockRect.t = t;
				psMapperMemInfo->sWriteLockRect.w = w;
				psMapperMemInfo->sWriteLockRect.h = h;
			}
		}

		psMapperMemInfo->lockUsage = usage;
	}

	psMapperMemInfo->ui32LockCount++;

	if(vaddr)
	{
		*vaddr = NULL;
		if(usage_sw(usage))
		{
			IMG_VOID **ppvVAddrs = vaddr;
			int i;
			for(i = 0; i < MAX_SUB_ALLOCS; i++)
			{
				if(!psMapperMemInfo->apsMemInfo[i])
					break;
				ppvVAddrs[i] = psMapperMemInfo->apsMemInfo[i]->pvLinAddr;
			}
		}
	}

err_put:
	if(err && bErrNeedsUnmap)
	{
		validate_map_unmap_params(psPrivateData, psNativeHandle,
								  psMapperMemInfo);

		CheckDeferFlushOp(psPrivateData, __unmap,
						  psMapperMemInfo, psNativeHandle->fd);
	}
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_module_unlock(gralloc_module_t const* module, buffer_handle_t handle)
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		err = -EINVAL;
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
	{
		err = -EINVAL;
		goto err_out;
	}

	psPrivateData = getModulePrivateData(module);

	psMapperMemInfo =
		MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);

	if(!psMapperMemInfo || !psMapperMemInfo->bRegistered)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot unlock unmapped buffer",
				 __func__));
		err = -EINVAL;
		goto err_put;
	}

	if(!psMapperMemInfo->ui32LockCount)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Buffer is already unlocked", __func__));
		err = -EINVAL;
		goto err_put;
	}

	PVR_ASSERT(psMapperMemInfo->ui32LockCount != 0);
	psMapperMemInfo->ui32LockCount--;

	/* Not the last lock; don't unmap it */
	if(psMapperMemInfo->ui32LockCount > 0)
		goto err_put;

	/* Only need to do HW synchronization for this buffer if there are
	 * HW usage bits set on it.
	 */
	if(usage_hw(psMapperMemInfo->usage) && usage_sw_cached(psMapperMemInfo->usage))
	{
		IMG_BOOL bIsFrameBuffer = usage_bypass_fb(psNativeHandle->usage);

		/* If the buffer was originally locked for write, we need to
		 * perform a CPU cache flush for the locked region.
		 */
		if(usage_write(psMapperMemInfo->lockUsage) && !bIsFrameBuffer)
		{
			const IMG_buffer_format_t *psBufferFormat =
				GetBufferFormat(psNativeHandle->iFormat);
			PVR_ASSERT(psBufferFormat != IMG_NULL);

			if(psBufferFormat->pfnFlushCache)
			{
				err = psBufferFormat->pfnFlushCache(
						psNativeHandle,
						psPrivateData->sDevData.psConnection,
						psMapperMemInfo->apsMemInfo,
						&psMapperMemInfo->sWriteLockRect);
				if(err)
					goto err_put;
			}
		}

		err = __unlock_buffer(psPrivateData, psMapperMemInfo, bIsFrameBuffer);
		if(err)
			goto err_put;
	}

	/* Shouldn't unmap if we own the buffer or it's persistently mapped */
	if(!psMapperMemInfo->bAllocatedByThisProcess &&
	   !usage_persistent(psMapperMemInfo->usage))
	{
		validate_map_unmap_params(psPrivateData, psNativeHandle,
								  psMapperMemInfo);

		err = CheckDeferFlushOp(psPrivateData, __unmap,
								psMapperMemInfo, psNativeHandle->fd);
		if(err)
			goto err_put;
	}

err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#if defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS)

IMG_INTERNAL
int gralloc_module_getphyaddrs(IMG_gralloc_module_public_t const* module,
							   buffer_handle_t handle,
							   unsigned int auiPhyAddr[MAX_SUB_ALLOCS])
{
	IMG_native_handle_t *psNativeHandle = (IMG_native_handle_t *)handle;
	const IMG_buffer_format_t *psBufferFormat;
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		goto err_out;
	}

	if(!validate_handle(psNativeHandle))
	{
		goto err_out;
	}

	psPrivateData = getModulePrivateData((gralloc_module_t *)module);

	psMapperMemInfo = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
	PVR_ASSERT(psMapperMemInfo != IMG_NULL);

	psBufferFormat = GetBufferFormat(psNativeHandle->iFormat);
	PVR_ASSERT(psBufferFormat != IMG_NULL);

	if(!psBufferFormat->pfnGetPhyAddrs)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Buffer does not allow physical "
								"address to be looked up", __func__));
		goto err_put;
	}

	PVR_ASSERT(sizeof(unsigned int) == sizeof(IMG_SYS_PHYADDR));

	if(!psBufferFormat->pfnGetPhyAddrs(psMapperMemInfo->apsMemInfo,
									   (IMG_SYS_PHYADDR *)auiPhyAddr))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to look up buffer "
								"physical address", __func__));
		goto err_put;
	}

	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#endif /* defined(SUPPORT_GET_DC_BUFFERS_SYS_PHYADDRS) */

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)

IMG_INTERNAL
int gralloc_module_setaccumbuffer(gralloc_module_t const* module,
								  IMG_UINT64 ui64Stamp,
								  IMG_UINT64 ui64AccumStamp)
{
	IMG_mapper_meminfo_t *psMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!module)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter (%p)",
				 __func__, module));
		goto err_out;
	}

	psPrivateData = getModulePrivateData((gralloc_module_t *)module);

	psMapperMemInfo = MapperPeek(psPrivateData, ui64Stamp);
	if(!psMapperMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unexpectedly failed to look "
								"up handle", __func__));
		goto err_put;
	}

	/* NOTE: May be zero if there is no accumulation */
	psMapperMemInfo->ui64AccumStamp = ui64AccumStamp;
	err = 0;
err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

#endif /* defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND) */

/* Blit from src to dest, possibly converting to format. The blit goes to
   the rectangle specified by w, h, x and y in the destination surface. */
static
int blit_internal(IMG_private_data_t *psPrivateData,
				  buffer_handle_t src,
				  PVR2DMEMINFO *apsDestMemInfo[MAX_SUB_ALLOCS],
				  int format, int w, int h, int x, int y,
				  IMG_BOOL bWait)
{
	IMG_native_handle_t *psSrcHandle = (IMG_native_handle_t *)src;
	const IMG_buffer_format_t *psDestBufferFormat;
	IMG_mapper_meminfo_t *psSrcMapperMemInfo;
	PVR2D_SURFACE_EXT sDstExt;
	int err = 0;

	PVR2D_3DBLT_EXT sBlitInfo = {
		.rcDest = {
			.left	= x,
			.top	= y,
			.right	= x + w,
			.bottom	= y + h,
		},
		.rcSource = {
			.left	= 0,
			.top	= 0,
			.right	= psSrcHandle->iWidth,
			.bottom	= psSrcHandle->iHeight,
		},
		.pSrc2 = IMG_NULL,
		.prcSource2 = IMG_NULL,
		.hUseCode = IMG_NULL,
		.pDstExt = IMG_NULL,	/* filled in below */
		.bDisableDestInput = IMG_FALSE,
	};

	ENTERED();

	psSrcMapperMemInfo = MapperPeek(psPrivateData, psSrcHandle->ui64Stamp);
	PVR_ASSERT(psSrcMapperMemInfo != NULL);

	sBlitInfo.sSrc.Format = HAL2PVR2DPixelFormat(psSrcHandle->iFormat);
	PVR_ASSERT(sBlitInfo.sSrc.Format != PVR2D_BAD);

	sBlitInfo.sDst.Format = HAL2PVR2DPixelFormat(format);
	PVR_ASSERT(sBlitInfo.sDst.Format != PVR2D_BAD);

	psDestBufferFormat = GetBufferFormat(format);
	if (!psDestBufferFormat)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid destination format (%d)",
								__func__, format));
		err = -EINVAL;
		goto err_out;
	}

	/* Compute the destination surface parameters for the blit info. The
	   width and height used here are for the destination surface. The
	   values come from the source surface - we require that they are the
	   same, even if w and h specify a different dest rectangle. */
	{
		int iStride = ALIGN(psSrcHandle->iWidth, HW_ALIGN)
			* psDestBufferFormat->base.uiBpp / 8;
		int iWidth = psSrcHandle->iWidth, iHeight = psSrcHandle->iHeight;

		sBlitInfo.sSrc.SurfWidth	= psSrcHandle->iWidth;
		sBlitInfo.sSrc.SurfHeight	= psSrcHandle->iHeight;
		sBlitInfo.sSrc.Stride =
			ALIGN(psSrcHandle->iWidth, HW_ALIGN) *
			psSrcHandle->uiBpp / 8;

		psDestBufferFormat->pfnComputeParams(0, &iWidth, &iHeight, &iStride);

		/* If the width or height was changed by pfnComputeParams, the
		   source surface didn't match the width/height padding
		   requirements. Signal an error. */
		if (iWidth != psSrcHandle->iWidth ||
			iHeight != psSrcHandle->iHeight)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Mismatching src/dest dimensions"
					 " (src %dx%d, dest %dx%d)", __func__,
					 psSrcHandle->iWidth, psSrcHandle->iHeight,
					 iWidth, iHeight));
			err = -EINVAL;
			goto err_out;
		}

		sBlitInfo.sDst.SurfWidth	= iWidth;
		sBlitInfo.sDst.SurfHeight	= iHeight;
		sBlitInfo.sDst.Stride		= iStride;
	}

	/* PVR2D needs wrapped meminfos -- do this before blitting */
	{
		PVRSRV_CLIENT_MEM_INFO *psSrcMemInfo =
			/* We assume that the source surface doesn't have sub-allocs, by
			   specifying [0] here. */
			psSrcMapperMemInfo->apsMemInfo[0];

		PVR2DMEMINFO sSrcMemInfo = {
			.hPrivateData		= psSrcMemInfo,
			.pBase				= psSrcMemInfo->pvLinAddr,
			.hPrivateMapData	= psSrcMemInfo->hKernelMemInfo,
			.ui32DevAddr		= psSrcMemInfo->sDevVAddr.uiAddr,
			.ui32MemSize		= (unsigned long)psSrcMemInfo->uAllocSize,
		};

		PVR2DERROR ePVR2DError;

		sBlitInfo.sSrc.pSurfMemInfo = &sSrcMemInfo;
		sBlitInfo.sDst.pSurfMemInfo = apsDestMemInfo[0];

		if (MAX_SUB_ALLOCS > 2 && apsDestMemInfo[2])
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Blits with >2 destination planes are "
					 "not yet implemented", __func__));
			err = -EINVAL;
			goto err_out;
		}

		PVRSRV_CLIENT_MEM_INFO *pb =(PVRSRV_CLIENT_MEM_INFO *)apsDestMemInfo[0]->hPrivateData;

		/* if planeOffsets[1] is not zero we have a two plane destination */
		if (MAX_SUB_ALLOCS > 1 && pb->planeOffsets[1] != 0)
		{
			/* This is a 2-destination-plane blit to a YUV format. PVR2D
			   expects to be given the offset of the start of the chroma
			   plane from the start of the source buffer (in device
			   virtual). We are relying on unsigned wraparound when the
			   chroma plane comes first. */
			sDstExt.uChromaPlane1 =
				(apsDestMemInfo[0]->ui32DevAddr + pb->planeOffsets[1]) - sSrcMemInfo.ui32DevAddr;

			sBlitInfo.pDstExt = &sDstExt;

			/* PVR2D does not support blending for YUV blits, so
			   bDisableDestInput must be set. */
			sBlitInfo.bDisableDestInput = IMG_TRUE;
		}

		ePVR2DError = PVR2DBlt3DExt(psPrivateData->hContext, &sBlitInfo);

		if(ePVR2DError != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Blit via PVR2D failed (%d)",
					 __func__, ePVR2DError));
			err = -EFAULT;
			goto err_out;
		}

		if (bWait)
		{
			/* Wait for the blit to complete */
			if (PVR2DQueryBlitsComplete(psPrivateData->hContext,
						sBlitInfo.sDst.pSurfMemInfo, 1) != PVR2D_OK)
			{
				/* If the blit times out, this might be a serious error, or
				   the blit might just have taken a long time. Print a
				   message, but don't return an error in this case. */
				PVR_DPF((PVR_DBG_ERROR, "%s: Timeout waiting for blit to complete",
						 __func__));
			}
		}
	}

err_out:
	EXITED();
	return err;
}

IMG_INTERNAL int
gralloc_module_blit_to_vaddr(const IMG_gralloc_module_public_t *module,
							 buffer_handle_t src,
							 void *dest[MAX_SUB_ALLOCS], int format)
{
	IMG_native_handle_t *psSrcHandle = (IMG_native_handle_t *)src;
	PVR2DMEMINFO *apsDestMemInfo[MAX_SUB_ALLOCS] = {0};
	const IMG_buffer_format_t *psDestBufferFormat;
	IMG_private_data_t *psPrivateData;
	int i, err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!validate_handle(psSrcHandle))
		goto err_out;

	psPrivateData = getModulePrivateData((gralloc_module_t *)module);
	psDestBufferFormat = GetBufferFormat(format);

	if (!psDestBufferFormat)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid destination format (%d)",
				 __func__, format));
		goto err_put;
	}

	for (i = 0; dest[i] && i < MAX_SUB_ALLOCS; i++)
	{
		int iStride, iWidth, iHeight;

		iStride = ALIGN(psSrcHandle->iWidth, HW_ALIGN)
			* psDestBufferFormat->base.uiBpp / 8;
		iWidth = psSrcHandle->iWidth;
		iHeight = psSrcHandle->iHeight;

		psDestBufferFormat->pfnComputeParams(i, &iWidth, &iHeight, &iStride);

		/* If the width or height was changed by pfnComputeParams, the
		   source surface didn't match the width/height padding
		   requirements. Signal an error. */
		if ((i == 0) &&
			(iWidth != psSrcHandle->iWidth ||
			iHeight != psSrcHandle->iHeight))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Mismatching src/dest dimensions"
					 " (src %dx%d, dest %dx%d)", __func__,
					 psSrcHandle->iWidth, psSrcHandle->iHeight,
					 iWidth, iHeight));
			goto err_put;
		}

		/* FIXME: Note this won't work with SGX544 TAG CSC because the
		 *        allocation size is a non-trivial function of iHeight.
		 */
		if (PVR2DMemWrap(psPrivateData->hContext,
						 dest[i],
						 0, /* flags */
						 iHeight * iStride, /* size */
						 IMG_NULL, /* physical page addresses */
						 &apsDestMemInfo[i]) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Wrap via PVR2D failed", __func__));
			goto err_put;
		}
	}

	err = blit_internal(psPrivateData, src, apsDestMemInfo, format,
						psSrcHandle->iWidth, psSrcHandle->iHeight,
						0, 0, IMG_TRUE);

	for (i = 0; dest[i] && i < MAX_SUB_ALLOCS; i++)
	{
		if (PVR2DMemFree(psPrivateData->hContext,
						 apsDestMemInfo[i]) != PVR2D_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unwrap via PVR2D failed", __func__));
			err = -EINVAL;
			goto err_put;
		}
	}

err_put:
	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL int
gralloc_module_blit_to_handle(IMG_gralloc_module_public_t const *module,
							  buffer_handle_t src, buffer_handle_t dest,
							  int w, int h, int x, int y)
{
	IMG_native_handle_t *psDestHandle = (IMG_native_handle_t *)dest;
	IMG_native_handle_t *psSrcHandle = (IMG_native_handle_t *)src;
	PVR2DMEMINFO *apsDestPVR2DMemInfo[MAX_SUB_ALLOCS] = {0};
	PVR2DMEMINFO asDestPVR2DMemInfo[MAX_SUB_ALLOCS];
	IMG_mapper_meminfo_t *psDestMapperMemInfo;
	IMG_private_data_t *psPrivateData;
	int i, err = -EINVAL;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	if(!validate_handle(psSrcHandle) || !validate_handle(psDestHandle))
		goto err_out;

	psPrivateData = getModulePrivateData((gralloc_module_t *)module);
	psDestMapperMemInfo = MapperPeek(psPrivateData, psDestHandle->ui64Stamp);

	for (i = 0; i < MAX_SUB_ALLOCS; i++)
	{
		PVRSRV_CLIENT_MEM_INFO *psDestMemInfo
			= psDestMapperMemInfo->apsMemInfo[i];

		if (!psDestMemInfo)
			break;

		asDestPVR2DMemInfo[i].hPrivateData = psDestMemInfo;
		asDestPVR2DMemInfo[i].pBase = psDestMemInfo->pvLinAddr;
		asDestPVR2DMemInfo[i].hPrivateMapData = psDestMemInfo->hKernelMemInfo;
		asDestPVR2DMemInfo[i].ui32DevAddr = psDestMemInfo->sDevVAddr.uiAddr;
		asDestPVR2DMemInfo[i].ui32MemSize = (unsigned long)psDestMemInfo->uAllocSize;
		apsDestPVR2DMemInfo[i] = &asDestPVR2DMemInfo[i];
	}

	err = blit_internal(psPrivateData, src, apsDestPVR2DMemInfo,
						psDestHandle->iFormat, w, h, x, y, IMG_TRUE);

	putModulePrivateData(psPrivateData);
err_out:
	EXITED();
	return err;
}

IMG_INTERNAL
int gralloc_setup(const hw_module_t *module, hw_device_t **device)
{
	alloc_device_t *psAllocDevice;
	hw_device_t *psHwDevice;
	int err = 0;

	ENTERED();

	if(!IsInitialized())
	{
		err = -ENOTTY;
		goto err_out;
	}

	psAllocDevice = malloc(sizeof(alloc_device_t));
	if(!psAllocDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate memory", __func__));
		err = -ENOMEM;
		goto err_out;
	}

	memset(psAllocDevice, 0, sizeof(alloc_device_t));
	psHwDevice = (hw_device_t *)psAllocDevice;

	psAllocDevice->common.tag		= HARDWARE_DEVICE_TAG;
	psAllocDevice->common.version	= 1;
	psAllocDevice->common.module	= (hw_module_t *)module;
	psAllocDevice->common.close		= gralloc_device_close;

	psAllocDevice->alloc			= gralloc_device_alloc;
	psAllocDevice->free				= gralloc_device_free;
#if defined(PVR_ANDROID_HAS_GRALLOC_DUMP)
	psAllocDevice->dump				= gralloc_device_dump;
#endif

	*device = psHwDevice;
err_out:
	EXITED();
	return err;
}
