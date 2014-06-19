/*!****************************************************************************
@File           mapper.c

@Title          Graphics HAL mapper helper

@Author         Imagination Technologies

@Date           2009/04/02

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

#include "hal_private.h"
#include "gralloc.h"
#include "mapper.h"

#if defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS)

static IMG_BOOL __create_sync_mod_obj(IMG_private_data_t *psPrivateData,
									  IMG_mapper_meminfo_t *psEntry)
{
	return PVRSRVCreateSyncInfoModObj(
		psPrivateData->sDevData.psConnection,
		&psEntry->hKernelSyncInfoModObj) == PVRSRV_OK;
}

static IMG_BOOL __destroy_sync_mod_obj(IMG_private_data_t *psPrivateData,
									   IMG_mapper_meminfo_t *psEntry)
{
	return PVRSRVDestroySyncInfoModObj(
		psPrivateData->sDevData.psConnection,
		psEntry->hKernelSyncInfoModObj) == PVRSRV_OK;
}

#else /* defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS) */

static inline
IMG_BOOL __create_sync_mod_obj(IMG_private_data_t *psPrivateData,
							   IMG_mapper_meminfo_t *psEntry)
{
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
	PVR_UNREFERENCED_PARAMETER(psEntry);
	return IMG_TRUE;
}

static inline
IMG_BOOL __destroy_sync_mod_obj(IMG_private_data_t *psPrivateData,
								IMG_mapper_meminfo_t *psEntry)
{
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
	PVR_UNREFERENCED_PARAMETER(psEntry);
	return IMG_TRUE;
}

#endif /* defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS) */

#if defined(PVRSRV_NEED_PVR_DPF)

IMG_INTERNAL
void MapperLogSyncObjects(IMG_private_data_t *psPrivateData)
{
	IMG_mapper_meminfo_t *psEntry;

	PVR_DPF((PVR_DBG_ERROR, "%s: Dumping all active sync objects..",
			 __func__));

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
	{
		PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo =
			psEntry->apsMemInfo[0]->psClientSyncInfo;

		PVR_DPF((PVR_DBG_ERROR, "ID=%llu, 0xP...FHWR=0x%.8x, "
								"WOP/WOC=0x%x/0x%x, ROP/ROC=0x%x/0x%x, "
								"ROP2/ROC2=0x%x/0x%x, WOC DevVA=0x%.8x, "
								"ROC DevVA=0x%.8x, ROC2 DevVA=0x%.8x",
				 psEntry->ui64Stamp, psEntry->usage,
				 psClientSyncInfo->psSyncData->ui32WriteOpsPending,
				 psClientSyncInfo->psSyncData->ui32WriteOpsComplete,
				 psClientSyncInfo->psSyncData->ui32ReadOpsPending,
				 psClientSyncInfo->psSyncData->ui32ReadOpsComplete,
				 psClientSyncInfo->psSyncData->ui32ReadOps2Pending,
				 psClientSyncInfo->psSyncData->ui32ReadOps2Complete,
				 psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
				 psClientSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
				 psClientSyncInfo->sReadOps2CompleteDevVAddr.uiAddr));
	}
}

#endif /* defined(PVRSRV_NEED_PVR_DPF) */

#if defined(PVR_ANDROID_HAS_GRALLOC_DUMP)

IMG_INTERNAL
size_t MapperDumpSyncObjects(IMG_private_data_t *psPrivateData,
							 char *pszBuffer, size_t maxLen)
{
	IMG_mapper_meminfo_t *psEntry;
	size_t count = 0;

	count += snprintf(&pszBuffer[count], maxLen - count,
					  "IMG Graphics HAL state:\n");
	if(count >= maxLen)
		return maxLen;

	count += snprintf(&pszBuffer[count], maxLen - count,
					  "  Dumping all active sync objects..\n");
	if(count >= maxLen)
		return maxLen;

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
	{
		PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo =
			psEntry->apsMemInfo[0]->psClientSyncInfo;

		count += snprintf(&pszBuffer[count], maxLen - count,
						  "    ID=%llu, 0xP...FHWR=0x%.8x, "
						  "WOP/WOC=0x%x/0x%x, ROP/ROC=0x%x/0x%x, "
						  "ROP2/ROC2=0x%x/0x%x, WOC DevVA=0x%.8x, "
						  "ROC DevVA=0x%.8x, ROC2 DevVA=0x%.8x\n",
				 psEntry->ui64Stamp, psEntry->usage,
				 psClientSyncInfo->psSyncData->ui32WriteOpsPending,
				 psClientSyncInfo->psSyncData->ui32WriteOpsComplete,
				 psClientSyncInfo->psSyncData->ui32ReadOpsPending,
				 psClientSyncInfo->psSyncData->ui32ReadOpsComplete,
				 psClientSyncInfo->psSyncData->ui32ReadOps2Pending,
				 psClientSyncInfo->psSyncData->ui32ReadOps2Complete,
				 psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
				 psClientSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
				 psClientSyncInfo->sReadOps2CompleteDevVAddr.uiAddr);
		if(count >= maxLen)
			return maxLen;
	}

	return count;
}

