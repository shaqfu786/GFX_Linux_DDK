/*!****************************************************************************
@File           mapper.h

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

#ifndef MAPPER_H
#define MAPPER_H

#include "hal_public.h"

#include "services.h"

struct _IMG_mapper_meminfo_
{
	/* Key for mapper cache entry */
	IMG_UINT64 ui64Stamp;

	/* Canonical usage bits for surface */
	int usage;

	/* Whether this entry was allocated by this process */
	IMG_BOOL bAllocatedByThisProcess;

	/* Whether or not this buffer has been registered */
	IMG_BOOL bRegistered;

	/* These will be NULL if not mapped yet */
	PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS];
	IMG_HANDLE hMemInfo;
	int iFormat;

	/* These will be zero until mapped/locked */
	IMG_UINT32 ui32LockCount;
	int lockUsage;

	IMG_write_lock_rect_t sWriteLockRect;

#if defined(PVR_ANDROID_SOFTWARE_SYNC_OBJECTS)
	IMG_HANDLE hKernelSyncInfoModObj;
#endif

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
	IMG_UINT64 ui64AccumStamp;
#endif

	/* FIXME: Use a hash table not a linked list! */
	struct _IMG_mapper_meminfo_ *psNext;
};

#include "hal.h"

#if defined(PVRSRV_NEED_PVR_DPF)
void MapperLogSyncObjects(IMG_private_data_t *psPrivateData);
#else
static inline void MapperLogSyncObjects(IMG_private_data_t *psPrivateData)
{
	PVR_UNREFERENCED_PARAMETER(psPrivateData);
}
#endif

#if defined(PVR_ANDROID_HAS_GRALLOC_DUMP)
size_t MapperDumpSyncObjects(IMG_private_data_t *psPrivateData,
							 char *pszBuffer, size_t maxLen);
#endif

IMG_mapper_meminfo_t *MapperPeek(IMG_private_data_t *psPrivateData,
								 IMG_UINT64 ui64Stamp);

#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
IMG_mapper_meminfo_t *
MapperPeekAccumPair(IMG_private_data_t *psPrivateData,
				    IMG_mapper_meminfo_t *psMapperMemInfo);
#endif /* defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND) */

IMG_BOOL MapperAddMappedFB(IMG_private_data_t *psPrivateData,
						   IMG_native_handle_t *psNativeHandle,
						   PVRSRV_CLIENT_MEM_INFO *psMemInfo,
						   void *hMemInfo);

IMG_BOOL MapperAddMapped(IMG_private_data_t *psPrivateData,
						 IMG_native_handle_t *psNativeHandle,
						 PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS]);

IMG_BOOL MapperAddUnmapped(IMG_private_data_t *psPrivateData,
						   IMG_native_handle_t *psNativeHandle);

void MapperRemove(IMG_private_data_t *psPrivateData,
				  IMG_mapper_meminfo_t *psMapperMemInfo);

IMG_BOOL MapperSanityCheck(IMG_private_data_t *psPrivateData);

#endif /* MAPPER_H */