#endif /* defined(PVR_ANDROID_HAS_GRALLOC_DUMP) */

IMG_INTERNAL
IMG_mapper_meminfo_t *MapperPeek(IMG_private_data_t *psPrivateData,
								 IMG_UINT64 ui64Stamp)
{
	IMG_mapper_meminfo_t *psEntry;

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
		if(ui64Stamp == psEntry->ui64Stamp)
			break;

	return psEntry;
}

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)

IMG_INTERNAL IMG_mapper_meminfo_t *
MapperPeekAccumPair(IMG_private_data_t *psPrivateData,
					IMG_mapper_meminfo_t *psMapperMemInfo)
{
	IMG_mapper_meminfo_t *psEntry;

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
		if(psEntry->ui64AccumStamp == psMapperMemInfo->ui64Stamp)
			break;

	return psEntry;
}

#endif /* defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND) */

static IMG_mapper_meminfo_t *
InsertBlankEntry(IMG_private_data_t *psPrivateData,
				 IMG_native_handle_t *psNativeHandle)
{
	IMG_mapper_meminfo_t *psEntry;

	/* Better not already be an entry for this ID */
	psEntry = MapperPeek(psPrivateData, psNativeHandle->ui64Stamp);
	if(psEntry)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: ID=%llu added already (meminfo=%p)",
			 	 __func__, psNativeHandle->ui64Stamp, psEntry->apsMemInfo[0]));
		return NULL;
	}

	psEntry = calloc(1, sizeof(IMG_mapper_meminfo_t));
	if(!psEntry)
	{
		perror("calloc");
		return NULL;
	}

	psEntry->ui64Stamp			= psNativeHandle->ui64Stamp;
	psEntry->iFormat			= psNativeHandle->iFormat;
	psEntry->usage				= psNativeHandle->usage;

	/* Fix up list pointers */
	psEntry->psNext				= psPrivateData->psMapList;
	psPrivateData->psMapList	= psEntry;

	if(usage_hw(psEntry->usage))
	{
		if(!__create_sync_mod_obj(psPrivateData, psEntry))
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to create sync info mod object", __func__));
			return NULL;
		}
	}

	return psEntry;
}

IMG_INTERNAL
IMG_BOOL MapperAddMappedFB(IMG_private_data_t *psPrivateData,
						   IMG_native_handle_t *psNativeHandle,
						   PVRSRV_CLIENT_MEM_INFO *psMemInfo,
						   void *hMemInfo)
{
	IMG_mapper_meminfo_t *psEntry;
	int i;

	psEntry = InsertBlankEntry(psPrivateData, psNativeHandle);
	if(!psEntry)
		return IMG_FALSE;

	psEntry->bAllocatedByThisProcess = IMG_TRUE;
	psEntry->bRegistered = IMG_TRUE;
	psEntry->hMemInfo = hMemInfo;

	psEntry->apsMemInfo[0] = psMemInfo;
	for(i = 1; i < MAX_SUB_ALLOCS; i++)
		psEntry->apsMemInfo[i] = IMG_NULL;

	return IMG_TRUE;
}

IMG_INTERNAL
IMG_BOOL MapperAddMapped(IMG_private_data_t *psPrivateData,
						 IMG_native_handle_t *psNativeHandle,
						 PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS])
{
	IMG_mapper_meminfo_t *psEntry;
	int i;

	psEntry = InsertBlankEntry(psPrivateData, psNativeHandle);
	if(!psEntry)
		return IMG_FALSE;

	psEntry->bAllocatedByThisProcess = IMG_TRUE;
	psEntry->bRegistered = IMG_TRUE;

	for(i = 0; i < MAX_SUB_ALLOCS; i++)
		psEntry->apsMemInfo[i] = apsMemInfo[i];

	return IMG_TRUE;
}

IMG_INTERNAL
IMG_BOOL MapperAddUnmapped(IMG_private_data_t *psPrivateData,
						   IMG_native_handle_t *psNativeHandle)
{
	IMG_mapper_meminfo_t *psEntry;

	psEntry = InsertBlankEntry(psPrivateData, psNativeHandle);
	if(!psEntry)
		return IMG_FALSE;

	psEntry->bRegistered = IMG_TRUE;

	return IMG_TRUE;
}

IMG_INTERNAL
void MapperRemove(IMG_private_data_t *psPrivateData,
				  IMG_mapper_meminfo_t *psMapperMemInfo)
{
	IMG_mapper_meminfo_t *psEntry, *psPrevEntry = NULL;

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
	{
		if(psMapperMemInfo == psEntry)
			break;
		psPrevEntry = psEntry;
	}

	/* Shouldn't happen, but keeps the static checkers happy */
	if(!psEntry)
		return;

	if(!psPrevEntry)
	{
		/* No previous entry means we're removing the list head */
		psPrivateData->psMapList = psEntry->psNext;
	}
	else
	{
		/* Just a normal list entry, relink as normal */
		psPrevEntry->psNext = psEntry->psNext;
	}

	if(usage_hw(psEntry->usage))
	{
		if(!__destroy_sync_mod_obj(psPrivateData, psEntry))
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to delete sync info mod object", __func__));
		}
	}

	free(psEntry);
}

IMG_INTERNAL
IMG_BOOL MapperSanityCheck(IMG_private_data_t *psPrivateData)
{
	IMG_mapper_meminfo_t *psEntry, *psEntry2;

	for(psEntry = psPrivateData->psMapList; psEntry; psEntry = psEntry->psNext)
	{
		for(psEntry2 = psEntry->psNext; psEntry2; psEntry2 = psEntry2->psNext)
		{
			if(psEntry->ui64Stamp == psEntry2->ui64Stamp)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate ID=%llu detected "
										"in mapper list!", __func__,
							(unsigned long long)psEntry->ui64Stamp));
				goto err_log_syncs;
			}

			/* FIXME: For now, just check the first alloc. We don't
			 * use the other syncs for HW synchronization so even if
			 * they were incorrectly duplicated, we wouldn't notice.
			 */
			{
				PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo;
				PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo2;

				if(!psEntry->apsMemInfo[0] || !psEntry2->apsMemInfo[0])
					break;

				psClientSyncInfo = psEntry->apsMemInfo[0]->psClientSyncInfo;
				psClientSyncInfo2 = psEntry2->apsMemInfo[0]->psClientSyncInfo;

				if(psEntry->apsMemInfo[0] == psEntry2->apsMemInfo[0])
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate psMemInfo "
											"detected in mapper list!",
											__func__));
					goto err_log_syncs;
				}

				if(psClientSyncInfo == psClientSyncInfo2)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate psClientSyncInfo "
											"detected in mapper list!",
											__func__));
					goto err_log_syncs;
				}

				if(psClientSyncInfo->psSyncData == psClientSyncInfo2->psSyncData)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate psSyncData "
											"detected in mapper list!",
											__func__));
					goto err_log_syncs;
				}

				if(psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr ==
				   psClientSyncInfo2->sWriteOpsCompleteDevVAddr.uiAddr)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate WOC DevVA=%.8x "
											"detected in mapper list!",
											__func__,
						psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr));
					goto err_log_syncs;
				}

				if(psClientSyncInfo->sReadOpsCompleteDevVAddr.uiAddr ==
				   psClientSyncInfo2->sReadOpsCompleteDevVAddr.uiAddr)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate ROC DevVA=%.8x "
											"detected in mapper list!",
											__func__,
						psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr));
					goto err_log_syncs;
				}

				if(psClientSyncInfo->sReadOps2CompleteDevVAddr.uiAddr ==
				   psClientSyncInfo2->sReadOps2CompleteDevVAddr.uiAddr)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Duplicate ROC2 DevVA=%.8x "
											"detected in mapper list!",
											__func__,
						psClientSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr));
					goto err_log_syncs;
				}
			}
		}
	}

	return IMG_TRUE;

err_log_syncs:
	MapperLogSyncObjects(psPrivateData);
	return IMG_FALSE;
}
